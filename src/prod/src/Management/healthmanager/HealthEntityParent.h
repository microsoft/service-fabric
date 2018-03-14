// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class HealthEntityParent
        {
            DENY_COPY(HealthEntityParent)

        public:
            typedef std::function<HealthEntitySPtr(void)> GetParentFromCacheCallback;

            HealthEntityParent(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                std::wstring const & childEntityId,
                HealthEntityKind::Enum parentEntityKind,
                GetParentFromCacheCallback const & getParentCallback);

            virtual ~HealthEntityParent();

            __declspec(property(get = get_Attributes)) AttributesStoreDataSPtr Attributes;
            AttributesStoreDataSPtr get_Attributes() const;

            __declspec(property(get=get_HasSystemReport)) bool const & HasSystemReport;
            virtual bool get_HasSystemReport() const;

            virtual bool IsSetAndUpToDate() const;

            HealthEntitySPtr GetLockedParent() const;

            virtual Common::ErrorCode HasAttributeMatch(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & hasMatch) const;
            
            // Check to determine whether the attributes or the parent must be updated.
            // If parent needs to be changed, take the parent cache lock and update it.
            // If attributes need to change due to staleness, take the parent lock and update them.
            // The initial check doesn't need any parent locks.
            // NOTE: this should NOT be called from the child's lock.
            void Update(Common::ActivityId const & activityId);

        protected:
            std::wstring const & childEntityId_;
            Store::PartitionedReplicaId const & partitionedReplicaId_;
            mutable Common::RwLock lock_;

        private:
            HealthEntityKind::Enum parentEntityKind_;
            GetParentFromCacheCallback getParentCallback_;
            
            // Keep parent as weak ptr, to avoid cyclic dependency
            std::weak_ptr<HealthEntity> parent_;
            AttributesStoreDataSPtr parentAttributes_;

            bool throttleNoParentTrace_;
        };

        // ================================================
        // Node specific class
        // ================================================
        class HealthEntityNodeParent : public HealthEntityParent
        {
        public:
            HealthEntityNodeParent(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                std::wstring const & childEntityId,
                GetParentFromCacheCallback const & getParentCallback);

            virtual ~HealthEntityNodeParent();

            bool get_HasSystemReport() const override;

            template <class TChildEntity>
            bool HasSameNodeInstance(AttributesStoreDataSPtr const & childAttributes) const;

            template <class TChildEntity>
            bool ShouldDeleteChild(AttributesStoreDataSPtr const & childAttributes) const;
        };
    }
}
