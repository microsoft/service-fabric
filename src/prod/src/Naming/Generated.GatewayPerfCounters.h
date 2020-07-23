
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once









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

            COUNTER_DEFINITION( 0+1, Common::PerformanceCounterType::RawData32, L"Client Connections", L"Number of client connections" )
            COUNTER_DEFINITION( 0+2, Common::PerformanceCounterType::RawData32, L"Outstanding Client Requests", L"Number of outstanding client requests" )

        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE( NumberOfClientConnections )
        DECLARE_COUNTER_INSTANCE( NumberOfOutstandingClientRequests )

        BEGIN_COUNTER_SET_INSTANCE(GatewayPerformanceCounters)

            DEFINE_COUNTER_INSTANCE( NumberOfClientConnections, 0+1 )
            DEFINE_COUNTER_INSTANCE( NumberOfOutstandingClientRequests, 0+2 )

        END_COUNTER_SET_INSTANCE()
    };

    typedef std::shared_ptr<GatewayPerformanceCounters> GatewayPerformanceCountersSPtr;
}
