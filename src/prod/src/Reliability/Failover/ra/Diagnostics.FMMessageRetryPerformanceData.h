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
            class FMMessageRetryPerformanceData
            {
            public:
                FMMessageRetryPerformanceData(Infrastructure::IClockSPtr const & clock) :
                    pendingEntityCount_(0),
                    generatedItemCount_(0),
                    clock_(clock)
                {
                }

                void OnSnapshotStart()
                {
                    TESTASSERT_IF(snapshotStartTimestamp_ != Common::StopwatchTime::Zero, "invalid enumeration timestamp at snapshot start");
                    snapshotStartTimestamp_ = clock_->Now();
                }

                void OnGenerationStart(int64 pendingEntityCount)
                {
                    TESTASSERT_IF(snapshotStartTimestamp_ == Common::StopwatchTime::Zero, "invalid enumeration timestamp at generation start");
                    snapshotDuration_ = clock_->Now() - snapshotStartTimestamp_;
                    pendingEntityCount_ = pendingEntityCount;
                }

                void OnGenerationEnd(int64 generatedCount)
                {
                    TESTASSERT_IF(snapshotStartTimestamp_ == Common::StopwatchTime::Zero, "invalid enumeration timestamp at generation start");
                    generationDuration_ = clock_->Now() - snapshotStartTimestamp_ - snapshotDuration_;
                    generatedItemCount_ = generatedCount;
                }

                void OnMessageSendStart()
                {
                    messageSendDurationStopwatch_.Start(*clock_);
                }

                void OnMessageSendFinish()
                {
                    messageSendDurationStopwatch_.Stop(*clock_);
                }

                void OnUpdateStart()
                {
                    updateDurationStopwatch_.Start(*clock_);
                }

                void OnUpdateEnd()
                {
                    updateDurationStopwatch_.Stop(*clock_);
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
                {
                    std::string format = "PendingCount: {0}. Generated: {1}. SnapshotDuration: {2}. GenerationDuration: {3}. MessageSendDuration: {4}. UpdateDuration: {5}";

                    size_t index = 0;
                    traceEvent.AddEventField<int64>(format, name + ".pending", index);
                    traceEvent.AddEventField<int64>(format, name + ".generated", index);

                    traceEvent.AddEventField<Common::TimeSpan>(format, name + ".snapshotDuration", index);
                    traceEvent.AddEventField<Common::TimeSpan>(format, name + ".generateDuration", index);
                    traceEvent.AddEventField<Common::TimeSpan>(format, name + ".messageSendDuration", index);
                    traceEvent.AddEventField<Common::TimeSpan>(format, name + ".updateDuration", index);

                    return format;
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.Write(pendingEntityCount_);
                    context.Write(generatedItemCount_);
                    context.Write(snapshotDuration_);
                    context.Write(generationDuration_);
                    context.WriteCopy(messageSendDurationStopwatch_.Elapsed);
                    context.WriteCopy(updateDurationStopwatch_.Elapsed);
                }

                void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
                {
                    auto msg = Common::wformatString(
                        "PendingCount: {0}. Generated: {1}. SnapshotDuration: {2}. GenerationDuration: {3}. MessageSendDuration: {4}. UpdateDuration: {5}",
                        pendingEntityCount_,
                        generatedItemCount_,
                        snapshotDuration_,
                        generationDuration_,
                        messageSendDurationStopwatch_.Elapsed,
                        updateDurationStopwatch_.Elapsed);

                    w << msg;
                }

            private:
                // The count of total entities that are pending
                int64 pendingEntityCount_;

                // The count of items generated (may be fewer)
                int64 generatedItemCount_;

                Common::StopwatchTime snapshotStartTimestamp_;
                Common::TimeSpan snapshotDuration_;
                Common::TimeSpan generationDuration_;

                Infrastructure::RAStopwatch messageSendDurationStopwatch_;
                Infrastructure::RAStopwatch updateDurationStopwatch_;

                Infrastructure::IClockSPtr clock_;
            };
        }
    }
}
