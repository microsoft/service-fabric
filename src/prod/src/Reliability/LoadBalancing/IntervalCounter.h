// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class IntervalCounter
        {
            DENY_COPY_ASSIGNMENT(IntervalCounter);

        public:
            IntervalCounter(Common::TimeSpan interval);
            IntervalCounter(IntervalCounter const & other);
            IntervalCounter(IntervalCounter && other);

            void Reset();

            void Record(size_t count, Common::StopwatchTime now);
            
            void Refresh(Common::StopwatchTime now);

            size_t GetCount() const;

            size_t GetCountsSize() const;

        private:
            void AdjustWindow(Common::StopwatchTime now);

            std::deque<std::pair<size_t, int64> > counts_;

            size_t totalCount_;

            int64 intervalTicks_;
        };
    }
}
