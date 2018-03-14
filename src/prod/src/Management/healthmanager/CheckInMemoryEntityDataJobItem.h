// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Job item that checks whether the in-memory data of the entity is consistent with the store data.
        // If not, the replica must be restarted to read the correct data.
        class CheckInMemoryEntityDataJobItem
            : public CleanupEntityJobItemBase
        {
            DENY_COPY(CheckInMemoryEntityDataJobItem);
            
        public:
            CheckInMemoryEntityDataJobItem(
                std::shared_ptr<HealthEntity> const & entity,
                Common::ActivityId const & activityId);

            virtual ~CheckInMemoryEntityDataJobItem();

            virtual HealthJobItemKind::Enum get_Type() const override { return HealthJobItemKind::CheckInMemoryEntityData; }

        protected:

            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr) override;

            // Called to finish async work
            virtual void FinishAsyncWork() override;
        };
    }
}
