// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/PerformanceCounter.h"

namespace ServiceGroup
{
    class AtomicGroupStateReplicatorCounters
    {
        DENY_COPY(AtomicGroupStateReplicatorCounters)

    public:

        BEGIN_COUNTER_SET_DEFINITION(
            L"7E378C17-4758-4147-B277-49D05EEBCC03",
            L"Atomic Group State Replicator",
            L"Stats for Atomic Group Replication",
            Common::PerformanceCounterSetInstanceType::Multiple)

            COUNTER_DEFINITION(1, Common::PerformanceCounterType::RawData64, L"#Atomic Groups in flight", L"Number of atomic groups that are currently active")

        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE(AtomicGroupsInFlight)

        BEGIN_COUNTER_SET_INSTANCE(AtomicGroupStateReplicatorCounters)
            DEFINE_COUNTER_INSTANCE(AtomicGroupsInFlight, 1)
        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<AtomicGroupStateReplicatorCounters> AtomicGroupStateReplicatorCountersSPtr;
}
