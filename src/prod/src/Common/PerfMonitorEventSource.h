// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
#define PERFMONITOR_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, PerfMonitor, trace_id, trace_level, __VA_ARGS__)

    class PerfMonitorEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(Sample, int32, int32, int64, int64, int64);

        PerfMonitorEventSource()
            : PERFMONITOR_STRUCTURED_TRACE(Sample, 4, Info, "Thread: {0}, ActiveCallback: {1}, Memory: {2}, Memory Average: {3}/{4}", "threadCount", "activeCallback", "memory", "shortavg", "longavg"
            )
        {
        }
    };
}
