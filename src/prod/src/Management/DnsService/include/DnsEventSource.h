// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/Common.h"

namespace DNS
{
#define DNS_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, DNS, trace_id, trace_level, __VA_ARGS__)

    class DnsEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(DnsServiceStartingOpen, uint64);
        DECLARE_STRUCTURED_TRACE(DnsExchangeOpRemoteResolve, uint64);
        DECLARE_STRUCTURED_TRACE(DnsExchangeOpReadQuestion, uint64);
        DECLARE_STRUCTURED_TRACE(DnsExchangeOpFabricResolve, uint64);
        DECLARE_STRUCTURED_TRACE(DnsServiceCreated, std::wstring);
        DECLARE_STRUCTURED_TRACE(DnsServiceDeleted, std::wstring);

        DnsEventSource() :
            DNS_STRUCTURED_TRACE(DnsServiceStartingOpen, 4, Info, "DnsService starting open on port {0}", "port"),
            DNS_STRUCTURED_TRACE(DnsExchangeOpRemoteResolve, 5, Info, "Dns Resolution by public DNS: {0}", "count"),
            DNS_STRUCTURED_TRACE(DnsExchangeOpReadQuestion, 6, Info, "Dns Resolution request received: {0}", "count"),
            DNS_STRUCTURED_TRACE(DnsExchangeOpFabricResolve, 7, Info, "Dns Resolution by naming service: {0}", "count"),
            DNS_STRUCTURED_TRACE(DnsServiceCreated, 8, Info, "Dns service created: {0}", "service"),
            DNS_STRUCTURED_TRACE(DnsServiceDeleted, 9, Info, "Dns service deleted: {0}", "service")
        {
        }
    };
}
