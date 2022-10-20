// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// This was adapted from open source code (APACHE 2.0)
// TODO: Replace with our implementation.

typedef void* (*OBJECT_NEW_FN)(void);
typedef void (*OBJECT_INIT_FN)(void* obj);
typedef void (*OBJECT_UNINIT_FN)(void* obj);
typedef void (*OBJECT_FREE_FN)(void* obj);
typedef BOOL (*OBJECT_EQUALS_FN)(void* objA, void* objB);

struct _wObject
{
        OBJECT_NEW_FN fnObjectNew;
        OBJECT_INIT_FN fnObjectInit;
        OBJECT_UNINIT_FN fnObjectUninit;
        OBJECT_FREE_FN fnObjectFree;
        OBJECT_EQUALS_FN fnObjectEquals;
};
typedef struct _wObject wObject;

/* System.Collections.Queue */

struct _wQueue
{
        int capacity;
        int growthFactor;
        BOOL synchronized;

        int head;
        int tail;
        int size;
        void** array;
        CRITICAL_SECTION lock;
        HANDLE event;

        wObject object;
};
typedef struct _wQueue wQueue;

int Queue_Count(wQueue* queue);

void Queue_Lock(wQueue* queue);
void Queue_Unlock(wQueue* queue);

HANDLE Queue_Event(wQueue* queue);

#define Queue_Object(_queue)    (&_queue->object)

void Queue_Clear(wQueue* queue);
BOOL Queue_Contains(wQueue* queue, void* obj);
BOOL Queue_Enqueue(wQueue* queue, void* obj);
void* Queue_Dequeue(wQueue* queue);
void* Queue_Peek(wQueue* queue);
wQueue* Queue_New(BOOL synchronized, int capacity, int growthFactor);
void Queue_Free(wQueue* queue);

struct _wArrayList
{
        int capacity;
        int growthFactor;
        BOOL synchronized;

        int size;
        void** array;
        CRITICAL_SECTION lock;

        wObject object;
};
typedef struct _wArrayList wArrayList;

int ArrayList_Capacity(wArrayList* arrayList);
int ArrayList_Count(wArrayList* arrayList);
int ArrayList_Items(wArrayList* arrayList, ULONG_PTR** ppItems);
BOOL ArrayList_IsFixedSized(wArrayList* arrayList);
BOOL ArrayList_IsReadOnly(wArrayList* arrayList);
BOOL ArrayList_IsSynchronized(wArrayList* arrayList);

void ArrayList_Lock(wArrayList* arrayList);
void ArrayList_Unlock(wArrayList* arrayList);

void* ArrayList_GetItem(wArrayList* arrayList, int index);
void ArrayList_SetItem(wArrayList* arrayList, int index, void* obj);

#define ArrayList_Object(_arrayList)    (&_arrayList->object)

void ArrayList_Clear(wArrayList* arrayList);
BOOL ArrayList_Contains(wArrayList* arrayList, void* obj);

int ArrayList_Add(wArrayList* arrayList, void* obj);
BOOL ArrayList_Insert(wArrayList* arrayList, int index, void* obj);

BOOL ArrayList_Remove(wArrayList* arrayList, void* obj);
BOOL ArrayList_RemoveAt(wArrayList* arrayList, int index);

int ArrayList_IndexOf(wArrayList* arrayList, void* obj, int startIndex, int count);
int ArrayList_LastIndexOf(wArrayList* arrayList, void* obj, int startIndex, int count);

wArrayList* ArrayList_New(BOOL synchronized);
void ArrayList_Free(wArrayList* arrayList);

struct _wCountdownEvent
{
        DWORD count;
        CRITICAL_SECTION lock;
        HANDLE event;
        DWORD initialCount;
};
typedef struct _wCountdownEvent wCountdownEvent;

DWORD CountdownEvent_CurrentCount(wCountdownEvent* countdown);
DWORD CountdownEvent_InitialCount(wCountdownEvent* countdown);
BOOL CountdownEvent_IsSet(wCountdownEvent* countdown);
HANDLE CountdownEvent_WaitHandle(wCountdownEvent* countdown);

void CountdownEvent_AddCount(wCountdownEvent* countdown, DWORD signalCount);
BOOL CountdownEvent_Signal(wCountdownEvent* countdown, DWORD signalCount);
void CountdownEvent_Reset(wCountdownEvent* countdown, DWORD count);

wCountdownEvent* CountdownEvent_New(DWORD initialCount);
void CountdownEvent_Free(wCountdownEvent* countdown);

PTP_POOL GetDefaultThreadpool(void);
PTP_CALLBACK_ENVIRON GetDefaultThreadpoolEnvironment(void);


