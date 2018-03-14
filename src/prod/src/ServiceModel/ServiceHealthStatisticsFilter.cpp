// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ServiceHealthStatisticsFilter");

ServiceHealthStatisticsFilter::ServiceHealthStatisticsFilter()
    : HealthStatisticsFilter()
{
}

ServiceHealthStatisticsFilter::ServiceHealthStatisticsFilter(
    bool excludeHealthStatistics)
    : HealthStatisticsFilter(excludeHealthStatistics)
{
}

ServiceHealthStatisticsFilter::ServiceHealthStatisticsFilter(ServiceHealthStatisticsFilter && other)
    : HealthStatisticsFilter(move(other))
{
}

ServiceHealthStatisticsFilter & ServiceHealthStatisticsFilter::operator =(ServiceHealthStatisticsFilter && other)
{
    HealthStatisticsFilter::operator=(move(other));
    return *this;
}

ServiceHealthStatisticsFilter::~ServiceHealthStatisticsFilter()
{
}

ErrorCode ServiceHealthStatisticsFilter::FromPublicApi(FABRIC_SERVICE_HEALTH_STATISTICS_FILTER const & publicFilter)
{
    SetExcludeHealthStatistics(publicFilter.ExcludeHealthStatistics);

    return ErrorCode::Success();
}

wstring ServiceHealthStatisticsFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ServiceHealthStatisticsFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ServiceHealthStatisticsFilter::FromString(
    wstring const & str,
    __out ServiceHealthStatisticsFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
