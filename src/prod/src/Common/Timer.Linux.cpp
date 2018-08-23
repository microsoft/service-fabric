// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/resource.h>

using namespace Common;

static const StringLiteral FinalizerTrace("Finalizer");

class Common::TimerFinalizer
{
    DENY_COPY(TimerFinalizer);

public:
    TimerFinalizer()
        // In order to use bitwise AND in place of modulo operation, _capacity must be a power of 2. This also
        // allow us to ignore overflow when incrementing _enqueueCount/_dequeueCount, since a power of 2 within
        // size_t's limit must be a divisor of the capacity of size_t, _enqueueCount/_dequeueCount's type
        : capacity_((size_t)1 << CommonConfig::GetConfig().TimerFinalizerQueueCapacityBitCount)
        , modularMask_(capacity_ - 1) 
    {
        Timer::WriteInfo(FinalizerTrace, "queue capacity_ = 0x{0:x}, modularMask_ = 0x{1:x}", capacity_, modularMask_);
        Invariant(capacity_ > CommonConfig::GetConfig().PosixTimerLimit);
        Invariant(capacity_ > CommonConfig::GetConfig().PosixTimerLimit_Fabric);
        queue_.resize(capacity_);
    }

    void Enqueue(Timer* timer)
    {
        Invariant(timer->thisSPtr_);

        size_t queueSize = 0;
        bool queueFull = false;
        auto tag = timer->tag_;
        {
            AcquireWriteLock grab(lock_);

            queueFull = IsFull_CallerHoldingLock(queueSize);
            Invariant(!queueFull); // asserts in constructor should ensure capacity is large enough

            Invariant(queue_[enqueueCount_ & modularMask_] == nullptr);
            queue_[enqueueCount_ & modularMask_] = move(timer); 
            ++ enqueueCount_; // overflow is fine

            if (shouldStartWorkerThread_)
            {
                shouldStartWorkerThread_ = false;

                auto disposeDelay = CommonConfig::GetConfig().TimerDisposeDelay;
                if (disposeDelay > TimeSpan::Zero)
                {
                    pthread_t tid;
                    pthread_attr_t pthreadAttr;
                    ZeroRetValAssert(pthread_attr_init(&pthreadAttr));
                    ZeroRetValAssert(pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED));
                    ZeroRetValAssert(pthread_create(&tid, &pthreadAttr, &PthreadFunc, this));
                    pthread_attr_destroy(&pthreadAttr);
                }
                else
                {
                    Timer::WriteInfo(FinalizerTrace, "TimerDisposeDelay={0}, periodic dispose disabled", disposeDelay); 
                }
            }
        }

        Timer::WriteNoise(FinalizerTrace, "enqueued({0}, {1}), qsize = {2}", tag, TextTracePtr(timer), queueSize);
    }

private:
    Timer* Dequeue_CallerHoldingLock()
    {
        if (IsEmpty_CallerHoldingLock()) return nullptr;

        auto result = queue_[dequeueCount_ & modularMask_];
        queue_[dequeueCount_ & modularMask_] = nullptr;
        ++ dequeueCount_;

        return result;
    }

    Timer* PeekHead_CallerHoldingLock() const
    {
        return queue_[dequeueCount_ & modularMask_]; 
    }

    bool IsFull_CallerHoldingLock(size_t & queueSize) const
    {
        queueSize = enqueueCount_ - dequeueCount_;
        return queueSize == capacity_;
    }

    bool IsEmpty_CallerHoldingLock() const
    {
        return enqueueCount_ == dequeueCount_;
    }

    static void* PthreadFunc(void* arg)
    {
        ((TimerFinalizer*)arg)->DisposeLoop();
    }

    void DisposeLoop()
    {
        auto delay = CommonConfig::GetConfig().TimerDisposeDelay;
        vector<Timer*> timersToDispose;

        Timer::WriteNoise(FinalizerTrace, "starting loop with delay {0}", delay);
        for(;;)
        {
            Sleep(delay.TotalPositiveMilliseconds());
            auto now = Stopwatch::Now();

            timersToDispose.reserve(capacity_);
            {
                AcquireWriteLock grab(lock_);

                while(!IsEmpty_CallerHoldingLock() && ((PeekHead_CallerHoldingLock()->CancelTime() + delay) < now))
                {
                    auto* dequeued = Dequeue_CallerHoldingLock();
                    timersToDispose.push_back(dequeued);
                }
            }

            Timer::WriteNoise(FinalizerTrace, "loop: disposing {0} timers", timersToDispose.size());
            for(auto& timer : timersToDispose)
            {
                timer->Dispose();
            }
            timersToDispose.clear();
        }
    }

private:
    RwLock lock_;

    size_t capacity_ = 0; 
    size_t modularMask_ = 0; 
    vector<Timer*> queue_;

    size_t enqueueCount_ = 0;
    size_t dequeueCount_ = 0;

    bool shouldStartWorkerThread_ = true;
};

atomic_uint64 Timer::ObjCount(0);

namespace
{
    Global<TimerEventSource> const trace = make_global<TimerEventSource>();

    Global<TimerFinalizer> timerCleanupQueue;
    timespec disposeSafetyMargin;
    TimeSpan disposeSafetyMarginTimeSpan;

    int pipeFd[2];
    INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;

    atomic_uint64 posixTimerCount;

    TimerQueue* timerQueue;
}

Timer::Timer(StringLiteral tag, TimerCallback const & callback, bool allowConcurrency, PTP_CALLBACK_ENVIRON)
    : tag_(tag.cbegin())
    , callback_(callback)
    , allowConcurrency_(allowConcurrency)
{
    Invariant(tag.size()); // empty tag is not allowed
    trace->Created(TraceThis, tag, ++ObjCount);

    Invariant(::InitOnceExecuteOnce(
        &initOnce,
        InitOnceFunction,
        nullptr,
        nullptr));
}

BOOL Timer::InitOnceFunction(PINIT_ONCE, PVOID, PVOID*)
{
    rlimit rlim = {};
    ZeroRetValAssert(getrlimit(RLIMIT_SIGPENDING, &rlim));
    WriteInfo("InitOnce", "uid = {0}, euid = {1}, RLIMIT_SIGPENDING = {2}/{3}", getuid(), geteuid(), rlim.rlim_cur, rlim.rlim_max); 

    // set up pipe before changing signal disposition
    ZeroRetValAssert(pipe2(pipeFd, O_CLOEXEC));
    pthread_t tid;
    pthread_attr_t pthreadAttr;
    ZeroRetValAssert(pthread_attr_init(&pthreadAttr));
    ZeroRetValAssert(pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED));
    ZeroRetValAssert(pthread_create(&tid, &pthreadAttr, &SignalPipeLoop, nullptr));
    pthread_attr_destroy(&pthreadAttr);

    struct sigaction sa = {};
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = SigHandler;
    sigemptyset(&sa.sa_mask);
    ZeroRetValAssert(sigaction(FABRIC_SIGNO_TIMER, &sa, nullptr));

    sigset_t mask = {};
    sigemptyset(&mask);
    sigaddset(&mask, FABRIC_SIGNO_TIMER);
    ZeroRetValAssert(sigprocmask(SIG_UNBLOCK, &mask, nullptr));

    timerCleanupQueue = make_global<TimerFinalizer>();

    disposeSafetyMargin.tv_sec = 1;
    disposeSafetyMargin.tv_nsec = 0;
    disposeSafetyMarginTimeSpan = TimeSpan::FromSeconds(disposeSafetyMargin.tv_sec);

    posixTimerCount.store(0);

    timerQueue = &TimerQueue::GetDefault();

    return TRUE;
}

void Timer::Dispose()
{
    WriteNoise(__FUNCTION__, "{0}: dispose", TextTraceThis);

    // no need to worry about shared_ptr thread safety as this is only called by Cancel() and finalizer
    thisSPtr_.reset(); 
}

void* Timer::SignalPipeLoop(void*)
{
    SigUtil::BlockAllFabricSignalsOnCallingThread(); //block timer signal to avoid deadlock when pipe is full

    static const uint readBatch = 1024;
    Timer* expiredTimers[readBatch];

    Timer::WriteInfo(__FUNCTION__, "start loop");
    for(;;)
    {
        auto len = read(pipeFd[0], expiredTimers, sizeof(expiredTimers));
        if (len< 0)
        {
            ASSERT_IF(errno != EINTR, "{0}: read failed with errno = {1:x}", __FUNCTION__, errno);
            continue;
        }

        if (len == 0)
        {
            Timer::WriteInfo(__FUNCTION__, "read returned 0, stop loop");
            break;
        }

        Invariant((len % sizeof(Timer*)) == 0);
        for(uint i = 0; i < len/sizeof(Timer*); ++i)
        {
            auto timerPtr= expiredTimers[i];
            Timer::WriteNoise(__FUNCTION__, "read returned timer {0}", TextTracePtr(timerPtr));
            Invariant(timerPtr);
            // no need to capture shared_ptr, as timer is kept alive by finalizer queue when needed
            Threadpool::Post([timerPtr] { timerPtr->Callback(); });
        }
    }

    return nullptr;
}

void Timer::SigHandler(int sig, siginfo_t *si, void*)
{
    auto savedErrno = errno;
    KFinally([=] { errno = savedErrno; });

    Timer* thisPtr = (Timer*)si->si_value.sival_ptr;
    auto overrun = si->si_overrun;
    for (uint i = 0; i <= overrun; ++i)
    {
        // Threadpool::Post is not signal safe, thus we need to write into a pipe
        auto written = write(pipeFd[1], &thisPtr, sizeof(thisPtr));
        Invariant(written == sizeof(thisPtr));
    }
}

Timer::~Timer()
{
    trace->Destructed(TraceThis, --ObjCount);
}

TimerSPtr Timer::Create(StringLiteral tag, TimerCallback const & callback, bool allowConcurrency, PTP_CALLBACK_ENVIRON pcbe)
{
    return std::make_shared<Timer>(tag, callback, allowConcurrency, pcbe);
}

void Timer::SetCallback(TimerCallback const & callback)
{
    CODING_ERROR_ASSERT(callback_ == nullptr);
    CODING_ERROR_ASSERT(callback != nullptr);
    callback_ = callback;
}

void Timer::SetCancelWait()
{
    ASSERT_IF(started_, "SetCancelWait() cannot be called after timer is started");
    ASSERT_IF(allowConcurrency_, "SetCancelWait() not supported for concurrent callbacks");
    waitForCallbackOnCancel_ = true;
    allCallbackCompleted_ = make_unique<ManualResetEvent>();
}

bool Timer::CreatePosixTimer_CallerHoldingLock()
{
    sigevent sigEvent = {};
    sigEvent.sigev_notify = SIGEV_SIGNAL;
    sigEvent.sigev_signo = FABRIC_SIGNO_TIMER;
    sigEvent.sigev_value.sival_ptr = this;

    thisSPtr_ = shared_from_this();

    auto const & config = CommonConfig::GetConfig();
    auto count = ++posixTimerCount;
    if (config.PosixTimerLimit < count)
    {
        --posixTimerCount; 
        WriteNoise(
            __FUNCTION__,
            "{0}: count = {1}, beyond PosixTimerLimit {2}, will add to timerQueue",
            TextTraceThis,
            count,
            config.PosixTimerLimit);

        return false;
    }

    if (timer_create(CLOCK_MONOTONIC, &sigEvent, &timer_))
    {
        --posixTimerCount; 
        auto err = errno;
        WriteWarning(
            __FUNCTION__,
            "{0}: timer_create failed: {1}, posixTimerCount = {2}, will add to timerQueue", 
            TextTraceThis,
            err,
            count);

        return false;
    }

    return true;
}

timespec Timer::ToTimeSpecWithLowerBound(TimeSpan interval)
{
    // need lower bound to avoid all zero timespec:
    // 1. all zero itimerspec::it_value disarms timer
    // 2. all zero itimerspec::it_interval disables timer period
    interval = max(interval, TimeSpan::FromTicks(1));
    auto seconds = interval.TotalSeconds();
    struct timespec timeSpec = {
        seconds, 
        (interval.Ticks - TimeSpan::FromSeconds(seconds).Ticks) * 100 };

    Invariant(timeSpec.tv_sec || timeSpec.tv_nsec);
    return timeSpec;
}

timespec Timer::ToTimeSpecDuetime(TimeSpan dueTime)
{
    if (dueTime == TimeSpan::MaxValue)
    {
        timespec timeSpec = {}; // all zero disables timer
        return timeSpec;
    }

    return ToTimeSpecWithLowerBound(dueTime);
}

timespec Timer::ToTimeSpecPeriod(TimeSpan period)
{
    if ((period == TimeSpan::MaxValue) || (period <= TimeSpan::Zero))
    {
        timespec timeSpec = {};
        return timeSpec;
    }

    return ToTimeSpecWithLowerBound(period);
}

void Timer::Change(TimeSpan dueTime, TimeSpan period) 
{
    itimerspec timerSpec = {
        .it_interval = ToTimeSpecPeriod(period),
        .it_value = ToTimeSpecDuetime(dueTime)};

    Invariant(!oneShotOnly_ || timespec_is_zero(timerSpec.it_interval));

    bool scheduled = false;
    {
        AcquireWriteLock grab(thisLock_);

        if (!cancelCalled_)
        {
            if (!started_)
            {
                if (!CreatePosixTimer_CallerHoldingLock())
                {
                    useTimerQueue_ = true;
                    queuedTimer_ = timerQueue->CreateTimer(
                        StringLiteral(tag_, tag_+strlen(tag_)),
                        [thisSPtr = thisSPtr_] { thisSPtr->Callback(); }); 
                }
                started_ = true;
            }
            else
            {
                Invariant(!oneShotOnly_);
            }

            if (useTimerQueue_)
            {
                period_ = period;
                timerQueue->Enqueue(queuedTimer_, dueTime);
            }
            else
            {
                ZeroRetValAssert(timer_settime(timer_, 0, &timerSpec, NULL));
            }

            scheduled = true;
        }
    }

    trace->Scheduled(
        TraceThis,
        scheduled,
        dueTime,
        period);
}

void Timer::ChangeWithLowerBoundDelay(TimeSpan dueTime, TimeSpan bound)
{
    Change(std::max(dueTime, bound));
}

void Timer::LimitToOneShot()
{
    Invariant(!started_);
    Invariant(!oneShotOnly_);
    oneShotOnly_ = true;
}

StopwatchTime Timer::CancelTime() const
{
    return cancelTime_;
}

void Timer::Cancel(bool disposeNow)
{
    trace->CancelCalled(TraceThis, waitForCallbackOnCancel_);

    bool shouldDelayDispose = false; 
    TimerCallback callback; //avoid reset callback_ under lock
    TimerQueue::TimerSPtr queuedTimer;
    TimerSPtr thisSPtr;
    {
        AcquireWriteLock grab(thisLock_);

        if (!cancelCalled_)
        {
            cancelCalled_ = true;
            cancelTime_ = Stopwatch::Now();

            callback = move(callback_);
            callback_ = nullptr; //test shows this is needed even after move, otherwise objects captured in callback_ won't be released

            auto callbackRunning = -- callbackRunning_;
            Invariant(callbackRunning >= 0);

            if (started_)
            {
                if (useTimerQueue_)
                {
                    Invariant(queuedTimer_);
                    queuedTimer = move(queuedTimer_);
                    shouldDelayDispose = false;
                }
                else
                {
                    ZeroRetValAssert(timer_delete(timer_));
                    --posixTimerCount;
                    timer_ = nullptr;

                    shouldDelayDispose = !disposeNow && (!oneShotOnly_ || !callbackCalled_);
                }
            }

            if (!shouldDelayDispose)
            {
                thisSPtr = move(thisSPtr_);
            }
        }
    }

    WriteNoise(__FUNCTION__, "{0}: shouldDelayDispose = {1}", TextTraceThis, shouldDelayDispose);

    // We need to delay timer clean up due to the following documentation about timer_delete():
    // "The treatment of any pending signal generated by the deleted timer is unspecified",
    // unless we are sure there is no pending callback.
    if (shouldDelayDispose)
    {
        timerCleanupQueue->Enqueue(this);
    }
    else if (queuedTimer)
    {
        timerQueue->Dequeue(queuedTimer);
    }
    
    WaitForCancelCompletionIfNeeded();
}

void Timer::SetCallbackTidIfNeeded()
{
    if (waitForCallbackOnCancel_)
    {
        callbackThreadId_ = pthread_self();
        callbackTidSet_ = true;
    }
}

void Timer::ClearCallbackTidIfNeeded()
{
    if (waitForCallbackOnCancel_)
    {
        callbackTidSet_ = false;
    }
}

void Timer::NotifyCancelCompletionIfNeeded()
{
    if (waitForCallbackOnCancel_)
    {
        allCallbackCompleted_->Set();
    }
}

void Timer::WaitForCancelCompletionIfNeeded()
{
    //WriteNoise(__FUNCTION__, "{0}: enter: callbackRunning = {0}", TextTraceThis, callbackRunning_.load());
    if (waitForCallbackOnCancel_ && callbackRunning_.load())
    {
        if (callbackTidSet_ && pthread_equal(pthread_self(), callbackThreadId_))
        {
            trace->CancelWaitSkipped(TraceThis);
            return;
        }

        trace->CancelWait(TraceThis);
        allCallbackCompleted_->WaitOne();
    }
    //WriteNoise(__FUNCTION__, "{0}: leave", TextTraceThis);
}

void Timer::Callback()
{
    TimerSPtr thisSPtr;

    trace->EnterCallback(TraceThis);

    callbackCalled_ = true;
    SetCallbackTidIfNeeded();

    // Make sure WaitForCancelCompletionIfNeeded wait for this running callback
    LONG callbackRunning = ++ callbackRunning_;
    if ((callbackRunning == 2) // is this the only callback instance running currently?
        // or, if there are more than callback instances running, is concurrency allowed? 
        || ((callbackRunning > 2) && allowConcurrency_))
    {
        //LINUXTODO consider avoid callback_ copy, which probably requires not resetting
        //callback_ in Cancel() and leave it to TimerFinalizer, if timer is started
        //The commented Callback() code should be something to start with
        TimerCallback callback;
        {
            AcquireReadLock grab(thisLock_);

            if (!cancelCalled_)
            {
                callback = callback_;
                thisSPtr = thisSPtr_; //copy in case timer is disposed inside callback_

                if (useTimerQueue_)
                {
                    bool periodEnabled = (period_ < TimeSpan::MaxValue);
                    if(periodEnabled)
                    {
                        timerQueue->Enqueue(queuedTimer_, period_);
                    }
                }
            }
        }

        if (callback)
        {
            Invariant(thisSPtr);
            callback(thisSPtr);
        }
    }

    if (-- callbackRunning_ == 0)
    {
        // check above may evaluate true more than once when allowConcurrency_ is true,
        NotifyCancelCompletionIfNeeded();
    }

    ClearCallbackTidIfNeeded();
    trace->LeaveCallback(TraceThis);
}

// uncomment per LINUXTODO above
/*
void Timer::Callback(Timer& thisSPtr)
{
    trace->EnterCallback(TraceThis);
    SetCallbackTidIfNeeded();

    // Make sure WaitForCancelCompletionIfNeeded wait for this running callback
    LONG callbackRunning = ++ callbackRunning_;
    if (callbackRunning >= 2)
    {
        // Check above returns true in the following cases:
        // 1. allowConcurrency_ is false
        //    so Cancel has not been called yet, it is safe to call callback_
        // 2. allowConcurrency_ is true, two possibilites
        // a) same as 1.
        // b) callbackRunning_ has been decremented in Cancel,
        //    there are 2 or more callbacks currently running, futher check is needed

        if (!cancelCalled_)
        {
            // It is safe to call callback_:
            // 1) 2.b above cannot be true because
            //     cancelCalled_ is set before callbackRunning_ is decremented in Cancel
            // 2) WaitForCancelCompletionIfNeeded returns only when callbackRunning_==0
            // 3) "++ callbackRunning_" above should create full memory barrier
            //    http://en.cppreference.com/w/cpp/atomic/atomic/fetch_add
            //    http://en.cppreference.com/w/cpp/atomic/memory_order
            callback_(thisSPtr);
        }
    }

    if (-- callbackRunning_ == 0)
    {
        // check above may return true more than once when allowConcurrency_ is true,
        // which is fine, because:
        // 1. this timer and callback_ is kept alive by TimerFinalizer,
        // 2. callback_ never runs after WaitForCancelCompletionIfNeeded returns.
        NotifyCancelCompletionIfNeeded();
    }

    ClearCallbackTidIfNeeded();
    trace->LeaveCallback(TraceThis);
}
*/

bool Timer::Test_IsSet() const
{
    AcquireReadLock grab(thisLock_);

    if (!started_ || cancelCalled_) return false;
    
    if (useTimerQueue_)
    {
        return timerQueue->IsTimerArmed(queuedTimer_);
    }

    itimerspec timerSpec = {};
    ZeroRetValAssert(timer_gettime(timer_, &timerSpec));
    return
        timerSpec.it_value.tv_sec ||
        timerSpec.it_value.tv_nsec ||
        (callbackRunning_.load() > 1); //The last check is to make the behavior consistent
        // across Windows and Linux, Windows API IsThreadpoolTimerSet returns true when
        // calling from a callback, while timer_gettime returns "not armed" in such a case
} 
