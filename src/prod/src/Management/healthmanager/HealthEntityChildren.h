// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Health entity children.
        template <class TEntity>
        class HealthEntityChildren 
        {
            DENY_COPY(HealthEntityChildren);

        public:
            HealthEntityChildren(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                std::wstring const & parentEntityId);
            ~HealthEntityChildren();
            
            void AddChild(std::shared_ptr<TEntity> const & child);

            // Returns valid children.
            // It also updated children with only valid and non-duplicate entries
            std::set<std::shared_ptr<TEntity>> GetChildren();

            bool CleanupChildren();

        private: 
            // This object is kept alive by the parent entity, so it's ok to keep the parent entity string by reference
            std::wstring const & parentEntityId_;
            Store::PartitionedReplicaId const & partitionedReplicaId_;

            // Children entities.
            // Some entities may go out of scope (in which case lock will return nullptr),
            // and some entities may be added multiple times.
            // When iterating the children, both situations must be taken into consideration.
            std::vector<std::weak_ptr<TEntity>> entities_;

            MUTABLE_RWLOCK(HM.EntityChildren, lock_);
        };
    }
}
