// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedServicePackageHealthStatesFilter");

DeployedServicePackageHealthStatesFilter::DeployedServicePackageHealthStatesFilter()
    : healthStateFilter_(0)
{
}

DeployedServicePackageHealthStatesFilter::DeployedServicePackageHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

DeployedServicePackageHealthStatesFilter::~DeployedServicePackageHealthStatesFilter()
{
}

bool DeployedServicePackageHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode DeployedServicePackageHealthStatesFilter::FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATES_FILTER const & publicDeployedServicePackageHealthStatesFilter)
{
    healthStateFilter_ = publicDeployedServicePackageHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring DeployedServicePackageHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<DeployedServicePackageHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode DeployedServicePackageHealthStatesFilter::FromString(
    wstring const & str,
    __out DeployedServicePackageHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}

