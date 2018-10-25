// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodeHealthStatesFilter");

NodeHealthStatesFilter::NodeHealthStatesFilter()
    : healthStateFilter_(0)
{
}

NodeHealthStatesFilter::NodeHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

NodeHealthStatesFilter::~NodeHealthStatesFilter()
{
}

bool NodeHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode NodeHealthStatesFilter::FromPublicApi(FABRIC_NODE_HEALTH_STATES_FILTER const & publicNodeHealthStatesFilter)
{
    healthStateFilter_ = publicNodeHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring NodeHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode NodeHealthStatesFilter::FromString(
    wstring const & str,
    __out NodeHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
