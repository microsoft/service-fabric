// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairPolicyEngine
{

#define DECLARE_PE_TRACE(trace_name, ...) \
    DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define PE_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, RepairPolicyEngineService, trace_id, trace_level, __VA_ARGS__),

    class RepairPolicyEngineEventSource
    {
        BEGIN_STRUCTURED_TRACES( RepairPolicyEngineEventSource )

        PE_TRACE( NodeLastHealthyAt, 10, Info, "{1}", "id", "lastHealthyAt" )

        END_STRUCTURED_TRACES

        DECLARE_PE_TRACE( NodeLastHealthyAt, std::wstring, Common::DateTime )

        static Common::Global<RepairPolicyEngineEventSource> Trace;
    };

    typedef RepairPolicyEngineEventSource PEEvents; 
}
