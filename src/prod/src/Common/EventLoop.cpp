// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace
{
    StringLiteral const TraceLoop("EventLoop");
    StringLiteral const TracePool("EventLoopPool");

    const int eventListCapacity = 64;
    const uint defaultEventMask = EPOLLONESHOT; 

    EventLoopPool* defaultPool = nullptr;
    INIT_ONCE initDefaultPoolOnce = INIT_ONCE_STATIC_INIT;

    BOOL CALLBACK InitDefaultPool(PINIT_ONCE, PVOID, PVOID*)
    {
        defaultPool = new EventLoopPool(L"Default");
        return TRUE;
    }
}

EventLoopPool* EventLoopPool::GetDefault()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = ::InitOnceExecuteOnce(
        &initDefaultPoolOnce,
        InitDefaultPool,
        nullptr,
        nullptr);

    ASSERT_IF(!bStatus, "Failed to initialize default EventLoopPool");
    return defaultPool; 
}

EventLoopPool::EventLoopPool(wstring const & tag, uint concurrency)
    : id_(tag.empty()? wformatString("{0}", TextTraceThis) : wformatString("{0}.{1}", tag, TextTraceThis))
    , assignmentIndex_(0)
{
    if (concurrency == 0)
    {
        concurrency = CommonConfig::GetConfig().EventLoopConcurrency; 
    }

    if (concurrency == 0)
    {
        concurrency = Environment::GetNumberOfProcessors();
    }

    //Need at least 2 loops to seperate input and output events for a given socket
    if (concurrency < 2) concurrency = 2;

    EventLoopPool::WriteInfo(TracePool, id_, "create: concurrency = {0}", concurrency);

    pool_.reserve(concurrency);
    for(uint i = 0; i < concurrency; ++i)
    {
        pool_.emplace_back(make_unique<EventLoop>());
    }
}

void EventLoopPool::SetSchedParam(int policy, int priority)
{
    WriteInfo(TracePool, id_, "setting sched param on loop threads: policy = {0}, priority = {1}", policy, priority); 
    for(auto const & loop : pool_)
    {
        loop->SetSchedParam(policy, priority);
    }
}

EventLoop* EventLoopPool::Assign_CallerHoldingLock()
{
    auto* result = &(*(pool_[assignmentIndex_]));
    assignmentIndex_ = (++assignmentIndex_) % pool_.size();
    return result;
}

EventLoop& EventLoopPool::Assign()
{
    EventLoop* result = nullptr;
    {
        AcquireWriteLock grab(lock_);
        result = Assign_CallerHoldingLock();
    }

    WriteNoise(TracePool, id_, "EventLoopPool::Assign: {0} assigned", TextTracePtr(result));
    return *result;
}

void EventLoopPool::AssignPair(EventLoop** inLoop, EventLoop** outLoop)
{
    {
        AcquireWriteLock grab(lock_);

        *inLoop = Assign_CallerHoldingLock();
        *outLoop = Assign_CallerHoldingLock();
        Invariant(*inLoop != *outLoop);
    }

    WriteNoise(TracePool, id_, "EventLoopPool::AssignPair: ({0}, {1}) assigned", TextTracePtr(*inLoop), TextTracePtr(*outLoop));
}


class EventLoop::FdContext
{
    DENY_COPY(FdContext);
public:
    typedef std::shared_ptr<FdContext> SPtr;

    FdContext(int fd, uint events, Callback const & cb, bool dispatchEventAsync);

    int Fd() const;
    uint Events() const;
    void FireEvent(uint event);
    void Close(bool waitForCallback);

    void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

private:
    void RunCallback(uint event);
    int CallbackRunningDec();

    const int fd_;
    const uint events_;
    const Callback cb_;
    const bool dispatchEventAsync_;
    std::atomic_int cbRunning_ {1};
    ManualResetEvent closedEvent_;
};

EventLoop::EventLoop() : id_(wformatString("{0}", TextTraceThis)), fdMapSize_(0)
{
    Setup();
}

EventLoop::~EventLoop()
{
    Cleanup();
}

void EventLoop::Setup()
{
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
    ASSERT_IF(epfd_ < 0, "epoll_create failed: {0}", errno);
    reportList_.resize(eventListCapacity);

    pthread_attr_t pthreadAttr;
    Invariant(pthread_attr_init(&pthreadAttr) == 0);
    Invariant(pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED) == 0);
    auto retval = pthread_create(&tid_, nullptr, &PthreadFunc, this);
    Invariant(retval == 0);
    pthread_attr_destroy(&pthreadAttr);
}

void EventLoop::SetSchedParam(int policy, int priority)
{
    sched_param param = { priority };
    auto retval = pthread_setschedparam(tid_, policy, &param);
    if (retval)
    {
        WriteWarning(
            TraceLoop,
            id_,
            "failed to set sched param on loop thread: pthread_t={0:x}, policy = {1}, priority = {2}, error = {3}",
            tid_,
            policy,
            priority,
            retval);
    }
}

void* EventLoop::PthreadFunc(void *arg)
{
    ((EventLoop*)arg)->Loop();
    return nullptr;
}

void EventLoop::Loop()
{
    WriteInfo(TraceLoop, id_, "starting event loop");

    for(;;)
    {
        auto count = epoll_wait(epfd_, reportList_.data(), reportList_.size(), -1);
        if (count < 0)
        {
            if (errno == EINTR) continue;

            WriteError(TraceLoop, id_, "epoll_wait failed: {0}", errno);
            break;
        }

        WriteTrace(
            (count == 0)? LogLevel::Info : LogLevel::Noise,
            TraceLoop,
            id_,
            "epoll_wait reported {0} events on {1} registered descriptor(s)",
            count,
            fdMapSize_);

        if (count == 0) break;

        for(uint i = 0; i < count; ++i)
        {
            FdContext* fdc = (FdContext*)(reportList_[i].data.ptr);
            auto evt = reportList_[i].events;
            auto errOrHup = IsFdClosedOrInError(evt);
            WriteTrace(
                errOrHup ? LogLevel::Info : LogLevel::Noise,
                TraceLoop,
                id_,
                "events {0:x} reported on {1}, EPOLLIN={2},EPOLLOUT={3},EPOLLHUP={4},EPOLLERR={5}",
                evt,
                *fdc,
                bool(evt & EPOLLIN),
                bool(evt & EPOLLOUT),
                bool(evt & EPOLLHUP),
                bool(evt & EPOLLERR));

            if (errOrHup)
            {
                // unregister with epoll otherwise EPOLLERR/EPOLLHUP will be reported
                // again at next call of epoll_wait. It is okay to unregister on EPOLLHUP
                // because read and write events are registered to seperate EventLoop instances
                epoll_ctl(epfd_, EPOLL_CTL_DEL, fdc->Fd(), nullptr);
            }

            fdc->FireEvent(evt);
        }
    }

    WriteInfo(TraceLoop, id_,"event loop ended");
}

void EventLoop::Cleanup()
{
    close(epfd_);
    fdMap_.clear();
    fdMapSize_ = fdMap_.size();
}

EventLoop::FdContext* EventLoop::RegisterFd(int fd, uint events, bool dispatchEventAsync, Callback const & cb)
{
    auto ctx = make_shared<FdContext>(fd, events, cb, dispatchEventAsync);
    {
        AcquireWriteLock grab(lock_);

        auto inserted = fdMap_.emplace(make_pair(fd, ctx));
        fdMapSize_ = fdMap_.size();
        Invariant(inserted.second);
    
        epoll_event ev;
        ev.events = 0;
        ev.data.ptr = ctx.get();

        auto added = epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
        ASSERT_IF(added < 0, "epoll_ctl(add) failed: {0}", errno);
    }

    WriteInfo(TraceLoop, id_, "RegisterFd: ctx={0}", *ctx);
    return ctx.get();
}

ErrorCode EventLoop::Activate(FdContext* fdc)
{
    WriteNoise(TraceLoop, id_, "Activate({0})", *fdc);

    epoll_event ev = { .events = fdc->Events()};
    ev.data.ptr = fdc;

    ErrorCode error;
    auto updated = epoll_ctl(epfd_, EPOLL_CTL_MOD, fdc->Fd(), &ev);
    if (updated < 0)
    {
        error = ErrorCode::FromErrno();
        WriteWarning(TraceLoop, id_, "epoll_ctl(mod) failed: {0}", error);
        return error;
    }

    return error;
}

void EventLoop::UnregisterFd(FdContext* fdc, bool waitForCallback)
{
    WriteInfo(TraceLoop, id_, "UnregisterFd({0}), waitForCallback={1}", *fdc, waitForCallback);
    fdc->Close(waitForCallback);

    shared_ptr<FdContext> ctx;
    {
        AcquireWriteLock grab(lock_);

        auto iter = fdMap_.find(fdc->Fd());
        if (iter == fdMap_.end())
        {
            WriteNoise(TraceLoop, id_, "{0} not found in fdMap_", *fdc);
            return;
        }

        if (iter->second.get() != fdc)
        {
            WriteInfo(TraceLoop, id_, "{0} does not match fdMap_ entry {1}", *fdc, *(iter->second));
            return;
        }

        ctx = move(iter->second);
        fdMap_.erase(iter);
        fdMapSize_ = fdMap_.size();

        epoll_ctl(epfd_, EPOLL_CTL_DEL, fdc->Fd(), nullptr);
    }

    //Keep ctx alive a little longer in case an event is being or about to be reported
    //in Loop(). Racing is possible, because, unlike this function, in Loop(), FdContext
    //is accessed without locking for reporting efficiency.
    TimerQueue::GetDefault().Enqueue(
        "EventLoop.Cleanup",
        [ctx = move(ctx)] {},
        CommonConfig::GetConfig().EventLoopCleanupDelay);
}

EventLoop::FdContext::FdContext(int fd, uint events, Callback const & cb, bool dispatchEventAsync)
    : fd_(fd), events_(events | defaultEventMask), cb_(cb), dispatchEventAsync_(dispatchEventAsync)
{
    WriteInfo(TraceLoop, "FdContext ctor: {0}", *this);
}

int EventLoop::FdContext::Fd() const
{
    return fd_;
}

uint EventLoop::FdContext::Events() const
{
    return events_;
}

int EventLoop::FdContext::CallbackRunningDec()
{
    auto after = --cbRunning_;
    if (after == 0)
    {
        closedEvent_.Set();
    }

    return after;
}

void EventLoop::FdContext::FireEvent(uint events)
{
    auto cbRunning = ++cbRunning_;
    //there are no concurrent ++cbRunning_ calls, as FireEvent is called sequentially after epoll_wait return
    if (cbRunning < 2) 
    {
        Invariant(cbRunning == 1);
        auto afterDec = CallbackRunningDec();
        Invariant(afterDec == 0);
        return;
    }

    if (dispatchEventAsync_ || EventLoop::IsFdClosedOrInError(events))
    {
        Threadpool::Post([events, this] { RunCallback(events); });
        return;
    }

    RunCallback(events);
}

void EventLoop::FdContext::RunCallback(uint events)
{
    cb_(fd_, events);
    CallbackRunningDec();
}

void EventLoop::FdContext::Close(bool waitForCallback)
{
    WriteInfo(TraceLoop, "FdContext({0}): closing {1}", TextTraceThis, *this);

    if (CallbackRunningDec() == 0)
    {
        return;
    } 
    if (!waitForCallback) return;

    closedEvent_.WaitOne();
}

void EventLoop::FdContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("(fdc={0},fd={1:x}, events={2:x}, dispatchEventAsync={3})", TextTraceThis, fd_, events_, dispatchEventAsync_);
}    
