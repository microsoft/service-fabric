// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;
using namespace Management::HealthManager;

template <class T>
EntityHealthEventStoreData<T>::EntityHealthEventStoreData()
    : HealthEventStoreData()
    , entityId_()
{
}

template <class T>
EntityHealthEventStoreData<T>::EntityHealthEventStoreData(
    T const & entityId,
    HealthInformation const & healthInfo,
    ServiceModel::Priority::Enum priority,
    Store::ReplicaActivityId const & activityId)
    : HealthEventStoreData(healthInfo, priority, activityId)
    , entityId_(entityId)
{
}

template <class T>
EntityHealthEventStoreData<T>::EntityHealthEventStoreData(
    EntityHealthEventStoreData<T> const & previousValue,
    Store::ReplicaActivityId const & activityId)
    : HealthEventStoreData(previousValue, activityId)
    , entityId_(previousValue.entityId_)
{
}

template <class T>
EntityHealthEventStoreData<T>::EntityHealthEventStoreData(EntityHealthEventStoreData && other)
    : HealthEventStoreData(move(other))
    , entityId_(move(other.entityId_))
{
}

template <class T>
EntityHealthEventStoreData<T> & EntityHealthEventStoreData<T>::operator = (EntityHealthEventStoreData && other)
{
    if (this != &other)
    {
        entityId_ = move(other.entityId_);
    }

    HealthEventStoreData::operator=(move(other));
    return *this;
}

template <class T>
EntityHealthEventStoreData<T>::~EntityHealthEventStoreData()
{
}

template <class T>
void EntityHealthEventStoreData<T>::GetDiffEvent(
    Store::ReplicaActivityId const & replicaActivityId,
    HealthEventStoreDataUPtr & newEvent) const
{
    newEvent = make_unique<EntityHealthEventStoreData<T>>(*this, replicaActivityId);
}

template <class T>
std::wstring EntityHealthEventStoreData<T>::ConstructKey() const
{
    return wformatString(
        "{0}{1}{2}{3}{4}",
        entityId_,
        Constants::TokenDelimeter,
        property_,
        Constants::TokenDelimeter,
        sourceId_);
}

// Template specializations
TEMPLATE_SPECIALIZATION_ENTITY_ID(Management::HealthManager::EntityHealthEventStoreData)

TEMPLATE_STORE_DATA_GET_TYPE(EntityHealthEventStoreData, HealthEvent)
