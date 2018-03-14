// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PlacementAndLoadBalancing;

        class MetricGraph
        {
            DENY_COPY_ASSIGNMENT(MetricGraph);

        public:
            MetricGraph();

            MetricGraph(MetricGraph && other);

            MetricGraph(MetricGraph const& other);

            MetricGraph & operator = (MetricGraph && other);

            void SetPLB(PlacementAndLoadBalancing * plb);
            void Clean();
            int GetMetricConnectionId(std::wstring const&);
            bool AddOrRemoveMetricAffinity(std::vector<ServiceMetric> const&, std::vector<ServiceMetric> const&, bool add = true);
            bool AddOrRemoveMetricAffinity(std::vector<ServiceMetric> const&, std::vector<std::wstring> const&, bool add = true);
            bool AddOrRemoveMetricConnection(std::vector<std::wstring> const&, bool add = true);
            bool RemoveMetricConnection(std::vector<std::wstring> const&);
            bool AddOrRemoveMetricConnectionsForService(std::vector<ServiceMetric> const&, bool add = true);
            bool RemoveMetricConnectionsForService(std::vector<ServiceMetric> const&);
            bool AddMetricConnection(std::wstring const&, std::wstring const&);
            bool AddMetricConnection(int, int);
            bool RemoveMetricConnection(int, int);
            bool RemoveMetricConnection(std::wstring const&, std::wstring const&);
            bool AreMetricsConnected(std::map<std::wstring, ServiceDomainMetric> const& metrics);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;
        private:

            //reference to the PLB. Not const, because we DO change plb members...
            PlacementAndLoadBalancing * plb_;
            //Map from metrics to metricIds. Makes metrics easier to represent in the metric connection graph
            std::unordered_map<std::wstring, int> metricConnectionIds_;
            //Sparse adjacency matrix representation of the metric connection graph
            std::vector<std::unordered_map<int, uint>*> metricConnections_;
            //List of all metrics in the system
            std::vector<std::wstring> metrics_;
        };
    }
}
