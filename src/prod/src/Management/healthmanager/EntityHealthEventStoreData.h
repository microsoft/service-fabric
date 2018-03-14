// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        template <class T>
        class EntityHealthEventStoreData : public HealthEventStoreData
        {
            DENY_COPY(EntityHealthEventStoreData)

        public:
            EntityHealthEventStoreData();

            // First event, no history transition
            EntityHealthEventStoreData(
                T const & entityId,
                ServiceModel::HealthInformation const & entityInfo,
                ServiceModel::Priority::Enum priority,
                Store::ReplicaActivityId const & activityId);

            EntityHealthEventStoreData(
                EntityHealthEventStoreData<T> const & previousValue,
                Store::ReplicaActivityId const & activityId);

            EntityHealthEventStoreData(EntityHealthEventStoreData<T> && other);
            EntityHealthEventStoreData & operator = (EntityHealthEventStoreData<T> && other);

            virtual ~EntityHealthEventStoreData();

            __declspec (property(get = get_EntityId)) T const & EntityId;
            T const & get_EntityId() const { return entityId_; }

            __declspec (property(get = get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;

            // Create new event based on diff to be inserted into store
            virtual void GetDiffEvent(Store::ReplicaActivityId const & replicaActivityId, __inout HealthEventStoreDataUPtr & newEvent) const override;

            FABRIC_FIELDS_14(entityId_, sourceId_, property_, state_, lastModifiedUtc_, sourceUtcTimestamp_, timeToLive_, description_, reportSequenceNumber_, removeWhenExpired_, lastOkTransitionAt_, lastWarningTransitionAt_, lastErrorTransitionAt_, persistedIsExpired_);

        protected:
            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;

        private:
            T entityId_;
        };
    }
}
