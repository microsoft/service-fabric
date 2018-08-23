//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Job item that cleans up events when there are too many reports.
        class AutoCleanupEventsJobItem
            : public CleanupEntityJobItemBase
        {
            DENY_COPY(AutoCleanupEventsJobItem);
            
        public:
             AutoCleanupEventsJobItem(
                std::shared_ptr<HealthEntity> const & entity,
                Common::ActivityId const & activityId);

            virtual HealthJobItemKind::Enum get_Type() const override { return HealthJobItemKind::UpdateEntityEvents; }

            virtual ~AutoCleanupEventsJobItem();

        protected:

            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr) override;

            // Called to finish async work
            virtual void FinishAsyncWork() override;
        };
    }
}