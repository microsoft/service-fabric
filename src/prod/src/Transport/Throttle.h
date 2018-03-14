// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class Throttle : Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(Throttle)

    public:
        static Throttle * GetThrottle();
        bool ShouldThrottle(Message const & message, bool mayThrottleReply);
        PTP_CALLBACK_ENVIRON MonitorCallbackEnv();

        static void Enable();
        static bool IsEnabled();
        
        // Helper function to reload config values for fabric test
        static void Test_Reload();

    private:
        class SampleHistory
        {
        public:
            SampleHistory();
            void Initialize(size_t samplesPerCycle);
            int64 AddSample(int64 sample);

        private:
            std::vector<int64> data_;
            size_t samplePerEntry_;
            size_t boundaryIndex_;
            size_t index1_;
            size_t index2_;
            int64 total1_;
            int64 total2_;
        };

        Throttle();
        void EnableInternal();
        void TimerCallback();
        void QueryPerfData();
        void GetMemorySize();
        void IncreaseMemoryThrottle(int64 baseValue, int64 upperLimit);

        bool IsThreadThresholdExceeded(LONG count);
        bool IsMemoryThresholdExceeded();

        void ReloadThreadThrottle();
        void ReloadMemoryThrottle();
        void ReloadMemoryThrottleUpperLimit();
        void ReloadMonitorInterval();
        void ReloadMonitorTraceRatio();

        static BOOL CALLBACK InitFunction(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;

        static Throttle* singleton_;

#ifdef PLATFORM_UNIX
        Common::ProcessInfo& processInfo_;
#endif

        Common::Global<Common::Threadpool> monitorThreadpool_;

        LONG threadThrottle_;
        Common::StopwatchTime threadThrottleStartedTime_;
        Common::StopwatchTime memoryThrottleStartedTime_;
        Common::TimeSpan throttleTestAssertThreshold_;
        Common::TimeSpan memoryThrottleInterval_;
        bool throttleEnabled_;
        int threadCount_;
        int64 memorySize_;
        int64 memoryThrottle_;
        int64 memoryThrottleUpperLimit_;
        double memoryThrottleIncrementRatio_;
        uint perMonitorTraceRatio_;
        uint perfQueryCount_;
        std::shared_ptr<Common::Timer> monitorTimer_;
        std::shared_ptr<PerfCounters> perfCounters_;
        MUTABLE_RWLOCK(Transport.Throttle, lock_);
        SampleHistory shortMemoryHistory_;
        SampleHistory longMemoryHistory_;
    };
}
