// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#ifdef PLATFORM_UNIX
#define TimerTagDefault __PRETTY_FUNCTION__
#else
#define TimerTagDefault __FUNCTION__
#endif

namespace Common
{
    class Timer;
    typedef std::shared_ptr<Timer> TimerSPtr;
    typedef std::weak_ptr<Timer> TimerWPtr;

#ifdef PLATFORM_UNIX
    class TimerFinalizer;
#endif

    class Timer : public std::enable_shared_from_this<Timer>, public TextTraceComponent<TraceTaskCodes::Timer>
    {
        DENY_COPY(Timer);

    public:
        typedef std::function<void(TimerSPtr const &)> TimerCallback;

        // Please call Create methods to instantiate, do NOT call constructor directly !!!
        // Constructors are made public only to enable make_shared for easier debugging.
        static TimerSPtr Create(
            StringLiteral tag, // string literal indicating what the timer is for
            // You can use TimerTagDefault if you do not want to come up with one
            TimerCallback const & callback,
            bool allowConcurrency = true,
            PTP_CALLBACK_ENVIRON pcbe = nullptr);

        Timer(StringLiteral tag, TimerCallback const & callback, bool allowConcurrency, PTP_CALLBACK_ENVIRON pcbe);
        virtual ~Timer();

        void SetCallback(TimerCallback const & callback);
        void SetCancelWait();

        void Change(Common::TimeSpan dueTime, Common::TimeSpan period = Common::TimeSpan::MaxValue);
        void ChangeWithLowerBoundDelay(Common::TimeSpan dueTime, Common::TimeSpan lowerBound = Common::TimeSpan::FromMilliseconds(100));

        // !!! Cancel() must be called when you are done with the timer, because most of time our timer is in some reference cycle.
        // Cancel breaks the reference cycle, stops the timer and cleans up. By default, to avoid deadlock, Cancel() doesn't wait for
        // callback copmletion. You can force wait for callback completion by calling SetCancelWait() before starting timer. With
        // "cancel wait" enabled, Cancel() cannot be called from timer callback.
#ifdef PLATFORM_UNIX
        void Cancel(bool disposeNow = false);
#else
        void Cancel();
#endif

        bool Test_IsSet() const; // For CIT only

        static Common::atomic_uint64 ObjCount;

#ifdef PLATFORM_UNIX
        void LimitToOneShot();

        static timespec ToTimeSpecDuetime(TimeSpan dueTime);
        static timespec ToTimeSpecPeriod(TimeSpan period);
        static timespec ToTimeSpecWithLowerBound(TimeSpan interval);

#else
        static void Callback(PTP_CALLBACK_INSTANCE, void* lpParameter, PTP_TIMER);
#endif

    private:
        mutable RwLock thisLock_;
        const char * const tag_; //only stores string literal

        TimerCallback callback_;

        const bool allowConcurrency_ = true;
        bool cancelCalled_ = false;
        bool started_ = false;
        bool waitForCallbackOnCancel_ = false;

        // callbackRunning_ is initialized to 1 to indicate that Cancel() hasn't been called yet
        atomic_long callbackRunning_{1};

#ifdef PLATFORM_UNIX
        StopwatchTime CancelTime() const;
        void SetCallbackTidIfNeeded();
        void ClearCallbackTidIfNeeded();
        void NotifyCancelCompletionIfNeeded();
        void WaitForCancelCompletionIfNeeded();
        void Dispose();
        bool CreatePosixTimer_CallerHoldingLock();
        void Callback();
        static void SigHandler(int sig, siginfo_t *si, void*);
        static BOOL InitOnceFunction(PINIT_ONCE, PVOID, PVOID*);
        static void* SignalPipeLoop(void*);

        TimerSPtr thisSPtr_;
        timer_t timer_ = nullptr;
        TimerQueue::TimerSPtr queuedTimer_;
        TimeSpan period_ = TimeSpan::MaxValue;
        volatile pthread_t callbackThreadId_ = 0;
        bool oneShotOnly_ = false;
        bool callbackCalled_ = false;
        bool callbackTidSet_ = false;
        bool useTimerQueue_ = false;
        std::unique_ptr<ManualResetEvent> allCallbackCompleted_;
        StopwatchTime cancelTime_ = StopwatchTime::MaxValue;

        friend class TimerFinalizer;
#else
        bool SetTimer(PFILETIME dueTimePtr, DWORD periodValue);
        PTP_TIMER GetTimerCallerHoldingLock();
        DWORD ComputeDWordPeriod(Common::TimeSpan period);

        PTP_CALLBACK_ENVIRON pcbe_ = nullptr;
        PTP_TIMER timer_ = nullptr;
        volatile DWORD callbackThreadId_ = 0;
#endif
    };
}
