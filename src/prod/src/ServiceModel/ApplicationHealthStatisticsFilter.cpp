// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ApplicationHealthStatisticsFilter");

ApplicationHealthStatisticsFilter::ApplicationHealthStatisticsFilter()
    : HealthStatisticsFilter()
{
}

ApplicationHealthStatisticsFilter::ApplicationHealthStatisticsFilter(
    bool excludeHealthStatistics)
    : HealthStatisticsFilter(excludeHealthStatistics)
{
}

ApplicationHealthStatisticsFilter::ApplicationHealthStatisticsFilter(ApplicationHealthStatisticsFilter && other)
    : HealthStatisticsFilter(move(other))
{
}

ApplicationHealthStatisticsFilter & ApplicationHealthStatisticsFilter::operator =(ApplicationHealthStatisticsFilter && other)
{
    HealthStatisticsFilter::operator=(move(other));
    return *this;
}

ApplicationHealthStatisticsFilter::~ApplicationHealthStatisticsFilter()
{
}

ErrorCode ApplicationHealthStatisticsFilter::FromPublicApi(FABRIC_APPLICATION_HEALTH_STATISTICS_FILTER const & publicFilter)
{
    SetExcludeHealthStatistics(publicFilter.ExcludeHealthStatistics);

    return ErrorCode::Success();
}

wstring ApplicationHealthStatisticsFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationHealthStatisticsFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ApplicationHealthStatisticsFilter::FromString(
    wstring const & str,
    __out ApplicationHealthStatisticsFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
