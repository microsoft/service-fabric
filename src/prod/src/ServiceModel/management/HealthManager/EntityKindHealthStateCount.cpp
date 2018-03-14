// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

using namespace std;

INITIALIZE_SIZE_ESTIMATION(EntityKindHealthStateCount)

EntityKindHealthStateCount::EntityKindHealthStateCount()
    : entityKind_(EntityKind::Invalid)
    , healthCount_()
{
}

EntityKindHealthStateCount::EntityKindHealthStateCount(
    EntityKind::Enum entityKind)
    : entityKind_(entityKind)
    , healthCount_()
{
}

EntityKindHealthStateCount::EntityKindHealthStateCount(
    EntityKind::Enum entityKind,
    HealthStateCount && healthCount)
    : entityKind_(entityKind)
    , healthCount_(move(healthCount))
{
}

EntityKindHealthStateCount::~EntityKindHealthStateCount()
{
}

EntityKindHealthStateCount::EntityKindHealthStateCount(EntityKindHealthStateCount const & other)
    : entityKind_(other.entityKind_)
    , healthCount_(other.healthCount_)
{
}

EntityKindHealthStateCount & EntityKindHealthStateCount::operator=(EntityKindHealthStateCount const & other)
{
    if (this != &other)
    {
        entityKind_ = other.entityKind_;
        healthCount_ = other.healthCount_;
    }

    return *this;
}

EntityKindHealthStateCount::EntityKindHealthStateCount(EntityKindHealthStateCount && other)
    : entityKind_(move(other.entityKind_))
    , healthCount_(move(other.healthCount_))
{
}

EntityKindHealthStateCount & EntityKindHealthStateCount::operator=(EntityKindHealthStateCount && other)
{
    if (this != &other)
    {
        entityKind_ = move(other.entityKind_);
        healthCount_ = move(other.healthCount_);
    }

    return *this;
}

Common::ErrorCode EntityKindHealthStateCount::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT & publicHealthCount) const
{
    publicHealthCount.EntityKind = EntityKind::ToPublicApi(entityKind_);

    auto publicHealthStateCount = heap.AddItem<FABRIC_HEALTH_STATE_COUNT>();
    healthCount_.ToPublicApi(heap, *publicHealthStateCount);

    publicHealthCount.HealthStateCount = publicHealthStateCount.GetRawPointer();
    return ErrorCode::Success();
}

Common::ErrorCode EntityKindHealthStateCount::FromPublicApi(
    FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT const & publicHealthCount)
{
    entityKind_ = EntityKind::FromPublicApi(publicHealthCount.EntityKind);

    if (publicHealthCount.HealthStateCount != nullptr)
    {
        auto error = healthCount_.FromPublicApi(*publicHealthCount.HealthStateCount);
        if (!error.IsSuccess()) { return error; }
    }

    return ErrorCode::Success();
}

void EntityKindHealthStateCount::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("{0}: {1}", entityKind_, healthCount_);
}
