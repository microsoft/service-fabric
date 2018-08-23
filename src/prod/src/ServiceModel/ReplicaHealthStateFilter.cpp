// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ReplicaHealthStateFilter");

ReplicaHealthStateFilter::ReplicaHealthStateFilter()
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(0)
    , replicaOrInstanceIdFilter_(FABRIC_INVALID_REPLICA_ID)
{
}

ReplicaHealthStateFilter::ReplicaHealthStateFilter(
    DWORD healthStateFilter)
    : Common::IFabricJsonSerializable()
    , Serialization::FabricSerializable()
    , healthStateFilter_(healthStateFilter)
    , replicaOrInstanceIdFilter_(FABRIC_INVALID_REPLICA_ID)
{
}

ReplicaHealthStateFilter::~ReplicaHealthStateFilter()
{
}

wstring ReplicaHealthStateFilter::ToJsonString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ReplicaHealthStateFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
         return wstring();
    }

    return objectString;
}

ErrorCode ReplicaHealthStateFilter::FromJsonString(wstring const & str, __inout ReplicaHealthStateFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

bool ReplicaHealthStateFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    bool returnAllOnDefault = (replicaOrInstanceIdFilter_ != FABRIC_INVALID_REPLICA_ID);
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState, returnAllOnDefault);
}

ErrorCode ReplicaHealthStateFilter::FromPublicApi(FABRIC_REPLICA_HEALTH_STATE_FILTER const & publicReplicaHealthStateFilter)
{
    healthStateFilter_ = publicReplicaHealthStateFilter.HealthStateFilter;
    replicaOrInstanceIdFilter_ = publicReplicaHealthStateFilter.ReplicaOrInstanceIdFilter;
    return ErrorCode::Success();
}

Common::ErrorCode ReplicaHealthStateFilter::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_REPLICA_HEALTH_STATE_FILTER & publicFilter) const
{
    UNREFERENCED_PARAMETER(heap);
    publicFilter.HealthStateFilter = healthStateFilter_;
    publicFilter.ReplicaOrInstanceIdFilter = replicaOrInstanceIdFilter_;
    return ErrorCode::Success();
}
