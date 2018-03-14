// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ResourceMonitor
    {
        class ResourceUsage;
        class ResourceUsageReport : public Serialization::FabricSerializable
        {
        public:

            static Common::GlobalWString ResourceUsageReportAction;

            ResourceUsageReport() = default;
            ResourceUsageReport(std::map<Reliability::PartitionId, ResourceUsage> && resourceUsageReports);

            __declspec(property(get=get_ResourceUsageReports)) std::map<Reliability::PartitionId, ResourceUsage> const & ResourceUsageReports;
            std::map<Reliability::PartitionId, ResourceUsage> const & get_ResourceUsageReports() const { return resourceUsageReports_; }

            FABRIC_FIELDS_01(resourceUsageReports_);

        private:
            std::map<Reliability::PartitionId, ResourceUsage> resourceUsageReports_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ResourceMonitor::ResourceUsage);
DEFINE_USER_MAP_UTILITY(Reliability::PartitionId, Management::ResourceMonitor::ResourceUsage);
