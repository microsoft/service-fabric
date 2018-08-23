#include "Win32ThreadpoolWrapper.h"

using namespace KtlThreadpool;

TP_POOL KtlDefaultPool = { new ThreadpoolMgr() };

VOID KtlInitializeThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe)
{
}

VOID KtlDestroyThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe)
{
}

VOID KtlSetThreadpoolCallbackPool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp)
{
    pcbe->Pool = ptpp;
}

PTP_POOL KtlCreateThreadpool(PVOID reserved)
{
    PTP_POOL pool = new TP_POOL();
    pool->pThreadpoolMgr = new ThreadpoolMgr();
    return pool;
}

VOID KtlCloseThreadpool(PTP_POOL ptpp)
{
    if (ptpp != &KtlDefaultPool)
    {
        delete ptpp->pThreadpoolMgr;
        delete ptpp;
    }
}

BOOL KtlSetThreadpoolThreadMinimum(PTP_POOL ptpp, DWORD cthrdMic)
{
    if (!ptpp)
    {
        ptpp = &KtlDefaultPool;
    }
    ptpp->pThreadpoolMgr->SetMinThreads(cthrdMic);
    return TRUE;
}

VOID KtlSetThreadpoolThreadMaximum(PTP_POOL ptpp, DWORD cthrdMost)
{
    if (!ptpp)
    {
        ptpp = &KtlDefaultPool;
    }
    ptpp->pThreadpoolMgr->SetMaxThreads(cthrdMost);
}

PTP_WORK KtlCreateThreadpoolWork(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe)
{
    PTP_WORK work = new TP_WORK();
    work->WorkCallback = pfnwk;
    work->CallbackParameter = pv;
    work->CallbackEnvironment = pcbe;
    return work;
}

VOID KtlCloseThreadpoolWork(PTP_WORK pwk)
{
    delete pwk;
}

VOID KtlSubmitThreadpoolWork(PTP_WORK pwk)
{
    PTP_POOL pool = pwk->CallbackEnvironment ? pwk->CallbackEnvironment->Pool : &KtlDefaultPool;
    ThreadpoolMgr *pThreadpoolMgr = pool->pThreadpoolMgr;

    pThreadpoolMgr->QueueUserWorkItem((LPTHREADPOOL_WORK_START_ROUTINE)pwk->WorkCallback, pwk->CallbackParameter, pwk);
}

VOID KtlWaitForThreadpoolWorkCallbacks(PTP_WORK pwk, BOOL fCancelPendingCallbacks)
{
}
