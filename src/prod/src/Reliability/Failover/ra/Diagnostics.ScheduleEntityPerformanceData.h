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
            class ScheduleEntityPerformanceData
            {
            public:
                __declspec(property(get = get_ScheduleTime)) Common::TimeSpan ScheduleDuration;
                Common::TimeSpan get_ScheduleTime() const { return scheduleDurationStopwatch_.Elapsed; }

                __declspec(property(get = get_QueueTime)) Common::TimeSpan QueueDuration;
                Common::TimeSpan get_QueueTime() const { return queueDurationStopwatch_.Elapsed; }

                void OnScheduleStart(Infrastructure::IClock & clock)
                {
                    scheduleDurationStopwatch_.Start(clock);
                }

                void OnScheduleEnd(Infrastructure::IClock & clock)
                {
                    scheduleDurationStopwatch_.Stop(clock);
                }

                void OnQueueStart(Infrastructure::IClock & clock)
                {
                    queueDurationStopwatch_.Start(clock);
                }

                void OnQueueEnd(Infrastructure::IClock & clock)
                {
                    queueDurationStopwatch_.Stop(clock);
                }

                void ReportPerformanceData(RAPerformanceCounters & perfCounters)
                {
                    perfCounters.UpdateAverageDurationCounter(scheduleDurationStopwatch_.Elapsed, perfCounters.AverageEntityScheduleTimeBase, perfCounters.AverageEntityScheduleTime);
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const &)
                {
                    size_t index = 0;
                    traceEvent.AddEventField<int64>("queueDuration", index);
                    traceEvent.AddEventField<int64>("scheduleDuration", index);

                    return "QueueDuration: {0}ms ScheduleDuration: {1}ms";
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.WriteCopy<int64>(QueueDuration.TotalMilliseconds());
                    context.WriteCopy<int64>(ScheduleDuration.TotalMilliseconds());
                }

                void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
                {
                    w << Common::wformatString("QueueDuration: {0}ms ScheduleDuration: {1}ms", QueueDuration.TotalMilliseconds(), ScheduleDuration.TotalMilliseconds());
                }

            private:
                Infrastructure::RAStopwatch scheduleDurationStopwatch_;
                Infrastructure::RAStopwatch queueDurationStopwatch_;
            };
        }
    }
}



