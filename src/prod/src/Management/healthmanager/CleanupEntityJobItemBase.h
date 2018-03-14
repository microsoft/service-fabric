// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Job item that cleans up expired transient events from an entity
        class CleanupEntityJobItemBase
            : public EntityJobItem
        {
            DENY_COPY(CleanupEntityJobItemBase);
            
        public:
             CleanupEntityJobItemBase(
                std::shared_ptr<HealthEntity> const & entity,
                Common::ActivityId const & activityId);

            virtual Store::ReplicaActivityId const & get_ReplicaActivityId() const override { return replicaActivityId_; }

            virtual ServiceModel::Priority::Enum get_JobPriority() const override { return ServiceModel::Priority::Normal; }

            virtual ~CleanupEntityJobItemBase();

            virtual bool CanCombine(IHealthJobItemSPtr const & other) const override;

            virtual void Append(IHealthJobItemSPtr && other) override;

        protected:

            // Called after the work is completed, on the same scheduling thread.
            virtual void OnComplete(Common::ErrorCode const & error) override;

        private:
            Store::ReplicaActivityId replicaActivityId_;
        };
    }
}
