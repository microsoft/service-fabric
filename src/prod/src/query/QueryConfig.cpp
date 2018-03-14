// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Query;

DEFINE_SINGLETON_COMPONENT_CONFIG(QueryConfig)

TimeSpan QueryConfig::GetAggregatedHealthQueryTimeout(TimeSpan const& totalRemainingTime) const
{
    double percentage = QueryConfig::GetConfig().AggregatedHealthTimeoutPercentage;
    if (percentage >= 0.0 && percentage <= 1.0)
    {
        return TimeSpan::FromSeconds(totalRemainingTime.TotalSeconds() * percentage);
    }
    else
    {
        return totalRemainingTime;
    }    
}

TimeSpan QueryConfig::GetStoppedNodeQueryTimeout(TimeSpan const& totalRemainingTime) const
{
    double percentage = QueryConfig::GetConfig().StoppedNodeQueryTimeoutPercentage;
    if (percentage >= 0.0 && percentage <= 1.0)
    {
        return TimeSpan::FromSeconds(totalRemainingTime.TotalSeconds() * percentage);
    }
    else
    {
        return totalRemainingTime;
    }    
}
