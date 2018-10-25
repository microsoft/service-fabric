// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ClusterHealthStatisticsFilter");

ClusterHealthStatisticsFilter::ClusterHealthStatisticsFilter()
    : HealthStatisticsFilter()
    , includeSystemApplicationHealthStatistics_(false)
{
}

ClusterHealthStatisticsFilter::ClusterHealthStatisticsFilter(
    bool excludeHealthStatistics)
    : HealthStatisticsFilter(excludeHealthStatistics)
    , includeSystemApplicationHealthStatistics_(false)
{
}

ClusterHealthStatisticsFilter::ClusterHealthStatisticsFilter(
    bool excludeHealthStatistics,
    bool includeSystemApplicationHealthStatistics)
    : HealthStatisticsFilter(excludeHealthStatistics)
    , includeSystemApplicationHealthStatistics_(includeSystemApplicationHealthStatistics)
{
}

ClusterHealthStatisticsFilter::~ClusterHealthStatisticsFilter()
{
}

ErrorCode ClusterHealthStatisticsFilter::FromPublicApi(FABRIC_CLUSTER_HEALTH_STATISTICS_FILTER const & publicFilter)
{
    SetExcludeHealthStatistics(publicFilter.ExcludeHealthStatistics);
    includeSystemApplicationHealthStatistics_ = (publicFilter.IncludeSystemApplicationHealthStatistics == TRUE);
    return ErrorCode::Success();
}

wstring ClusterHealthStatisticsFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ClusterHealthStatisticsFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ClusterHealthStatisticsFilter::FromString(
    wstring const & str,
    __out ClusterHealthStatisticsFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
