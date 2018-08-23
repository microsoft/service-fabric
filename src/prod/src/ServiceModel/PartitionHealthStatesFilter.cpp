// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("PartitionHealthStatesFilter");

PartitionHealthStatesFilter::PartitionHealthStatesFilter()
    : healthStateFilter_(0)
{
}

PartitionHealthStatesFilter::PartitionHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

PartitionHealthStatesFilter::~PartitionHealthStatesFilter()
{
}

bool PartitionHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode PartitionHealthStatesFilter::FromPublicApi(FABRIC_PARTITION_HEALTH_STATES_FILTER const & publicPartitionHealthStatesFilter)
{
    healthStateFilter_ = publicPartitionHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring PartitionHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<PartitionHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode PartitionHealthStatesFilter::FromString(
    wstring const & str,
    __out PartitionHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
