// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "NodeMetrics.h"
#include "Accumulator.h"
#include "Placement.h"
#include "BalanceChecker.h"
#include "SearcherSettings.h"
#include "DynamicNodeLoadSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadBalancingDomainEntry;

        class Score
        {
            DENY_COPY_ASSIGNMENT(Score);

        public:
            class DomainMetricAccs
            {
            public:
                DomainMetricAccs()
                {
                }

                DomainMetricAccs(LoadBalancingDomainEntry::DomainMetricAccsWithMinMax const& node)
                {
                    AccEntries.reserve(node.AccMinMaxEntries.size());

                    for (int i = 0; i < node.AccMinMaxEntries.size(); i++)
                    {
                        AccEntries.push_back(Accumulator(node.AccMinMaxEntries[i]));
                    }
                }

                DomainMetricAccs(DomainMetricAccs const& other)
                    : AccEntries(other.AccEntries)
                {
                }

                DomainMetricAccs(DomainMetricAccs && other)
                    : AccEntries(move(other.AccEntries))
                {
                }

                DomainMetricAccs & operator = (DomainMetricAccs && other)
                {
                    if (this != &other)
                    {
                        AccEntries = move(other.AccEntries);
                    }

                    return *this;
                }

                std::vector<Accumulator> AccEntries;
            };

            typedef Common::Tree<DomainMetricAccs> DomainAccTree;


            // passing totalReplicaCount = 0 to turn off the cost portion of the energy calculation
            Score(
                size_t totalMetricCount,
                std::vector<LoadBalancingDomainEntry> const& lbDomainEntries,
                size_t totalReplicaCount,
                LoadBalancingDomainEntry::DomainAccMinMaxTree const& faultDomainInitialLoads,
                LoadBalancingDomainEntry::DomainAccMinMaxTree const& upgradeDomainInitialLoads,
                bool existDefragMetric,
                bool existScopedDefragMetric,
                SearcherSettings const & settings,
                DynamicNodeLoadSet* dynamicNodeLoads);

            Score(Score && other);
            Score(Score const& other);
            Score & operator = (Score && other);

            __declspec (property(get=get_Energy)) double Energy;
            double get_Energy() const { return energy_; }

            __declspec (property(get=get_AvgStdDev)) double AvgStdDev;
            double get_AvgStdDev() const { return avgStdDev_; }

            __declspec (property(get=get_Cost)) double Cost;
            double get_Cost() const { return cost_; }

            __declspec (property(get = get_existDefragMetric)) bool ExistDefragMetric;
            bool get_existDefragMetric() const { return existDefragMetric_; }

            __declspec (property(get = get_defragTargetEmptyNodesAchieved)) bool DefragTargetEmptyNodesAchieved;
            bool get_defragTargetEmptyNodesAchieved() const { return defragTargetEmptyNodesAchieved_ || !existScopedDefragMetric_; }

            _declspec (property(get = get_FaultDomainScores)) const std::vector<Accumulator>& FaultDomainScores;
            const std::vector<Accumulator>& get_FaultDomainScores() const { return fdMetricScores_; }

            _declspec (property(get = get_UpgradeDomainScores)) const std::vector<Accumulator>& UpgradeDomainScores;
            const std::vector<Accumulator>& get_UpgradeDomainScores() const { return udMetricScores_; }

            _declspec (property(get = get_MetricScores)) const std::vector<Accumulator>& MetricScores;
            const std::vector<Accumulator>& get_MetricScores() const { return nodeMetricScores_; }

            _declspec (property(get = get_DynamicNodeLoads, put = put_DynamicNodeLoads)) DynamicNodeLoadSet* DynamicNodeLoads;
            const DynamicNodeLoadSet* get_DynamicNodeLoads() const { return dynamicNodeLoads_; }
            const void put_DynamicNodeLoads(DynamicNodeLoadSet* dynamicNodeLoads) { dynamicNodeLoads_ = dynamicNodeLoads; }

            void Calculate(double moveCost);

            void UpdateMetricScores(NodeMetrics const& nodeChanges);

            void UpdateMetricScores(NodeMetrics const& newChanges, NodeMetrics const& oldChanges);

            void UndoMetricScoresForDynamicLoads(NodeMetrics const& newNodeChanges, NodeMetrics const& oldNodeChanges);

            double CalculateAvgStdDevForMetric(std::wstring const & metricName);

            void ResetDynamicNodeLoads();

        private:
            void ForEachValidMetric(NodeEntry const* node, std::function<void(size_t, size_t, bool, bool)> processor);

            // initialize upgrade/fault domain load tree with Accumulators from tree with AccumulatorWithMinMax
            void InitializeDomainAccTree(
                LoadBalancingDomainEntry::DomainAccMinMaxTree const& sourceTree,
                DomainAccTree & targetTree);

            void UpdateDomainLoads(
                DomainAccTree & domainLoads,
                Common::TreeNodeIndex const& nodeDomainIndex,
                int64 nodeLoadOld,
                int64 nodeLoadNew,
                int64 nodeCapacity,
                size_t totalMetricIndex,
                std::vector<Accumulator> & aggregatedDomainLoads,
                bool updateAggregates = true);

            void UpdateDynamicNodeLoads(DynamicNodeLoadSet* nodeLoadSet, int nodeIndex, int64 newNodeLoad, size_t totalMetricIndex);

            void CalculateAvgStdDev();
            void CalculateCost(double moveCost);
            void CalculateEnergy();

            double StdDevCaculationHelper(size_t metricIndex,
                bool isDefrag,
                bool isScopedDefrag,
                Metric::PlacementStrategy const& placementStrategy,
                int32 emptyNodesTarget,
                double defragEmptyNodeWeight,
                double defragNonEmptyNodeWeight,
                Metric::DefragDistributionType const& defragDistributionType,
                size_t reservationLoad,
                double weight);

            double CalculateFreeNodeScoreHelper(size_t metricScoreIndex,
                int32 emptyNodesTarget,
                Metric::DefragDistributionType const& defragDistributionType,
                size_t reservationLoad);

            size_t totalMetricCount_;
            std::vector<LoadBalancingDomainEntry> const& lbDomainEntries_;
            size_t totalReplicaCount_;

            std::vector<Accumulator> nodeMetricScores_;

            std::vector<Accumulator> udMetricScores_;
            std::vector<Accumulator> fdMetricScores_;

            DomainAccTree faultDomainInitialLoads_;
            DomainAccTree upgradeDomainInitialLoads_;
            DynamicNodeLoadSet* dynamicNodeLoads_;

            bool existDefragMetric_;
            bool existScopedDefragMetric_;

            bool defragTargetEmptyNodesAchieved_;

            double avgStdDev_;

            double cost_;

            double energy_;

            SearcherSettings const & settings_;
        };
    }
}
