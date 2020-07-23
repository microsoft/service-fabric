// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    #define NAMING_GATEWAY_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, NamingGateway, trace_id, trace_level, __VA_ARGS__)

    class GatewayEventSource
    { 
    public:        
        DECLARE_STRUCTURED_TRACE( GatewayOpenComplete, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE( BroadcastEventManagerReceivedCuid, std::wstring);

        GatewayEventSource()
            : NAMING_GATEWAY_STRUCTURED_TRACE( GatewayOpenComplete, 4, Info, "{0}: Gateway has opened with {1}", "gatewayTraceId", "error")
            , NAMING_GATEWAY_STRUCTURED_TRACE( BroadcastEventManagerReceivedCuid, 10, Noise, "BroadcastEventManager received CUID {0}.", "cuid")
        {
        }  
    };
}
