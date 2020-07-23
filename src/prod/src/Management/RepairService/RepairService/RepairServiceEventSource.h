// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace RepairService
{

#define DECLARE_RS_TRACE(trace_name, ...) \
    DECLARE_STRUCTURED_TRACE(trace_name, __VA_ARGS__);

#define RS_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, RepairService, trace_id, trace_level, __VA_ARGS__),

    class RepairServiceEventSource
    {
        BEGIN_STRUCTURED_TRACES( RepairServiceEventSource )

        RS_TRACE( NodeLastHealthyAt, 10, Info, "{1}", "id", "lastHealthyAt" )

        END_STRUCTURED_TRACES

        DECLARE_RS_TRACE( NodeLastHealthyAt, std::wstring, Common::DateTime )

        static Common::Global<RepairServiceEventSource> Trace;
    };

    typedef RepairServiceEventSource RSEvents; 
}
