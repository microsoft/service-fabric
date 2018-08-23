#include <unistd.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/resource.h>
#include <pthread.h>
#include <string.h>
#include "Threadpool.h"
#include "HillClimbing.h"
#include "ThreadpoolRequest.h"
#include "Tracer.h"
#include "Win32ThreadpoolConfig.h"

typedef int                 BOOL, *PBOOL, *LPBOOL;
typedef unsigned int        DWORD,  *PDWORD, *LPDWORD;
typedef void                VOID, *PVOID, *LPVOID;
typedef VOID                *HANDLE;
typedef DWORD (__stdcall *PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#define CREATE_THREAD_DETACHED 0x80000000

HANDLE KtlCreateThread(
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        DWORD dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameter,
        DWORD dwCreationFlags,
        LPDWORD lpThreadId);

BOOL KtlCloseHandle(HANDLE hObject);

namespace KtlThreadpool{

    extern int GetThreadpoolThrottle();

    static int PerfTrace(ULONGLONG tb, ULONGLONG te, const std::string & msg, int cpuutil)
    {
        uint64_t duration = (tb <= te) ? (te - tb) : (~tb + 1 + te);
        if (duration > 1000) {
            TP_TRACE(Info, msg.c_str(), duration, cpuutil);
        }
        return 0;
    }

#define THREADPOOL_PERF_TRACE(msg, cpuutil)                                                   \
    for(uint64_t runonce = 1, t0 = GetTickCount64();                                          \
        runonce;                                                                              \
        runonce = 0, PerfTrace(t0, GetTickCount64(), msg, cpuutil))

    BOOL ThreadpoolMgr::Initialize()
    {
        NumberOfProcessors = GetNumActiveProcessors();
        CpuUtilization = ThreadpoolConfig::CpuUtilizationLow;

        ThreadAdjustmentInterval = HillClimbingConfig::SampleIntervalLow;

        WorkerSemaphore = new UnfairSemaphore(NumberOfProcessors, ThreadCounter::MaxPossibleCount);

        RetiredWorkerSemaphore = new Semaphore();
        RetiredWorkerSemaphore->Create(0);

        RecycledLists.Initialize(NumberOfProcessors);

        // initialize Worker thread settings
        DWORD forceMin = ThreadpoolConfig::ForceMinWorkerThreads;
        MinLimitTotalWorkerThreads = forceMin > 0 ? (LONG)forceMin :
                                                    (LONG)(NumberOfProcessors + 1) / ThreadpoolConfig::MinWorkerThreadsStartupFactor;

        DWORD forceMax = ThreadpoolConfig::ForceMaxWorkerThreads;
        MaxLimitTotalWorkerThreads = forceMax > 0 ? (LONG)forceMax :
                                                    (LONG)GetDefaultMaxLimitWorkerThreads(MinLimitTotalWorkerThreads);

        ThreadCounter::Counts counts;
        counts.NumActive = 0;
        counts.NumWorking = 0;
        counts.NumRetired = 0;
        counts.MaxWorking = MinLimitTotalWorkerThreads;
        WorkerCounter.counts.AsLongLong = counts.AsLongLong;

        TickCountAdjustment = ThreadpoolConfig::TickCountAdjustment;

        HillClimbingInstance.Initialize(this);

        ThreadpoolRequestInstance.Initialize(this);

        return TRUE;
    }

    void ThreadpoolMgr::EnsureInitialized()
    {
        if (IsInitialized())
        {
            return;
        }

        DWORD dwSwitchCount = 0;

    retry:

        if (InterlockedCompareExchange(&Initialization, 1, 0) == 0)
        {
            if (Initialize())
                Initialization = -1;
            else
                Initialization = 0;
        }
        else
        {
            while (Initialization != -1)
            {
                __SwitchToThread(0, ++dwSwitchCount);

                goto retry;
            }
        }
    }

    DWORD ThreadpoolMgr::GetDefaultMaxLimitWorkerThreads(DWORD minLimit)
    {
        // Calling into TransportConfig is causing deadlock
        //ULONGLONG limit = GetThreadpoolThrottle();
        ULONGLONG limit = ThreadpoolConfig::DefaultMaxWorkerThreads;

        limit = __max(limit, (ULONGLONG)minLimit);
        limit = __min(limit, (ULONGLONG)ThreadCounter::MaxPossibleCount);
        return (DWORD)limit;
    }

    BOOL ThreadpoolMgr::SetMaxThreadsHelper(DWORD MaxWorkerThreads)
    {
        BOOL result = FALSE;

        CriticalSectionHolder csh
        (&WorkerCriticalSection);

        if (MaxWorkerThreads >= (DWORD)MinLimitTotalWorkerThreads)
        {
            if (ThreadpoolConfig::ForceMaxWorkerThreads == 0)
            {
                MaxLimitTotalWorkerThreads = __min(MaxWorkerThreads, (DWORD)ThreadCounter::MaxPossibleCount);

                ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();
                while (counts.MaxWorking > MaxLimitTotalWorkerThreads)
                {
                    ThreadCounter::Counts newCounts = counts;
                    newCounts.MaxWorking = MaxLimitTotalWorkerThreads;

                    ThreadCounter::Counts oldCounts = WorkerCounter.CompareExchangeCounts(newCounts, counts);
                    if (oldCounts == counts)
                    {
                        counts = newCounts;
                    }
                    else
                    {
                        counts = oldCounts;
                    }
                }
            }
            result = TRUE;
        }
        return result;
     }

    BOOL ThreadpoolMgr::SetMaxThreads(DWORD MaxWorkerThreads)
    {
        if (IsInitialized())
        {
            return SetMaxThreadsHelper(MaxWorkerThreads);
        }

        if (InterlockedCompareExchange(&Initialization, 1, 0) == 0)
        {
            Initialize();
            Initialization = -1;

            return SetMaxThreadsHelper(MaxWorkerThreads);
        }
        else // someone else is initializing. Too late, return false
        {
            return FALSE;
        }
    }

    BOOL ThreadpoolMgr::GetMaxThreads(DWORD* MaxWorkerThreads)
    {
        if (!MaxWorkerThreads)
        {
            return FALSE;
        }

        if (IsInitialized())
        {
            *MaxWorkerThreads = (DWORD)MaxLimitTotalWorkerThreads;
        }
        else
        {
            NumberOfProcessors = GetNumActiveProcessors();

            DWORD min = ThreadpoolConfig::ForceMinWorkerThreads;
            if (min == 0)
            {
                min = NumberOfProcessors;
            }

            DWORD forceMax = ThreadpoolConfig::ForceMaxWorkerThreads;
            if (forceMax > 0)
            {
                *MaxWorkerThreads = forceMax;
            }
            else
            {
                *MaxWorkerThreads = GetDefaultMaxLimitWorkerThreads(min);
            }
        }
        return TRUE;
    }

    BOOL ThreadpoolMgr::SetMinThreads(DWORD MinWorkerThreads)
    {
        if (!IsInitialized())
        {
            if (InterlockedCompareExchange(&Initialization, 1, 0) == 0)
            {
                Initialize();
                Initialization = -1;
            }
        }

        if (IsInitialized())
        {
            CriticalSectionHolder csh(&WorkerCriticalSection);

            BOOL init_result = false;

            if (MinWorkerThreads <= (DWORD) MaxLimitTotalWorkerThreads)
            {
                if (ThreadpoolConfig::ForceMinWorkerThreads == 0)
                {
                    MinLimitTotalWorkerThreads = __min(MinWorkerThreads, (DWORD)ThreadCounter::MaxPossibleCount);

                    ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();
                    while (counts.MaxWorking < MinLimitTotalWorkerThreads)
                    {
                        ThreadCounter::Counts newCounts = counts;
                        newCounts.MaxWorking = MinLimitTotalWorkerThreads;

                        ThreadCounter::Counts oldCounts = WorkerCounter.CompareExchangeCounts(newCounts, counts);
                        if (oldCounts == counts)
                        {
                            counts = newCounts;
                            if (newCounts.MaxWorking > oldCounts.MaxWorking)
                            {
                                MaybeAddWorkingWorker();
                            }
                        }
                        else
                        {
                            counts = oldCounts;
                        }
                    }
                }
                init_result = TRUE;
            }
            return init_result;
        }
        // someone else is initializing. Too late, return false
        return FALSE;
    }

    BOOL ThreadpoolMgr::GetMinThreads(DWORD* MinWorkerThreads)
    {
        if (!MinWorkerThreads)
            return FALSE;

        if (IsInitialized())
        {
            *MinWorkerThreads = (DWORD)MinLimitTotalWorkerThreads;
        }
        else
        {
            NumberOfProcessors = GetNumActiveProcessors();

            DWORD forceMin;
            forceMin = ThreadpoolConfig::ForceMinWorkerThreads;
            *MinWorkerThreads = forceMin > 0 ? forceMin : NumberOfProcessors;
        }
        return TRUE;
    }

    BOOL ThreadpoolMgr::GetAvailableThreads(DWORD* AvailableWorkerThreads)
    {
        if (!AvailableWorkerThreads)
            return FALSE;

        if (IsInitialized())
        {
            ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();
            if (MaxLimitTotalWorkerThreads < counts.NumActive)
                *AvailableWorkerThreads = 0;
            else
                *AvailableWorkerThreads = MaxLimitTotalWorkerThreads - counts.NumWorking;
        }
        else
        {
            GetMaxThreads(AvailableWorkerThreads);
        }
        return TRUE;
    }

    int ThreadpoolMgr::TakeMaxWorkingThreadCount()
    {
        TP_ASSERT(ThreadpoolConfig::EnableWorkerTracking, "TakeMaxWorkingThreadCount: ThreadpoolConfig::EnableWorkerTracking");
        while (true)
        {
            WorkingThreadCounts currentCounts, newCounts;
            currentCounts.asLong = VolatileLoad(&WorkingThreadCountsTracking.asLong);

            newCounts = currentCounts;
            newCounts.maxWorking = 0;

            if (currentCounts.asLong == InterlockedCompareExchange(&WorkingThreadCountsTracking.asLong, newCounts.asLong, currentCounts.asLong))
            {
                // If we haven't updated the counts since the last call to TakeMaxWorkingThreadCount, then we never updated maxWorking.
                // In that case, the number of working threads for the whole period since the last TakeMaxWorkingThreadCount is the
                // current number of working threads.
                return currentCounts.maxWorking == 0 ? currentCounts.currentWorking : currentCounts.maxWorking;
            }
        }
    }

    BOOL ThreadpoolMgr::QueueUserWorkItem(LPTHREADPOOL_WORK_START_ROUTINE Function, PVOID Parameter, PVOID Context)
    {
        EnsureInitialized();

        ThreadpoolRequestInstance.QueueWorkRequest(Function, Parameter, Context);
        return TRUE;
    }

    bool ThreadpoolMgr::ShouldWorkerKeepRunning()
    {
        bool shouldThisThreadKeepRunning = true;
        ThreadCounter::Counts counts = WorkerCounter.DangerousGetDirtyCounts();
        while (true)
        {
            if (counts.NumActive <= counts.MaxWorking)
            {
                shouldThisThreadKeepRunning = true;
                break;
            }

            ThreadCounter::Counts newCounts = counts;

            newCounts.NumWorking--;
            newCounts.NumActive--;
            newCounts.NumRetired++;

            ThreadCounter::Counts oldCounts = WorkerCounter.CompareExchangeCounts(newCounts, counts);

            if (oldCounts == counts)
            {
                shouldThisThreadKeepRunning = false;
                break;
            }
            counts = oldCounts;
        }
        return shouldThisThreadKeepRunning;
    }

    void ThreadpoolMgr::AdjustMaxWorkersActive()
    {
        TP_ASSERT(ThreadAdjustmentLock.IsHeld(), "AdjustMaxWorkersActive: ThreadAdjustmentLock.IsHeld()");

        DWORD currentTicks = GetTickCount();
        LONG totalNumCompletions = VolatileLoad(&TotalCompletedWorkRequests);
        LONG numCompletions = totalNumCompletions - PriorCompletedWorkRequests;

        LARGE_INTEGER startTime = CurrentSampleStartTime;
        LARGE_INTEGER endTime;
        QueryPerformanceCounter(&endTime);

        static LARGE_INTEGER freq = {0};
        static int updateCounts = 0;

        if (freq.QuadPart == 0) {
            QueryPerformanceFrequency(&freq);
        }

        double elapsed = (double)(endTime.QuadPart - startTime.QuadPart) / freq.QuadPart;
        if (elapsed * 1000.0 >= (ThreadAdjustmentInterval / 2))
        {
            updateCounts ++;
            ThreadCounter::Counts currentCounts = WorkerCounter.GetCleanCounts();

            int oldInterval = ThreadAdjustmentInterval;
            int newInterval;
            int newMax = HillClimbingInstance.Update(currentCounts.MaxWorking, elapsed, numCompletions, &newInterval);

            //_ASSERTE(ThreadAdjustmentInterval >= HillClimbingConfig::SampleIntervalLow);
            //_ASSERTE(ThreadAdjustmentInterval <= HillClimbingConfig::SampleIntervalHigh);
            ThreadAdjustmentInterval = __max(newInterval, HillClimbingConfig::SampleIntervalLow);
            ThreadAdjustmentInterval = __min(ThreadAdjustmentInterval, HillClimbingConfig::SampleIntervalHigh);

            while (newMax != currentCounts.MaxWorking)
            {
                ThreadCounter::Counts newCounts = currentCounts;
                newCounts.MaxWorking = newMax;

                ThreadCounter::Counts oldCounts = WorkerCounter.CompareExchangeCounts(newCounts, currentCounts);
                if (oldCounts == currentCounts)
                {
                    if (newMax > oldCounts.MaxWorking)
                    {
                        MaybeAddWorkingWorker();
                    }
                    break;
                }
                else
                {
                    if (oldCounts.MaxWorking > currentCounts.MaxWorking && oldCounts.MaxWorking >= newMax)
                    {
                        break;
                    }
                    currentCounts = oldCounts;
                }
            }

            PriorCompletedWorkRequests = totalNumCompletions;
            PriorCompletedWorkRequestsTime = currentTicks;

            NextCompletedWorkRequestsTime = PriorCompletedWorkRequestsTime + ThreadAdjustmentInterval;

            CurrentSampleStartTime = endTime;

            //if (updateCounts % 100 == 0)
            //{
            //    TRACE(Info, "AdjustMaxWorkersActive: %d in %.2fms, T %.2f, QSize %d, W/A/R/M %d/%d/%d/%d. Intv %d IntvHC %d. CPU %d\n",
            //          numCompletions, elapsed * 1000, numCompletions / elapsed,
            //          ThreadpoolRequestInstance.GetPendingRequestNum(),
            //          currentCounts.NumWorking, currentCounts.NumActive, currentCounts.NumRetired,
            //          currentCounts.MaxWorking, ThreadAdjustmentInterval, newInterval, CpuUtilization);
            //}
        }
    }

    void ThreadpoolMgr::MaybeAddWorkingWorker()
    {
        if (!ThreadpoolRequestInstance.IsRequestPending())
        {
            return;
        }

        ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();
        ThreadCounter::Counts newCounts;
        while (true)
        {
            newCounts = counts;
            newCounts.NumWorking = __max(counts.NumWorking, __min(counts.NumWorking + 1, counts.MaxWorking));
            newCounts.NumActive = __max(counts.NumActive, newCounts.NumWorking);
            newCounts.NumRetired = __max(0, counts.NumRetired - (newCounts.NumActive - counts.NumActive));

            if (newCounts == counts)
            {
                return;
            }

            ThreadCounter::Counts oldCounts = WorkerCounter.CompareExchangeCounts(newCounts, counts);

            if (oldCounts == counts)
            {
                break;
            }

            counts = oldCounts;
        }

        int toUnretire = counts.NumRetired - newCounts.NumRetired;
        int toCreate = (newCounts.NumActive - counts.NumActive) - toUnretire;
        int toRelease = (newCounts.NumWorking - counts.NumWorking) - (toUnretire + toCreate);

        TP_ASSERT(toUnretire >= 0, "MaybeAddWorkingWorker: toUnretire >= 0");
        TP_ASSERT(toCreate >= 0, "MaybeAddWorkingWorker: toCreate >= 0");
        TP_ASSERT(toRelease >= 0, "MaybeAddWorkingWorker: toRelease >= 0");
        TP_ASSERT(toUnretire + toCreate + toRelease <= 1, "MaybeAddWorkingWorker: toUnretire + toCreate + toRelease <= 1");

        if (toUnretire > 0)
        {
            BOOL success = RetiredWorkerSemaphore->Release((LONG)toUnretire);
            TP_ASSERT(success, "MaybeAddWorkingWorker: RetiredWorkerSemaphore->Release()");
        }

        if (toRelease > 0)
        {
            WorkerSemaphore->Release(toRelease);
        }

        while (toCreate > 0)
        {
            if (CreateWorkerThread())
            {
                toCreate--;
            }
            else
            {
                counts = WorkerCounter.GetCleanCounts();
                while (true)
                {
                    newCounts = counts;
                    newCounts.NumWorking -= toCreate;
                    newCounts.NumActive -= toCreate;

                    ThreadCounter::Counts oldCounts = WorkerCounter.CompareExchangeCounts(newCounts, counts);

                    if (oldCounts == counts)
                    {
                        break;
                    }
                    counts = oldCounts;
                }
                toCreate = 0;
            }
        }
    }

    void ThreadpoolMgr::EnqueueWorkRequest(WorkRequest* workRequest)
    {
        AppendWorkRequest(workRequest);
    }

    WorkRequest* ThreadpoolMgr::DequeueWorkRequest()
    {
        WorkRequest* entry = RemoveWorkRequest();
        if (entry != NULL)
        {
            UpdateLastDequeueTime();
        }

        return entry;
    }

    void ThreadpoolMgr::ExecuteWorkRequest(bool* foundWork, bool* wasNotRecalled)
    {
        ThreadpoolRequestInstance.DispatchWorkItem(foundWork, wasNotRecalled);
    }

    LPVOID ThreadpoolMgr::GetRecycledMemory(enum MemType memType)
    {
        LPVOID result = NULL;
        if(RecycledLists.IsInitialized())
        {
            RecycledListInfo& list = RecycledLists.GetRecycleMemoryInfo(memType);
            result = list.Remove();
        }

        if(result == NULL)
        {
            switch (memType)
            {
                case MEMTYPE_WorkRequest:
                    result =  new WorkRequest;
                    break;

                default:
                    TP_ASSERT(0, "GetRecycledMemory: Unknown Memtype");
                    result = NULL;
                    break;
            }
        }
        return result;
    }

    void ThreadpoolMgr::RecycleMemory(LPVOID mem, enum MemType memType)
    {
        if(RecycledLists.IsInitialized())
        {
            RecycledListInfo& list = RecycledLists.GetRecycleMemoryInfo(memType);
            if(list.CanInsert())
            {
                list.Insert(mem);
                return;
            }
        }

        switch (memType)
        {
            case MEMTYPE_WorkRequest:
                delete (WorkRequest*) mem;
                break;

            default:
                TP_ASSERT(0, "Unknown Memtype");
                break;
        }
    }

    BOOL ThreadpoolMgr::CreateWorkerThread()
    {
        HANDLE hThread = KtlCreateThread(NULL, 0, (PTHREAD_START_ROUTINE)ThreadpoolMgr::WorkerThreadStart, this, CREATE_THREAD_DETACHED, NULL);

        if (hThread != INVALID_HANDLE_VALUE)
        {
            KtlCloseHandle(hThread);
        }
        else
        {
            TP_ASSERT(0, "CreateThread() cannot fail!");
        }

        return true;
    }

    void* ThreadpoolMgr::WorkerThreadStart(LPVOID lpArgs)
    {
        DWORD dwSwitchCount = 0;
        ThreadCounter::Counts counts;
        ThreadCounter::Counts oldCounts;
        ThreadCounter::Counts newCounts;
        bool foundWork = true;
        bool wasNotRecalled = true;

        int tid = GetCurrentThreadId();
        ThreadpoolMgr *pThis = (ThreadpoolMgr*)lpArgs;

    Work:

        counts = pThis->WorkerCounter.GetCleanCounts();
        while (true)
        {
            TP_ASSERT(counts.NumActive > 0, "WorkerThreadStart: counts.NumActive > 0");
            TP_ASSERT(counts.NumWorking > 0, "WorkerThreadStart: counts.NumWorking > 0");

            newCounts = counts;
            bool retired = false;
            if (counts.NumActive > counts.MaxWorking)
            {
                newCounts.NumWorking--;
                newCounts.NumActive--;
                newCounts.NumRetired++;
                retired = true;
            }
            else
            {
                retired = false;
                if (foundWork)
                {
                    break;
                }
                else
                {
                    newCounts.NumWorking--;
                }
            }

            // retire, or no work
            oldCounts = pThis->WorkerCounter.CompareExchangeCounts(newCounts, counts);
            if (oldCounts == counts)
            {
                if (retired)
                {
                    goto Retire;
                }
                else
                {
                    goto WaitForWork;
                }
            }
            counts = oldCounts;
        }

        pThis->ExecuteWorkRequest(&foundWork, &wasNotRecalled);

        if (wasNotRecalled)
        {
            goto Work;
        }

    Retire:

        counts = pThis->WorkerCounter.GetCleanCounts();

        if (pThis->ThreadpoolRequestInstance.IsRequestPending())
        {
            pThis->MaybeAddWorkingWorker();
        }

        while (true)
        {
    RetryRetire:
            // wait on retire semaphore
            bool result = pThis->RetiredWorkerSemaphore->Wait(ThreadpoolConfig::WorkerRetireSemaphoreTimeout);
            if (result)
            {
                foundWork = true;

                counts = pThis->WorkerCounter.GetCleanCounts();
                TP_ASSERT(counts.NumWorking > 0, "WorkerThreadStart: counts.NumWorking > 0");
                goto Work;
            }

            counts = pThis->WorkerCounter.GetCleanCounts();
            while (true)
            {
                // avoid a nasty race
                if (counts.NumRetired == 0)
                {
                    goto RetryRetire;
                }

                newCounts = counts;
                newCounts.NumRetired--;
                oldCounts = pThis->WorkerCounter.CompareExchangeCounts(newCounts, counts);
                if (oldCounts == counts)
                {
                    counts = newCounts;
                    break;
                }
                counts = oldCounts;
            }
            goto Exit;
        }

    WaitForWork:

        if (pThis->ThreadpoolRequestInstance.IsRequestPending())
        {
            foundWork = true;
            pThis->MaybeAddWorkingWorker();
        }

    RetryWaitForWork:

        if (!pThis->WorkerSemaphore->Wait(ThreadpoolConfig::WorkerUnfairSemaphoreTimeout))
        {
            counts = pThis->WorkerCounter.GetCleanCounts();
            while (true)
            {
                // avoid a nasty race
                if (counts.NumActive == counts.NumWorking)
                {
                    goto RetryWaitForWork;
                }

                newCounts = counts;
                newCounts.NumActive--;

                // if timed out, we need fewer threads
                newCounts.MaxWorking = __max(pThis->MinLimitTotalWorkerThreads, __min(newCounts.NumActive, newCounts.MaxWorking));
                oldCounts = pThis->WorkerCounter.CompareExchangeCounts(newCounts, counts);
                if (oldCounts == counts)
                {
                    DangerousSpinLock::Holder tal(&pThis->ThreadAdjustmentLock);
                    pThis->HillClimbingInstance.ForceChange(newCounts.MaxWorking, HillClimbing::ThreadTimedOut);
                    goto Exit;
                }
                counts = oldCounts;
            }
        }
        else
        {
            foundWork = true;
            counts = pThis->WorkerCounter.GetCleanCounts();
            TP_ASSERT(counts.NumWorking > 0, "WorkerThreadStart: counts.NumWorking > 0");
            goto Work;
        }

    Exit:
        counts = pThis->WorkerCounter.GetCleanCounts();
        return NULL;
    }

    int ThreadpoolMgr::GetCPUBusyTime_NT(TP_CPU_INFORMATION* pOldInfo)
    {
        return GetCPUBusyTime(pOldInfo);
    }

    void ThreadpoolMgr::EnsureGateThreadRunning()
    {
        while (true)
        {
            switch (GateThreadStatus)
            {
                case GateThreadRequested:
                    return;

                case GateThreadWaitingForRequest:
                    InterlockedCompareExchange(&GateThreadStatus, GateThreadRequested, GateThreadWaitingForRequest);
                    break;

                case GateThreadNotRunning:
                    if (InterlockedCompareExchange(&GateThreadStatus, GateThreadRequested, GateThreadNotRunning)
                        == GateThreadNotRunning)
                    {
                        if (!CreateGateThread())
                        {
                            GateThreadStatus = GateThreadNotRunning;
                        }
                        return;
                    }
                    break;

                default:
                    TP_ASSERT(0, "EnsureGateThreadRunning: Invalid GateThreadStatus");
            }
        }
    }

    bool ThreadpoolMgr::ShouldGateThreadKeepRunning()
    {
        TP_ASSERT(GateThreadStatus == GateThreadWaitingForRequest || GateThreadStatus == GateThreadRequested,
        "ShouldGateThreadKeepRunning: GateThreadStatus == GateThreadWaitingForRequest || GateThreadStatus == GateThreadRequested");

        bool shouldRunning = true;
        LONG previousStatus = InterlockedExchange(&GateThreadStatus, GateThreadWaitingForRequest);

        if (previousStatus == GateThreadWaitingForRequest)
        {
            ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();

            bool needGateThreadForWorkerThreads = ThreadpoolRequestInstance.IsRequestPending();
            bool needGateThreadForWorkerTracking = (0 != ThreadpoolConfig::EnableWorkerTracking);

            if (!(needGateThreadForWorkerThreads || needGateThreadForWorkerTracking))
            {
                previousStatus = InterlockedCompareExchange(&GateThreadStatus, GateThreadNotRunning,
                                                            GateThreadWaitingForRequest);
                if (previousStatus == GateThreadWaitingForRequest)
                {
                    shouldRunning = false;
                }
            }
        }
        TP_ASSERT(GateThreadStatus == GateThreadWaitingForRequest || GateThreadStatus == GateThreadRequested,
                  "ShouldGateThreadKeepRunning: GateThreadStatus == GateThreadWaitingForRequest || GateThreadStatus == GateThreadRequested");
        return shouldRunning;
    }

    BOOL ThreadpoolMgr::CreateGateThread()
    {
        pthread_t pthread;
        pthread_attr_t pthreadAttr;

        int err = pthread_attr_init(&pthreadAttr);

        if (err != 0)
        {
            return false;
        }

        err = pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED);

        if (err == 0)
        {
            err = pthread_create(&pthread, &pthreadAttr, ThreadpoolMgr::GateThreadStart, this);
        }

        pthread_attr_destroy(&pthreadAttr);

        return (err == 0);
    }

    void* ThreadpoolMgr::GateThreadStart(LPVOID lpArgs)
    {
        ThreadpoolMgr *pThis = (ThreadpoolMgr*)lpArgs;
        static int forceIncrease = 0;
        static int starvationState = 0;

        TP_ASSERT(pThis->GateThreadStatus == GateThreadRequested, "GateThreadStart: pThis->GateThreadStatus == GateThreadRequested");

        Sleep(ThreadpoolConfig::GateThreadDelay/ThreadpoolConfig::GateThreadCpuUtilFactor);

        pThis->GetCPUBusyTime_NT(&pThis->PrevCPUInfo);

        int epoch = 0;
        int lastCompleted = VolatileLoad(&pThis->TotalCompletedWorkRequests);
        do
        {
            int waitcount = 0;
            int waitincrement = 0;
            const int interval = ThreadpoolConfig::GateThreadDelay / ThreadpoolConfig::GateThreadCpuUtilFactor;
            do
            {
                auto tick_start = GetTickCount64();

                Sleep(ThreadpoolConfig::GateThreadDelay/ThreadpoolConfig::GateThreadCpuUtilFactor);
                pThis->CpuUtilization = pThis->GetCPUBusyTime_NT(&pThis->PrevCPUInfo);

                auto tick_end = GetTickCount64();
                auto tick_diff = tick_end > tick_start ? tick_end - tick_start : ~tick_start + 1 + tick_end;
                waitincrement = (tick_diff / interval == 0) ? 1 : (tick_diff / interval);
                if (tick_diff >= 100 * interval)
                {
                    TP_TRACE(Info, "Gater.Wait is sleeping %lu ms, longer than expected %lu ms. CPU Util: %d", tick_diff, interval, pThis->CpuUtilization);
                }
                if (waitcount + waitincrement < ThreadpoolConfig::GateThreadCpuUtilFactor)
                {
                    waitcount += waitincrement;
                }
                else
                {
                    break;
                }
            } while(true);

            DWORD oldest = 0;
            DWORD diff = 0;
            if(pThis->ThreadpoolRequestInstance.IsRequestPending() && pThis->ThreadpoolRequestInstance.PeekWorkRequestAge(oldest))
            {
                DWORD now = pThis->GetTickCount();
                diff = now > oldest? (now - oldest) : (~oldest + 1 + now);
                if(diff >= ThreadpoolConfig::StarvationMaxAge)
                {
                    TP_TRACE(Error, "WorkItem has been stuck in queue for more than %d ms. ", diff/1000);
                }
            }

            if(epoch++ % (ThreadpoolConfig::GaterThreadPeriodicDumpTime / ThreadpoolConfig::GateThreadDelay) == 0)
            {
                int curCompleted = VolatileLoad(&pThis->TotalCompletedWorkRequests);
                ThreadCounter::Counts currentCounts = pThis->WorkerCounter.GetCleanCounts();
                TP_TRACE(Info, "Threadpool Status: "
                        "Total-finished %d, "
                        "QSize %d, QAge %d ms, Working/Active/Retired/Max %d/%d/%d/%d, "
                        "HC-Adj-Interval %d, CPU-Util %d, Throughput: %d",
                         curCompleted,
                         pThis->ThreadpoolRequestInstance.GetPendingRequestNum(), diff,
                          currentCounts.NumWorking, currentCounts.NumActive, currentCounts.NumRetired,
                          currentCounts.MaxWorking, pThis->ThreadAdjustmentInterval, pThis->CpuUtilization,
                         curCompleted - lastCompleted
                );
                lastCompleted = curCompleted;
            }

            if (0 == ThreadpoolConfig::DisableStarvationDetection)
            {
                if (pThis->ThreadpoolRequestInstance.GetPendingRequestNum()>0 &&
                        pThis->SufficientDelaySinceLastDequeue())
                {
                    forceIncrease = (forceIncrease == 0 ? 1 : forceIncrease * 2);
                    starvationState = ThreadpoolConfig::GaterThreadStarvationClearance;

                    ThreadCounter::Counts counts = pThis->WorkerCounter.GetCleanCounts();
                    while ((counts.NumActive < pThis->MaxLimitTotalWorkerThreads) && (counts.NumActive >= counts.MaxWorking))
                    {
                        ThreadCounter::Counts newCounts = counts;
                        newCounts.MaxWorking = __min(counts.NumActive + forceIncrease, pThis->MaxLimitTotalWorkerThreads);

                        ThreadCounter::Counts oldCounts = pThis->WorkerCounter.CompareExchangeCounts(newCounts, counts);
                        if (oldCounts == counts)
                        {
                            THREADPOOL_PERF_TRACE("Gater StarvationDetection and ForceChange %l. CPU Util: %d", pThis->CpuUtilization)
                            {
                                DangerousSpinLock::Holder tal(&pThis->ThreadAdjustmentLock);
                                pThis->HillClimbingInstance.ForceChange(newCounts.MaxWorking, HillClimbing::Starvation);
                            }
                            break;
                        }
                        else
                        {
                            counts = oldCounts;
                        }
                    }
                    THREADPOOL_PERF_TRACE("Gater StarvationDetection and MaybeAddWorkingWorker %l. CPU Util: %d", pThis->CpuUtilization)
                    {
                        pThis->MaybeAddWorkingWorker();
                    }
                }
                else
                {
                    if(starvationState == 0)
                        forceIncrease = 0;
                    else
                        starvationState--;
                }
            }
        }
        while (pThis->ShouldGateThreadKeepRunning());
        return 0;
    }

    BOOL ThreadpoolMgr::SufficientDelaySinceLastDequeue()
    {
        unsigned int tooLong;
        unsigned int curr = GetTickCount();
        unsigned int delay = (curr > LastDequeueTime ? curr - LastDequeueTime : ~LastDequeueTime + 1 + curr);

        if(CpuUtilization < ThreadpoolConfig::CpuUtilizationLow)
        {
            tooLong = ThreadpoolConfig::GateThreadDelay;
        }
        else
        {
            ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();
            tooLong = counts.MaxWorking * ThreadpoolConfig::DequeueDelayThreshold;
        }

        return (delay > tooLong);
    }

    void ThreadpoolMgr::ReportThreadStatus(bool isWorking)
    {
        TP_ASSERT(ThreadpoolConfig::EnableWorkerTracking, "ReportThreadStatus: ThreadpoolConfig::EnableWorkerTracking");
        while (true)
        {
            WorkingThreadCounts currentCounts;
            WorkingThreadCounts newCounts;

            currentCounts.asLong = VolatileLoad(&WorkingThreadCountsTracking.asLong);

            newCounts = currentCounts;

            if (isWorking)
            {
                newCounts.currentWorking++;
            }

            if (newCounts.currentWorking > newCounts.maxWorking)
            {
                newCounts.maxWorking = newCounts.currentWorking;
            }

            if (!isWorking)
            {
                newCounts.currentWorking--;
            }

            if (currentCounts.asLong ==
                    InterlockedCompareExchange(&WorkingThreadCountsTracking.asLong, newCounts.asLong, currentCounts.asLong))
            {
                break;
            }
        }
    }
}
