#pragma once

#include "MinPal.h"
#include "Interlock.h"
#include "Volatile.h"
#include "HillClimbing.h"
#include "UnfairSemaphore.h"
#include "ThreadpoolRequest.h"

namespace KtlThreadpool{

    // Configrature parameters
    struct ThreadpoolConfig
    {
        static const DWORD MinWorkerThreadsStartupFactor    = 2;                    // MinWorker = NumProcessors / Factor
        static const DWORD ForceMinWorkerThreads            = 0;
        static const DWORD ForceMaxWorkerThreads            = 0;
        static const DWORD DefaultMaxWorkerThreads          = 800;

        static const DWORD TickCountAdjustment              = 0;

        static const DWORD EnableWorkerTracking             = 1;

        static const DWORD DebugBreakOnWorkerStarvation     = 0;

        static const DWORD DisableStarvationDetection       = 0;
        static const DWORD DisableCongestDetection          = 0;
        static const DWORD StarvationQueueLimit             = 50;
        static const DWORD StarvationMaxAge                 = 600000;               // 600s, 10min

        static const DWORD CpuUtilizationHigh               = 95;                   // Remove threads when above this
        static const DWORD CpuUtilizationLow                = 80;                   // Inject more threads if below this

        // Internal configuration
        static const DWORD GateThreadDelay                  = 200;                  // ms
        static const DWORD GateThreadCpuUtilFactor          = 20;                   // ms
        static const DWORD DequeueDelayThreshold            = GateThreadDelay * 2;  // ms
        static const DWORD GaterThreadPeriodicDumpTime      = 10000;                // ms
        static const DWORD GaterThreadStarvationClearance   = 50;                   // times * GateThreadDelay, 10s

        static const DWORD WorkerUnfairSemaphoreTimeout     = 5 * 1000;

        static const DWORD WorkerRetireSemaphoreTimeout     = 5 * 1000;

        static const DWORD SpinLimitPerProcessor            = 50;

        //static const DWORD UnfairSemaphoreSpinTime          = 100;
    };

    // Definitions and data structures to support recycling of high-frequency
    // memory blocks. We use a spin-lock to access the list
    class RecycledListInfo
    {
        static const unsigned int MaxCachedEntries = 40;

        struct Entry
        {
            Entry* next;
        };

        Volatile<LONG>  lock;   		// this is a spin lock
        DWORD           count;  		// count of number of elements in the list
        Entry*          root;   		// ptr to first element of recycled list
        DWORD           filler;         // Pad the structure to a multiple of the 16.

    public:
        RecycledListInfo()
        {
            lock = 0;
            root = NULL;
            count = 0;
        }

        inline bool CanInsert()
        {
            return count < MaxCachedEntries;
        }

        inline LPVOID Remove()
        {
            if(root == NULL) return NULL;

            AcquireLock();
            Entry* ret = (Entry*)root;
            if(ret)
            {
                root = ret->next;
                count -= 1;
            }
            ReleaseLock();

            return ret;
        }

        inline VOID Insert( LPVOID mem )
        {
            AcquireLock();
            Entry* entry = (Entry*)mem;
            entry->next = root;
            root   = entry;
            count += 1;
            ReleaseLock();
        }

    private:
        inline VOID AcquireLock()
        {
            unsigned int rounds = 0;
            DWORD dwSwitchCount = 0;

            while(lock != 0 || InterlockedExchange( &lock, 1 ) != 0)
            {
                YieldProcessor();
                rounds++;
                if((rounds % 32) == 0)
                {
                    ++dwSwitchCount;
                    __SwitchToThread(0, dwSwitchCount);
                }
            }
        }

        inline VOID ReleaseLock()
        {
            lock = 0;
        }
    };

    enum MemType
    {
        MEMTYPE_WorkRequest     = 0,
        MEMTYPE_COUNT           = 1,
    };

    class RecycledListsWrapper
    {
    private:

        DWORD CacheGuardPre[64/sizeof(DWORD)];
        RecycledListInfo (*pRecycledListPerProcessor)[MEMTYPE_COUNT];  // [numProc][MEMTYPE_COUNT]
        DWORD CacheGuardPost[64/sizeof(DWORD)];
        DWORD NumOfProcessors;

    public:

        inline void Initialize(unsigned int numProcs)
        {
            NumOfProcessors = numProcs;
            pRecycledListPerProcessor = new RecycledListInfo[numProcs][MEMTYPE_COUNT];
        }

        inline bool IsInitialized()
        {
            return pRecycledListPerProcessor != NULL;
        }

        inline RecycledListInfo& GetRecycleMemoryInfo(enum MemType memType)
        {
            return pRecycledListPerProcessor[GetCurrentProcessorNumber() % NumOfProcessors][memType];
        }
    };

    class ThreadCounter
    {
    public:

        static const int MaxPossibleCount = 0x7fff;

        union Counts
        {
            struct
            {
                int MaxWorking : 16;  //Determined by HillClimbing; adjusted elsewhere for timeouts, etc.
                int NumActive  : 16;  //Active means working or waiting on WorkerSemaphore.  These are "warm/hot" threads.
                int NumWorking : 16;  //Trying to get work from various queues.  Not waiting on either semaphore.
                int NumRetired : 16;  //Not trying to get work; waiting on RetiredWorkerSemaphore.  These are "cold" threads.
            };

            LONGLONG AsLongLong;

            bool operator==(Counts other) { return AsLongLong == other.AsLongLong; }

        } counts;

        inline Counts GetCleanCounts()
        {
            Counts result;
            result.AsLongLong = InterlockedCompareExchange64(&counts.AsLongLong, 0, 0);
            ValidateCounts(result);
            return result;
        }

        inline Counts DangerousGetDirtyCounts()
        {
            Counts result;
            result.AsLongLong = VolatileLoad(&counts.AsLongLong);
            return result;
        }

        inline Counts CompareExchangeCounts(Counts newCounts, Counts oldCounts)
        {
            Counts result;
            result.AsLongLong = InterlockedCompareExchange64(&counts.AsLongLong, newCounts.AsLongLong, oldCounts.AsLongLong);
            if (result == oldCounts)
            {
                ValidateCounts(result);
                ValidateCounts(newCounts);
            }
            return result;
        }

    private:
        static inline void ValidateCounts(Counts counts)
        {
            _ASSERTE(counts.MaxWorking > 0);
            _ASSERTE(counts.NumActive >= 0);
            _ASSERTE(counts.NumWorking >= 0);
            _ASSERTE(counts.NumRetired >= 0);
            _ASSERTE(counts.NumWorking <= counts.NumActive);
        }
    };

    union WorkingThreadCounts
    {
        struct
        {
            int currentWorking : 16;
            int maxWorking     : 16;
        };
        LONG asLong;
    };

    class ThreadpoolMgr
    {
        friend class HillClimbing;
        friend class ThreadpoolRequest;

    public:

        BOOL Initialize();

        inline BOOL IsInitialized()
        {
            return Initialization == -1;
        }

        BOOL SetMaxThreads(DWORD MaxWorkerThreads);
        BOOL GetMaxThreads(DWORD* MaxWorkerThreads);

        BOOL SetMinThreads(DWORD MinWorkerThreads);
        BOOL GetMinThreads(DWORD* MinWorkerThreads);

        BOOL GetAvailableThreads(DWORD* AvailableWorkerThreads);

        BOOL QueueUserWorkItem(LPTHREADPOOL_WORK_START_ROUTINE Function, PVOID Parameter, PVOID Context);

    private:

        void EnsureInitialized();

        BOOL SetMaxThreadsHelper(DWORD MaxWorkerThreads);

        DWORD GetDefaultMaxLimitWorkerThreads(DWORD minLimit);

        void EnqueueWorkRequest(WorkRequest* wr);

        WorkRequest* DequeueWorkRequest();

        inline void UpdateLastDequeueTime()
        {
            LastDequeueTime = GetTickCount();
        }

        void ExecuteWorkRequest(bool* foundWork, bool* wasNotRecalled);

        BOOL CreateWorkerThread();

        int TakeMaxWorkingThreadCount();

    private:

        inline WorkRequest* MakeWorkRequest(LPTHREADPOOL_WORK_START_ROUTINE function, PVOID parameter, PVOID context)
        {
            WorkRequest* wr = (WorkRequest*) GetRecycledMemory(MEMTYPE_WorkRequest);
            _ASSERTE(wr);

            if (NULL == wr)
            {
                return NULL;
            }

            wr->Function = function;
            wr->Context = context;
            wr->Parameter = parameter;
            wr->next = NULL;
            wr->AgeTick = GetTickCount();
            return wr;
        }

        inline void AppendWorkRequest(WorkRequest* entry)
        {
            if (WorkRequestTail)
            {
                _ASSERTE(WorkRequestHead != NULL);
                WorkRequestTail->next = entry;
            }
            else
            {
                _ASSERTE(WorkRequestHead == NULL);
                WorkRequestHead = entry;
            }

            WorkRequestTail = entry;
            _ASSERTE(WorkRequestTail->next == NULL);
        }

        inline WorkRequest* RemoveWorkRequest()
        {
            WorkRequest* entry = NULL;
            if (WorkRequestHead)
            {
                entry = WorkRequestHead;
                WorkRequestHead = entry->next;
                if (WorkRequestHead == NULL)
                    WorkRequestTail = NULL;
            }
            return entry;
        }

        inline BOOL PeekWorkRequestAge(DWORD &age)
        {
            if (WorkRequestHead)
            {
                age = WorkRequestHead->AgeTick;
                return TRUE;
            }
            return FALSE;
        }

        inline void FreeWorkRequest(WorkRequest* workRequest)
        {
            RecycleMemory(workRequest, MEMTYPE_WorkRequest);
        }

        void RecycleMemory(LPVOID mem, enum MemType memType);

        LPVOID GetRecycledMemory(enum MemType memType);

    private:

        static void* WorkerThreadStart(LPVOID lpArgs);

        void NotifyWorkItemCompleted()
        {
            InterlockedIncrement(&TotalCompletedWorkRequests);
        }

        void MaybeAddWorkingWorker();

        inline bool ShouldAdjustMaxWorkersActive()
        {
            DWORD requiredInterval = NextCompletedWorkRequestsTime - PriorCompletedWorkRequestsTime;
            DWORD elapsedInterval = GetTickCount() - PriorCompletedWorkRequestsTime;
            if (elapsedInterval >= requiredInterval)
            {
                ThreadCounter::Counts counts = WorkerCounter.GetCleanCounts();
                if (counts.NumActive <= counts.MaxWorking)
                    return true;
            }
            return false;
        }

        void AdjustMaxWorkersActive();

        bool ShouldWorkerKeepRunning();

    public:

        enum GateThreadStatus : LONG
        {
            GateThreadNotRunning = 0,
            GateThreadRequested = 1,
            GateThreadWaitingForRequest = 2,
        };

    private:

        BOOL CreateGateThread();

        static void* GateThreadStart(LPVOID lpArgs);

        void EnsureGateThreadRunning();

        bool ShouldGateThreadKeepRunning();

        BOOL SufficientDelaySinceLastDequeue();

    private:

        int GetCPUBusyTime_NT(TP_CPU_INFORMATION* pOldInfo);

        inline DWORD GetTickCount() { return KtlThreadpool::GetTickCount() + TickCountAdjustment; }

    private:

        LONG Initialization = 0;

        LONG MinLimitTotalWorkerThreads;
        LONG MaxLimitTotalWorkerThreads;

        HillClimbing HillClimbingInstance;

        ThreadpoolRequest ThreadpoolRequestInstance;

        Volatile<LONG> TotalCompletedWorkRequests = 0;

        Volatile<LONG> PriorCompletedWorkRequests = 0;

        Volatile<DWORD> PriorCompletedWorkRequestsTime = 0;

        Volatile<DWORD> NextCompletedWorkRequestsTime = 0;

        Volatile<DWORD> LastDequeueTime;

        LARGE_INTEGER CurrentSampleStartTime = {0};

        INT ThreadAdjustmentInterval = 0;
        DangerousSpinLock ThreadAdjustmentLock;

        WorkRequest* WorkRequestHead = NULL;
        WorkRequest* WorkRequestTail = NULL;

        LONG GateThreadStatus = GateThreadNotRunning;

        DWORD NumberOfProcessors;
        LONG CpuUtilization = 0;
        TP_CPU_INFORMATION PrevCPUInfo = {{0},{0},{0}};
        DWORD TickCountAdjustment = 0;

    private:

        ThreadCounter WorkerCounter;
        UnfairSemaphore* WorkerSemaphore;

        Semaphore* RetiredWorkerSemaphore;

        CriticalSection WorkerCriticalSection;

        WorkingThreadCounts WorkingThreadCountsTracking = {0};

        void ReportThreadStatus(bool isWorking);

        RecycledListsWrapper RecycledLists;
    };
}
