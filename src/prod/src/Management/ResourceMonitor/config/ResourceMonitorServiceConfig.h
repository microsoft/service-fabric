// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Common/Common.h"

namespace ResourceMonitor
{
    class ResourceMonitorServiceConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ResourceMonitorServiceConfig, "ResourceMonitorService")

    public:
        // Controls if the service is enabled in the cluster or not.
        PUBLIC_CONFIG_ENTRY(bool, L"ResourceMonitorService", IsEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        // Instance count for this service.
        PUBLIC_CONFIG_ENTRY(int, L"ResourceMonitorService", InstanceCount, -1, Common::ConfigEntryUpgradePolicy::Static);
        //This is the time period to scan for new application host events and send them out to RM service
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", ApplicationHostEventsInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Interval at which timer for monitoring should be triggered
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", ResourceMonitorInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Interval at which resource usage is sent to RA
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", ResourceSendingInterval, Common::TimeSpan::FromSeconds(10 * 60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Timeout for opening the instance of this service
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", InstanceOpenTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic)
        // Timeout for closing the instance of this service
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", InstanceCloseTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic)
        // Interval at which we should trace out resource usage information
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", ResourceTracingInterval, Common::TimeSpan::FromSeconds(60*60), Common::ConfigEntryUpgradePolicy::Dynamic)
        // Interval for communication with fabric host
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", FabricHostCommunicationInterval, Common::TimeSpan::FromSeconds(70), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Interval at which each host should be checked
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"ResourceMonitorService", HostResourceCheck, Common::TimeSpan::FromSeconds(3*60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Maximum number of hosts to measure with a single message - used to throttle number of requests to fabric hosts
        INTERNAL_CONFIG_ENTRY(int, L"ResourceMonitorService", MaxHostsToMeasure, 20, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Enables test mode with additional tracing
        INTERNAL_CONFIG_ENTRY(bool, L"ResourceMonitorService", IsTestMode, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        //Smoothing factors for Cpu
        INTERNAL_CONFIG_ENTRY(double, L"ResourceMonitorService", MemorySmoothingFactor, 0.9, Common::ConfigEntryUpgradePolicy::Dynamic);
        //Smoothing factors for Memory
        INTERNAL_CONFIG_ENTRY(double, L"ResourceMonitorService", CpuSmoothingFactor, 0.9, Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}
