// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        struct GroupHealthStateCount
        {
            DENY_COPY(GroupHealthStateCount)

        public:
            explicit GroupHealthStateCount(BYTE maxPercentUnhealthy)
                : MaxPercentUnhealthy(maxPercentUnhealthy)
                , Count()
                , AggregatedHealthState(FABRIC_HEALTH_STATE_UNKNOWN)
                , entries_()
            {
            }

            GroupHealthStateCount(GroupHealthStateCount && other) 
                : MaxPercentUnhealthy(std::move(other.MaxPercentUnhealthy))
                , Count(std::move(other.Count))
                , AggregatedHealthState(std::move(other.AggregatedHealthState))
                , entries_(std::move(other.entries_))
            {
            }

            GroupHealthStateCount & operator = (GroupHealthStateCount && other)
            {
                if (this != &other)
                {
                    MaxPercentUnhealthy = std::move(other.MaxPercentUnhealthy);
                    Count = std::move(other.Count);
                    AggregatedHealthState = std::move(other.AggregatedHealthState);
                    entries_ = std::move(other.entries_);
                }

                return *this;
            }

            ~GroupHealthStateCount() {}

            FABRIC_HEALTH_STATE GetHealthState() 
            { 
                AggregatedHealthState = Count.GetHealthState(MaxPercentUnhealthy); 
                return AggregatedHealthState;
            }

            void Add(FABRIC_HEALTH_STATE aggregatedHealthState, ServiceModel::HealthEvaluationBaseSPtr && entry)
            {
                Count.AddResult(aggregatedHealthState);

                if (entry)
                {
                    entries_.push_back(ServiceModel::HealthEvaluation(std::move(entry)));
                }
            }

            std::vector<ServiceModel::HealthEvaluation> GetUnhealthy() const
            {
                return HealthCount::FilterUnhealthy(entries_, AggregatedHealthState);
            }

            BYTE MaxPercentUnhealthy;
            HealthCount Count;
            FABRIC_HEALTH_STATE AggregatedHealthState;

        private:
            std::vector<ServiceModel::HealthEvaluation> entries_;
        };
    }
}
