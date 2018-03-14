// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RAW_INDEX 0

#define RAW_COUNTER( Id, Name, Description ) \
    COUNTER_DEFINITION( RAW_INDEX+Id, Common::PerformanceCounterType::RawData32, Name, Description )

#define DEFINE_RAW_COUNTER( Id, Name) \
    DEFINE_COUNTER_INSTANCE( Name, RAW_INDEX+Id )

namespace Naming
{
    class GatewayPerformanceCounters
    {
        DENY_COPY(GatewayPerformanceCounters)

    public:
    
        BEGIN_COUNTER_SET_DEFINITION(
            L"6D18D6F9-7799-40B8-924D-F923D89C0134",
            L"Naming Gateway",
            L"Counters for Naming Gateway",
            Common::PerformanceCounterSetInstanceType::Multiple)

            RAW_COUNTER( 1, L"Client Connections", L"Number of client connections")
            RAW_COUNTER( 2, L"Outstanding Client Requests", L"Number of outstanding client requests")

        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE( NumberOfClientConnections )
        DECLARE_COUNTER_INSTANCE( NumberOfOutstandingClientRequests )

        BEGIN_COUNTER_SET_INSTANCE(GatewayPerformanceCounters)

            DEFINE_RAW_COUNTER( 1, NumberOfClientConnections )
            DEFINE_RAW_COUNTER( 2, NumberOfOutstandingClientRequests )

        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<GatewayPerformanceCounters> GatewayPerformanceCountersSPtr;
}
