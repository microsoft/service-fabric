// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "StdAfx.h"

namespace Common
{
    atomic_uint64 Timer::ObjCount(0);
    static Global<TimerEventSource> const trace = make_global<TimerEventSource>();

    Timer::Timer(StringLiteral tag, TimerCallback const & callback, bool allowConcurrency, PTP_CALLBACK_ENVIRON pcbe)
        : tag_(tag.cbegin())
        , pcbe_(pcbe)
        , callback_(callback)
        , allowConcurrency_(allowConcurrency)
    {
        Invariant(callback_);
        Invariant(tag.size()); // empty tag is not allowed
        trace->Created(TraceThis, tag, ++ObjCount);
    }
    
    Timer::~Timer()
    {
        LONG callbackRunning = -- callbackRunning_;
        ASSERT_IFNOT(
            callbackRunning == -1,
            "Timer('{0}')@{1}: Cancel() must be called before destruction, callbackRunning = {2}",
            tag_? tag_ : "",
            TextTraceThis,
            callbackRunning);

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

    void Timer::Change(TimeSpan dueTime, TimeSpan period) 
    {
        int64 ticks = (dueTime <= TimeSpan::Zero)? -1 : -dueTime.Ticks; // -1 means firing timer immediately
        PFILETIME dueTimePtr = (dueTime == TimeSpan::MaxValue) ? NULL : reinterpret_cast<PFILETIME>(&ticks);
        DWORD periodValue = ComputeDWordPeriod(period);

        bool scheduled = SetTimer(dueTimePtr, periodValue);
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

    bool Timer::SetTimer(PFILETIME dueTimePtr, DWORD periodValue)
    {
        started_ = true;

        AcquireExclusiveLock grab(thisLock_);
        PTP_TIMER timer = GetTimerCallerHoldingLock();
        if (timer != nullptr)
        {
            // It is assumed that Windows thread pool won't schedule timer callback synchronously inside SetThreadpoolTimer().
            SetThreadpoolTimer(timer, dueTimePtr, periodValue, 0);
            return true;
        }

        return false;
    }

    void Timer::SetCancelWait()
    {
        ASSERT_IF(started_, "SetCancelWait() cannot be called after timer is started");
        ASSERT_IF(allowConcurrency_, "SetCancelWait() not supported for concurrent callbacks");
        waitForCallbackOnCancel_ = true;
    }

    void Timer::Cancel()
    {
        trace->CancelCalled(TraceThis, waitForCallbackOnCancel_);

        PTP_TIMER timerToCancel = nullptr;
        bool shouldCancel;
        {
            AcquireExclusiveLock grab(thisLock_);
            shouldCancel = ! cancelCalled_;
            if (shouldCancel)
            {
                cancelCalled_ = true;
                timerToCancel = timer_;
                timer_ = nullptr;
            }
        }

        if (shouldCancel)
        {
            if (timerToCancel != nullptr)
            {
                if (waitForCallbackOnCancel_ && (::GetCurrentThreadId() == callbackThreadId_))
                {
                    trace->CancelWaitSkipped(TraceThis);
                }
                else
                {
                    trace->CancelWait(TraceThis);
                    SetThreadpoolTimer(timerToCancel, NULL, 0, 0 );
                    WaitForThreadpoolTimerCallbacks(timerToCancel, TRUE /* cancel pending callbacks */);
                    CloseThreadpoolTimer(timerToCancel);
                }
            }

            // "--callbackRunning_" to indicate cancellation to Timer::Callback(). This cancel decrement has to happen after the 
            // call to WaitForThreadpoolTimerCallbacks() above. Otherwise, the result of "++callbackRunning_" in Timer::Callback()
            // may be affected by this, thus cannot be used to determine whether client callback should be called.
            LONG callbackRunning = -- callbackRunning_;
            ASSERT_IF(
                callbackRunning < 0,
                "Timer('{0}')@{1}: Cancel() must be called before destruction, callbackRunning = {2}",
                tag_? tag_ : "",
                TextTraceThis,
                callbackRunning);

            if (callbackRunning == 0)
            {
                callback_ = nullptr;
            }
        }
    }

    void Timer::Callback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_TIMER)
    {
        // Increment ref count to make sure Timer object is alive 
        TimerSPtr thisSPtr = reinterpret_cast<Timer*>(callbackParameter)->shared_from_this();
        trace->EnterCallback(TracePtr(thisSPtr.get()));

        if (thisSPtr->waitForCallbackOnCancel_)
        {
            thisSPtr->callbackThreadId_ = ::GetCurrentThreadId();
        }

        LONG callbackRunning = ++ thisSPtr->callbackRunning_;
        if ((callbackRunning == 2) // is this the only callback instance running currently?
            // or, if there are more than callback instances running, is concurrency allowed? 
            || ((callbackRunning > 2) && thisSPtr->allowConcurrency_))
        {
            if (!thisSPtr->waitForCallbackOnCancel_)
            {
                // The following call makes sure WaitForThreadpoolTimerCallbacks() won't wait for this callback to complete. This way,
                // Timer::Cancel() won't block on callbacks, since it only needs to wait for the completion of minimal work above.
                DisassociateCurrentThreadFromCallback(pci);
            }

            thisSPtr->callback_(thisSPtr);
        }

        // Clean up
        if (-- thisSPtr->callbackRunning_ == 0)
        { 
            // We cannot do this under the lock, since disconnecting callback can trigger destructor, which might call back into Timer.
            thisSPtr->callback_ = nullptr;
        }

        if (thisSPtr->waitForCallbackOnCancel_)
        {
            thisSPtr->callbackThreadId_ = 0;
        }

        trace->LeaveCallback(TracePtr(thisSPtr.get()));
    }

    PTP_TIMER Timer::GetTimerCallerHoldingLock()
    {
        if (cancelCalled_) { return nullptr; }
        if (timer_ == nullptr)
        {
            timer_ = ::CreateThreadpoolTimer(&Threadpool::TimerCallback<Timer>, this, pcbe_);
            CHK_WBOOL( timer_ != nullptr );
        }

        return timer_;
    }

    DWORD Timer::ComputeDWordPeriod(TimeSpan period)
    {
        if (period == TimeSpan::MaxValue)
        {
            return 0;
        }

        ASSERT_IFNOT(period >= TimeSpan::Zero, "period cannot be negative: {0}", period);
        TESTASSERT_IFNOT(period.TotalMilliseconds() <= std::numeric_limits<DWORD>().max(), "period in milliseconds is beyond DWORD range: {0}", period);
        int64 totalMilliseconds = (period.TotalMilliseconds() <= std::numeric_limits<DWORD>().max()) ? period.TotalMilliseconds() : std::numeric_limits<DWORD>().max();
        return static_cast<DWORD>(totalMilliseconds);
    }

    bool Timer::Test_IsSet() const
    {
        AcquireReadLock grab(thisLock_);

        if (!timer_)
        {
            return false;
        }

        return IsThreadpoolTimerSet(timer_) == TRUE;
    }
}
