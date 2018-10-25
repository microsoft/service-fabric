// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "NodeDescription.h"
#include "Metric.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PartitionClosure;
        class ServiceDomainMetric;
        class Service;
        class Node;
        class LoadBalancingDomainEntry;
        class LoadEntry;
        class NodeEntry;
        class BalancingDiagnosticsData;
        class SearcherSettings;
        class NodeSet;

        typedef std::shared_ptr<BalancingDiagnosticsData> BalancingDiagnosticsDataSPtr;

        class BalanceChecker;
        typedef std::unique_ptr<BalanceChecker> BalanceCheckerUPtr;

        /// <summary>
        /// Class which offers balancing related information only.
        /// This serves as an "early stage" of the placement object.
        /// </summary>
        class BalanceCheckerCreator
        {
            DENY_COPY(BalanceCheckerCreator);

        public:
            struct DomainData
            {
                DomainData()
                    : Segment(), NodeCount(0)
                {
                }

                DomainData(std::wstring && segment, size_t nodeCount)
                    : Segment(move(segment)), NodeCount(nodeCount)
                {
                }

                std::wstring Segment;
                size_t NodeCount;
            };

            typedef Common::Tree<DomainData> DomainTree;

            BalanceCheckerCreator(
                PartitionClosure const& partitionClosure,
                std::vector<Node> const& nodes,
                std::map<std::wstring, ServiceDomainMetric> const& serviceDomainMetrics,
                BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr,
                std::set<std::wstring> const& upgradeCompletedUDs,
                SearcherSettings const & settings);

            BalanceCheckerUPtr Create();


        private:
            void CreateNodes();

            static Common::TreeNodeIndex AddDomainId(NodeDescription::DomainId const& domainId, DomainTree & domainStructure);
            static Common::TreeNodeIndex GetDomainIndex(NodeDescription::DomainId const& domainId, DomainTree const& domainStructure);

            void GetUpgradedUDsIndex(std::set<std::wstring> const& UDs, std::set<Common::TreeNodeIndex> & UDsIndex) const;

            void CreateLoadBalancingDomainEntries();
            void CreateLoadsPerLBDomainList();
            void CreateNodeEntries();
            void CreateApplicationUpgradedUDs();

            void CreateLoadsPerLBDomainListInternal(std::vector<std::vector<LoadEntry>> & loadsPerLBDomainList, bool shouldDisappear);

            bool IsDefragmentationMetric(std::wstring const & metricName);
            int32 GetDefragEmptyNodesCount(std::wstring const& metricName, bool scopedDefragEnabled);
            int64 GetDefragEmptyNodeLoadTreshold(std::wstring const& metricName);
            int64 GetReservationLoadAmount(std::wstring const & metricName, int64 nodeCapacity);
            Metric::DefragDistributionType GetDefragDistribution(std::wstring const& metricName);
            double GetPlacementHeuristicIncomingLoadFactor(std::wstring const& metricName);
            double GetPlacementHeuristicEmptySpacePercent(std::wstring const& metricName);
            bool GetDefragmentationScopedAlgorithmEnabled(std::wstring const& metricName);
            Metric::PlacementStrategy GetPlacementStrategy(std::wstring const& metricName);
            double GetDefragmentationEmptyNodeWeight(std::wstring const& metricName, Metric::PlacementStrategy placementStrategy);
            double GetDefragmentationNonEmptyNodeWeight(std::wstring const& metricName, Metric::PlacementStrategy placementStrategy);
            bool IsBalancingByPercentageEnabled(std::wstring const & metricName);

            std::vector<Node> const& nodes_;
            PartitionClosure const& partitionClosure_;
            std::map<std::wstring, ServiceDomainMetric> const& serviceDomainMetrics_;
            BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr_;

            DomainTree faultDomainStructure_; //empty if fd is disabled in config, or if every node has distinct domain id
            DomainTree upgradeDomainStructure_; //empty if ud is disabled in config, or if every node has distinct domain id

            std::vector<std::map<std::wstring, uint>> capacityRatios_;
            std::vector<std::map<std::wstring, uint>> bufferedCapacities_;
            std::vector<std::map<std::wstring, uint>> totalCapacities_;
            std::vector<Common::TreeNodeIndex> faultDomainIndices_; //lenth 0 if no need to create fd structure
            std::vector<Common::TreeNodeIndex> upgradeDomainIndices_; //lenth 0 if no need to create ud structure

            std::map<Common::TreeNodeIndex, NodeSet> faultDomainNodeSet_;
            std::map<Common::TreeNodeIndex, NodeSet> upgradeDomainNodeSet_;

            std::vector<LoadBalancingDomainEntry> lbDomainEntries_;
            std::vector<std::vector<LoadEntry>> loadsPerLBDomainList_;
            std::vector<std::vector<LoadEntry>> shouldDisappearLoadsPerLBDomainList_;
            std::vector<NodeEntry> nodeEntries_;

            std::vector<int> deactivatedNodes_;

            // Safety checks are in progress, replicas can still be placed
            std::vector<int> deactivatingNodesAllowPlacement_;

            // Safety checks completed, statless services with replicas on every node can still be placed
            std::vector<int> deactivatingNodesAllowServiceOnEveryNode_;

            // Nodes that are down.
            std::vector<int> downNodes_;

            bool existDefragMetric_;
            bool existScopedDefragMetric_;

            std::set<std::wstring> const& upgradeCompletedUDs_;

            std::map<uint64, std::set<Common::TreeNodeIndex>> applicationUpgradedUDs_;

            map<Common::TreeNodeIndex, NodeDescription::DomainId> faultDomainMappings_;
            map<Common::TreeNodeIndex, wstring> upgradeDomainMappings_;

            SearcherSettings const & settings_;

            // Metrics that has capacity defined on node level
            // If no capacity defined for certain metric, node capacity constraint doesn't need to go through all nodes
            std::set<std::wstring> metricsWithNodeCapacity_;

        };

    }
}
