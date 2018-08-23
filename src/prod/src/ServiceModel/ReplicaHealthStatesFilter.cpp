// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ReplicaHealthStatesFilter");

ReplicaHealthStatesFilter::ReplicaHealthStatesFilter()
    : healthStateFilter_(0)
{
}

ReplicaHealthStatesFilter::ReplicaHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

ReplicaHealthStatesFilter::~ReplicaHealthStatesFilter()
{
}

bool ReplicaHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode ReplicaHealthStatesFilter::FromPublicApi(FABRIC_REPLICA_HEALTH_STATES_FILTER const & publicReplicaHealthStatesFilter)
{
    healthStateFilter_ = publicReplicaHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring ReplicaHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ReplicaHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ReplicaHealthStatesFilter::FromString(
    wstring const & str,
    __out ReplicaHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
