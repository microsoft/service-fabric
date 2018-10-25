#include "Threadpool.h"
#include "ThreadpoolRequest.h"
#include "Tracer.h"

namespace KtlThreadpool{

    void ThreadpoolRequest::ResetState()
    {
        m_NumRequests = 0;
        m_outstandingThreadRequestCount = 0;
    }

    BOOL ThreadpoolRequest::IsRequestPending()
    {
        return (m_outstandingThreadRequestCount != 0);
    }

    LONG ThreadpoolRequest::GetPendingRequestNum()
    {
        return m_outstandingThreadRequestCount;
    }

    void ThreadpoolRequest::ClearRequestsActive()
    {
        m_outstandingThreadRequestCount = 0;
    }

    void ThreadpoolRequest::SetRequestsActive()
    {
        LONG count = m_outstandingThreadRequestCount;
        while (true) {
            LONG prevCount = InterlockedCompareExchange(&m_outstandingThreadRequestCount, count + 1, count);
            if (prevCount == count) {
                m_threadpoolMgr->MaybeAddWorkingWorker();
                m_threadpoolMgr->EnsureGateThreadRunning();
                break;
            }
            count = prevCount;
        }
    }

    bool ThreadpoolRequest::TakeActiveRequest()
    {
        LONG count = m_outstandingThreadRequestCount;

        while (count > 0) {
            LONG prevCount = InterlockedCompareExchange(&m_outstandingThreadRequestCount, count - 1, count);
            if (prevCount == count)
                return true;
            count = prevCount;
        }
        return false;
    }

    void ThreadpoolRequest::ReleaseWorkRequest(WorkRequest *workRequest)
    {
        m_threadpoolMgr->RecycleMemory(workRequest, MEMTYPE_WorkRequest);
    }

    void ThreadpoolRequest::QueueWorkRequest(LPTHREADPOOL_WORK_START_ROUTINE function, PVOID parameter, PVOID context)
    {
        WorkRequest *pWorkRequest;

        pWorkRequest = m_threadpoolMgr->MakeWorkRequest(function, parameter, context);
        TP_ASSERT(pWorkRequest != NULL, "QueueWorkRequest: pWorkRequest != NULL");

        SpinLock::Holder slh(&m_lock);
        m_threadpoolMgr->EnqueueWorkRequest(pWorkRequest);

        m_NumRequests++;
        SetRequestsActive();
    }

    PVOID ThreadpoolRequest::DeQueueWorkRequest(bool* lastOne)
    {
        *lastOne = true;

        SpinLock::Holder slh(&m_lock);

        WorkRequest * pWorkRequest = m_threadpoolMgr->DequeueWorkRequest();
        if (pWorkRequest) {
            m_NumRequests--;
            if(m_NumRequests > 0) {
                *lastOne = false;
            }

            TakeActiveRequest();
        }

        return (PVOID) pWorkRequest;
    }

    BOOL ThreadpoolRequest::PeekWorkRequestAge(DWORD& age)
    {
        SpinLock::Holder slh(&m_lock);
        return m_threadpoolMgr->PeekWorkRequestAge(age);
    }

    void ThreadpoolRequest::DispatchWorkItem(bool* foundWork, bool* wasNotRecalled)
    {
        *foundWork = false;
        bool lastOne = false;
        bool enableWorkerTracking = ThreadpoolConfig::EnableWorkerTracking;

        static int numDispatched = 0;

        WorkRequest *pWorkRequest = (WorkRequest*) DeQueueWorkRequest(&lastOne);

        *wasNotRecalled = m_threadpoolMgr->ShouldWorkerKeepRunning();

        if (NULL != pWorkRequest) {
            *foundWork = !lastOne;

            LPTHREADPOOL_WORK_START_ROUTINE wrFunction = pWorkRequest->Function;
            LPVOID wrParameter = pWorkRequest->Parameter;
            LPVOID wrContext = pWorkRequest->Context;

            m_threadpoolMgr->FreeWorkRequest(pWorkRequest);

            if (enableWorkerTracking) {
                m_threadpoolMgr->ReportThreadStatus(true);
                (wrFunction)(NULL, wrParameter, wrContext);
                m_threadpoolMgr->ReportThreadStatus(false);
            }
            else {
                (wrFunction)(NULL, wrParameter, wrContext);
            }

            m_threadpoolMgr->NotifyWorkItemCompleted();
            if (m_threadpoolMgr->ShouldAdjustMaxWorkersActive()) {
                DangerousSpinLock::TryHolder tal(&m_threadpoolMgr->ThreadAdjustmentLock);
                if (tal.Acquired()) {
                    m_threadpoolMgr->AdjustMaxWorkersActive();
                }
                else {
                    // the lock is held by someone else, so they will take care of this for us.
                }
            }
        }
    }
}
