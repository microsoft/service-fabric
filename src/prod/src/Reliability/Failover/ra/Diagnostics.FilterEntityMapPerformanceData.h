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
            class FilterEntityMapPerformanceData
            {
            public:
                FilterEntityMapPerformanceData(Infrastructure::IClockSPtr const & clock) : 
                    clock_(clock),
                    mapItemCount_(0),
                    filteredItemCount_(0)
                {
                }

                __declspec(property(get = get_SnapshotDuration)) Common::TimeSpan SnapshotDuration;
                Common::TimeSpan get_SnapshotDuration() const { return snapshotDuration_;}

                __declspec(property(get = get_MapItemCount)) int64 MapItemCount;
                int64 get_MapItemCount() const { return mapItemCount_; }

                __declspec(property(get = get_FilterDuration)) Common::TimeSpan FilterDuration;
                Common::TimeSpan get_FilterDuration() const { return filterDuration_; }

                __declspec(property(get = get_FilteredItemCount)) int64 FilteredItemCount;
                int64 get_FilteredItemCount() const { return filteredItemCount_; }

                void OnStarted()
                {
                    startTime_ = clock_->Now();
                }

                void OnSnapshotComplete(size_t itemCount)
                {
                    auto now = clock_->Now();
                    snapshotDuration_ = now - startTime_;
                    mapItemCount_ = static_cast<int64>(itemCount);
                    startTime_ = now;
                }

                void OnFilterComplete(size_t filterCount)
                {
                    filterDuration_ = clock_->Now() - startTime_;
                    filteredItemCount_ = static_cast<int64>(filterCount);
                }

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const &)
                {
                    size_t index = 0;
                    traceEvent.AddEventField<double>("snapShotDurationMs", index);
                    traceEvent.AddEventField<int64>("mapItemCount", index);
                    traceEvent.AddEventField<double>("filterDurationMs", index);
                    traceEvent.AddEventField<int64>("filteredItemCount", index);

                    return "Snapshot Duration: {0}ms. Map Item Count: {1}. Filter Duration: {2}ms. Filtered Item Count: {3}.";
                }

                void FillEventData(Common::TraceEventContext & context) const
                {
                    context.WriteCopy<double>(snapshotDuration_.TotalMillisecondsAsDouble());
                    context.Write<int64>(mapItemCount_);
                    context.WriteCopy<double>(filterDuration_.TotalMillisecondsAsDouble());
                    context.Write<int64>(filteredItemCount_);
                }

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
                {
                    auto msg = Common::wformatString(
                        "Snapshot Duration: {0}ms. Map Item Count: {1}. Filter Duration: {2}ms. Filtered Item Count: {3}.",
                        snapshotDuration_.TotalMillisecondsAsDouble(),
                        mapItemCount_,
                        filterDuration_.TotalMillisecondsAsDouble(),
                        filteredItemCount_);

                    w << msg;
                }

            private:
                Infrastructure::IClockSPtr clock_;
                Common::StopwatchTime startTime_;
                Common::TimeSpan snapshotDuration_;
                int64 mapItemCount_;
                Common::TimeSpan filterDuration_;
                int64 filteredItemCount_;
            };
        }
    }
}



