// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/FabricSignal.h"
#include "Common/TimerEventSource.h"

using namespace Common;
using namespace std;

namespace
{
    TimerEventSource const trace;
    const StringLiteral TraceType("TimerQueue");
    atomic_uint64 LeaseTimerCount(0);
    constexpr size_t InvalidHeapIndex = numeric_limits<decltype(InvalidHeapIndex)>::max();
}

class TimerQueue::Timer
{
    DENY_COPY(Timer);

public:
    Timer(TimerQueue const* queue, StringLiteral const tag, Callback const & callback)
        : tag_(tag.cbegin())
        , callback_(callback)
    {
        trace.CreatedQueued(TraceThis, tag, ++LeaseTimerCount, TracePtr(queue));
        ClearHeapIndex();
    }

    ~Timer()
    {
        trace.DestructedQueued(TraceThis, --LeaseTimerCount);
    }

    bool operator < (Timer const & rhs) const noexcept { return dueTime_ < rhs.dueTime_; }
    bool operator <= (Timer const & rhs) const noexcept { return dueTime_ <= rhs.dueTime_; }

    StopwatchTime DueTime() const noexcept { return dueTime_; }

    bool ShouldAddToHeap(StopwatchTime dueTime) noexcept
    {
        dueTime_ = dueTime;
        return !IsInHeap(); 
    }

    const char* Tag() const noexcept { return tag_; }

    void Fire()
    {
        WriteNoise(TraceType, "{0}: fire", TextTraceThis);
        callback_();
    }

    size_t HeapIndex() const noexcept { return heapIndex_; }
    void SetHeapIndex(ssize_t idx) noexcept { heapIndex_ = idx; }

    bool IsInHeap() const noexcept { return heapIndex_ != InvalidHeapIndex; }

    size_t ClearHeapIndex() noexcept
    {
        auto index = heapIndex_; 
        heapIndex_ = InvalidHeapIndex; 
        return index;
    }

private:
    const char * const tag_; //only stores string literal
    const Callback callback_;

    StopwatchTime dueTime_ = StopwatchTime::Zero;
    size_t heapIndex_;
};

namespace
{
    size_t HeapNodeParentIndex(size_t nodeIndex)
    {
        return (nodeIndex - 1) / 2; // underflow is fine in the way we are using it
    }

    size_t HeapNodeLeftChildIndex(size_t nodeIndex)
    {
        return nodeIndex*2 + 1;
    }

    size_t HeapNodeRightChildIndex(size_t nodeIndex)
    {
        return nodeIndex*2 + 2;
    }
}

bool TimerQueue::IsTimerArmed(TimerSPtr const & timer)
{
    AcquireReadLock grab(lock_);
    return timer->IsInHeap() && (timer->DueTime() < StopwatchTime::MaxValue);
}

void TimerQueue::HeapCheck_Dbg()
{
#ifdef DBG
//    wstring str;
//    StringWriter w(str);
//    w.WriteLine();
//
//    for(size_t i = 0; i < heap_.size(); ++i)
//    {
//        w.WriteLine("{0}-([{1}] = {2})", TextTracePtr(heap_[i].get()), heap_[i]->HeapIndex(), heap_[i]->DueTime());
//    }
//    TimerQueue::WriteNoise(TraceType, "heap: {0}", str);

    for(size_t i = 0; i < heap_.size(); ++i)
    {
        Invariant(heap_[i]);
        Invariant(heap_[i]->IsInHeap());

        auto leftChildIndex = HeapNodeLeftChildIndex(i);
        auto rightChildIndex = HeapNodeRightChildIndex(i);

        if (leftChildIndex >= heap_.size()) return;

        ASSERT_IFNOT(*(heap_[i]) <= *(heap_[leftChildIndex]), "[{0}] <= [{1}] assert failed", i, rightChildIndex);


        if (rightChildIndex >= heap_.size()) return;

        ASSERT_IFNOT(*(heap_[i]) <= *(heap_[rightChildIndex]), "[{0}] <= [{1}] assert failed", i, rightChildIndex);
    }
#endif
}

void TimerQueue::HeapNodeSwap_LockHeld(size_t index1, size_t index2)
{
    Invariant(index1 != index2);
    heap_[index1]->SetHeapIndex(index2);
    heap_[index2]->SetHeapIndex(index1);
    std::swap(heap_[index1], heap_[index2]);
}

void TimerQueue::HeapAdjustUp_LockHeld(size_t nodeIndex)
{
    auto parentIndex = HeapNodeParentIndex(nodeIndex);
    while (parentIndex < nodeIndex)
    {
        if (*(heap_[nodeIndex]) < *(heap_[parentIndex]))
        {
            HeapNodeSwap_LockHeld(nodeIndex, parentIndex);

            nodeIndex = parentIndex;
            parentIndex = HeapNodeParentIndex(nodeIndex); 
            continue;
        }

        HeapCheck_Dbg();
        return;
    }
}

void TimerQueue::HeapAdjustDown_LockHeld(size_t nodeIndex)
{
    for(;;)
    {
        auto leftChildIndex = HeapNodeLeftChildIndex(nodeIndex); 
        if (leftChildIndex >= heap_.size())
        {
            HeapCheck_Dbg();
            return;
        }

        auto leftChildDueTime = heap_[leftChildIndex]->DueTime();
        auto rightChildIndex = HeapNodeRightChildIndex(nodeIndex); 
        auto rightChildDueTime = (rightChildIndex < heap_.size()) ? heap_[rightChildIndex]->DueTime() : StopwatchTime::MaxValue;

        bool minOnLeft = (leftChildDueTime <= rightChildDueTime);
        auto minChildDueTime = minOnLeft? leftChildDueTime : rightChildDueTime; 

        if (heap_[nodeIndex]->DueTime() <= minChildDueTime)
        {
            HeapCheck_Dbg();
            return;
        }

        if (minOnLeft)
        {
            HeapNodeSwap_LockHeld(nodeIndex, leftChildIndex);
            nodeIndex = leftChildIndex;
            continue;
        }

        Invariant(rightChildIndex < heap_.size());
        HeapNodeSwap_LockHeld(nodeIndex, rightChildIndex);
        nodeIndex = rightChildIndex;
    }
}

template <typename TSPtr>
void TimerQueue::EnqueueT(TSPtr && timer, TimeSpan t)
{
    WriteNoise(TraceType, "{0}: Enqueue, due in {1}", TextTracePtr(timer.get()), t);
    Invariant(timer);
    StopwatchTime dueTime = Stopwatch::Now() + t;
    {
        AcquireWriteLock grab(lock_);

        bool shouldScheduleTimer = heap_.empty() || (dueTime < heap_.front()->DueTime());

        bool dueTimeIncreased = timer->DueTime() < dueTime; 
        if (timer->ShouldAddToHeap(dueTime))
        {
            auto nodeIndex = heap_.size();
            timer->SetHeapIndex(nodeIndex);
            heap_.emplace_back(std::forward<TSPtr>(timer));
            HeapAdjustUp_LockHeld(nodeIndex); //timer may have been moved by statement above
        }
        else
        {
            Invariant(!heap_.empty());
            if (dueTimeIncreased)
            {
                HeapAdjustDown_LockHeld(timer->HeapIndex());
            }
            else
            {
                HeapAdjustUp_LockHeld(timer->HeapIndex());
            }
        }

        if (shouldScheduleTimer)
        {
            SetTimer(t);
        }
    }
}

void TimerQueue::Enqueue(TimerSPtr const & timer, TimeSpan dueTime)
{
    EnqueueT(timer, dueTime);
}

void TimerQueue::Enqueue(TimerSPtr && timer, TimeSpan dueTime)
{
    EnqueueT(move(timer), dueTime);
}

void TimerQueue::Enqueue(StringLiteral tag, Callback const & callback, TimeSpan dueTime)
{
    Enqueue(CreateTimer(tag, callback), dueTime);
}

bool TimerQueue::Dequeue(TimerSPtr const & timer)
{
    Invariant(timer);

    AcquireWriteLock grab(lock_);

    if (!timer->IsInHeap())
    {
        WriteNoise(TraceType, "{0}: Dequeue: false", TextTracePtr(timer.get()));
        return false;
    }

    Invariant(!heap_.empty());

    if (heap_.back() == timer)
    {
        timer->ClearHeapIndex();
        heap_.pop_back();
        WriteNoise(TraceType, "{0}: Dequeue: true", TextTracePtr(timer.get()));
        return true;
    }

    Invariant(heap_.back()->HeapIndex() != timer->HeapIndex());

    //timer node and back() has no ancestor-descendent relationship
    bool shouldAdjustUp = heap_.back()->DueTime() < timer->DueTime();

    auto heapIndex = timer->ClearHeapIndex();
    heap_.back()->SetHeapIndex(heapIndex);
    heap_[heapIndex] = move(heap_.back()); //replaced *(heap_[heapIndex]) will not destruct under lock because caller has "timer"
    heap_.pop_back();

    if (shouldAdjustUp)
    {
        HeapAdjustUp_LockHeld(heapIndex);
        WriteNoise(TraceType, "{0}: Dequeue: true", TextTracePtr(timer.get()));
        return true;
    }

    HeapAdjustDown_LockHeld(heapIndex);
    WriteNoise(TraceType, "{0}: Dequeue: true", TextTracePtr(timer.get()));
    return true;
}

void TimerQueue::FireDueTimers()
{
    auto now = Stopwatch::Now();
    vector<TimerSPtr> timersToFire;
    {
        AcquireWriteLock grab(lock_);

        while(!heap_.empty() && (heap_.front()->DueTime() <= now))
        {
            auto idx = heap_.front()->ClearHeapIndex();
            Invariant(idx == 0);

            WriteNoise(
                TraceType,
                "{0}: {1} '{2}': calling callback,  asyncDispatch_ = {3}",
                TextTraceThis, TextTracePtr(heap_.front().get()), heap_.front()->Tag(), asyncDispatch_);

            if (asyncDispatch_)
            {
               Threadpool::Post([timerToFire = move(heap_.front())] { timerToFire->Fire(); });
            }
            else
            {
                timersToFire.emplace_back(move(heap_.front()));
            }

            if (heap_.size() > 1)
            {
                heap_.front() = move(heap_.back()); 
                heap_.front()->SetHeapIndex(0);
                heap_.pop_back();

                HeapAdjustDown_LockHeld(0);
                continue;
            }

            heap_.pop_back();
        }

        if (!heap_.empty())
        {
            SetTimer(heap_.front()->DueTime() - now); 
        }
    }

    if (!timersToFire.empty())
    {
        auto beforeDispatch = Stopwatch::Now();
        for(auto const & timerToFire : timersToFire)
        {
            timerToFire->Fire();
            auto afterDispatch = Stopwatch::Now();
            if ((afterDispatch - beforeDispatch) >= dispatchTimeThreshold_)
            {
                WriteInfo(
                    TraceType,
                    "{0}: {1} '{2}': slow callback, dispatchTimeThreshold_ = {3}", 
                    TextTraceThis, TextTracePtr(timerToFire.get()), timerToFire->Tag(), dispatchTimeThreshold_);
            }

            beforeDispatch = afterDispatch;
        }
    }
}

TimerQueue::TimerSPtr TimerQueue::CreateTimer(Common::StringLiteral const tag, Callback const & callback)
{
    return make_shared<Timer>(this, tag, callback);
}

TimerQueue::TimerQueue(bool asyncDispatch) : asyncDispatch_(asyncDispatch), dispatchTimeThreshold_(CommonConfig::GetConfig().TimerQueueDispatchTimeThreshold)
{
    WriteInfo(TraceType, "{0}: asyncDispatch_ = {1}, dispatchTimeThreshold_  = {2}", TextTraceThis, asyncDispatch_, dispatchTimeThreshold_); 

    InitSignalPipe();
    CreatePosixTimer();
    heap_.reserve(200000);
}

void TimerQueue::InitSignalPipe()
{
    ZeroRetValAssert(pipe2(pipeFd_, O_CLOEXEC));

    pthread_t tid;
    ZeroRetValAssert(pthread_create(&tid, nullptr, &SignalPipeLoopStatic, this));

    struct sigaction sa = {};
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sa.sa_sigaction = SigHandler;
    sigemptyset(&sa.sa_mask);
    ZeroRetValAssert(sigaction(FABRIC_SIGNO_LEASE_TIMER, &sa, nullptr));

    sigset_t mask = {};
    sigemptyset(&mask);
    sigaddset(&mask, FABRIC_SIGNO_LEASE_TIMER);
    ZeroRetValAssert(sigprocmask(SIG_UNBLOCK, &mask, nullptr));
}

void* TimerQueue::SignalPipeLoopStatic(void* arg)
{
    auto thisPtr = (TimerQueue*)arg;
    thisPtr->SignalPipeLoop();
}

void TimerQueue::SignalPipeLoop()
{
    SigUtil::BlockAllFabricSignalsOnCallingThread(); //block timer signal to avoid deadlock when pipe is full

    static const uint readBatch = 64;
    TimerQueue* expiredTimers[readBatch];

    WriteInfo(TraceType, "start signal pipe loop");
    for(;;)
    {
        auto len = read(pipeFd_[0], expiredTimers, sizeof(expiredTimers));
        if (len< 0)
        {
            ASSERT_IF(errno != EINTR, "{0}: pipe read failed with errno = {1:x}", __FUNCTION__, errno);
        }

        if (len == 0)
        {
            WriteInfo(TraceType, "pipe read returned 0, stop loop");
            break;
        }

        FireDueTimers();
    }
}

void TimerQueue::CreatePosixTimer()
{
    sigevent sigEvent = {};
    sigEvent.sigev_notify = SIGEV_SIGNAL;
    sigEvent.sigev_signo = FABRIC_SIGNO_LEASE_TIMER;
    sigEvent.sigev_value.sival_ptr = this;

    ZeroRetValAssert(timer_create(CLOCK_MONOTONIC, &sigEvent, &timer_));
}

void TimerQueue::SigHandler(int sig, siginfo_t *si, void*)
{
    TimerQueue* thisPtr = (TimerQueue*)si->si_value.sival_ptr;
    // No need to check overrun as we maintain our own queue
    // Threadpool::Post is not signal safe, thus we need to write into a pipe
    auto written = write(thisPtr->pipeFd_[1], &thisPtr, sizeof(thisPtr));
    Invariant(written == sizeof(thisPtr));
}

void TimerQueue::SetTimer(TimeSpan dueTime)
{
    WriteNoise(TraceType, "{0}: SetTimer({1})", TextTraceThis, dueTime);

    itimerspec timerSpec = {};
    timerSpec.it_value = Common::Timer::ToTimeSpecDuetime(dueTime);

    auto retval = timer_settime(timer_, 0, &timerSpec, NULL);
    Invariant(retval == 0);
}

static INIT_ONCE initOnce;
static Global<TimerQueue> singleton;

static BOOL CALLBACK InitOnceFunc(PINIT_ONCE, PVOID, PVOID *)
{
    singleton = make_global<TimerQueue>();
    return TRUE;
}

TimerQueue & TimerQueue::GetDefault()
{
    PVOID lpContext = NULL;
    BOOL result = ::InitOnceExecuteOnce(&initOnce, InitOnceFunc, NULL, &lpContext);
    Invariant(result);
    return *singleton;
}
