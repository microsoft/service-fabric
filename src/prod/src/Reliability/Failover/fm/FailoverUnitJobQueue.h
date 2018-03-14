// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        struct FailoverUnitJob : public Common::JobItem<FailoverManager>
        {
        public:
            FailoverUnitJob()
            {
            }

            explicit FailoverUnitJob(FailoverUnitId const & failoverUnitId, Federation::NodeInstance from, bool isFromPLB = false);

            FailoverUnitJob(FailoverUnitJob && other) 
                : failoverUnitId_(std::move(other.failoverUnitId_))
            {
            }

            FailoverUnitJob & operator=(FailoverUnitJob && other)
            {
                if (this == &other)
                {
                    return *this;
                }

                failoverUnitId_ = std::move(other.failoverUnitId_);
                return *this;
            }

            bool NeedThrottle(FailoverManager & fm);

            bool ProcessJob(FailoverManager & fm);
            
            void OnQueueFull(FailoverManager &, size_t);

        private:
            FailoverUnitId failoverUnitId_;
            bool isFromPLB_;
            Federation::NodeInstance from_;
            Common::TimeSpan minTimeBetweeRejectMessages_ = FailoverConfig::GetConfig().MinSecondsBetweenQueueFullRejectMessages;
        };

        class FailoverUnitJobQueue : public Common::JobQueue<FailoverUnitJob, FailoverManager>
        {
            DENY_COPY(FailoverUnitJobQueue);

        public:
            FailoverUnitJobQueue(FailoverManager & fm);
        };
    }

}
