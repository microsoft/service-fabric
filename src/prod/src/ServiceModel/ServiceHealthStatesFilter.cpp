// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceHealthStatesFilter");

ServiceHealthStatesFilter::ServiceHealthStatesFilter()
    : healthStateFilter_(0)
{
}

ServiceHealthStatesFilter::ServiceHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

ServiceHealthStatesFilter::~ServiceHealthStatesFilter()
{
}

bool ServiceHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode ServiceHealthStatesFilter::FromPublicApi(FABRIC_SERVICE_HEALTH_STATES_FILTER const & publicServiceHealthStatesFilter)
{
    healthStateFilter_ = publicServiceHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring ServiceHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ServiceHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ServiceHealthStatesFilter::FromString(
    wstring const & str,
    __out ServiceHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
