// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType>::EntitySequenceStreamStoreData()
    : SequenceStreamStoreData()
{
}

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType>::EntitySequenceStreamStoreData(
    std::wstring const & sourceId,
    FABRIC_INSTANCE_ID instance,
    FABRIC_SEQUENCE_NUMBER upToLsn,
    FABRIC_SEQUENCE_NUMBER invalidateLsn,
    DateTime invalidateTime,
    FABRIC_SEQUENCE_NUMBER highestLsn,
    Store::ReplicaActivityId const & activityId)
    : SequenceStreamStoreData(sourceId, instance, upToLsn, invalidateLsn, invalidateTime, highestLsn, activityId)
{
}

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType>::EntitySequenceStreamStoreData(EntitySequenceStreamStoreData const & other)
    : SequenceStreamStoreData(other)
{
}

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType> & EntitySequenceStreamStoreData<TEntityType>::operator = (EntitySequenceStreamStoreData const & other)
{
    SequenceStreamStoreData::operator=(other);
    return *this;
}

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType>::EntitySequenceStreamStoreData(EntitySequenceStreamStoreData && other)
    : SequenceStreamStoreData(move(other))
{
}

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType> & EntitySequenceStreamStoreData<TEntityType>::operator = (EntitySequenceStreamStoreData && other)
{
    SequenceStreamStoreData::operator=(move(other));
    return *this;
}

template <class TEntityType>
EntitySequenceStreamStoreData<TEntityType>::~EntitySequenceStreamStoreData()
{
}

template <class TEntityType>
std::wstring EntitySequenceStreamStoreData<TEntityType>::ConstructKey() const
{
    return wformatString(
        "{0}{1}{2}",
        sourceId_,
        Constants::TokenDelimeter,
        GetEntityTypeString());
}

// Template specializations
TEMPLATE_SPECIALIZATION_ENTITY_ID(Management::HealthManager::EntitySequenceStreamStoreData)

TEMPLATE_STORE_DATA_GET_TYPE(EntitySequenceStreamStoreData, SequenceStream)

TEMPLATE_STORE_DATA_GET_TYPE_STRING(EntitySequenceStreamStoreData)
