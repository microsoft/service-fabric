// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("DeployedApplicationHealthStatisticsFilter");

DeployedApplicationHealthStatisticsFilter::DeployedApplicationHealthStatisticsFilter()
    : HealthStatisticsFilter()
{
}

DeployedApplicationHealthStatisticsFilter::DeployedApplicationHealthStatisticsFilter(
    bool excludeHealthStatistics)
    : HealthStatisticsFilter(excludeHealthStatistics)
{
}

DeployedApplicationHealthStatisticsFilter::DeployedApplicationHealthStatisticsFilter(DeployedApplicationHealthStatisticsFilter && other)
    : HealthStatisticsFilter(move(other))
{
}

DeployedApplicationHealthStatisticsFilter & DeployedApplicationHealthStatisticsFilter::operator =(DeployedApplicationHealthStatisticsFilter && other)
{
    HealthStatisticsFilter::operator=(move(other));
    return *this;
}

DeployedApplicationHealthStatisticsFilter::~DeployedApplicationHealthStatisticsFilter()
{
}

ErrorCode DeployedApplicationHealthStatisticsFilter::FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATISTICS_FILTER const & publicFilter)
{
    SetExcludeHealthStatistics(publicFilter.ExcludeHealthStatistics);

    return ErrorCode::Success();
}

wstring DeployedApplicationHealthStatisticsFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<DeployedApplicationHealthStatisticsFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode DeployedApplicationHealthStatisticsFilter::FromString(
    wstring const & str,
    __out DeployedApplicationHealthStatisticsFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
