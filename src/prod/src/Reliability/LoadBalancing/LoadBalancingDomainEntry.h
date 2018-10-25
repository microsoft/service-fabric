// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "AccumulatorWithMinMax.h"
#include "DynamicNodeLoadSet.h"
#include "Metric.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class BalancingDiagnosticsData;
        typedef std::shared_ptr<BalancingDiagnosticsData> BalancingDiagnosticsDataSPtr;

        class LoadBalancingDomainEntry
        {
            DENY_COPY(LoadBalancingDomainEntry);

        public:
            static Common::GlobalWString const TraceDescription;

            class DomainMetricAccsWithMinMax
            {
            public:
                DomainMetricAccsWithMinMax(size_t metricCount)
                    : AccMinMaxEntries(metricCount, AccumulatorWithMinMax(false))
                {
                }

                DomainMetricAccsWithMinMax(DomainMetricAccsWithMinMax const & other)
                    : AccMinMaxEntries(other.AccMinMaxEntries)
                {
                }

                DomainMetricAccsWithMinMax(DomainMetricAccsWithMinMax && other)
                    : AccMinMaxEntries(move(other.AccMinMaxEntries))
                {
                }

                DomainMetricAccsWithMinMax & operator = (DomainMetricAccsWithMinMax && other)
                {
                    if (this != &other)
                    {
                        AccMinMaxEntries = move(other.AccMinMaxEntries);
                    }

                    return *this;
                }

                std::vector<AccumulatorWithMinMax> AccMinMaxEntries;
            };

            typedef Common::Tree<DomainMetricAccsWithMinMax> DomainAccMinMaxTree;

            LoadBalancingDomainEntry(std::vector<Metric> && metrics, double metricWeightSum, size_t metricStartIndex, int serviceIndex);

            LoadBalancingDomainEntry(LoadBalancingDomainEntry && other);

            LoadBalancingDomainEntry & operator = (LoadBalancingDomainEntry && other);

            // properties
            __declspec (property(get = get_Metrics)) std::vector<Metric> const& Metrics;
            std::vector<Metric> const& get_Metrics() const { return metrics_; }

            __declspec (property(get = get_IsGlobal)) bool IsGlobal;
            bool get_IsGlobal() const { return serviceIndex_ == -1; }

            __declspec (property(get = get_ServiceIndex)) int ServiceIndex;
            int get_ServiceIndex() const { return serviceIndex_; }

            __declspec (property(get = get_MetricStartIndex)) size_t MetricStartIndex;
            size_t get_MetricStartIndex() const { return metricStartIndex_; }

            __declspec (property(get = get_MetricCount)) size_t MetricCount;
            size_t get_MetricCount() const { return Metrics.size(); }

            __declspec (property(get = get_MetricWeightSum)) double MetricWeightSum;
            double get_MetricWeightSum() const { return metricWeightSum_; }

            __declspec (property(get = get_IsBalanced)) bool IsBalanced;
            bool get_IsBalanced() const { return isBalanced_; }

            AccumulatorWithMinMax & GetLoadStat(size_t metricIndex) { return loadStats_[metricIndex]; }
            AccumulatorWithMinMax const& GetLoadStat(size_t metricIndex) const { return loadStats_[metricIndex]; }

            AccumulatorWithMinMax & GetFdLoadStat(size_t metricIndex) { return fdLoadStats_[metricIndex]; }
            AccumulatorWithMinMax const& GetFdLoadStat(size_t metricIndex) const { return fdLoadStats_[metricIndex]; }

            AccumulatorWithMinMax & GetUdLoadStat(size_t metricIndex) { return udLoadStats_[metricIndex]; }
            AccumulatorWithMinMax const& GetUdLoadStat(size_t metricIndex) const { return udLoadStats_[metricIndex]; }

            void RefreshIsBalanced(
                std::vector<size_t> const* globalMetricIndices,
                LoadBalancingDomainEntry const& globalLBDomain,
                DomainAccMinMaxTree const& fdsTotalLoads,
                DomainAccMinMaxTree const& udsTotalLoads,
                DynamicNodeLoadSet& dynamicLoads,
                BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr = nullptr);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            // Check if defrag need to be triggered by previous implementation
            bool ShouldDefragRunByDomain(
                DomainAccMinMaxTree const & fdAccumulator,
                DomainAccMinMaxTree const & udAccumulator,
                size_t globalMetricIndex,
                Metric const & metric) const;

            bool ShouldDefragRunByDomain(
                DomainAccMinMaxTree const & accumulator,
                size_t globalMetricIndex,
                Metric const & metric) const;

            // Check if defrag need to be triggered by spreading empty nodes across FDs/UDs
            bool ShouldDefragRunByNodeSpreadDomain(
                DomainAccMinMaxTree const & fdAccumulator,
                DomainAccMinMaxTree const & udAccumulator,
                size_t globalMetricIndex,
                Metric const & metric) const;

            bool ShouldDefragRunByNodeSpreadDomain(
                DomainAccMinMaxTree const & accumulator,
                size_t globalMetricIndex,
                Metric const & metric) const;

            // Check if defrag need to be triggered without any specified empty node distribution
            bool ShouldDefragRunByNumberOfEmptyNodes(
                DomainAccMinMaxTree const & accumulator,
                size_t globalMetricIndex,
                Metric const & metric) const;

            std::vector<Metric> metrics_;
            double metricWeightSum_;
            size_t metricStartIndex_;

            // The corresponding service index if this is a local load balancing domain; otherwise -1
            int serviceIndex_;

            // the load statistics of each metric for all nodes, dimension is equal to # of metrics
            std::vector<AccumulatorWithMinMax> loadStats_;

            // the load statistics of each metric for all fault domains, accumulator values are FD loads
            std::vector<AccumulatorWithMinMax> fdLoadStats_;

            // the load statistics of each metric for all upgrade domains, accumulator values are UD loads
            std::vector<AccumulatorWithMinMax> udLoadStats_;

            // whether the cluster is already balanced according to current threshold settings
            // this is updated during placement creation
            bool isBalanced_;
        };
    }
}
