// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Job item that deletes an entity from the store and changes the internal state of the passed in entity
        class CleanupEntityJobItem
            : public CleanupEntityJobItemBase
        {
            DENY_COPY(CleanupEntityJobItem);
            
        public:
            CleanupEntityJobItem(
                std::shared_ptr<HealthEntity> const & entity,
                std::wstring const & invalidateSource,
                FABRIC_SEQUENCE_NUMBER invalidateLsn,
                Common::ActivityId const & activityId);

            virtual HealthJobItemKind::Enum get_Type() const override { return HealthJobItemKind::CleanupEntity; }

            virtual ~CleanupEntityJobItem();
            
        protected:
            // Actual processing of the task, that starts work on the entity
            virtual void ProcessInternal(IHealthJobItemSPtr const & thisSPtr) override;

            // Called to finish async work
            virtual void FinishAsyncWork() override;

        private:
            std::wstring invalidateSource_;
            FABRIC_SEQUENCE_NUMBER invalidateLsn_;
        };
    }
}
