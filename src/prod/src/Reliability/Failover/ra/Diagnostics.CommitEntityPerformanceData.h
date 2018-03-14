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
            class CommitEntityPerformanceData
            {
            public:
                CommitEntityPerformanceData() : wasReported_(false) {}

                __declspec(property(get = get_CommitDuration)) Common::TimeSpan CommitDuration;
                Common::TimeSpan get_CommitDuration() const { return stopwatch_.Elapsed; }

                void OnStoreCommitStart(Infrastructure::IClock & clock)
                {
                    wasReported_ = true;
                    stopwatch_.Start(clock);
                }

                void OnStoreCommitEnd(Infrastructure::IClock & clock)
                {
                    stopwatch_.Stop(clock);
                }

                void ReportPerformanceData(RAPerformanceCounters & perfCounters) const
                {
                    if (!wasReported_)
                    {
                        return;
                    }

                    perfCounters.UpdateAverageDurationCounter(
                        CommitDuration,
                        perfCounters.AverageEntityCommitTimeBase,
                        perfCounters.AverageEntityCommitTime);
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & )
                {
                    size_t index = 0;
                    traceEvent.AddEventField<int64>("commitDuration", index);

                    return "CommitDuration: {0}ms";
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.WriteCopy<int64>(CommitDuration.TotalMilliseconds());
                }

                void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
                {
                    w << Common::wformatString("CommitDuration: {0}ms", CommitDuration.TotalMilliseconds());
                }

            private:
                bool wasReported_;
                Infrastructure::RAStopwatch stopwatch_;
            };
        }
    }
}



