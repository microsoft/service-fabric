// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationHealthStatesFilter");

DeployedApplicationHealthStatesFilter::DeployedApplicationHealthStatesFilter()
    : healthStateFilter_(0)
{
}

DeployedApplicationHealthStatesFilter::DeployedApplicationHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

DeployedApplicationHealthStatesFilter::~DeployedApplicationHealthStatesFilter()
{
}

bool DeployedApplicationHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode DeployedApplicationHealthStatesFilter::FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATES_FILTER const & publicDeployedApplicationHealthStatesFilter)
{
    healthStateFilter_ = publicDeployedApplicationHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring DeployedApplicationHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<DeployedApplicationHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode DeployedApplicationHealthStatesFilter::FromString(
    wstring const & str,
    __out DeployedApplicationHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
