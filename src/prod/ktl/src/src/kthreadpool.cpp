/*++

Module Name:

    kthreadpool.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KThreadPool object.

Author:

    Norbert P. Kusters (norbertk) 27-Sep-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:
    5/24/13 - Added User Mode (System Pool) Thread Detach support and thread level work queue

--*/

#include <ktl.h>
#include <ktrace.h>

#if defined(PLATFORM_UNIX)
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <vector>
#include <queue>
#include <bitset>
#include <map>
#include <palio.h>
#include <syscall.h>

#define NULL 0

using namespace std;

#endif  // PLATFORM_UNIX

#pragma region ** KThreadPool common and default implementation
KThreadPool::KThreadPool()
{
}
 
KThreadPool::~KThreadPool()
{
}
 
#if KTL_USER_MODE

NTSTATUS
KThreadPool::RegisterIoCompletionCallback(
    __in HANDLE Handle,
    __in IoCompletionCallback CompletionCallback,
    __in_opt VOID* Context,
    __out VOID** RegistrationContext
    )
{
    UNREFERENCED_PARAMETER(Handle);
    UNREFERENCED_PARAMETER(CompletionCallback);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(RegistrationContext);

    return STATUS_NOT_SUPPORTED;
}

VOID
KThreadPool::UnregisterIoCompletionCallback(
    __inout VOID* RegistrationContext
    )
{
    UNREFERENCED_PARAMETER(RegistrationContext);
}

#endif
 
KThreadPool::WorkItem*
KThreadPool::GetLink(
    __in WorkItem& WI)
{
    return WI._Link;
}
 
VOID
KThreadPool::SetLink(
    __inout WorkItem& WI,
    __in_opt WorkItem* Link)
{
    WI._Link = Link;
}
 
KThreadPool::WorkItem::WorkItem()
{
    _Link = this;
    _EnqueuerCallerAddress = nullptr;
}
 
KThreadPool::WorkItem::~WorkItem()
{
    KInvariant(_EnqueuerCallerAddress == nullptr);          // WorkItem dtor'd while on ThreadPool queue
}

#pragma optimize( "", off )
_declspec(noinline) void* KThreadPool::WorkItem::Dispatch(WorkItem& WI)
{
    void* enqueuerCallerAddress = WI._EnqueuerCallerAddress;        // For DEBUG: if Execute() fails - this is the ret addr of the enqueuer
    WI._EnqueuerCallerAddress = nullptr;
    WI.Execute();
    return enqueuerCallerAddress;
}
#pragma optimize( "", on )

 
VOID
KThreadPool::WorkQueue::Insert(__inout KThreadPool::WorkItem& WorkItem)

/*++

Routine Description:

    This routine inserts the given work item into the given queue.

Arguments:

    WorkItem    - Supplies the work item to insert into the queue.

Return Value:

    None.

--*/

{
    // Whenever a work item is not queued up anywhere, it should have its link set to point to itself.
    KInvariant(GetLink(WorkItem) == &WorkItem);

    // When in the queue, at the last position, the 'link field' is NULL.
    SetLink(WorkItem, NULL);

    if (_Back)
    {
        // Set the former last element's link to the new last element.
        SetLink(*_Back, &WorkItem);

    } else {
        // If this is the first insert, then the front pointer points to this new element.
        _Front = &WorkItem;
    }

    // The newly inserted element is the last element.
    _Back = &WorkItem;
}
 
KThreadPool::WorkItem*
KThreadPool::WorkQueue::Remove()

/*++

Routine Description:

    This routine removes the first element from the given thread queue.

Arguments:

    None.

Return Value:

    The work item at the front of the thread queue.

--*/

{
    WorkItem* workItem;

    //
    // The one we want is at the front.
    //

    workItem = _Front;

    //
    // Short circuit here if the queue is empty.
    //

    if (!workItem) {
        return NULL;
    }

    //
    // Set the new front to point to the link.
    //

    _Front = GetLink(*workItem);

    //
    // If the queue is empty then remember to set the back pointer to NULL.
    //

    if (!_Front) {
        _Back = NULL;
    }

    //
    // Set the link to indicate that the work item is not on any queue.
    //

    SetLink(*workItem, workItem);

    return workItem;
}

#pragma endregion ** KThreadPool common and default implementation


#pragma region ** Standard KTL ThreadPool implementation - Used by KAsyncContextBase
class KThreadPoolStandard : public KThreadPool
{
    K_FORCE_SHARED(KThreadPoolStandard);

public:
    VOID
    QueueWorkItem(__inout WorkItem& WorkItem, __in BOOLEAN UseCurrentThreadQueue) override;

    VOID
    Wait() override;

    #if KTL_USER_MODE
        NTSTATUS
        RegisterIoCompletionCallback(
            __in HANDLE Handle,
            __in IoCompletionCallback CompletionCallback,
            __in_opt VOID* Context,
            __out VOID** RegistrationContext
            ) override;

        VOID
        UnregisterIoCompletionCallback(__inout VOID* RegistrationContext) override;
    #endif

    BOOLEAN
    IsCurrentThreadOwnedSupported() override    { return TRUE; }

    BOOLEAN
    IsCurrentThreadOwned() override;

    BOOLEAN
    SetThreadPriority(__in KThread::Priority Priority) override;

    BOOLEAN
    SetWorkItemExecutionTimeBound(__in ULONG BoundInMilliSeconds) override;

    LONGLONG
    GetWorkItemExecutionTooLongCount() override;

    ULONG
    GetThreadCount() override { return _NumberOfThreads; }

private:
    FAILABLE
    KThreadPoolStandard(
        __in ULONG MaximumNumberOfThreads,
        __in ULONG AllocationTag);

    VOID
    Zero();

    VOID
    Cleanup();

    NTSTATUS
    Initialize(
        __in ULONG MaximumNumberOfThreads,
        __in ULONG AllocationTag);

    static
    ULONG
    QueryMaximumProcessors();

    static
    NTSTATUS
    FixProcessor(__in ULONG ProcessorIndex);

    static
    BOOLEAN
    DoesProcessorExist(__in ULONG ProcessorIndex);

    static
    NTSTATUS
    QueryCacheLineSize(__out ULONG& CacheLineSize, __in KAllocator& Allocator);

    #if !KTL_USER_MODE
    _IRQL_requires_same_
    __drv_maxIRQL(PASSIVE_LEVEL)
    _Function_class_(WORKER_THREAD_ROUTINE)
    #endif
    static
    VOID
    Worker(__inout VOID* Parameter);

    struct KPerWorkerThreadState
    {
        KThreadPoolStandard*           ThreadPool;
        USHORT                         ProcessorIndex;
        BOOLEAN volatile               ThreadIsDetached;
        KThreadPoolStandard::WorkQueue PerThreadQueue;

        #if KTL_USER_MODE
            DWORD                      ThreadId;
        #else
            PKTHREAD                   Thread;
        #endif

        KThread::Id __inline GetThreadId()
        {
            #if KTL_USER_MODE
                return static_cast<KThread::Id>(ThreadId);
            #else
                return reinterpret_cast<KThread::Id>(Thread);
            #endif
        }
    };

    VOID
    ExecuteWorkItem(__in WorkItem& Work);

    struct WorkerThreadStartupInfo
    {
        KPerWorkerThreadState*  ThreadState;
        KEvent*                 ReadyEvent;
    };

    typedef KArray<KThread::SPtr> KWorkerThreadArray;

    friend KThreadPool;

    ULONG               _NumberOfThreads;
    KWorkerThreadArray  _Threads;
    VOID*               _CacheAlignedState;
    VOID*               _FirstCacheAlignedState;
    ULONG               _CacheAlignedStateSize;
    ULONG               _NumberOfWorkerStates;
    ULONG               _PerWorkerStateSize;
    BOOLEAN             _WaitPerformed;
    KSpinLock           _SpinLock;
    ULONGLONG            _WorkItemExecutionTimeBoundInMilliseconds;
    LONGLONG            _WorkItemExecutionTooLongCount;

    #if KTL_USER_MODE
        struct HandleRegistrationContext
        {
            KListEntry           ListEntry;
            IoCompletionCallback Callback;
            VOID*                Context;
        };

        HANDLE                               _CompletionPort;
        KNodeList<HandleRegistrationContext> _HandleRegistrationList;
    #else
        KSemaphore                          _Semaphore;
        WorkQueue                           _Queue;
    #endif
};

NTSTATUS
KThreadPool::Create(
    __out KThreadPool::SPtr& ThreadPool,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in ULONG MaximumNumberOfThreads
    )

/*++

Routine Description:

    This routine creates a new thread pool, passing back a smart pointer to it.

Arguments:

    ThreadPool              - Returns a smart pointer to the newly created thread pool.

    AllocationTag           - Supplies the pool tag for the allocation.

    MaximumNumberOfThreads  - Supplies the maximum number of threads that this pool will have.  If 0 is supplied here then
                                the maximum number of threads will be equal to the number of processors.

    OverflowWorkPool        - Identifies another KThreadPool that will receive work item in the case where all dedicated
                              threads are detached. This optional parameter must be supplied to enable the detached thread
                              behaviors.

Return Value:

    NTSTATUS

--*/

{
    KThreadPoolStandard* threadPool;
    NTSTATUS status;

    // Limit number of used threads to no more than 32 until underlying PAL support is added for > 32
    MaximumNumberOfThreads = __min(MaximumNumberOfThreads, 32);

    threadPool = _new(AllocationTag, Allocator) KThreadPoolStandard(MaximumNumberOfThreads, AllocationTag);
    if (!threadPool)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = threadPool->Status();

    if (!NT_SUCCESS(status))
    {
        _delete(threadPool);
        return status;
    }

    ThreadPool = threadPool;

    return STATUS_SUCCESS;
}
 
KThreadPoolStandard::KThreadPoolStandard(
    __in ULONG MaximumNumberOfThreads,
    __in ULONG AllocationTag
    )
    #if KTL_USER_MODE
        :   _HandleRegistrationList(FIELD_OFFSET(HandleRegistrationContext, ListEntry)),
            _Threads(GetThisAllocator())
    #else
        :   _Threads(GetThisAllocator())
    #endif
{
    Zero();
    SetConstructorStatus(Initialize(MaximumNumberOfThreads, AllocationTag));
}
 
KThreadPoolStandard::~KThreadPoolStandard()
{
    Cleanup();
}
 
VOID
KThreadPoolStandard::Zero()
{
    _NumberOfThreads = 0;
    _CacheAlignedState = NULL;
    _FirstCacheAlignedState = NULL;
    _CacheAlignedStateSize = 0;
    _NumberOfWorkerStates = 0;
    _PerWorkerStateSize = 0;
    _WaitPerformed = FALSE;
    _WorkItemExecutionTimeBoundInMilliseconds = 0;
    _WorkItemExecutionTooLongCount = 0;
    #if KTL_USER_MODE
        _CompletionPort = NULL;
    #endif
}
 
VOID
KThreadPoolStandard::Cleanup()

/*++

Routine Description:

    This routine cleans up the thread pool.

Arguments:

    None.

Return Value:

    None.

--*/

{
    Wait();

    _delete(_CacheAlignedState);

    #if KTL_USER_MODE
    {
        if (_CompletionPort)
        {
            CloseHandle(_CompletionPort);
        }

        while (_HandleRegistrationList.Count())
        {
            HandleRegistrationContext* registrationContext = _HandleRegistrationList.RemoveHead();
            _delete(registrationContext);
        }
    }
    #endif
}

NTSTATUS
KThreadPoolStandard::Initialize(
    __in ULONG MaximumNumberOfThreads,
    __in ULONG AllocationTag)

/*++

Routine Description:

    This routine initializes the thread pool.

Arguments:

    MaximumNumberOfThreads  - Supplies, optionally, the maximum number of threads.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status = STATUS_SUCCESS;
    ULONG                   maximumProcessors;
    ULONG                   highestProcessorIndex;
    ULONG                   cacheLineSize;
    ULONG                   i;
    KPerWorkerThreadState*  threadState;
    KThread::SPtr           thread;

    // Check to see that the sub-objects got constructed correctly.
    if (!NT_SUCCESS(_Threads.Status()))
    {
        return _Threads.Status();
    }

    #if KTL_USER_MODE
    #else
        if (!NT_SUCCESS(_Semaphore.Status()))
        {
            return _Semaphore.Status();
        }
    #endif

    // Figure out a maximum number of processors that can be used for this thread pool.  In kernel mode, we can
    // use threads from accross all processor groups.  In user mode, we can use threads within the processor group
    // assigned to this process.
    maximumProcessors = QueryMaximumProcessors();

    // Figure out the cache line size.
    status = QueryCacheLineSize(cacheLineSize, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        //
        // If the cache line size cannot be determined then assume 64
        //
        cacheLineSize = 64;
        status = STATUS_SUCCESS;
    }

    // Figure out how large to make the per-worker state array.
    highestProcessorIndex = 0;
    _NumberOfThreads = 0;

    for (i = 0; i < maximumProcessors; i++)
    {
        if (!DoesProcessorExist(i))
        {
            continue;
        }

        highestProcessorIndex = i;
        _NumberOfThreads++;
        if (MaximumNumberOfThreads && _NumberOfThreads == MaximumNumberOfThreads)
        {
            break;
        }
    }

    // Allocate the per-worker state array, align it to the cache line size.
    _PerWorkerStateSize = (sizeof(KPerWorkerThreadState) + cacheLineSize - 1)/cacheLineSize*cacheLineSize;
    _NumberOfWorkerStates = highestProcessorIndex + 1;

    HRESULT hr;
    ULONG result1;
    ULONG result2;
    hr = ULongMult(_NumberOfWorkerStates, _PerWorkerStateSize, &result1);
    KInvariant(SUCCEEDED(hr));
    hr = ULongSub(cacheLineSize, 1, &result2);
    KInvariant(SUCCEEDED(hr));
    hr = ULongAdd(result1, result2, &_CacheAlignedStateSize);
    KInvariant(SUCCEEDED(hr));

    _CacheAlignedState = _new(AllocationTag, GetThisAllocator()) UCHAR[_CacheAlignedStateSize];
    if (!_CacheAlignedState)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(_CacheAlignedState, _CacheAlignedStateSize);
    _FirstCacheAlignedState = (VOID*) ((((ULONG_PTR) _CacheAlignedState) + cacheLineSize - 1)/cacheLineSize*cacheLineSize);

    #if KTL_USER_MODE
        // Create the completion port.
        _CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _NumberOfThreads);
        if (!_CompletionPort)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    #endif

    // Iterate through the processors again, creating processor affinity threads.
    KEvent                      threadReadySignal(FALSE, FALSE);
    WorkerThreadStartupInfo     startupInfo;

    startupInfo.ReadyEvent = &threadReadySignal;
    _NumberOfThreads = 0;

    for (i = 0; i < maximumProcessors; i++)
    {
        // Don't go beyond the bounds of the allocated array, even if a processor that we are counting on
        // gets yanked from the system between the previous loop and this one.
        if (i > highestProcessorIndex)
        {
            break;
        }

        // Check to see if there is a processor for the given index.
        if (!DoesProcessorExist(i))
        {
            continue;
        }

        threadState = (KPerWorkerThreadState*) ((PCHAR) _FirstCacheAlignedState + i*_PerWorkerStateSize);
        threadState->ThreadPool = this;
        threadState->ProcessorIndex = (USHORT)i;
        threadState->ThreadIsDetached = FALSE;

        startupInfo.ThreadState = threadState;

        status = KThread::Create(KThreadPoolStandard::Worker, &startupInfo, thread, GetThisAllocator(), AllocationTag);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        // Wait till the thread has inited itself @ *threadState
        KInvariant(threadReadySignal.WaitUntilSet());

        status = _Threads.Append(thread);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        // Quit once we have the maximum number of threads requested.
        _NumberOfThreads++;
        if (MaximumNumberOfThreads && _NumberOfThreads >= MaximumNumberOfThreads)
        {
            break;
        }
    }

    return STATUS_SUCCESS;
}

ULONG
KThreadPoolStandard::QueryMaximumProcessors()

/*++

Routine Description:

    This routine returns the maximum number of processors to be considered for the thread pool.

Arguments:

    None.

Return Value:

    The maximum number of processors to be considered for the thread pool.

--*/

{
    ULONG maximumProcessors;

    #if KTL_USER_MODE
        maximumProcessors = sizeof(DWORD_PTR)*8;
    #else
        //
        // BUGBUG
        // This code does not correctly accommodate changes to CPU count in hot-add CPU environments.
        // It cannot use processors in non-zero groups either.
        //
        // It is a work-around for a bug in KeGetProcessorNumberFromIndex()
        // that always returns success for processor indexes higher than KeNumberProcessors.
        //
        maximumProcessors = KeQueryActiveProcessorCountEx(0);
    #endif

    // Limit the max proc count to 32 until underlying PAL... is updated to support more
    maximumProcessors = __min(maximumProcessors, 32);

    return maximumProcessors;
}

NTSTATUS
KThreadPoolStandard::FixProcessor(__in ULONG ProcessorIndex)

/*++

Routine Description:

    This routine sets the thread to run on the given processor, and only the given processor.

Arguments:

    ProcessorIndex  - Supplies the processor index.

Return Value:

    NTSTATUS

--*/

{
        KInvariant(ProcessorIndex < 32);            // Guarantee limit to 32 procs until PAL... supports more

    #if KTL_USER_MODE
        DWORD       currentThreadId;
        HANDLE      h;
        DWORD_PTR   affinityMask;
        DWORD_PTR   r = 0;

        currentThreadId = GetCurrentThreadId();
    #if !defined(PLATFORM_UNIX)
        h = OpenThread(THREAD_ALL_ACCESS, FALSE, currentThreadId);
        if (!h)
        {
            return STATUS_UNSUCCESSFUL;
        }

        affinityMask = (DWORD_PTR)(((ULONGLONG)1)<< ProcessorIndex);

        r = SetThreadAffinityMask(h, affinityMask);
        KInvariant(r);

        CloseHandle(h);
        if (!r)
        {
            return STATUS_UNSUCCESSFUL;
        }
    #else
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(ProcessorIndex, &set);
        sched_setaffinity(currentThreadId, sizeof(set), &set);
    #endif
    #else
        PROCESSOR_NUMBER    procNumber;
        GROUP_AFFINITY      affinity;

        KeGetProcessorNumberFromIndex(ProcessorIndex, &procNumber);

        RtlZeroMemory(&affinity, sizeof(affinity));
        affinity.Mask = AFFINITY_MASK(procNumber.Number);
        affinity.Group = procNumber.Group;

        KeSetSystemGroupAffinityThread(&affinity, NULL);
    #endif

    return STATUS_SUCCESS;
}
 
NTSTATUS
KThreadPoolStandard::QueryCacheLineSize(
    __out ULONG& CacheLineSize,
    __in KAllocator& Allocator)

    /*++

    Routine Description:

        This routine returns the cache line size of the processors on which this thread pool will run.

    Arguments:

        CacheLineSize   - Returns the cache line size of the processors on which this thread pool will run.

        Allocator       - Allocator to use for any internal allocations

    Return Value:

        NTSTATUS

    --*/

{
    ULONG       size;
    ULONG       n;
    ULONG       i;
    NTSTATUS    status;

#if !defined(PLATFORM_UNIX)
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION  logicalProcessorInformation;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pLogicalProcessorInformation;

    status = KNt::QuerySystemInformation(
        SystemLogicalProcessorInformation,
        &logicalProcessorInformation,
        sizeof(logicalProcessorInformation),
        &size);

    if (NT_SUCCESS(status))
    {
        if (logicalProcessorInformation.Relationship != RelationCache)
        {
            // In this case there is only one relation, and it is not the cache relation.  Fail.
            return STATUS_UNSUCCESSFUL;
        }

        CacheLineSize = logicalProcessorInformation.Cache.LineSize;
        return status;
    }

    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        pLogicalProcessorInformation = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)_new(KTL_TAG_THREAD_POOL, Allocator) UCHAR[size];
        if (!pLogicalProcessorInformation)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = KNt::QuerySystemInformation(SystemLogicalProcessorInformation, pLogicalProcessorInformation, size, &size);

        // Find the cache relation in the list of relations.
        n = size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        for (i = 0; i < n; i++)
        {
            if (pLogicalProcessorInformation[i].Relationship == RelationCache)
            {
                CacheLineSize = pLogicalProcessorInformation[i].Cache.LineSize;
                break;
            }
        }

        if (i == n)
        {
            // No cache relation was found.
            status = STATUS_UNSUCCESSFUL;
        }

        _delete(pLogicalProcessorInformation);
    }

#else
    FILE * fp = 0;
    status = S_OK;
    fp = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
    if (fp) {
        fscanf(fp, "%d", &size);
        CacheLineSize = size;
        fclose(fp);
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }
#endif

    return status;
}
 
BOOLEAN
KThreadPoolStandard::DoesProcessorExist(__in ULONG ProcessorIndex)

/*++

Routine Description:

    This routine returns whether or not there is a processor in the system with the given index.  In user mode this means
    a processor in the processor group assigned to the process.  In kernel mode this means a processor in any group.

Arguments:

    ProcessorIndex  - Supplies the index of the processor to look up.

Return Value:

    FALSE   - There is no such processor in the system.

    TRUE    - The given processor exists in the system.

--*/

{
    BOOLEAN b;

        KInvariant(ProcessorIndex < 32);        // Limit proc count to less than 33 until PAL... support added for more

    #if KTL_USER_MODE
        DWORD_PTR processorMask;
        DWORD_PTR allowedMask;

        //
        // Restrict the KTL worker threads to just the process affinity mask in
        // case the owner of the process affiitized the process.
        //
        BOOL bOk;
        DWORD_PTR systemMask;
        bOk = GetProcessAffinityMask(GetCurrentProcess(), &allowedMask, &systemMask);
        if ((! bOk) || (systemMask == 0))
        {
            //
            // If we get here then either the GetProcessAffinityMask()
            // call failed or the process is configured with multiple
            // processor groups. In either case there is no way for the
            // thread pool configuration to recover and decide which
            // processors are valid. Fail fast.
            //
            KDbgPrintf("GetProcessAffineityMask() call failed or the process is configured with multiple processor groups system mas - 0x%x GLE %d\n",
                       systemMask,
                       GetLastError());

            KInvariant(FALSE);
            return(FALSE);
        }

        processorMask = (DWORD_PTR)(((ULONGLONG)1) << ProcessorIndex);
        if (processorMask & allowedMask)
        {
            b = TRUE;
        } else {
            b = FALSE;
        }
    #else
        NTSTATUS status;
        PROCESSOR_NUMBER procNumber;

        status = KeGetProcessorNumberFromIndex(ProcessorIndex, &procNumber);
        if (NT_SUCCESS(status))
        {
            b = TRUE;
        } else {
            b = FALSE;
        }
    #endif

    return b;
}

VOID
KThreadPoolStandard::QueueWorkItem(__inout KThreadPool::WorkItem& WorkItem, __in BOOLEAN UseCurrentThreadQueue)

/*++

Routine Description:

    This routine queues the given work item to the given work queue.  In case where 'eThreadQueue' is specified but that
    this call is not being made from the this thread, the request will instead be queued to 'eHigh'.

    Each worker thread services first its own thread queue, followed by the 'eHigh', 'eNormal', 'eLow', in that order.

Arguments:

    WorkItem    - Supplies the work item.

    QueueType   - Supplies the queue type.

Return Value:

    None.

--*/

{
    WorkItem._EnqueuerCallerAddress = _ReturnAddress();         // Capture caller's return address - cleared automatically by 
                                                                // WorkItem::Dispatch() but captured on it's stack in case of dispatch fault

    // Check to see if we are at passive level.
    #if KTL_USER_MODE
    // None of our user mode code runs at any level other than passive.
    {

    #else
    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
    #endif

        // Find the thread state for the thread assigned to this processor.
        #if KTL_USER_MODE
            ULONG thisProcessorIndex = GetCurrentProcessorNumber();
        #else
            UseCurrentThreadQueue = TRUE;
            ULONG thisProcessorIndex = KeGetCurrentProcessorNumberEx(NULL);
        #endif

        // If there isn't any thread state then queue this to the central queue.
        KPerWorkerThreadState* threadState = nullptr;
        if (UseCurrentThreadQueue && (thisProcessorIndex < _NumberOfWorkerStates))
        {
            threadState = (KPerWorkerThreadState*) ((PCHAR) _FirstCacheAlignedState + thisProcessorIndex*_PerWorkerStateSize);

            // Check to see if we are running on the worker thread assigned to this processor.
            if (threadState->GetThreadId() == KThread::GetCurrentThreadId())
            {
                // Queue to this worker thread's own queue.
                threadState->PerThreadQueue.Insert(WorkItem);
                return;
            }
        }
    }

    // Not one of this pool's threads, not the right Irql, or the thread has been detached - put on central queue
    #if KTL_USER_MODE
    {
        // Queue to the completion port.  This can fail, unfortunately.  In the case of user mode and failing this call,
        // the process will just exit to be restarted later.
        KInvariant(PostQueuedCompletionStatus(_CompletionPort, 0, (ULONG_PTR) &WorkItem, NULL));
    }
    #else
    {
        // Queue to the central queue, signal the semaphore.
        _SpinLock.Acquire();
        {
            _Queue.Insert(WorkItem);
        }
        _SpinLock.Release();
        _Semaphore.Add();
    }
    #endif
}

VOID
KThreadPoolStandard::Wait()

/*++

Routine Description:

    This routine will kill all thread pool threads and wait until they have completed.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (_WaitPerformed)
    {
        return;
    }
    _WaitPerformed = TRUE;

    ULONG numThreads = _Threads.Count();

    #if KTL_USER_MODE
        for (ULONG i = 0; i < numThreads; i++)
        {
            KInvariant(PostQueuedCompletionStatus(_CompletionPort, 0, 0, NULL));
        }
    #else
        _Semaphore.Add(numThreads);
    #endif

    for (ULONG i = 0; i < numThreads; i++)
    {
        _Threads[i]->Wait();
    }
}
 
#if KTL_USER_MODE

NTSTATUS
KThreadPoolStandard::RegisterIoCompletionCallback(
    __in HANDLE Handle,
    __in IoCompletionCallback CompletionCallback,
    __in_opt VOID* Context,
    __out VOID** RegistrationContext
    )

/*++

Routine Description:

    This routine will register the given handle with this thread pool's completion port.  For every completed IO, the given
    completion callback will be called with the given context.

Arguments:

    Handle              - Supplies the handle to bind with this thread pool's completion port.

    CompletionCallback  - Supplies the completion callback.

    Context             - Supplies the completion callback context.

Return Value:

    NTSTATUS

--*/

{
    HandleRegistrationContext* registrationContext = _new(KTL_TAG_THREAD_POOL, GetThisAllocator()) HandleRegistrationContext;
    if (!registrationContext)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    registrationContext->Callback = CompletionCallback;
    registrationContext->Context = Context;

    HANDLE h = CreateIoCompletionPort(Handle, _CompletionPort, (ULONG_PTR) registrationContext, _NumberOfThreads);
    if (!h)
    {
        _delete(registrationContext);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SpinLock.Acquire();
        _HandleRegistrationList.AppendTail(registrationContext);
    _SpinLock.Release();

    *RegistrationContext = registrationContext;
    return STATUS_SUCCESS;
}

#endif
 
#if KTL_USER_MODE

VOID
KThreadPoolStandard::UnregisterIoCompletionCallback(
    __inout VOID* RegistrationContext
    )
{
    HandleRegistrationContext* registrationContext = (HandleRegistrationContext*) RegistrationContext;

    _SpinLock.Acquire();
        _HandleRegistrationList.Remove(registrationContext);
    _SpinLock.Release();

    _delete(registrationContext);
}

#endif
 
VOID
KThreadPoolStandard::ExecuteWorkItem(__in WorkItem& Work)
{
    ULONGLONG bound = _WorkItemExecutionTimeBoundInMilliseconds;

    if (!bound) 
    {
        WorkItem::Dispatch(Work);
    } 
    else 
    {
        ULONGLONG startTime = KNt::GetTickCount64();
        WorkItem::Dispatch(Work);
        ULONGLONG timeTaken = KNt::GetTickCount64() - startTime;

        if (timeTaken > bound)
        {
            #if !KTL_USER_MODE
                TRACE_ThreadHeldTooLong(NULL, timeTaken);
            #endif
            InterlockedIncrement64(&_WorkItemExecutionTooLongCount);
        }
    }
}

#if !KTL_USER_MODE
_IRQL_requires_same_
__drv_maxIRQL(PASSIVE_LEVEL)
_Function_class_(WORKER_THREAD_ROUTINE)
#endif
VOID
KThreadPoolStandard::Worker(__inout VOID* Parameter)

/*++

Routine Description:

    This routine is the thread worker routine for this thread pool.

Arguments:

    Parameter   - Supplies the per-worker thread state.

Return Value:

    None.

--*/

{
    KPerWorkerThreadState*  threadState = ((WorkerThreadStartupInfo*)Parameter)->ThreadState;
    KThreadPoolStandard*    threadPool = threadState->ThreadPool;
    NTSTATUS                status = STATUS_SUCCESS;
    KThreadPool::WorkItem*  workItem = nullptr;
    KtlSystemBase* volatile system = (KtlSystemBase*)(&threadPool->GetThisKtlSystem());

    // Set up this thread to run on the processor to which it is assigned.
    status = FixProcessor(threadState->ProcessorIndex);
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return;
    }

    // Set the thread field correctly in the thread state.
    #if KTL_USER_MODE
    {
#ifdef PLATFORM_UNIX
        InterlockedExchange((LONG*)&threadState->ThreadId, (LONG)GetCurrentThreadId());
#else
        InterlockedExchange(&threadState->ThreadId, GetCurrentThreadId());
#endif
    }
    #else
    {
        InterlockedExchangePointer((VOID**) &threadState->Thread, KeGetCurrentThread());
    }
    #endif

    // Signal this thread is ready to do work
    ((WorkerThreadStartupInfo*)Parameter)->ReadyEvent->SetEvent();
    Parameter = nullptr;

    //** Main thread loop.
    for (;;)
    {
        #if KTL_USER_MODE
        {
            DWORD       numBytes = 0;
            ULONG_PTR   completionKey = 0;
            OVERLAPPED* overlapped = nullptr;

            BOOL    b = GetQueuedCompletionStatus(threadPool->_CompletionPort, &numBytes, &completionKey, &overlapped, INFINITE);
            if (overlapped != nullptr)
            {
                // We have an overlapped structure as the result of an IO completion.  So, call the completion callback registered
                // on the handle on which the IO completed.
                HandleRegistrationContext* registrationContext = (HandleRegistrationContext*) completionKey;
                registrationContext->Callback(registrationContext->Context, overlapped, b ? NO_ERROR : GetLastError(), numBytes);

            } else {

                KInvariant(b);
                workItem = (WorkItem*)completionKey;
                if (!workItem)
                {
                    // If the workItem is empty then this indicates the signal to exit the thread.
                    break;
                }

                // Call the work item that was on the global queue.
                threadPool->ExecuteWorkItem(*workItem);
            }
        }
        #else
        {
            // Wait until the semaphore indicates that there are work items on the queue.
            threadPool->_Semaphore.Subtract();

            // Acquire the lock for the queue.
            threadPool->_SpinLock.Acquire();
            {
                // Check the central queue.
                workItem = threadPool->_Queue.Remove();
            }
            // Release the queue lock.
            threadPool->_SpinLock.Release();

            // If the queue is empty then this indicates the signal to exit the thread.
            if (!workItem)
            {
                break;
            }

            // Call the work item that was on the global queue.
            threadPool->ExecuteWorkItem(*workItem);
        }
        #endif

        // The work item might have generated more per-thread-queue work, continue to process this work until complete.
        for (;;)
        {
            // This thread queue is only accessed by this thread, on this processor, at passive level.  So, it is
            // perfectly safe to manipulate this queue without a lock.
            workItem = threadState->PerThreadQueue.Remove();
            if (!workItem)
            {
                break;
            }

            // Call the induced work item.
            threadPool->ExecuteWorkItem(*workItem);
        }
    }

    KInvariant(system != nullptr);
}
 
BOOLEAN
KThreadPoolStandard::IsCurrentThreadOwned()
{
    ULONG   thisProcessorIndex;

    // Find the thread state for the thread assigned to this processor.
    #if KTL_USER_MODE
        thisProcessorIndex = GetCurrentProcessorNumber();
    #else
        thisProcessorIndex = KeGetCurrentProcessorNumberEx(NULL);
    #endif

    if (thisProcessorIndex >= _NumberOfWorkerStates)
    {
        return FALSE;
    }

    KPerWorkerThreadState*  threadState =
        (KPerWorkerThreadState*) ((PCHAR) _FirstCacheAlignedState + thisProcessorIndex * _PerWorkerStateSize);

    return (threadState->GetThreadId() == KThread::GetCurrentThreadId());
}

BOOLEAN
KThreadPoolStandard::SetThreadPriority(__in KThread::Priority Priority)
{
    for (ULONG i = 0; i < _Threads.Count(); i++)
    {
        BOOLEAN b = _Threads[i]->SetThreadPriority(Priority);
        if (!b)
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
KThreadPoolStandard::SetWorkItemExecutionTimeBound(__in ULONG BoundInMilliSeconds)
{
    // The bound can only be set once at the beginning, synchronization is not a concern
    KInvariant(_WorkItemExecutionTimeBoundInMilliseconds== 0);
    _WorkItemExecutionTimeBoundInMilliseconds = BoundInMilliSeconds;
    return TRUE;
}

LONGLONG
KThreadPoolStandard::GetWorkItemExecutionTooLongCount()
{
    return InterlockedCompareExchange64(&_WorkItemExecutionTooLongCount, 0, 0);
}
#pragma endregion ** Standard KTL ThreadPool implementation - Used by KAsyncContextBase

 
#pragma region ** System KSystemThreadPool implementation
class KSystemThreadPoolStandard : public KThreadPool
{
    K_FORCE_SHARED(KSystemThreadPoolStandard);

    public:

        VOID
        QueueWorkItem(__inout WorkItem& WorkItem, __in BOOLEAN UseCurrentThreadQueue) override;

        VOID
        Wait() override;

        BOOLEAN
        SetThreadPriority(__in KThread::Priority Priority) override
        {
            UNREFERENCED_PARAMETER(Priority);
            return FALSE;
        }

        BOOLEAN
        SetWorkItemExecutionTimeBound(__in ULONG BoundInMilliSeconds) override
        {
            UNREFERENCED_PARAMETER(BoundInMilliSeconds);
            return FALSE;
        }

        LONGLONG
        GetWorkItemExecutionTooLongCount() override
        {
            return 0;
        }

        #if KTL_USER_MODE
            BOOLEAN
            DetachCurrentThread() override;

            BOOLEAN
            IsCurrentThreadOwnedSupported() override { return TRUE; }

            BOOLEAN
            IsCurrentThreadOwned() override;
        #endif

        ULONG 
        GetThreadCount() override { return 0; }

    private:
        FAILABLE
        KSystemThreadPoolStandard(__in ULONG AllocationTag, __in ULONG MaxActive);

        VOID
        Zero();

        VOID
        Cleanup();

        NTSTATUS
        Initialize(__in ULONG AllocationTag, __in ULONG MaxActive);

        #if KTL_USER_MODE
            // TLS Support
            __declspec(align(8)) struct KtlTlsUsage
            {
                ULONG volatile  AllocatedSlot;
                LONG volatile   RefCount;

                __inline KtlTlsUsage volatile& operator=(KtlTlsUsage volatile& CopyFrom)
                {
                    *((ULONGLONG volatile*)this) = (ULONGLONG volatile&)CopyFrom;
                    return *this;
                }

                __inline KtlTlsUsage(KtlTlsUsage volatile& CopyFrom)
                {
                    *this = CopyFrom;
                }
            };
            static_assert(sizeof(KtlTlsUsage) == sizeof(LONGLONG), "KtlTlsUsage - LONGLONG size mispatch");

            static ULONG KtlTlsUsageStorage[sizeof(KtlTlsUsage) / sizeof(ULONG)];
            static KtlTlsUsage volatile& g_KtlTlsUsage;
            BOOLEAN                      _TtlSlotAllocated;

            NTSTATUS
            TryAcquireTLSSlot();

            void
            ReleaseTLSSlot();

            struct PerThreadState
            {
                K_DENY_COPY(PerThreadState);

            public:
                WorkQueue                           _WorkQueue;
                BOOLEAN                             _IsDetached;
                KSystemThreadPoolStandard&          _ParentThreadPool;
                TP_CALLBACK_INSTANCE&               _CallbackInstance;

                PerThreadState(KSystemThreadPoolStandard& ParentThreadPool, TP_CALLBACK_INSTANCE& CallbackInstance)
                    :   _ParentThreadPool(ParentThreadPool),
                        _CallbackInstance(CallbackInstance),
                        _IsDetached(FALSE)
                {
                }

                void
                Detach();

                ~PerThreadState()
                {
                    // _WorkQueue must be empty!
                    KInvariant(_WorkQueue.Remove() == nullptr);
                }
            };

            using KThreadPool::SetLink;
            using KThreadPool::GetLink;

            // UM Worker entry
            static
            VOID
            Worker(
                __inout TP_CALLBACK_INSTANCE* Instance,
                __inout_opt VOID* Context,
                __inout TP_WORK* Work);

        #else
            _IRQL_requires_same_
            __drv_maxIRQL(PASSIVE_LEVEL)
            _Function_class_(WORKER_THREAD_ROUTINE)
            static
            VOID
            Worker(__inout VOID* Context);
        #endif

        struct PreAllocatedWorkItem
        {
            KListEntry ListEntry;
            WorkItem* WorkItem;
            KSystemThreadPoolStandard* ThreadPool;
            void* QueueWorkItemCaller;

            #if KTL_USER_MODE
                TP_WORK*        HostWorkItem;
                BOOLEAN         UseThreadQueue;
            #else
                WORK_QUEUE_ITEM HostWorkItem;
            #endif
        };

        friend class KThreadPool;
        friend struct PerThreadState;

        KSpinLock _SpinLock;
        WorkQueue _Queue;
        ULONG _RefCount;
        BOOLEAN _Draining;
        KEvent _Drained;
        KNodeList<PreAllocatedWorkItem> _PreAllocatedWorkItems;

        #if defined(PLATFORM_UNIX)
        BOOL
        CallbackMayRunLong(
            __inout PTP_CALLBACK_INSTANCE pci
            );
        #endif
};

#if KTL_USER_MODE
 __declspec(align(8)) ULONG KSystemThreadPoolStandard::KtlTlsUsageStorage[sizeof(KtlTlsUsage) / sizeof(ULONG)] = { TLS_OUT_OF_INDEXES, 0 };
KSystemThreadPoolStandard::KtlTlsUsage volatile& KSystemThreadPoolStandard::g_KtlTlsUsage =
                       *((KSystemThreadPoolStandard::KtlTlsUsage *)&KSystemThreadPoolStandard::KtlTlsUsageStorage);

NTSTATUS
KSystemThreadPoolStandard::TryAcquireTLSSlot()
{
    ULONG   ktlTlsSlotAllocated = TLS_OUT_OF_INDEXES;
    KFinally([&] () -> void
    {
        // If we allocated a slot but did not us it - free it back
        if (ktlTlsSlotAllocated != TLS_OUT_OF_INDEXES)
        {
            TlsFree(ktlTlsSlotAllocated);
        }
    });

    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        // Compute desired value of g_KtlTlsUsage
        KtlTlsUsage    currentValue(g_KtlTlsUsage);
        KtlTlsUsage    newValue(currentValue);

        if (currentValue.RefCount == 0)
        {
            KInvariant(currentValue.AllocatedSlot == TLS_OUT_OF_INDEXES);

            // May need new slot - we appear to be the first thread here in the process
            if (ktlTlsSlotAllocated == TLS_OUT_OF_INDEXES)
            {
                // Looks that way - maybe - still racing however - try and allocate slot
                ktlTlsSlotAllocated = TlsAlloc();
                if (ktlTlsSlotAllocated == TLS_OUT_OF_INDEXES)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            newValue.AllocatedSlot = ktlTlsSlotAllocated;
        }
        else
        {
            KInvariant(currentValue.AllocatedSlot != TLS_OUT_OF_INDEXES);
        }
        newValue.RefCount++;

        // Try to establish new process-wide tls state (higher ref count + potentially new allocated slot)
        if (InterlockedCompareExchange64(
            (LONGLONG volatile*)&g_KtlTlsUsage,
            *((LONGLONG*)&newValue),
            *((LONGLONG*)&currentValue)) == *(LONGLONG*)&currentValue)
        {
            // state set
            if (currentValue.RefCount == 0)
            {
                ktlTlsSlotAllocated = TLS_OUT_OF_INDEXES;
            }
            return STATUS_SUCCESS;
        }
    };

    KInvariant(FALSE);
    return STATUS_SUCCESS;
}

void
KSystemThreadPoolStandard::ReleaseTLSSlot()
{
    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        // Compute desired value of g_KtlTlsUsage
        KtlTlsUsage    currentValue(g_KtlTlsUsage);
        KtlTlsUsage    newValue(currentValue);

        KInvariant(newValue.RefCount > 0);
        KInvariant(newValue.AllocatedSlot != TLS_OUT_OF_INDEXES);
        newValue.RefCount--;
        if (newValue.RefCount == 0)
        {
            newValue.AllocatedSlot = TLS_OUT_OF_INDEXES;
        }

        // Try to establish new process-wide tls state (lower ref count + potentially deallocated slot)
        if (InterlockedCompareExchange64(
            (LONGLONG volatile*)&g_KtlTlsUsage,
            *((LONGLONG*)&newValue),
            *((LONGLONG*)&currentValue)) == *((LONGLONG*)&currentValue))
        {
            // state set - if last one out free slot
            if (newValue.RefCount == 0)
            {
                TlsFree(currentValue.AllocatedSlot);
            }
            return;
        }
    };

    KInvariant(FALSE);
    return;
}

void
KSystemThreadPoolStandard::PerThreadState::Detach()
{
    if (!_IsDetached)
    {
        _IsDetached = TRUE;

        // Requeue any pending work - will avoid the per-thread queue given _IsDetached is true
        WorkItem*   currentWorkItem = _WorkQueue.PeekFront();
        _WorkQueue.Reset();
        while (currentWorkItem != nullptr)
        {
            WorkItem*   nextWorkItem = KSystemThreadPoolStandard::GetLink(*currentWorkItem);
            KSystemThreadPoolStandard::SetLink(*currentWorkItem, currentWorkItem);
            _ParentThreadPool.QueueWorkItem(*currentWorkItem, TRUE);

            currentWorkItem = nextWorkItem;
        }
    }
}
#endif
 
NTSTATUS
KThreadPool::CreateSystem(
    __out KThreadPool::SPtr& ThreadPool,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in ULONG MaxActive)

/*++

Routine Description:

    This rouitne creates a new system thread pool, passing back a smart pointer to it.

Arguments:

    ThreadPool              - Returns a smart pointer to the newly created thread pool.

    AllocationTag           - Supplies the pool tag for the allocation.

    MaxActive               - Supplies the maximum number of simultaneously running work items.

Return Value:

    NTSTATUS

--*/

{
    KSystemThreadPoolStandard* threadPool = _new(AllocationTag, Allocator) KSystemThreadPoolStandard(AllocationTag, MaxActive);
    if (!threadPool)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = threadPool->Status();
    if (!NT_SUCCESS(status))
    {
        _delete(threadPool);
        return status;
    }

    ThreadPool = threadPool;
    return STATUS_SUCCESS;
}
 
KSystemThreadPoolStandard::KSystemThreadPoolStandard(
    __in ULONG AllocationTag,
    __in ULONG MaxActive)
    :   _PreAllocatedWorkItems(FIELD_OFFSET(PreAllocatedWorkItem, ListEntry))
{
    Zero();
    SetConstructorStatus(Initialize(AllocationTag, MaxActive));
}
 
KSystemThreadPoolStandard::~KSystemThreadPoolStandard()
{
    Cleanup();
}

VOID
KSystemThreadPoolStandard::Zero()
{
    #if KTL_USER_MODE
    {
        _TtlSlotAllocated = FALSE;
    }
    #endif

    _RefCount = 0;
    _Draining = FALSE;
}
 
#pragma warning(push)
#pragma warning( disable : 4996 )
NTSTATUS
KSystemThreadPoolStandard::Initialize(__in ULONG AllocationTag, __in ULONG MaxActive)

/*++

Routine Description:

    This routine initializes this standard system thread pool object.

Arguments:

    AllocationTag   - Supplies the allocation tag.

    Allocator       - Supplies the allocator.

    MaxActive       - Supplies the maximum number of simultaneously running work items.

Return Value:

    NTSTATUS

--*/

{
    const ULONG MaxActiveLimit = 256;

    if (!NT_SUCCESS(_Drained.Status()))
    {
        return _Drained.Status();
    }

    #if KTL_USER_MODE
    {
        NTSTATUS    status = TryAcquireTLSSlot();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
        _TtlSlotAllocated = TRUE;
    }
    #endif

    // Set the maximum number of simultaneously running work items to within some reasonable limits.
    if (!MaxActive)
    {
        MaxActive = 1;
    }
    if (MaxActive > MaxActiveLimit)
    {
        MaxActive = MaxActiveLimit;
    }

    // Allocate the maximum number of host work items needed in advance, for better perf and for NOFAIL.
    for (ULONG i = 0; i < MaxActive; i++)
    {
        PreAllocatedWorkItem*  preAllocatedWorkItem = _new(AllocationTag, GetThisAllocator()) PreAllocatedWorkItem();
        if (!preAllocatedWorkItem)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        preAllocatedWorkItem->WorkItem = nullptr;
        preAllocatedWorkItem->ThreadPool = this;

        #if KTL_USER_MODE
        {
            preAllocatedWorkItem->UseThreadQueue = FALSE;
            preAllocatedWorkItem->QueueWorkItemCaller = nullptr;
            preAllocatedWorkItem->HostWorkItem = CreateThreadpoolWork(Worker, preAllocatedWorkItem, NULL);
            if (!preAllocatedWorkItem->HostWorkItem)
            {
                _delete(preAllocatedWorkItem);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        #else
        {
            ExInitializeWorkItem(&preAllocatedWorkItem->HostWorkItem, Worker, preAllocatedWorkItem);
        }
        #endif

        _PreAllocatedWorkItems.AppendTail(preAllocatedWorkItem);
    }

    return STATUS_SUCCESS;
}
#pragma warning(pop)

VOID
KSystemThreadPoolStandard::Cleanup()

/*++

Routine Description:

    This routine cleans up the given thread pool.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PreAllocatedWorkItem* preAllocatedWorkItem;

    Wait();

    while (_PreAllocatedWorkItems.Count())
    {
        preAllocatedWorkItem = _PreAllocatedWorkItems.RemoveHead();

        #if KTL_USER_MODE
        {
            WaitForThreadpoolWorkCallbacks(preAllocatedWorkItem->HostWorkItem, NULL);
            CloseThreadpoolWork(preAllocatedWorkItem->HostWorkItem);
        }
        #endif

        _delete(preAllocatedWorkItem);
    }

    #if KTL_USER_MODE
    {
        if (_TtlSlotAllocated)
        {
            _TtlSlotAllocated = FALSE;
            ReleaseTLSSlot();
        }
    }
    #endif

}

#if KTL_USER_MODE
BOOLEAN
KSystemThreadPoolStandard::IsCurrentThreadOwned()
{
    ULONG		allocatedSlot = KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot;

    if (allocatedSlot != TLS_OUT_OF_INDEXES)
    {
        // We have a TLS slot - recover this thread's State - on the thread's stack
        PerThreadState*     pts = (PerThreadState*)TlsGetValue(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot);
        if ((pts != nullptr) && !pts->_IsDetached)
        {
            // It is a thread with our slot used and this thread is NOT detached - it is an assigned thread
            return TRUE;
        }
    }

    return FALSE;
}
#endif

#pragma warning(push)
#pragma warning( disable : 4996 )
VOID
KSystemThreadPoolStandard::QueueWorkItem(__inout WorkItem& WorkItem, __in BOOLEAN UseCurrentThreadQueue)

/*++

Routine Description:

    This routine queues a work item to the given queue.

Arguments:

    WorkItem    - Supplies the work item.

    Type        - Supplies the queue type.

Return Value:

    None.

--*/

{
    WorkItem._EnqueuerCallerAddress = _ReturnAddress();         // Capture caller's return address - cleared automatically by 
                                                                // WorkItem::Dispatch() but captured on it's stack in case of dispatch fault

    #if KTL_USER_MODE
    {
        // Check to see if this thread has a tls-based work queue - if it does, then just add the work item there and return, else...
        //
        if (UseCurrentThreadQueue && (KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot != TLS_OUT_OF_INDEXES))
        {
            PerThreadState*     pts = (PerThreadState*)TlsGetValue(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot);
            if ((pts != nullptr) && !pts->_IsDetached)
            {
                // Add work item to this thread's own queue - iif attached
                pts->_WorkQueue.Insert(WorkItem);
                return;
            }
        }
    }
    #else
    {
        UNREFERENCED_PARAMETER(UseCurrentThreadQueue);
    }
    #endif

    PreAllocatedWorkItem* preAllocatedWorkItem;

    _SpinLock.Acquire();
    {
        _RefCount++;

        if (_PreAllocatedWorkItems.Count() == 0)
        {
            // There are no available work items.  This means that 'MaxActive' threads are being used.  So, just queue
            // the request to be handled by one of the already active threads.
            _Queue.Insert(WorkItem);
            _SpinLock.Release();
            return;
        }

        // There is an available work item.  Use it and send the work item on its way.
        preAllocatedWorkItem = _PreAllocatedWorkItems.RemoveTail();
    }
    _SpinLock.Release();

    preAllocatedWorkItem->WorkItem = &WorkItem;

    #if KTL_USER_MODE
    {
        preAllocatedWorkItem->UseThreadQueue = UseCurrentThreadQueue;
        preAllocatedWorkItem->QueueWorkItemCaller = _ReturnAddress();
        SubmitThreadpoolWork(preAllocatedWorkItem->HostWorkItem);
    }
    #else
    {
        ExQueueWorkItem(&preAllocatedWorkItem->HostWorkItem, DelayedWorkQueue);
    }
    #endif
}
#pragma warning(pop)

VOID
KSystemThreadPoolStandard::Wait()

/*++

Routine Description:

    This routine makes it so that no new work can be queued up and waits until all existing work has finished.

Arguments:

    None.

Return Value:

    None.

--*/

{
    // Set the _Drain value to TRUE and then wait for all requests to complete.
    _SpinLock.Acquire();
    {
        if (!_Draining)
        {
            _Draining = TRUE;
            if (_RefCount == 0)
            {
                _Drained.SetEvent();
            }
        }
    }
    _SpinLock.Release();

    _Drained.WaitUntilSet();
}
 
 
#if KTL_USER_MODE
VOID
KSystemThreadPoolStandard::Worker(
    __inout TP_CALLBACK_INSTANCE* Instance,
    __inout_opt VOID* Context,
    __inout TP_WORK* Work)
#else

_IRQL_requires_same_
__drv_maxIRQL(PASSIVE_LEVEL)
_Function_class_(WORKER_THREAD_ROUTINE)
VOID
KSystemThreadPoolStandard::Worker(__inout VOID* Context)
#endif

/*++

Routine Description:

    This is the thread pool worker routine.

Arguments:

    Instance    - Supplies the callback instance.

    Context     - Supplies the callback context.

    Work        - Supplies the TP_WORK structure.

Return Value:

    None.

--*/

{
    #if KTL_USER_MODE
    {
        UNREFERENCED_PARAMETER(Work);
    }
    #endif

    PreAllocatedWorkItem*       preAllocatedWorkItem = (PreAllocatedWorkItem*) Context;
    KSystemThreadPoolStandard*  threadPool = preAllocatedWorkItem->ThreadPool;
    WorkItem*                   workItem = preAllocatedWorkItem->WorkItem;
    BOOLEAN                     signalDrained = FALSE;

    // Take the work - item
    KInvariant(workItem);
    preAllocatedWorkItem->WorkItem = NULL;

    #if KTL_USER_MODE
        // Establish this threads tls-based state descriptor - provides per thread (lock free work queue)
        PerThreadState          threadState(*threadPool, *Instance);
        KAssert(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot != TLS_OUT_OF_INDEXES);
        KAssert(TlsGetValue(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot) == nullptr);

        // Only enable thread-queued work items if this work item was scheduled as such
        if (preAllocatedWorkItem->UseThreadQueue)
        {
            KInvariant(TlsSetValue(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot, &threadState));
        }
        else
        {
            // Otherwise act as detached thread
            threadState._IsDetached = TRUE;
        }
    #endif

    // Just execute the work item.
    WorkItem::Dispatch(*workItem);

    // Return this work item to the pool.
    preAllocatedWorkItem->QueueWorkItemCaller = nullptr;
    threadPool->_SpinLock.Acquire();
    {
        // NOTE: We do not release the ref count on threadPool->_RefCount yet.
        //       Semantically we are and then taking another to represent the
        //       life of this worker thread.
        //
        // threadPool->_RefCount++;         // Hold ref for life of this worker thread callout
        // threadPool->_RefCount--;         // drop ref for

        #pragma prefast(suppress:28183, "preAllocatedWorkItem cannot be null")
        threadPool->_PreAllocatedWorkItems.AppendTail(preAllocatedWorkItem);
    }
    threadPool->_SpinLock.Release();

    // Check for pending work items and for draining.
    ULONG       toReleaseCount = 1;     // Note: The '1' represets the ref count held by this thread
    for (;;)
    {
        #if KTL_USER_MODE
        if (!threadState._IsDetached)
        {
            for (;;)
            {
                // The work item might have generated more per-thread-queue work, continue to process this work until complete.

                // This thread queue is only accessed by this thread, on this processor.  So, it is perfectly safe to
                // manipulate this queue without a lock.
                workItem = threadState._WorkQueue.Remove();
                if (workItem == nullptr)
                {
                    break;
                }

                // Dispatch work item
                WorkItem::Dispatch(*workItem);
                if (threadState._IsDetached)
                {
                    KAssert(threadState._WorkQueue.PeekFront() == nullptr);

                    // TODO: consider cleaning the thread if we are going to use it more -which we are now;
                    //       Leaning toward the ReAttach() option
                    break;
                }
            }

            // Automatically detach the thread once all thread-queued work items have been dispatched. NOTE: This
            // is being done here so that a triggering and explictly non-thread-queued work item (the default
            // for QueueWorkItem()) will automatically disable the queued feature. In other words the burden is on
            // those that call QueueWorkItem(..., TRUE) to not keep their thread or detach explicitly. Those
            // that call QueueWorkItem(..., FALSE) - the defualt - will result in a thread that is detached here.
            //
            // NOTE: This is done here because we lean toward adding ReAttach() at some point.
            threadState._IsDetached = TRUE;
        }
        #endif

        threadPool->_SpinLock.Acquire();
        {
            // Check to see if there is a work item on the common queue waiting to run.
            workItem = threadPool->_Queue.Remove();
            if (workItem == nullptr)
            {
                break;
            }
        }
        threadPool->_SpinLock.Release();

        // If such a work item exists, just execute it.
        WorkItem::Dispatch(*workItem);
        toReleaseCount++;
    }

    // All work items that this thread can process have been executed - none todo on either shared or
    // this thread's per thread queue (UM-only) or this thread has been detached (UM-only)
    KInvariant(threadPool->_RefCount >= toReleaseCount);
    threadPool->_RefCount -= toReleaseCount;

    // There is nothing pending, at this point just decrement the ref count and check for draining.
    signalDrained = (threadPool->_Draining && (threadPool->_RefCount == 0));

    #if KTL_USER_MODE
    {
        // clear tls-based thread state
        KInvariant(TlsSetValue(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot, nullptr));
    }
    #endif

    threadPool->_SpinLock.Release();

    // Signal the pool is drained after we release the lock.
    // Once we set the event, this thread pool object can be destructed at any moment.
    // It will no longer be safe to access any threadPool internal data structure.
    if (signalDrained)
    {
        threadPool->_Drained.SetEvent();
    }
}

#if KTL_USER_MODE
BOOLEAN
KSystemThreadPoolStandard::DetachCurrentThread()
{
    // Check to see if this thread has a tls-based work queue - if it does, then just add the work item there and return, else...
    //
    if (KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot != TLS_OUT_OF_INDEXES)
    {
        PerThreadState*     pts = (PerThreadState*)TlsGetValue(KSystemThreadPoolStandard::g_KtlTlsUsage.AllocatedSlot);
        if ((pts != nullptr) && !pts->_IsDetached)
        {
            // Detach this thread from it's per-thread thread pool duties
            pts->Detach();

            // And tell the system thread pool the thread may run for a long time
            CallbackMayRunLong(&pts->_CallbackInstance);
        }
    }

    return TRUE;
}
#endif

#if defined(PLATFORM_UNIX)
BOOL
KSystemThreadPoolStandard::CallbackMayRunLong(
    PTP_CALLBACK_INSTANCE pci
    )
{
    return TRUE;
}
#endif

#pragma endregion ** System KSystemThreadPool implementation
