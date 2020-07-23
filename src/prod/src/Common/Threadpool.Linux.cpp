// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// This was adapted from open source code (APACHE 2.0)
// TODO: Replace with our implementation. 

#include "stdafx.h"
#include "Threadpool.Linux.h"

static TP_CALLBACK_ENVIRON DEFAULT_CALLBACK_ENVIRONMENT =
{
    1, /* Version */
    NULL, /* Pool */
    NULL, /* CleanupGroup */
    NULL, /* CleanupGroupCancelCallback */
    NULL, /* RaceDll */
    NULL, /* ActivationContext */
    NULL, /* FinalizationCallback */
    { 0 } /* Flags */
};

PTP_CALLBACK_ENVIRON GetDefaultThreadpoolEnvironment()
{
    PTP_CALLBACK_ENVIRON environment = &DEFAULT_CALLBACK_ENVIRONMENT;
    environment->Pool = GetDefaultThreadpool();
    return environment;
}

static TP_POOL DEFAULT_POOL =
{
    0,    /* DWORD Minimum */
    500,  /* DWORD Maximum */
    NULL, /* PVOID Threads */
    NULL, /* PVOID PendingQueue */
    NULL  /* HANDLE TerminateEvent */
};

static void* thread_pool_work_func(void* arg)
{
    DWORD status;
    PTP_POOL pool;
    PTP_WORK work;
    HANDLE events[2];
    PTP_CALLBACK_INSTANCE callbackInstance;

    pool = (PTP_POOL)arg;

    events[0] = pool->TerminateEvent;
    events[1] = Queue_Event(pool->PendingQueue);

    while (1)
    {
        status = WaitForMultipleObjects(2, events, FALSE, INFINITE);

        if (status == WAIT_OBJECT_0)
            break;

        if (status != (WAIT_OBJECT_0 + 1))
            break;

        callbackInstance = (PTP_CALLBACK_INSTANCE)Queue_Dequeue(pool->PendingQueue);

        if (callbackInstance)
        {
            work = callbackInstance->Work;
            work->WorkCallback(callbackInstance, work->CallbackParameter, work);
            CountdownEvent_Signal(pool->WorkComplete, 1);
            free(callbackInstance);
        }
    }

    ExitThread(0);
    return NULL;
}

static void threads_close(void *thread)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

static BOOL InitializeThreadpool(PTP_POOL pool)
{
    int index;
    HANDLE thread;

    if (pool->Threads)
        return TRUE;

    pool->Minimum = 0;
    pool->Maximum = 500;

    if (!(pool->PendingQueue = Queue_New(TRUE, -1, -1)))
        goto fail_queue_new;

    if (!(pool->WorkComplete = CountdownEvent_New(0)))
        goto fail_countdown_event;

    if (!(pool->TerminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL)))
        goto fail_terminate_event;

    if (!(pool->Threads = ArrayList_New(TRUE)))
        goto fail_thread_array;

    pool->Threads->object.fnObjectFree = threads_close;

    for (index = 0; index < 100; index++)
    {
        if (!(thread = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)thread_pool_work_func,
            (void*)pool, 0, NULL)))
        {
            goto fail_create_threads;
        }

        if (ArrayList_Add(pool->Threads, thread) < 0)
            goto fail_create_threads;
    }

    return TRUE;

fail_create_threads:
    SetEvent(pool->TerminateEvent);
    ArrayList_Free(pool->Threads);
    pool->Threads = NULL;
fail_thread_array:
    CloseHandle(pool->TerminateEvent);
    pool->TerminateEvent = NULL;
fail_terminate_event:
    CountdownEvent_Free(pool->WorkComplete);
    pool->WorkComplete = NULL;
fail_countdown_event:
    Queue_Free(pool->PendingQueue);
    pool->WorkComplete = NULL;
fail_queue_new:

    return FALSE;

}

PTP_POOL GetDefaultThreadpool()
{
    PTP_POOL pool = NULL;

    pool = &DEFAULT_POOL;

    if (!InitializeThreadpool(pool))
        return NULL;

    return pool;
}

PTP_POOL CreateThreadpool(PVOID reserved)
{
    PTP_POOL pool = NULL;

    if (!(pool = (PTP_POOL)calloc(1, sizeof(TP_POOL))))
        return NULL;

    if (!InitializeThreadpool(pool))
    {
        free(pool);
        return NULL;
    }

    return pool;
}

VOID CloseThreadpool(PTP_POOL ptpp)
{
    SetEvent(ptpp->TerminateEvent);

    ArrayList_Free(ptpp->Threads);
    Queue_Free(ptpp->PendingQueue);
    CountdownEvent_Free(ptpp->WorkComplete);
    CloseHandle(ptpp->TerminateEvent);

    if (ptpp == &DEFAULT_POOL)
    {
        ptpp->Threads = NULL;
        ptpp->PendingQueue = NULL;
        ptpp->WorkComplete = NULL;
        ptpp->TerminateEvent = NULL;
    }
    else
    {
        free(ptpp);
    }
}

BOOL SetThreadpoolThreadMinimum(PTP_POOL ptpp, DWORD cthrdMic)
{
    HANDLE thread;

    ptpp->Minimum = cthrdMic;

    while (ArrayList_Count(ptpp->Threads) < ptpp->Minimum)
    {
        if (!(thread = CreateThread(NULL, 0,
            (LPTHREAD_START_ROUTINE)thread_pool_work_func,
            (void*)ptpp, 0, NULL)))
        {
            return FALSE;
        }

        if (ArrayList_Add(ptpp->Threads, thread) < 0)
            return FALSE;
    }
}

VOID SetThreadpoolThreadMaximum(PTP_POOL ptpp, DWORD cthrdMost)
{
    ptpp->Maximum = cthrdMost;
}

PTP_WORK CreateThreadpoolWork(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe)
{
    PTP_WORK work = NULL;
    work = (PTP_WORK)malloc(sizeof(TP_WORK));

    if (work)
    {
        work->WorkCallback = pfnwk;
        work->CallbackParameter = pv;

        if (!pcbe)
            pcbe = GetDefaultThreadpoolEnvironment();

        work->CallbackEnvironment = pcbe;
    }
    return work;
}

VOID CloseThreadpoolWork(PTP_WORK pwk)
{
    free(pwk);
}

VOID SubmitThreadpoolWork(PTP_WORK pwk)
{
    PTP_POOL pool;
    PTP_CALLBACK_INSTANCE callbackInstance;
    pool = pwk->CallbackEnvironment->Pool;
    callbackInstance = (PTP_CALLBACK_INSTANCE)malloc(sizeof(TP_CALLBACK_INSTANCE));

    if (callbackInstance)
    {
        callbackInstance->Work = pwk;
        CountdownEvent_AddCount(pool->WorkComplete, 1);
        Queue_Enqueue(pool->PendingQueue, callbackInstance);
    }
}

BOOL TrySubmitThreadpoolCallback(PTP_SIMPLE_CALLBACK pfns, PVOID pv, PTP_CALLBACK_ENVIRON pcbe)
{
    return FALSE;
}

VOID WaitForThreadpoolWorkCallbacks(PTP_WORK pwk, BOOL fCancelPendingCallbacks)
{
    HANDLE event;
    PTP_POOL pool;
    pool = pwk->CallbackEnvironment->Pool;
    event = CountdownEvent_WaitHandle(pool->WorkComplete);

    //if (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0)
    //    WLog_ERR(TAG, "error waiting on work completion");

}

///////////////////////////////////////////////////////////////////////////////////////////////

int ArrayList_Capacity(wArrayList *arrayList)
{
    return arrayList->capacity;
}
int ArrayList_Count(wArrayList *arrayList)
{
    return arrayList->size;
}
int ArrayList_Items(wArrayList* arrayList, ULONG_PTR** ppItems)
{
    *ppItems = (ULONG_PTR*) arrayList->array;
    return arrayList->size;
}
BOOL ArrayList_IsFixedSized(wArrayList *arrayList)
{
    return FALSE;
}
BOOL ArrayList_IsReadOnly(wArrayList *arrayList)
{
    return FALSE;
}

BOOL ArrayList_IsSynchronized(wArrayList *arrayList)
{
    return arrayList->synchronized;
}
void ArrayList_Lock(wArrayList *arrayList)
{
    EnterCriticalSection(&arrayList->lock);
}
void ArrayList_Unlock(wArrayList *arrayList)
{
    LeaveCriticalSection(&arrayList->lock);
}
void *ArrayList_GetItem(wArrayList *arrayList, int index)
{
    void *obj = NULL;

    if ((index >= 0) && (index < arrayList->size))
    {
        obj = arrayList->array[index];
    }

    return obj;
}
void ArrayList_SetItem(wArrayList *arrayList, int index, void *obj)
{
    if ((index >= 0) && (index < arrayList->size))
    {
        arrayList->array[index] = obj;
    }
}
BOOL ArrayList_Shift(wArrayList *arrayList, int index, int count)
{
    if (count > 0)
    {
        if (arrayList->size + count > arrayList->capacity)
        {
            void **newArray;
            int newCapacity = arrayList->capacity * arrayList->growthFactor;
            newArray = (void **)realloc(arrayList->array, sizeof(void *) * newCapacity);

            if (!newArray)
                return FALSE;

            arrayList->array = newArray;
            arrayList->capacity = newCapacity;
        }

        MoveMemory(&arrayList->array[index + count], &arrayList->array[index], (arrayList->size - index) * sizeof(void *));
        arrayList->size += count;
    }
    else if (count < 0)
    {
        int chunk = arrayList->size - index + count;

        if (chunk > 0)
            MoveMemory(&arrayList->array[index], &arrayList->array[index - count], chunk * sizeof(void *));

        arrayList->size += count;
    }

    return TRUE;
}
void ArrayList_Clear(wArrayList *arrayList)
{
    int index;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    for (index = 0; index < arrayList->size; index++)
    {
        if (arrayList->object.fnObjectFree)
            arrayList->object.fnObjectFree(arrayList->array[index]);

        arrayList->array[index] = NULL;
    }

    arrayList->size = 0;

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);
}
BOOL ArrayList_Contains(wArrayList *arrayList, void *obj)
{
    DWORD index;
    BOOL rc = FALSE;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    for (index = 0; index < arrayList->size; index++)
    {
        rc = arrayList->object.fnObjectEquals(arrayList->array[index], obj);

        if (rc)
            break;
    }

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return rc;
}
int ArrayList_Add(wArrayList *arrayList, void *obj)
{
    int index = -1;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    if (arrayList->size + 1 > arrayList->capacity)
    {
        void **newArray;
        int newCapacity = arrayList->capacity * arrayList->growthFactor;
        newArray = (void **)realloc(arrayList->array, sizeof(void *) * newCapacity);

        if (!newArray)
            goto out;

        arrayList->array = newArray;
        arrayList->capacity = newCapacity;
    }

    arrayList->array[arrayList->size++] = obj;
    index = arrayList->size;
out:

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return index;
}
BOOL ArrayList_Insert(wArrayList *arrayList, int index, void *obj)
{
    BOOL ret = TRUE;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    if ((index >= 0) && (index < arrayList->size))
    {
        if (!ArrayList_Shift(arrayList, index, 1))
        {
            ret = FALSE;
        }
        else
        {
            arrayList->array[index] = obj;
        }
    }

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return ret;
}
BOOL ArrayList_Remove(wArrayList *arrayList, void *obj)
{
    int index;
    BOOL found = FALSE;
    BOOL ret = TRUE;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    for (index = 0; index < arrayList->size; index++)
    {
        if (arrayList->array[index] == obj)
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        if (arrayList->object.fnObjectFree)
            arrayList->object.fnObjectFree(arrayList->array[index]);

        ret = ArrayList_Shift(arrayList, index, -1);
    }

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return ret;
}
BOOL ArrayList_RemoveAt(wArrayList *arrayList, int index)
{
    BOOL ret = TRUE;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    if ((index >= 0) && (index < arrayList->size))
    {
        if (arrayList->object.fnObjectFree)
            arrayList->object.fnObjectFree(arrayList->array[index]);

        ret = ArrayList_Shift(arrayList, index, -1);
    }

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return ret;
}

int ArrayList_IndexOf(wArrayList *arrayList, void *obj, int startIndex, int count)
{
    int index;
    BOOL found = FALSE;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    if (startIndex < 0)
        startIndex = 0;

    if (count < 0)
        count = arrayList->size;

    for (index = startIndex; index < startIndex + count; index++)
    {
        if (arrayList->object.fnObjectEquals(arrayList->array[index], obj))
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
        index = -1;

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return index;
}
int ArrayList_LastIndexOf(wArrayList *arrayList, void *obj, int startIndex, int count)
{
    int index;
    BOOL found = FALSE;

    if (arrayList->synchronized)
        EnterCriticalSection(&arrayList->lock);

    if (startIndex < 0)
        startIndex = 0;

    if (count < 0)
        count = arrayList->size;

    for (index = startIndex + count - 1; index >= startIndex; index--)
    {
        if (arrayList->object.fnObjectEquals(arrayList->array[index], obj))
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
        index = -1;

    if (arrayList->synchronized)
        LeaveCriticalSection(&arrayList->lock);

    return index;
}

static BOOL ArrayList_DefaultCompare(void *objA, void *objB)
{
    return objA == objB ? TRUE : FALSE;
}
wArrayList *ArrayList_New(BOOL synchronized)
{
    wArrayList *arrayList = NULL;
    arrayList = (wArrayList *)calloc(1, sizeof(wArrayList));

    if (!arrayList)
        return NULL;

    arrayList->synchronized = synchronized;
    arrayList->capacity = 32;
    arrayList->growthFactor = 2;
    arrayList->object.fnObjectEquals = ArrayList_DefaultCompare;
    arrayList->array = (void **)malloc(arrayList->capacity * sizeof(void *));

    if (!arrayList->array)
        goto out_free;

    InitializeCriticalSection(&arrayList->lock);
    return arrayList;
out_free:
    free(arrayList);
    return NULL;
}

void ArrayList_Free(wArrayList *arrayList)
{
    ArrayList_Clear(arrayList);
    DeleteCriticalSection(&arrayList->lock);
    free(arrayList->array);
    free(arrayList);
}

///////////////////////////////////////////////////////////////////////////////////////////////

int Queue_Count(wQueue* queue)
{
    int ret;
    if (queue->synchronized)
        EnterCriticalSection(&queue->lock);

    ret = queue->size;

    if (queue->synchronized)
        LeaveCriticalSection(&queue->lock);
    return ret;
}
void Queue_Lock(wQueue* queue)
{
    EnterCriticalSection(&queue->lock);
}
void Queue_Unlock(wQueue* queue)
{
    LeaveCriticalSection(&queue->lock);
}
HANDLE Queue_Event(wQueue* queue)
{
    return queue->event;
}
void Queue_Clear(wQueue* queue)
{
    int index;

    if (queue->synchronized)
        EnterCriticalSection(&queue->lock);

    for (index = queue->head; index != queue->tail; index = (index + 1) % queue->capacity)
    {
        if (queue->object.fnObjectFree)
            queue->object.fnObjectFree(queue->array[index]);

        queue->array[index] = NULL;
    }

    queue->size = 0;
    queue->head = queue->tail = 0;

    if (queue->synchronized)
        LeaveCriticalSection(&queue->lock);
}
BOOL Queue_Contains(wQueue* queue, void* obj)
{
    int index;
    BOOL found = FALSE;

    if (queue->synchronized)
        EnterCriticalSection(&queue->lock);

    for (index = 0; index < queue->tail; index++)
    {
        if (queue->object.fnObjectEquals(queue->array[index], obj))
        {
            found = TRUE;
            break;
        }
    }

    if (queue->synchronized)
        LeaveCriticalSection(&queue->lock);

    return found;
}
BOOL Queue_Enqueue(wQueue* queue, void* obj)
{
    BOOL ret = TRUE;

    if (queue->synchronized)
        EnterCriticalSection(&queue->lock);

    if (queue->size == queue->capacity)
    {
        int old_capacity;
        int new_capacity;
        void **newArray;

        old_capacity = queue->capacity;
        new_capacity = queue->capacity * queue->growthFactor;

        newArray = (void **)realloc(queue->array, sizeof(void*) * new_capacity);
        if (!newArray)
        {
            ret = FALSE;
            goto out;
        }

        queue->capacity = new_capacity;
        queue->array = newArray;
        ZeroMemory(&(queue->array[old_capacity]), old_capacity * sizeof(void*));

        if (queue->tail < old_capacity)
        {
            CopyMemory(&(queue->array[old_capacity]), queue->array, queue->tail * sizeof(void*));
            queue->tail += old_capacity;
        }
    }

    queue->array[queue->tail] = obj;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;

    SetEvent(queue->event);

out:
    if (queue->synchronized)
        LeaveCriticalSection(&queue->lock);
    return ret;
}
void* Queue_Dequeue(wQueue* queue)
{
    void* obj = NULL;

    if (queue->synchronized)
        EnterCriticalSection(&queue->lock);

    if (queue->size > 0)
    {
        obj = queue->array[queue->head];
        queue->array[queue->head] = NULL;
        queue->head = (queue->head + 1) % queue->capacity;
        queue->size--;
    }

    if (queue->size < 1)
        ResetEvent(queue->event);

    if (queue->synchronized)
        LeaveCriticalSection(&queue->lock);

    return obj;
}
void* Queue_Peek(wQueue* queue)
{
    void* obj = NULL;

    if (queue->synchronized)
        EnterCriticalSection(&queue->lock);

    if (queue->size > 0)
        obj = queue->array[queue->head];

    if (queue->synchronized)
        LeaveCriticalSection(&queue->lock);

    return obj;
}

static BOOL default_queue_equals(void *obj1, void *obj2)
{
    return (obj1 == obj2);
}
wQueue* Queue_New(BOOL synchronized, int capacity, int growthFactor)
{
    wQueue* queue = NULL;

    queue = (wQueue *)calloc(1, sizeof(wQueue));
    if (!queue)
        return NULL;

    queue->capacity = 32;
    queue->growthFactor = 2;

    queue->synchronized = synchronized;

    if (capacity > 0)
        queue->capacity = capacity;

    if (growthFactor > 0)
        queue->growthFactor = growthFactor;

    queue->array = (void **)calloc(queue->capacity, sizeof(void *));
    if (!queue->array)
        goto out_free;

    queue->event = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!queue->event)
        goto out_free_array;

    InitializeCriticalSection(&queue->lock);

    queue->object.fnObjectEquals = default_queue_equals;
    return queue;

out_free_event:
    CloseHandle(queue->event);
out_free_array:
    free(queue->array);
out_free:
    free(queue);
    return NULL;
}
void Queue_Free(wQueue* queue)
{
    if (!queue)
        return;

    Queue_Clear(queue);

    CloseHandle(queue->event);
    DeleteCriticalSection(&queue->lock);
    free(queue->array);
    free(queue);
}

/////////////////////////////////////////////////////////////////////////

DWORD CountdownEvent_CurrentCount(wCountdownEvent* countdown)
{
    return countdown->count;
}
DWORD CountdownEvent_InitialCount(wCountdownEvent* countdown)
{
    return countdown->initialCount;
}
BOOL CountdownEvent_IsSet(wCountdownEvent* countdown)
{
    BOOL status = FALSE;

    if (WaitForSingleObject(countdown->event, 0) == WAIT_OBJECT_0)
        status = TRUE;

    return status;
}
HANDLE CountdownEvent_WaitHandle(wCountdownEvent* countdown)
{
    return countdown->event;
}
void CountdownEvent_AddCount(wCountdownEvent* countdown, DWORD signalCount)
{
    EnterCriticalSection(&countdown->lock);

    countdown->count += signalCount;

    if (countdown->count > 0)
        ResetEvent(countdown->event);

    LeaveCriticalSection(&countdown->lock);
}
BOOL CountdownEvent_Signal(wCountdownEvent* countdown, DWORD signalCount)
{
    BOOL status;
    BOOL newStatus;
    BOOL oldStatus;

    status = newStatus = oldStatus = FALSE;

    EnterCriticalSection(&countdown->lock);

    if (WaitForSingleObject(countdown->event, 0) == WAIT_OBJECT_0)
        oldStatus = TRUE;

    if (signalCount <= countdown->count)
        countdown->count -= signalCount;
    else
        countdown->count = 0;

    if (countdown->count == 0)
        newStatus = TRUE;

    if (newStatus && (!oldStatus))
    {
        SetEvent(countdown->event);
        status = TRUE;
    }

    LeaveCriticalSection(&countdown->lock);

    return status;
}
void CountdownEvent_Reset(wCountdownEvent* countdown, DWORD count)
{
    countdown->initialCount = count;
}
wCountdownEvent* CountdownEvent_New(DWORD initialCount)
{
    wCountdownEvent* countdown = NULL;

    if (!(countdown = (wCountdownEvent*) calloc(1, sizeof(wCountdownEvent))))
        return NULL;

    countdown->count = initialCount;
    countdown->initialCount = initialCount;

    InitializeCriticalSection(&countdown->lock);

    if (!(countdown->event = CreateEvent(NULL, TRUE, FALSE, NULL)))
        goto fail_create_event;

    if (countdown->count == 0)
        if (!SetEvent(countdown->event))
            goto fail_set_event;

    return countdown;

fail_set_event:
    CloseHandle(countdown->event);
fail_create_event:
    DeleteCriticalSection(&countdown->lock);
fail_critical_section:
    free(countdown);

    return NULL;
}
void CountdownEvent_Free(wCountdownEvent* countdown)
{
    DeleteCriticalSection(&countdown->lock);
    CloseHandle(countdown->event);

    free(countdown);
}

