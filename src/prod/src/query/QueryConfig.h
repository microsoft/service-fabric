// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Query
{
    class QueryConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(QueryConfig, "Query")
 
        // AggregatedHealthTimeoutPercentage indicates the amount of time from the user given timeout until which we will wait for the health manager to return
        // the aggregated health state. It is a value between 0 and 1.0 with 1.0 indicating that the HM query should also get the entire user specified timeout.
        INTERNAL_CONFIG_ENTRY(double, L"Query", AggregatedHealthTimeoutPercentage, 0.6, Common::ConfigEntryUpgradePolicy::Dynamic);
        INTERNAL_CONFIG_ENTRY(double, L"Query", StoppedNodeQueryTimeoutPercentage, 0.4, Common::ConfigEntryUpgradePolicy::Dynamic);

    public:
        Common::TimeSpan GetAggregatedHealthQueryTimeout(Common::TimeSpan const& totalRemainingTime) const;
        Common::TimeSpan GetStoppedNodeQueryTimeout(Common::TimeSpan const& totalRemainingTime) const;        
    };
}
