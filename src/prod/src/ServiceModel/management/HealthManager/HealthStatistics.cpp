// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Management::HealthManager;

using namespace std;

INITIALIZE_SIZE_ESTIMATION(HealthStatistics)

HealthStatistics::HealthStatistics()
    : healthCounts_()
{
}

HealthStatistics::~HealthStatistics()
{
}

HealthStatistics::HealthStatistics(HealthStatistics const & other)
    : healthCounts_(other.healthCounts_)
{
}

HealthStatistics & HealthStatistics::operator=(HealthStatistics const & other)
{
    if (this != &other)
    {
        healthCounts_ = other.healthCounts_;
    }

    return *this;
}

HealthStatistics::HealthStatistics(HealthStatistics && other)
    : healthCounts_(move(other.healthCounts_))
{
}
            
HealthStatistics & HealthStatistics::operator=(HealthStatistics && other)
{
    if (this != &other)
    {
        healthCounts_ = move(other.healthCounts_);
    }

    return *this;
}

HealthStateCountMap HealthStatistics::GetHealthCountMap() const
{
    HealthStateCountMap map;
    for (auto const & entry : healthCounts_)
    {
        map.insert(make_pair(entry.EntityKind, entry.StateCount));
    }

    return map;
}

Common::ErrorCode HealthStatistics::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_HEALTH_STATISTICS & publicStats) const
{
    return PublicApiHelper::ToPublicApiList<EntityKindHealthStateCount, FABRIC_ENTITY_KIND_HEALTH_STATE_COUNT, FABRIC_HEALTH_STATISTICS>(
        heap,
        healthCounts_,
        publicStats);
}

Common::ErrorCode HealthStatistics::FromPublicApi(
    FABRIC_HEALTH_STATISTICS const * publicStats)
{
    ErrorCode error(ErrorCodeValue::Success);
    if (publicStats == nullptr)
    {
        // Nothing to do
        return error;
    }

    return PublicApiHelper::FromPublicApiList<EntityKindHealthStateCount, FABRIC_HEALTH_STATISTICS>(
        publicStats,
        healthCounts_);
}

void HealthStatistics::Add(EntityKind::Enum entityKind)
{
    Add(entityKind, HealthStateCount());
}

void HealthStatistics::Add(EntityKind::Enum entityKind, HealthStateCount && count)
{
    auto it = find_if(healthCounts_.begin(),healthCounts_.end(), 
        [entityKind](EntityKindHealthStateCount const & entry) { return entry.EntityKind == entityKind; });
    if (it == healthCounts_.end())
    {
        healthCounts_.push_back(EntityKindHealthStateCount(entityKind, move(count)));
    }
    else
    {
        it->AppendCount(count);
    }
}

void HealthStatistics::Add(EntityKind::Enum entityKind, FABRIC_HEALTH_STATE healthState)
{
    auto it = find_if(healthCounts_.begin(), healthCounts_.end(),
        [entityKind](EntityKindHealthStateCount const & entry) { return entry.EntityKind == entityKind; });
    if (it == healthCounts_.end())
    {
        HealthStateCount count;
        count.Add(healthState);
        healthCounts_.push_back(EntityKindHealthStateCount(entityKind, move(count)));
    }
    else
    {
        it->Add(healthState);
    }
}

void HealthStatistics::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    for (auto const & entry : healthCounts_)
    {
        w.Write("{0}; ", entry);
    }
}
