// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServicePackageDescription;
        class NodeDescription;
        class Snapshot;
        class ServiceDescription;

        class ResourceMetricStatistics
        {
        public:
            ResourceMetricStatistics();

            __declspec(property(get = get_ResourceUsed, put = put_ResourceUsed)) double ResourceUsed;
            double get_ResourceUsed() const { return resourceUsed_; }
            void put_ResourceUsed(double value) { resourceUsed_ = value; }

            __declspec(property(get = get_ResourceTotal)) uint64_t ResourceTotal;
            uint64_t get_ResourceTotal() const { return resourceTotal_; }
            void AddResourceTotal(uint64_t res) { resourceTotal_ += res; }
            void RemoveResourceTotal(uint64_t res);

            __declspec(property(get = get_MinRequest)) double MinRequest;
            double get_MinRequest() const { return minRequest_; }

            __declspec(property(get = get_MaxRequest)) double MaxRequest;
            double get_MaxRequest() const { return maxRequest_; }

            void ProcessServicePackage(ServicePackageDescription const&, std::wstring const&);

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        private:
            // How much is currently used in the cluster
            double resourceUsed_;
            // Total amount of resource in the cluser (this is not capacity)
            uint64_t resourceTotal_;
            // Minimum resource request
            double minRequest_;
            // Maximum resource request (0 means nonthing is governed)
            double maxRequest_;
        };

        class RGStatistics
        {
        public:
            RGStatistics();

            // Tracking service package (add, delete, update) for statistics
            void AddServicePackage(ServicePackageDescription const&);
            void RemoveServicePackage(ServicePackageDescription const&);
            void UpdateServicePackage(ServicePackageDescription const&, ServicePackageDescription const&);

            // Functions that update nodes, needed to track available resources
            // In case when node capacity is not defined on at least one node, PLB will consider total capacity to be infinite.
            // We want to avoid that and to track how much resources are in the cluster, even if there are such nodes.
            void AddNode(NodeDescription const&);
            void RemoveNode(NodeDescription const&);
            void UpdateNode(NodeDescription const&, NodeDescription const&);

            // Tracking services for statistics (add, delete, update)
            void AddService(ServiceDescription const&);
            void DeleteService(ServiceDescription const&);
            void UpdateService(ServiceDescription const & service1, ServiceDescription const & service2);

            // Updates loads and configuration parameters
            void Update(Snapshot const&);

            __declspec(property(get = get_ServicePackageCount)) uint64_t ServicePackageCount;
            uint64_t get_ServicePackageCount() const { return servicePackageCount_; }

            __declspec(property(get = get_GovernedServicePackageCount)) uint64_t GovernedServicePackageCount;
            uint64_t get_GovernedServicePackageCount() const { return governedServicePackageCount_; }

            __declspec(property(get = get_NumSharedServices)) uint64_t SharedServicesCount;
            uint64_t get_NumSharedServices() const { return numSharedServices_; }

            __declspec(property(get = get_NumExclusiveServices)) uint64_t ExclusiveServicesCount;
            uint64_t get_NumExclusiveServices() const { return numExclusiveServices_; }

            __declspec(property(get = get_MemoryInMBResource)) ResourceMetricStatistics MemoryInMBStatistics;
            ResourceMetricStatistics get_MemoryInMBResource() const { return memoryInMBStats_; }

            __declspec(property(get = get_CpuCoresResource)) ResourceMetricStatistics CpuCoresStatistics;
            ResourceMetricStatistics get_CpuCoresResource() const { return cpuCoresStats_; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            // True if auto detect of resources is available
            bool autoDetectEnabled_;

            // Total number of service packages present in PLB.
            uint64_t servicePackageCount_;
            // Total number of governed service packages present in PLB.
            uint64_t governedServicePackageCount_;

            // Total number of services with exclusive service package.
            uint64_t numSharedServices_;
            // Total number of services with shared service package.
            uint64_t numExclusiveServices_;

            // Statistics related to CPU resource.
            ResourceMetricStatistics cpuCoresStats_;
            // Statistics related to Memory resource.
            ResourceMetricStatistics memoryInMBStats_;
        };
    }
}
