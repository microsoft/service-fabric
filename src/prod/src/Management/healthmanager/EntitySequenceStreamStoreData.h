// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Templated class based on the type of entity
        template <class TEntityType>
        class EntitySequenceStreamStoreData : public SequenceStreamStoreData
        {
        public:
            EntitySequenceStreamStoreData();

            EntitySequenceStreamStoreData(
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER upToLsn,
                FABRIC_SEQUENCE_NUMBER invalidateLsn,
                Common::DateTime invalidateTime,
                FABRIC_SEQUENCE_NUMBER highestLsn,
                Store::ReplicaActivityId const & activityId);

            EntitySequenceStreamStoreData(EntitySequenceStreamStoreData<TEntityType> const & other);
            EntitySequenceStreamStoreData & operator = (EntitySequenceStreamStoreData<TEntityType> const & other);

            EntitySequenceStreamStoreData(EntitySequenceStreamStoreData<TEntityType> && other);
            EntitySequenceStreamStoreData & operator = (EntitySequenceStreamStoreData<TEntityType> && other);
            
            virtual ~EntitySequenceStreamStoreData();
            
            __declspec (property(get = get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;
            
            static std::wstring const & GetEntityTypeString();
            
        protected:

            // Gets the key that will be used as unique identifier of the record in the store
            virtual std::wstring ConstructKey() const;
        };
    }
}
