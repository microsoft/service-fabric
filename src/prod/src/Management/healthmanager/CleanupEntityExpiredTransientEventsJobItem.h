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
        class CleanupEntityExpiredTransientEventsJobItem
            : public CleanupEntityJobItemBase
        {
            DENY_COPY(CleanupEntityExpiredTransientEventsJobItem);
            
        public:
             CleanupEntityExpiredTransientEventsJobItem(
                std::shared_ptr<HealthEntity> const & entity,
                Common::ActivityId const & activityId);

            virtual HealthJobItemKind::Enum get_Type() const override { return HealthJobItemKind::UpdateEntityEvents; }

            virtual ~CleanupEntityExpiredTransientEventsJobItem();

        protected:

            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr) override;

            // Called to finish async work
            virtual void FinishAsyncWork() override;
        };
    }
}
