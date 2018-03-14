// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class TimedCounter
        {
            DENY_COPY_ASSIGNMENT(TimedCounter);

        public:
            TimedCounter(Common::TimeSpan interval, size_t windowCount);
            TimedCounter(TimedCounter const & other);
            TimedCounter(TimedCounter && other);

            void Reset();

            void Record(size_t count, Common::StopwatchTime now);

            void Refresh(Common::StopwatchTime now);

            size_t GetCount() const;

            void Merge(TimedCounter & other, Common::StopwatchTime now);

        private:
            void AdjustWindows(Common::StopwatchTime now);

            size_t windowCount_;
            Common::StopwatchTime startTime_;
            Common::StopwatchTime lastOperationTime_;

            Common::TimeSpan intervalPerWindow_;
            std::deque<size_t> counts_;

            size_t totalCount_;
        };
    }
}
