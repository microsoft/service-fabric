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

HealthStatisticsFilter::~HealthStatisticsFilter()
{
}
