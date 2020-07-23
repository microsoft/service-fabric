// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "LoadBalancingDomainEntry.h"
#include "NodeEntry.h"
#include "ServiceDomainMetric.h"
#include "PartitionClosure.h"
#include "DynamicNodeLoadSet.h"
#include "NodeSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class BalancingDiagnosticsData;
        typedef std::shared_ptr<BalancingDiagnosticsData> BalancingDiagnosticsDataSPtr;

        class BalanceChecker;
        typedef std::unique_ptr<BalanceChecker> BalanceCheckerUPtr;

        class Score;
        class NodeMetrics;
        class SearcherSettings;

        class BalanceChecker
        {
            DENY_COPY(BalanceChecker);

        public:
            struct DomainData
            {
                DomainData(size_t nodeCount)
                    : NodeCount(nodeCount)
                {
                }

                size_t NodeCount;
            };

            typedef Common::Tree<DomainData> DomainTree;

            BalanceChecker(
                DomainTree && faultDomainStructure,
                DomainTree && upgradeDomainStructure,
                std::vector<LoadBalancingDomainEntry> && lbDomainEntries,
                std::vector<NodeEntry> && nodeEntries,
                std::vector<int> && deactivatedNodes,
                std::vector<int> && deactivatingNodesAllowPlacement,
                std::vector<int> && deactivatingNodesAllowServiceOnEveryNode,
                std::vector<int> && downNodes,
                bool existDefragMetric,
                bool existScopedDefragMetric,
                BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr,
                std::set<Common::TreeNodeIndex> && UDsIndex,
                std::map<uint64, std::set<Common::TreeNodeIndex>> && appUDs,
                std::map<Common::TreeNodeIndex, std::vector<wstring>> && faultDomainMappings,
                std::map<Common::TreeNodeIndex, wstring> && upgradeDomainMappings,
                SearcherSettings const & settings,
                PartitionClosureType::Enum closureType,
                std::set<std::wstring> && metricsWithNodeCapacity,
                std::map<Common::TreeNodeIndex, NodeSet> && faultDomainNodeSet,
                std::map<Common::TreeNodeIndex, NodeSet> && upgradeDomainNodeSet
                );

            __declspec (property(get=get_FaultDomainStructure)) DomainTree const& FaultDomainStructure;
            DomainTree const& get_FaultDomainStructure() const { return faultDomainStructure_; }

            __declspec (property(get=get_UpgradeDomainStructure)) DomainTree const& UpgradeDomainStructure;
            DomainTree const& get_UpgradeDomainStructure() const { return upgradeDomainStructure_; }

            __declspec (property(get=get_FaultDomainLoads)) LoadBalancingDomainEntry::DomainAccMinMaxTree const& FaultDomainLoads;
            LoadBalancingDomainEntry::DomainAccMinMaxTree const& get_FaultDomainLoads() const { return faultDomainLoads_; }

            __declspec (property(get=get_UpgradeDomainLoads)) LoadBalancingDomainEntry::DomainAccMinMaxTree const& UpgradeDomainLoads;
            LoadBalancingDomainEntry::DomainAccMinMaxTree const& get_UpgradeDomainLoads() const { return upgradeDomainLoads_; }

            __declspec (property(get = get_DynamicNodeLoads)) DynamicNodeLoadSet& DynamicNodeLoads;
            DynamicNodeLoadSet& get_DynamicNodeLoads() { return dynamicNodeLoads_; }

            __declspec (property(get=get_LBDomains)) std::vector<LoadBalancingDomainEntry> const& LBDomains;
            std::vector<LoadBalancingDomainEntry> const& get_LBDomains() const { return lbDomainEntries_; }

            __declspec (property(get=get_Nodes)) std::vector<NodeEntry> const& Nodes;
            std::vector<NodeEntry> const& get_Nodes() const { return nodeEntries_; }

            __declspec (property(get = get_DeactivatedNodes)) std::vector<int> const& DeactivatedNodes;
            std::vector<int> const& get_DeactivatedNodes() const { return deactivatedNodes_; }

            __declspec (property(get = get_DeactivatingNodesAllowPlacement)) std::vector<int> const& DeactivatingNodesAllowPlacement;
            std::vector<int> const& get_DeactivatingNodesAllowPlacement() const { return deactivatingNodesAllowPlacement_; }

            __declspec (property(get = get_DeactivatingNodesAllowServiceOnEveryNode)) std::vector<int> const& DeactivatingNodesAllowServiceOnEveryNode;
            std::vector<int> const& get_DeactivatingNodesAllowServiceOnEveryNode() const { return deactivatingNodesAllowServiceOnEveryNode_; }

            __declspec (property(get = get_DownNodes)) std::vector<int> const& DownNodes;
            std::vector<int> const& get_DownNodes() const { return downNodes_; }

            __declspec (property(get=get_TotalMetricCount)) size_t TotalMetricCount;
            size_t get_TotalMetricCount() const { return totalMetricCount_; }

            __declspec (property(get=get_IsBalanced)) bool IsBalanced;
            bool get_IsBalanced() const { return isBalanced_; }

            __declspec (property(get=get_ExistDefragMetric)) bool ExistDefragMetric;
            bool get_ExistDefragMetric() const { return existDefragMetric_; }

            __declspec (property(get = get_ExistScopedDefragMetric)) bool ExistScopedDefragMetric;
            bool get_ExistScopedDefragMetric() const { return existScopedDefragMetric_; }

            __declspec (property(get = get_UpgradeCompletedUDs)) std::set<Common::TreeNodeIndex> const& UpgradeCompletedUDs;
            std::set<Common::TreeNodeIndex> const& get_UpgradeCompletedUDs() const { return upgradeCompletedUDsIndex_; }

            __declspec (property(get = get_ApplicationUpgradedUDs)) std::map<uint64, std::set<Common::TreeNodeIndex>> const& ApplicationUpgradedUDs;
            std::map<uint64, std::set<Common::TreeNodeIndex>> const& get_ApplicationUpgradedUDs() const { return applicationUpgradedUDs_; }

            __declspec (property(get = getSearcherSettings)) SearcherSettings const& Settings;
            SearcherSettings const& getSearcherSettings() const { return settings_; }

            __declspec (property(get = get_ClosureType)) PartitionClosureType::Enum ClosureType;
            PartitionClosureType::Enum get_ClosureType() const { return closureType_; }

            __declspec (property(get = get_MetricsWithNodeCapacity)) std::set<std::wstring> const& MetricsWithNodeCapacity;
            std::set<std::wstring> const& get_MetricsWithNodeCapacity() const { return metricsWithNodeCapacity_; }

            __declspec (property(get = get_FaultDomainNodeSet)) std::map<Common::TreeNodeIndex, NodeSet> const& FaultDomainNodeSet;
            std::map<Common::TreeNodeIndex, NodeSet> const& get_FaultDomainNodeSet() const { return faultDomainNodeSet_; }

            __declspec (property(get = get_UpgradeDomainNodeSet)) std::map<Common::TreeNodeIndex, NodeSet> const& UpgradeDomainNodeSet;
            std::map<Common::TreeNodeIndex, NodeSet> const& get_UpgradeDomainNodeSet() const { return upgradeDomainNodeSet_; }

            void RefreshIsBalanced();
            void CreateGlobalMetricIndicesList();
            void CalculateMetricStatisticsForTracing(bool isBegin, const Score& score, const NodeMetrics& nodeChanges);

            void UpdateNodeThrottlingLimit(int nodeIndex, int throttlingLimit);
        private:
            void InitializeEmptyDomainAccMinMaxTree(
                size_t totalMetricCount,
                DomainTree const & sourceTree,
                LoadBalancingDomainEntry::DomainAccMinMaxTree & targetTree);

            void FilterDomainTree(DomainTree & tree, std::vector<int> const& nodesToRemove, bool isFaultDomain);

            DomainTree faultDomainStructure_;
            DomainTree upgradeDomainStructure_;
            std::vector<LoadBalancingDomainEntry> lbDomainEntries_;
            std::vector<NodeEntry> nodeEntries_;
            std::vector<int> deactivatedNodes_;
            std::vector<int> deactivatingNodesAllowPlacement_;
            std::vector<int> deactivatingNodesAllowServiceOnEveryNode_;
            std::vector<int> downNodes_;

            size_t totalMetricCount_;
            bool isBalanced_;

            // dimension: # local lb domain count
            std::vector<std::vector<size_t>> globalMetricIndicesList_;

            LoadBalancingDomainEntry::DomainAccMinMaxTree faultDomainLoads_;
            LoadBalancingDomainEntry::DomainAccMinMaxTree upgradeDomainLoads_;
            DynamicNodeLoadSet dynamicNodeLoads_;

            bool existDefragMetric_;
            bool existScopedDefragMetric_;

            BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr_;

            std::set<Common::TreeNodeIndex> upgradeCompletedUDsIndex_;
            std::map<uint64, std::set<Common::TreeNodeIndex>> applicationUpgradedUDs_;

            std::map<Common::TreeNodeIndex, std::vector<wstring>> faultDomainMappings_;
            std::map<Common::TreeNodeIndex, wstring> upgradeDomainMappings_;

            SearcherSettings const & settings_;

            PartitionClosureType::Enum closureType_;

            // Metrics that has capacity defined on node level
            std::set<std::wstring> metricsWithNodeCapacity_;

            std::map<Common::TreeNodeIndex, NodeSet> faultDomainNodeSet_;
            std::map<Common::TreeNodeIndex, NodeSet> upgradeDomainNodeSet_;

        };
    }
}
