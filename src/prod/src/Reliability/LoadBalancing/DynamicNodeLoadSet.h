// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "LoadEntry.h"
#include "NodeEntry.h"
#include "Metric.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class DynamicNodeLoadSet
        {
        public:
            typedef enum Enum
            {
                Ascending = 0,
                Descending = 1
            } Order;

            DynamicNodeLoadSet();
            DynamicNodeLoadSet(vector<NodeEntry> const& nodeEntries, vector<LoadBalancingDomainEntry> const& lbDomainEntries, bool overlapping);
            DynamicNodeLoadSet(DynamicNodeLoadSet const& other);
            DynamicNodeLoadSet(DynamicNodeLoadSet && other);
            DynamicNodeLoadSet& operator=(DynamicNodeLoadSet& other);
            ~DynamicNodeLoadSet();

            __declspec (property(get = get_Overlapping)) bool Overlapping;
            bool get_Overlapping() const { return overlapping_; }

            void ResetLoads();

            void UpdateNodeLoad(int nodeIndex, int64 newNodeLoad, size_t metricIndex);

            void UpdateMoveCost(int nodeIndex, int64 moveCost);

            void ForEachCandidateNode(
                size_t metricIndex,
                int32 emptyNodesTarget,
                Metric::DefragDistributionType const& defragDistributionType,
                size_t reservationLoad,
                std::function<void(double nodeLoadPercentage)> processor);

            void ForEachCandidateNode(
                size_t metricIndex,
                int32 emptyNodesTarget,
                Metric::DefragDistributionType const& defragDistributionType,
                size_t reservationLoad,
                std::function<void(double nodeLoadPercentage)> processor,
                bool markingBeneficialNodes);

            void PrepareBeneficialNodes(
                size_t metricIndex,
                int32 emptyNodesTarget,
                Metric::DefragDistributionType const& defragDistributionType,
                size_t reservationLoad);

            void PrepareMoveCostBeneficialNodes(
                int32 emptyNodesTarget,
                Metric::DefragDistributionType const& defragDistributionType);

            void ForEachNodeOrdered(
                size_t metricIndex,
                DynamicNodeLoadSet::Order order,
                std::function<bool(int nodeIndex, const LoadEntry* load)> processor);

            bool IsBeneficialNode(size_t nodeIndex, size_t metricIndex) const;

            bool IsBeneficialNodeByMoveCost(size_t nodeIndex) const;

            bool IsEnoughLoadReserved(
                size_t totalMetricIndex,
                int32 numberOfNodesWithReservation,
                Metric::DefragDistributionType const& defragDistributionType,
                size_t reservedLoad);

            void AdvanceVersion();

        private:
            class DynamicNodeLoad
            {
            public:
                DynamicNodeLoad(LoadEntry const* load,
                    LoadEntry const* capacity,
                    Common::TreeNodeIndex const* fdIndex,
                    Common::TreeNodeIndex const* udIndex,
                    bool isUp,
                    int nodeIndex,
                    double& overlappingLoad,
                    int64& moveCost);

                DynamicNodeLoad(DynamicNodeLoad const& other);
                DynamicNodeLoad(DynamicNodeLoad && other);

                __declspec (property(get = get_Load)) const LoadEntry* Loads;
                const LoadEntry* get_Load() const { return load_; }

                __declspec (property(get = get_Capacity)) const LoadEntry* Capacity;
                const LoadEntry* get_Capacity() const { return capacity_; }

                __declspec (property(get = get_fdIndex)) const Common::TreeNodeIndex* FDIndex;
                const Common::TreeNodeIndex* get_fdIndex() const { return fdIndex_; }

                __declspec (property(get = get_udIndex)) const Common::TreeNodeIndex* UDIndex;
                const Common::TreeNodeIndex* get_udIndex() const { return udIndex_; }

                __declspec (property(get = get_IsUp)) bool IsUp;
                bool get_IsUp() const { return isUp_; }

                __declspec (property(get = get_NodeIndex)) int NodeIndex;
                int get_NodeIndex() const { return nodeIndex_; }

                __declspec (property(get = get_OverlappingLoad)) double OverlappingLoad;
                double get_OverlappingLoad() const { return overlappingLoad_; }

                __declspec (property(get = get_MoveCost)) int64 MoveCost;
                int64 get_MoveCost() const { return moveCost_; }

                double Load(size_t totalMetricIndex, size_t globalMetricIndex) const;
                double Load(size_t totalMetricIndex, size_t globalMetricIndex, size_t reservationLoad) const;

            private:
                const LoadEntry* load_;
                const LoadEntry* capacity_;
                const Common::TreeNodeIndex* fdIndex_;
                const Common::TreeNodeIndex* udIndex_;
                bool isUp_;
                int nodeIndex_;
                double& overlappingLoad_;
                int64& moveCost_;
            };

            class NodeLoadComparator
            {
            public:
                NodeLoadComparator(size_t totalMetricIndex, size_t globalMetricIndex, size_t reservation);

                bool operator()(DynamicNodeLoad const* a, DynamicNodeLoad const* b) const;

                __declspec (property(get = get_TotalMetricIndex)) size_t TotalMetricIndex;
                size_t get_TotalMetricIndex() const { return totalMetricIndex_; }

                __declspec (property(get = get_GlobalMetricIndex)) size_t GlobalMetricIndex;
                size_t get_GlobalMetricIndex() const { return globalMetricIndex_; }
                
                __declspec (property(get = get_ReservationAmount)) size_t ReservationAmount;
                size_t get_ReservationAmount() const { return reservationAmount_; }

            private:
                size_t totalMetricIndex_;
                size_t globalMetricIndex_;
                size_t reservationAmount_;
            };

            static const size_t MoveCostIndex = SIZE_MAX;
            static const size_t OverlappingIndex;

            void UpdateLoadOnNode(int nodeIndex, int64 newNodeLoad, size_t metricIndex);

            void CalculateOverlappingLoadOnNode(int nodeIndex, double newLoadPercentage, double oldLoadPercentage);
            bool IsNodeBeneficial(size_t nodeIndex, size_t metricIndex) const;

            size_t nodeCount_;
            std::vector<DynamicNodeLoad> allDynamicNodeLoads_;
            std::vector<LoadEntry> nodeLoads_;
            std::vector<LoadEntry> nodeLoadBackup_;
            std::vector<LoadEntry> nodeCapacities_;
            std::vector<Common::TreeNodeIndex> fdIndexes_;
            std::vector<Common::TreeNodeIndex> udIndexes_;
            std::vector<bool> nodeUp_;
            std::map<size_t, std::set<DynamicNodeLoad*, NodeLoadComparator>> nodeLoadIndexes_;

            // Artificial metric calculated based on all defrag metrics in order to know are empty defrag nodes overlapped
            std::vector<double> overlappingLoads_;
            bool overlapping_;
            std::vector<int64> moveCosts_;

            int version_;
            std::map<size_t, std::vector<int>> lastBeneficialVersions_;
            std::map<size_t, std::map<Common::TreeNodeIndex, int>> fdDistributionHelper_;
            std::map<size_t, std::map<Common::TreeNodeIndex, int>> udDistributionHelper_;
            std::map<size_t, std::pair<size_t, size_t>> reservedLoads_;
        };
    }
}
