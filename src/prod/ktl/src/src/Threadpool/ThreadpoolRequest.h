#pragma once

#include "MinPal.h"
#include "Volatile.h"
#include "Synch.h"
#include "MinPal.h"
#include "Threadpool.h"

namespace KtlThreadpool{

    typedef VOID (*PTHREADPOOL_WORK_START_ROUTINE)(LPVOID lpCallbackInst, LPVOID lpThreadParameter, LPVOID lpThreadContext);
    typedef PTHREADPOOL_WORK_START_ROUTINE LPTHREADPOOL_WORK_START_ROUTINE;

    struct WorkRequest
    {
        WorkRequest*                        next;
        LPTHREADPOOL_WORK_START_ROUTINE     Function;
        PVOID                               Parameter;
        PVOID                               Context;
        DWORD                               AgeTick;
    };

    class ThreadpoolRequest
    {
    public:
        ThreadpoolRequest() { m_lock.Init(); ResetState(); }

        void Initialize(ThreadpoolMgr *tpm) { m_threadpoolMgr = tpm; }

        BOOL IsRequestPending();
        LONG GetPendingRequestNum();

        void SetRequestsActive();
        bool TakeActiveRequest();
        void ClearRequestsActive();

        void ReleaseWorkRequest(WorkRequest *workRequest);

        void QueueWorkRequest(LPTHREADPOOL_WORK_START_ROUTINE function, PVOID parameter, PVOID context);

        PVOID DeQueueWorkRequest(bool* lastOne);

        BOOL PeekWorkRequestAge(DWORD& age);

        void DispatchWorkItem(bool* foundWork, bool* wasNotRecalled);

    private:
        void ResetState();

    private:
        ULONG m_NumRequests;
        Volatile<LONG> m_outstandingThreadRequestCount;
        SpinLock m_lock;

        ThreadpoolMgr *m_threadpoolMgr;
    };
}
