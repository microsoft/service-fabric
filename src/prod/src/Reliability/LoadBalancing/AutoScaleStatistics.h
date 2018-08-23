// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceDescription;

        class AutoScaleStatisticsEntry
        {
        public:
            AutoScaleStatisticsEntry() = default;

            __declspec(property(get = get_PartitionCount, put = set_PartitionCount)) int PartitionCount;
            int get_PartitionCount() const { return partitionCount_; }
            void set_PartitionCount(int value) { partitionCount_ = value; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        private:
            int partitionCount_;
        };

        class AutoScaleStatistics
        {
        public:
            AutoScaleStatistics() = default;

            void AddService(ServiceDescription const& serviceDescription);
            void DeleteService(ServiceDescription const& serviceDescription);
            void UpdateService(ServiceDescription const& service1, ServiceDescription const& service2);

            __declspec(property(get = get_MemoryInMBResource)) AutoScaleStatisticsEntry const & MemoryInMBStatistics;
            AutoScaleStatisticsEntry const & get_MemoryInMBResource() const { return memoryInMBStats_; }

            __declspec(property(get = get_CpuCoresResource)) AutoScaleStatisticsEntry const & CpuCoresStatistics;
            AutoScaleStatisticsEntry const & get_CpuCoresResource() const { return cpuCoresStats_; }

            __declspec(property(get = get_CustomResource)) AutoScaleStatisticsEntry const & CustomMetricStatistics;
            AutoScaleStatisticsEntry const & get_CustomResource() const { return customMetricStats_; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            // Statistics related to CPU resource.
            AutoScaleStatisticsEntry cpuCoresStats_;
            // Statistics related to Memory resource.
            AutoScaleStatisticsEntry memoryInMBStats_;
            // Statistics related to custom metrics.
            AutoScaleStatisticsEntry customMetricStats_;
        };
    }
}
