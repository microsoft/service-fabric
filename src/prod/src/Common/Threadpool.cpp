// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

volatile LONG Threadpool::activeCallbackCount_ = 0;

class Threadpool::WorkItem : public enable_shared_from_this<WorkItem>
{
    DENY_COPY(WorkItem);

public:
    WorkItem(ThreadpoolCallback const & callback)
        : enable_shared_from_this<WorkItem>(),
        callback_(callback), work_(nullptr), selfReferenceSPtr_(nullptr)
    {
    }

    PTP_WORK CreateWork(PTP_CALLBACK_ENVIRON pcbe)
    {
        KAssert(nullptr == work_);

        work_ = ::CreateThreadpoolWork(&Threadpool::WorkCallback<WorkItem>, this, pcbe);
        CHK_WBOOL(work_ != nullptr);

        // Keep this WorkItem referenced until its callback has been
        // executed
        selfReferenceSPtr_ = shared_from_this();

        return work_;
    }

    static void Callback(PTP_CALLBACK_INSTANCE, PVOID state, PTP_WORK)
    {
        WorkItem* workItem = static_cast<WorkItem*>(state);

        workItem->callback_();
        ::CloseThreadpoolWork(workItem->work_);
        workItem->selfReferenceSPtr_.reset();
    }

private:
    ThreadpoolCallback callback_;
    PTP_WORK work_;
    shared_ptr<WorkItem> selfReferenceSPtr_;
};

#if !defined(PLATFORM_UNIX)
class Threadpool::TimerSimple
{
    DENY_COPY(TimerSimple);

public:
    typedef std::function<void()> TimerCallback;
    static void Start(PTP_CALLBACK_ENVIRON pcbe, TimerCallback const & callback, Common::TimeSpan dueTime);
    static void Callback(PTP_CALLBACK_INSTANCE, void* lpParameter, PTP_TIMER);

private:
    TimerSimple(TimerCallback const & callback, Common::TimeSpan dueTime);
    static void SafeStart(PTP_CALLBACK_ENVIRON pcbe, TimerSimple* timer);
    void Start(PTP_CALLBACK_ENVIRON pcbe);

private:
    TimerCallback callback_;
    int64 dueTimeTicks_;
    PFILETIME dueTimePtr_;
    PTP_TIMER timer_;
};

Threadpool::TimerSimple::TimerSimple(TimerCallback const & callback, Common::TimeSpan dueTime)
    : callback_(callback), 
    dueTimeTicks_((dueTime <= Common::TimeSpan::Zero)? -1 /*-1 means firing immediately */ : -dueTime.Ticks),
    dueTimePtr_(reinterpret_cast<PFILETIME>(&dueTimeTicks_)),
    timer_(nullptr)
{
    CODING_ERROR_ASSERT(callback_ != nullptr);
    CODING_ERROR_ASSERT(dueTime != Common::TimeSpan::MaxValue);
}

void Threadpool::TimerSimple::Start(PTP_CALLBACK_ENVIRON pcbe, TimerCallback const & callback, Common::TimeSpan dueTime)
{
    TimerSimple* timer = new TimerSimple(callback, dueTime);
    SafeStart(pcbe, timer);
}

void Threadpool::TimerSimple::SafeStart(PTP_CALLBACK_ENVIRON pcbe, TimerSimple* timer)
{
    try
    {
        timer->Start(pcbe);
    }
    catch(std::exception const& e)
    {
        delete timer;
        timer = nullptr;
        throw e;
    }
}

void Threadpool::TimerSimple::Start(PTP_CALLBACK_ENVIRON pcbe)
{
    timer_ = CreateThreadpoolTimer(&Threadpool::TimerCallback<TimerSimple>, this, pcbe);
    CHK_WBOOL( timer_ != nullptr );

    SetThreadpoolTimer(timer_, dueTimePtr_, 0, 0);
}

void Threadpool::TimerSimple::Callback(PTP_CALLBACK_INSTANCE, void* callbackParameter, PTP_TIMER)
{
    TimerSimple* timerPtr = reinterpret_cast<TimerSimple*>(callbackParameter);
    timerPtr->callback_();
    timerPtr->callback_ = nullptr;
    CloseThreadpoolTimer(timerPtr->timer_);
    delete timerPtr;
    timerPtr = nullptr;
}
#endif

Threadpool::Threadpool() : ptpPool_(nullptr), pcbe_(nullptr)
{
}

Threadpool::~Threadpool()
{
#ifndef PLATFORM_UNIX
    if (pcbe_)
    {
        ::DestroyThreadpoolEnvironment(pcbe_);
    }
#endif

    if (ptpPool_)
    {
        ::CloseThreadpool(ptpPool_);
    }
}


void Threadpool::InitializeCustomPool()
{
#ifndef PLATFORM_UNIX
    ptpPool_ = ::CreateThreadpool(nullptr);
    ::InitializeThreadpoolEnvironment(&cbe_);
    pcbe_ = &cbe_;
    ::SetThreadpoolCallbackPool(pcbe_, ptpPool_);
#endif
}


Global<Threadpool> Threadpool::CreateCustomPool(DWORD minThread, DWORD maxThread)
{
    Global<Threadpool> pool = make_global<Threadpool>();
#ifndef PLATFORM_UNIX
    pool->InitializeCustomPool();
    if (minThread)
    {
        pool->SetThreadMin(minThread);
    }

    if (maxThread)
    {
        pool->SetThreadMax(maxThread);
    }
#endif

    return pool;
}

bool Threadpool::SetThreadMin(DWORD min)
{
    Invariant(ptpPool_);
    return ::SetThreadpoolThreadMinimum(ptpPool_, min) == TRUE;
}

void Threadpool::SetThreadMax(DWORD max)
{
    Invariant(ptpPool_);
    ::SetThreadpoolThreadMaximum(ptpPool_, max);
}

PTP_POOL Threadpool::PoolPtr()
{
    return ptpPool_;
}

PTP_CALLBACK_ENVIRON Threadpool::CallbackEnvPtr()
{
    return pcbe_;
}

void Threadpool::SubmitInternal(PTP_CALLBACK_ENVIRON pcbe, ThreadpoolCallback const & callback)
{
    auto workItem = make_shared<WorkItem>(callback);
    ::SubmitThreadpoolWork(workItem->CreateWork(pcbe));
}

void Threadpool::SubmitInternal(PTP_CALLBACK_ENVIRON pcbe, ThreadpoolCallback const & callback, Common::TimeSpan delay, bool allowSyncCall)
{
    if (delay <= TimeSpan::Zero)
    {
        if (allowSyncCall)
        {
            // No need to adjust active callback count here, as it is already covered in a parent stack frame, if this is a thread pool thread.
            callback();
        }
        else
        {
            SubmitInternal(pcbe, callback);
        }
    }
    else
    {
#ifdef PLATFORM_UNIX
        static const StringLiteral tag("threadpool");
        TimerSPtr timer = Timer::Create(tag, nullptr, false, pcbe);
        timer->LimitToOneShot();
        timer->SetCallback([timer, callback] (TimerSPtr const &)
        {
            callback();
            timer->Cancel(true); //One-shot timer, cancel called in callback
        });

        timer->Change(delay);
#else
        TimerSimple::Start(pcbe, callback, delay);
#endif
    }
}

void Threadpool::Submit(ThreadpoolCallback const & callback)
{
    SubmitInternal(pcbe_, callback);
}

void Threadpool::Submit(ThreadpoolCallback const & callback, TimeSpan delay, bool allowSyncCall)
{
    SubmitInternal(pcbe_, callback, delay, allowSyncCall);
}

void Threadpool::Post(ThreadpoolCallback const & callback)
{
    SubmitInternal(nullptr, callback);
}

void Threadpool::Post(ThreadpoolCallback const & callback, TimeSpan delay, bool allowSyncCall)
{
    SubmitInternal(nullptr, callback, delay, allowSyncCall);
}
