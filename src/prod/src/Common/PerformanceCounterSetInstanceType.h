// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace PerformanceCounterSetInstanceType
    {
        enum Enum
        {
            Single = PERF_COUNTERSET_SINGLE_INSTANCE,
            Multiple = PERF_COUNTERSET_MULTI_INSTANCES,
            GlobalAggregate = PERF_COUNTERSET_SINGLE_AGGREGATE,
            GlobalAggregateWithHistory = PERF_COUNTERSET_SINGLE_AGGREGATE_HISTORY,
            MultipleAggregate = PERF_COUNTERSET_MULTI_AGGREGATE,
            InstanceAggregate = PERF_COUNTERSET_INSTANCE_AGGREGATE
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}

