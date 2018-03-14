// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Diagnostics
        {
            class ExecuteEntityJobItemListPerformanceData
            {
            public:
                ExecuteEntityJobItemListPerformanceData() :
                    jobItemCount_(0)
                {
                }

                __declspec(property(get = get_LockDuration)) Common::TimeSpan LockDuration;
                Common::TimeSpan get_LockDuration() const { return lockDuration_.Elapsed; }

                __declspec(property(get = get_ProcessDuration)) Common::TimeSpan ProcessDuration;
                Common::TimeSpan get_ProcessDuration() const { return  processDuration_.Elapsed; }

                __declspec(property(get = get_PostProcessDuration)) Common::TimeSpan PostProcessDuration;
                Common::TimeSpan get_PostProcessDuration() const { return postProcessDuration_.Elapsed; }

                void OnExecutionStart(size_t jobItemCount, Infrastructure::IClock & clock)
                {
                    jobItemCount_ = jobItemCount;

                    lockDuration_.Start(clock);
                    processDuration_.Start(clock);
                }

                void OnExecutionEnd(Infrastructure::IClock & clock)
                {
                    postProcessDuration_.Stop(clock);
                    lockDuration_.Stop(clock);
                }

                void OnProcessed(Infrastructure::IClock & clock)
                {
                    processDuration_.Stop(clock);
                }

                void OnPostProcessStart(Infrastructure::IClock & clock)
                {
                    postProcessDuration_.Start(clock);
                }

                void ReportPerformanceData(RAPerformanceCounters & perfCounters) const
                {
                    ASSERT_IF(jobItemCount_ == 0, "Cannot have 0 job items");

                    perfCounters.AverageJobItemsPerEntityLock.IncrementBy(jobItemCount_);
                    perfCounters.AverageJobItemsPerEntityLockBase.Increment();

                    perfCounters.UpdateAverageDurationCounter(
                        lockDuration_.Elapsed,
                        perfCounters.AverageEntityLockTimeBase,
                        perfCounters.AverageEntityLockTime);
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & )
                {
                    size_t index = 0;
                    traceEvent.AddEventField<int64>("lockDuration", index);
                    traceEvent.AddEventField<int64>("processDuration", index);
                    traceEvent.AddEventField<int64>("postProcessDuration", index);

                    return "LockDuration: {0}ms ProcessDuration: {1}ms PostProcessDuration: {2}ms";
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.WriteCopy<int64>(LockDuration.TotalMilliseconds());
                    context.WriteCopy<int64>(ProcessDuration.TotalMilliseconds());
                    context.WriteCopy<int64>(PostProcessDuration.TotalMilliseconds());
                }

                void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
                {
                    w << Common::wformatString("LockDuration: {0}ms ProcessDuration: {1}ms PostProcessDuration: {2}ms", LockDuration.TotalMilliseconds(), ProcessDuration.TotalMilliseconds(), PostProcessDuration.TotalMilliseconds());
                }

            private:
                Infrastructure::RAStopwatch lockDuration_;
                Infrastructure::RAStopwatch processDuration_;
                Infrastructure::RAStopwatch postProcessDuration_;
                size_t jobItemCount_;
            };
        }
    }
}
