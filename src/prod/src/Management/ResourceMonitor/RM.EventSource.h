// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RMS_STRUCTURED_TRACE(traceName, traceId, traceLevel, ...) \
        traceName(Common::TraceTaskCodes::Enum::ResourceMonitor, traceId, #traceName, Common::LogLevel::traceLevel, __VA_ARGS__)

namespace ResourceMonitor
{
    class RMSEventSource
    {
    public:
        DECLARE_STRUCTURED_TRACE(ResourceUsageReportSharedHost, std::wstring, std::wstring, std::wstring, std::wstring, double, int64, double, int64);

        RMSEventSource() :
            RMS_STRUCTURED_TRACE(ResourceUsageReportSharedHost, 4, Info, "Resource usage for application {0} service package {1} code package {2} on node {3} weighted average cpu usage {4} cores weighted average memory usage {5} MB raw cpu usage {6} cores raw memory usage {7} MB ", "appName", "servicePackageName", "codePackageName", "nodeInstance", "cpuUsageWeightedAverage", "memoryUsageWeightedAverage", "cpuUsageRaw", "memoryUsageRaw")
        {
        }
    };
}
