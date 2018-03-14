// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    class TimerQueue : public TextTraceComponent<TraceTaskCodes::Timer>
    {
        DENY_COPY(TimerQueue);

    public:
        typedef std::function<void(void)> Callback;

        class Timer;
        typedef std::shared_ptr<Timer> TimerSPtr;
        TimerSPtr CreateTimer(StringLiteral tag, Callback const & callback);

        static TimerQueue & GetDefault();
        TimerQueue(bool asyncDispatch = true);

        void Enqueue(TimerSPtr const & timer, TimeSpan dueTime);
        void Enqueue(TimerSPtr && timer, TimeSpan dueTime);
        void Enqueue(StringLiteral tag, Callback const & callback, TimeSpan dueTime);
        bool Dequeue(TimerSPtr const & timer);

        bool IsTimerArmed(TimerSPtr const & timer);

    private:
        static void SigHandler(int sig, siginfo_t *si, void*);
        static void* SignalPipeLoopStatic(void*);

        template <typename TSPtr>
        void EnqueueT(TSPtr &&  timer, TimeSpan dueTime);

        void InitSignalPipe();
        void CreatePosixTimer();
        void SignalPipeLoop();
        void SetTimer(Common::TimeSpan dueTime);
        void FireDueTimers();

        void HeapAdjustUp_LockHeld(size_t nodeIndex);
        void HeapAdjustDown_LockHeld(size_t nodeIndex);
        void HeapNodeSwap_LockHeld(size_t index1, size_t index2);
        void HeapCheck_Dbg();

        mutable Common::RwLock lock_;

        timer_t timer_;
        int pipeFd_[2];

        std::vector<TimerSPtr> heap_;

        const bool asyncDispatch_;
        const TimeSpan dispatchTimeThreshold_;
    };
}
