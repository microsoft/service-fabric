// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("HealthStatisticsFilter");

HealthStatisticsFilter::HealthStatisticsFilter()
    : excludeHealthStatistics_(false)
{
}

HealthStatisticsFilter::HealthStatisticsFilter(bool excludeHealthStatistics)
    : excludeHealthStatistics_(excludeHealthStatistics)
{
}

HealthStatisticsFilter::HealthStatisticsFilter(HealthStatisticsFilter && other)
    : excludeHealthStatistics_(move(other.excludeHealthStatistics_))
{
}

HealthStatisticsFilter & HealthStatisticsFilter::operator =(HealthStatisticsFilter && other)
{
    if (this != &other)
    {
        excludeHealthStatistics_ = move(other.excludeHealthStatistics_);
    }

    return *this;
}

HealthStatisticsFilter::~HealthStatisticsFilter()
{
}
