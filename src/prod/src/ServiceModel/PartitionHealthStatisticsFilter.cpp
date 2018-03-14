// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("PartitionHealthStatisticsFilter");

PartitionHealthStatisticsFilter::PartitionHealthStatisticsFilter()
    : HealthStatisticsFilter()
{
}

PartitionHealthStatisticsFilter::PartitionHealthStatisticsFilter(
    bool excludeHealthStatistics)
    : HealthStatisticsFilter(excludeHealthStatistics)
{
}

PartitionHealthStatisticsFilter::PartitionHealthStatisticsFilter(PartitionHealthStatisticsFilter && other)
    : HealthStatisticsFilter(move(other))
{
}

PartitionHealthStatisticsFilter & PartitionHealthStatisticsFilter::operator =(PartitionHealthStatisticsFilter && other)
{
    HealthStatisticsFilter::operator=(move(other));
    return *this;
}

PartitionHealthStatisticsFilter::~PartitionHealthStatisticsFilter()
{
}

ErrorCode PartitionHealthStatisticsFilter::FromPublicApi(FABRIC_PARTITION_HEALTH_STATISTICS_FILTER const & publicFilter)
{
    SetExcludeHealthStatistics(publicFilter.ExcludeHealthStatistics);

    return ErrorCode::Success();
}

wstring PartitionHealthStatisticsFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<PartitionHealthStatisticsFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode PartitionHealthStatisticsFilter::FromString(
    wstring const & str,
    __out PartitionHealthStatisticsFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
