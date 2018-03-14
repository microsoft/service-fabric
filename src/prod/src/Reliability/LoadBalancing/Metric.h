// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Constants.h"
#include "DynamicBitSet.h"
#include "ServiceMetric.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        // metric used in load balancing domain
        class Metric
        {
            DENY_COPY_ASSIGNMENT(Metric);

        public:
            typedef enum Enum
            {
                SpreadAcrossFDs_UDs = 0,
                NumberOfEmptyNodes = 1
            } DefragDistributionType;

            typedef enum PlacementStrategy
            {
                Balancing = 0,
                ReservationAndBalance = 1,
                Reservation = 2,
                ReservationAndPack = 3,
                Defragmentation = 4,

            } PlacementStrategy;

            Metric(
                std::wstring && name,
                double weight,
                double balancingThreshold,
                DynamicBitSet && blockList,
                uint activityThreshold,
                int64 clusterTotalCapacity,
                int64 clusterBufferedCapacity,
                int64 clusterLoad,
                bool isDefrag,
                int32 defragEmptyNodeCount,
                size_t defragEmptyNodeLoadThreshold,
                DefragDistributionType defragEmptyNodesDistribution,
                double placementHeuristicIncomingLoadFactor,
                double placementHeuristicEmptyLoadFactor,
                bool defragmentationScopedAlgorithmEnabled,
                PlacementStrategy placementStrategy,
                double defragmentationEmptyNodeWeight);

            Metric(Metric const & other);

            Metric(Metric && other);

            Metric & operator = (Metric && other);

            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_Weight)) double Weight;
            double get_Weight() const { return weight_; }

            __declspec (property(get=get_BalancingThreshold)) double BalancingThreshold;
            double get_BalancingThreshold() const { return balancingThreshold_; }

            __declspec (property(get=get_IsBalanced, put=set_IsBalanced)) bool IsBalanced;
            bool get_IsBalanced() const { return isBalanced_; }
            void set_IsBalanced(bool value) { isBalanced_ = value; }

            __declspec (property(get=get_ActivityThreshold)) uint ActivityThreshold;
            uint get_ActivityThreshold() const { return activityThreshold_; }

            __declspec (property(get = get_ClusterTotalCapacity)) int64 ClusterTotalCapacity;
            int64 get_ClusterTotalCapacity() const { return clusterTotalCapacity_; }

            __declspec (property(get = get_ClusterBufferedCapacity)) int64 ClusterBufferedCapacity;
            int64 get_ClusterBufferedCapacity() const { return clusterBufferedCapacity_; }

            __declspec (property(get=get_ClusterLoad)) int64 ClusterLoad;
            int64 get_ClusterLoad() const { return clusterLoad_; }

            __declspec (property(get=get_IsDefrag)) bool IsDefrag;
            bool get_IsDefrag() const { return isDefrag_; }

            __declspec (property(get = get_DefragNodeCount)) int32 DefragNodeCount;
            int32 get_DefragNodeCount() const { return defragEmptyNodeCount_; }

            __declspec (property(get = get_DefragEmptyNodeLoadThreshold)) size_t DefragEmptyNodeLoadThreshold;
            size_t get_DefragEmptyNodeLoadThreshold() const { return defragEmptyNodeLoadThreshold_; }

            __declspec (property(get = get_DefragDistribution)) DefragDistributionType DefragDistribution;
            DefragDistributionType get_DefragDistribution() const { return defragEmptyNodesDistribution_; }

            __declspec (property(get = get_PlacementHeuristicIncomingLoadFactor)) double PlacementHeuristicIncomingLoadFactor;
            double get_PlacementHeuristicIncomingLoadFactor() const { return placementHeuristicIncomingLoadFactor_; }

            __declspec (property(get = get_PlacementHeuristicEmptySpacePercent)) double PlacementHeuristicEmptySpacePercent;
            double get_PlacementHeuristicEmptySpacePercent() const { return placementHeuristicEmptySpacePercent_; }

            __declspec (property(get = get_ShouldCalculateBeneficialNodesForPlacement)) bool ShouldCalculateBeneficialNodesForPlacement;
            bool get_ShouldCalculateBeneficialNodesForPlacement() const;

            __declspec (property(get = get_DefragmentationScopedAlgorithmEnabled)) bool DefragmentationScopedAlgorithmEnabled;
            bool get_DefragmentationScopedAlgorithmEnabled() const { return defragmentationScopedAlgorithmEnabled_; }

            __declspec (property(get = get_PlacementStrategy)) PlacementStrategy placementStrategy;
            PlacementStrategy get_PlacementStrategy() const { return placementStrategy_; }

            __declspec (property(get = get_DefragmentationEmptyNodeWeight)) double DefragmentationEmptyNodeWeight;
            double get_DefragmentationEmptyNodeWeight() const { return defragmentationEmptyNodeWeight_; }

            bool IsValidNode(uint64 nodeIndex) const
            {
                return !blockList_.Check(nodeIndex);
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

        private:

            std::wstring name_;
            double weight_;
            double balancingThreshold_;
            DynamicBitSet blockList_;
            bool isBalanced_;
            uint activityThreshold_;
            // Total capacity on all nodes in the cluster for this metric
            int64 clusterTotalCapacity_;
            // Available capacity on all nodes in the cluster for this metric without node buffer capacity
            int64 clusterBufferedCapacity_;
            // Total load on all nodes in the cluster for this metric
            int64 clusterLoad_;

            // Should this metric be used for defragmentation or for balancing
            bool isDefrag_;
            // Number of nodes which needs to be empty in order to consider cluster defragmented and not to trigger defrag run
            int32 defragEmptyNodeCount_;
            // Node load which is considering node to be empty
            size_t defragEmptyNodeLoadThreshold_;
            // Should defragmentation 0 - keep free nodes balanced among UDs/FDs or 1 - it doesn't matter how nodes are distributed
            DefragDistributionType defragEmptyNodesDistribution_;
            // First placement heuristic factor
            double placementHeuristicIncomingLoadFactor_;
            // Second placement heuristic factor
            double placementHeuristicEmptySpacePercent_;
            bool defragmentationScopedAlgorithmEnabled_;
            PlacementStrategy placementStrategy_;
            double defragmentationEmptyNodeWeight_;
        };

        void WriteToTextWriter(Common::TextWriter & w, Metric::DefragDistributionType const & val);
        void WriteToTextWriter(Common::TextWriter & w, Metric::PlacementStrategy const & val);
    }
}
