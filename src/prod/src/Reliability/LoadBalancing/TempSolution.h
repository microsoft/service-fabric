// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "NodeMetrics.h"
#include "LazyMap.h"
#include "ReplicaSet.h"
#include "PartitionPlacement.h"
#include "CandidateSolution.h"
#include "SearchInsight.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        class Movement;
        class NodeEntry;
        class CandidateSolution;

        /// <summary>
        /// The class store moves from a CandidateSolution. It doesn't store 
        /// metrics and scores. It is used for constraints to explore solution 
        /// spaces
        /// </summary>
        class TempSolution
        {
            DENY_COPY(TempSolution);
        public:
            TempSolution(CandidateSolution const& baseSolution);

            TempSolution(TempSolution && other);

            TempSolution& operator=(TempSolution && other);

            void ForEachPartition(bool changedOnly, std::function<bool(PartitionEntry const *)> processor) const;
            void ForEachApplication(bool changedOnly, std::function<bool(ApplicationEntry const *)> processor) const;

            // Iterate only through nodes where a specific application has its replicas
            void ForEachNode(ApplicationEntry const* app, bool changedOnly, std::function<bool(NodeEntry const *)> processor) const;

            // Iterate through all nodes that have node capacity or throttling defined
            void ForEachNode(bool changedOnly,
                std::function<bool(NodeEntry const *)> processor,
                bool requireCapacity,
                bool requireThrottling) const;

            void ForEachReplica(bool changedOnly, std::function<bool(PlacementReplica const *)> processor) const;

            NodeEntry const* GetReplicaLocation(PlacementReplica const* replica) const;

            // modify the solution by placing a replica on a specified node
            void PlaceReplica(PlacementReplica const* replica, NodeEntry const* targetNode);
            void PlaceReplicaAndPromote(PlacementReplica * replica, NodeEntry const* targetNode);
            size_t PromoteSecondary(PartitionEntry const* replica, Common::Random & random, NodeEntry const* targetNode);
            void PromoteSecondaryForPartitions(std::vector<PartitionEntry const*> const& partitions, Common::Random & random, NodeEntry const* targetNode);

            void AddVoidMovement(PartitionEntry const* partition, Common::Random & random, NodeEntry const* sourceNode);

            // Modify the solution by dropping the specified replica
            size_t DropReplica(PlacementReplica const* replica, Common::Random & random, NodeEntry const* sourceNode);

            // modify the solution by moving a replica to a specific node at the specified position
            bool MoveReplica(PlacementReplica const* replica,
                NodeEntry const* targetNode,
                Common::Random & random,
                size_t position = SIZE_MAX,
                bool forUpgrade = false);

            void CancelMovement(size_t index);

            bool TrySwapMovement(
                PlacementReplica const* sorceReplica,
                NodeEntry const* targetNode,
                Common::Random & random,
                bool forUpgrade = false);

            void Clear();

            Movement const& GetMove(size_t index) const;
            size_t GetReplicaMovementPosition(PlacementReplica const* replica, bool checkBase) const;

            __declspec (property(get=get_BaseSolution)) CandidateSolution const& BaseSolution;
            CandidateSolution const& get_BaseSolution() const { return baseSolution_; }

            __declspec (property(get=get_OriginalPlacement)) Placement const* OriginalPlacement;
            Placement const* get_OriginalPlacement() const { return baseSolution_.OriginalPlacement; }

            __declspec (property(get=get_MoveChanges)) std::map<size_t, Movement> const& MoveChanges;
            std::map<size_t, Movement> const& get_MoveChanges() const { return moveChanges_; }

            __declspec (property(get=get_NodePlacements)) NodePlacement const& NodePlacements;
            NodePlacement const& get_NodePlacements() const { return nodePlacements_; }

            __declspec (property(get = get_NodeMovingInPlacements)) NodePlacement const& NodeMovingInPlacements;
            NodePlacement const& get_NodeMovingInPlacements() const { return nodeMovingInPlacements_; }

            __declspec (property(get=get_PartitionPlacements)) PartitionPlacement const& PartitionPlacements;
            PartitionPlacement const& get_PartitionPlacements() const { return partitionPlacements_; }

            __declspec(property(get = get_InBuildCountsPerNode)) InBuildCountPerNode const& InBuildCountsPerNode;
            InBuildCountPerNode const& get_InBuildCountsPerNode() const { return inBuildCountsPerNode_; }

            __declspec(property(get = get_ServicePackagePlacement)) ServicePackagePlacement const& ServicePackagePlacements;
            ServicePackagePlacement const& get_ServicePackagePlacement() const { return servicePackagePlacements_; }

            __declspec (property(get = get_ApplicationPlacements)) ApplicationPlacement const& ApplicationPlacements;
            ApplicationPlacement const& get_ApplicationPlacements() const { return applicationPlacements_; }

            __declspec (property(get = get_ApplicationNodeCounts)) ApplicationNodeCount const& ApplicationNodeCounts;
            ApplicationNodeCount const& get_ApplicationNodeCounts() const { return applicationNodeCounts_; }

            __declspec(property(get = get_ApplicationTotalLoad)) ApplicationTotalLoad const& ApplicationTotalLoads;
            ApplicationTotalLoad const& get_ApplicationTotalLoad() const { return applicationTotalLoad_; }

            __declspec (property(get = get_ApplicationNodeLoads)) ApplicationNodeLoads const& ApplicationNodeLoad;
            ApplicationNodeLoads const& get_ApplicationNodeLoads() const { return applicationNodeLoads_; }

            __declspec (property(get = get_ApplicationReservedLoads)) ApplicationReservedLoad const& ApplicationReservedLoads;
            ApplicationReservedLoad const& get_ApplicationReservedLoads() const { return applicationReservedLoads_; }

            __declspec (property(get = get_FaultDomainStructures)) PartitionDomainStructure const& FaultDomainStructures;
            PartitionDomainStructure const& get_FaultDomainStructures() const { return faultDomainStructures_; }

            __declspec (property(get=get_UpgradeDomainStructures)) PartitionDomainStructure const& UpgradeDomainStructures;
            PartitionDomainStructure const& get_UpgradeDomainStructures() const { return upgradeDomainStructures_; }

            __declspec (property(get = get_SolutionSearchInsight, put = set_SolutionSearchInsight)) SearchInsight& SolutionSearchInsight;
            SearchInsight& get_SolutionSearchInsight() { return solutionSearchInsight_; }
            void set_SolutionSearchInsight(SearchInsight&& value) { solutionSearchInsight_ = move(value); }
            bool HasSearchInsight() const { return solutionSearchInsight_.HasInsight; }

            __declspec (property(get=get_NodeChanges)) NodeMetrics const& NodeChanges;
            NodeMetrics const& get_NodeChanges() const { return nodeChanges_; }

            __declspec (property(get = get_NodeMovingInChanges)) NodeMetrics const& NodeMovingInChanges;
            NodeMetrics const& get_NodeMovingInChanges() const { return nodeMovingInChanges_; }

            __declspec (property(get=get_ValidMoveCount)) size_t ValidMoveCount;
            size_t get_ValidMoveCount() const { return validMoveCount_; }

            __declspec (property(get = get_ValidAddCount)) size_t ValidAddCount;
            size_t get_ValidAddCount() const { return validAddCount_; }

            __declspec (property(get = get_ValidSwapCount)) size_t ValidSwapCount;
            size_t get_ValidSwapCount() const { return validSwapCount_; }

            __declspec (property(get=get_MoveCost)) size_t MoveCost;
            size_t get_MoveCost() const { return moveCost_; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return moveChanges_.empty(); }

            __declspec (property(get = get_IsSwapPreferred, put = set_IsSwapPreferred)) bool IsSwapPreferred;
            bool get_IsSwapPreferred() const { return isSwapPreferred_; }
            void set_IsSwapPreferred(bool isSwapPreferred) { isSwapPreferred_ = isSwapPreferred; }

            // Get reserved load on a node from this object.
            int64 GetApplicationReservedLoad(NodeEntry const* node, size_t capacityIndex) const
            {
                return ((LoadEntry const&)applicationReservedLoads_[node]).Values[capacityIndex];
            }

            // Get reserved load on a node from base solution.
            int64 GetBaseApplicationReservedLoad(NodeEntry const* node, size_t capacityIndex) const
            {
                return ((LoadEntry const&)baseSolution_.ApplicationReservedLoads[node]).Values[capacityIndex];
            }

            // Get reserved load on a node from original placement.
            int64 GetOriginalApplicationReservedLoad(NodeEntry const* node, size_t capacityIndex) const
            {
                return ((LoadEntry const&)baseSolution_.OriginalPlacement->ApplicationReservedLoads[node]).Values[capacityIndex];
            }

        private:
            CandidateSolution const& baseSolution_;

            // those changed movements
            std::map<size_t, Movement> moveChanges_;

            // only store those changed replicas, if a replica is moved in base
            // but the move is canceled here, the position will be SIZE_MAX(-1).
            std::map<PlacementReplica const*, size_t> replicaMovementPositions_;

            // node entry mapping to replicas
            NodePlacement nodePlacements_;

            // node entry mapping to replicas that are moved into the node during simulated annealing run
            NodePlacement nodeMovingInPlacements_;

            // partition entry mapping to replica placements
            PartitionPlacement partitionPlacements_;

            // NodeEntry to InBuild replica count mapping
            InBuildCountPerNode inBuildCountsPerNode_;

            // partition entry mapping to replica placements
            ApplicationPlacement applicationPlacements_;

            ApplicationNodeCount applicationNodeCounts_;

            // Applications total load mapping
            ApplicationTotalLoad applicationTotalLoad_;

            // Application -> Node -> load mapping
            ApplicationNodeLoads applicationNodeLoads_;

            // Node -> Reserved load mapping (for all applications)
            ApplicationReservedLoad applicationReservedLoads_;

            // partition entry mapping to fault domain structures
            PartitionDomainStructure faultDomainStructures_;

            // partition entry mapping to upgrade domain structures
            PartitionDomainStructure upgradeDomainStructures_;

            // only store those changed nodes
            NodeMetrics nodeChanges_;

            // only store nodes whose load is increased by some of the movements
            NodeMetrics nodeMovingInChanges_;

            size_t validSwapCount_;

            size_t validMoveCount_;

            size_t validAddCount_;

            // Move cost for all moves that were made in this temporary solution
            size_t moveCost_;

            // find and clear the movement position for a replica
            size_t FindMovementPosition(PlacementReplica const* replica, std::vector<std::pair<size_t, Movement>> & additionalMovements, Common::Random & random) const;

            size_t FindEmptyMigration(Common::Random & random, bool forDroppingExtraReplica = false) const;

            void AddMovement(size_t index, Movement && movement);

            void ModifyValidMoveCount(Movement const & oldMove, Movement const & newMove);

            // NodeEntry -> ServicePackageEntry -> pair<ReplicaSet, count of replicas>
            ServicePackagePlacement servicePackagePlacements_;

            // Solution insight for search run
            SearchInsight solutionSearchInsight_;

            //Used in CC fixing
            bool isSwapPreferred_;
        };
    }
}
