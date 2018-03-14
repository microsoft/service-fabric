// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Win32ThreadpoolWrapper.h"

using namespace Threadpool;

TP_POOL DefaultPool = { new ThreadpoolMgr() };

VOID InitializeThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe)
{
}

VOID DestroyThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe)
{
}

VOID SetThreadpoolCallbackPool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp)
{
    pcbe->Pool = ptpp;
}

PTP_POOL CreateThreadpool(PVOID reserved)
{
    PTP_POOL pool = new TP_POOL();
    pool->pThreadpoolMgr = new ThreadpoolMgr();
    return pool;
}

VOID CloseThreadpool(PTP_POOL ptpp)
{
    if (ptpp != &DefaultPool)
    {
        delete ptpp->pThreadpoolMgr;
        delete ptpp;
    }
}

BOOL SetThreadpoolThreadMinimum(PTP_POOL ptpp, DWORD cthrdMic)
{
    if (!ptpp)
    {
        ptpp = &DefaultPool;
    }
    ptpp->pThreadpoolMgr->SetMinThreads(cthrdMic);
    return TRUE;
}

VOID SetThreadpoolThreadMaximum(PTP_POOL ptpp, DWORD cthrdMost)
{
    if (!ptpp)
    {
        ptpp = &DefaultPool;
    }
    ptpp->pThreadpoolMgr->SetMaxThreads(cthrdMost);
}

PTP_WORK CreateThreadpoolWork(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe)
{
    PTP_WORK work = new TP_WORK();
    work->WorkCallback = pfnwk;
    work->CallbackParameter = pv;
    work->CallbackEnvironment = pcbe;
    return work;
}

VOID CloseThreadpoolWork(PTP_WORK pwk)
{
    delete pwk;
}

VOID SubmitThreadpoolWork(PTP_WORK pwk)
{
    PTP_POOL pool = pwk->CallbackEnvironment ? pwk->CallbackEnvironment->Pool : &DefaultPool;
    ThreadpoolMgr *pThreadpoolMgr = pool->pThreadpoolMgr;

    pThreadpoolMgr->QueueUserWorkItem((LPTHREADPOOL_WORK_START_ROUTINE)pwk->WorkCallback, pwk->CallbackParameter, pwk);
}

VOID WaitForThreadpoolWorkCallbacks(PTP_WORK pwk, BOOL fCancelPendingCallbacks)
{
}
