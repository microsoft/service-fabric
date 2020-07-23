// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LazyMap.h"
#include "ReplicaSet.h"
#include "PartitionPlacement.h"
#include "ApplicationPlacement.h"
#include "ApplicationNodeCount.h"
#include "ApplicationTotalLoad.h"
#include "ApplicationNodeLoads.h"
#include "ApplicationReservedLoads.h"
#include "NodeMetrics.h"
#include "Accumulator.h"
#include "Score.h"
#include "Movement.h"
#include "BoundedSet.h"
#include "SearcherSettings.h"
#include "ServicePackagePlacement.h"
#include "SearchInsight.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        class TempSolution;

        /// <summary>
        /// The class is used in Load Balancing algorithms to generate 
        /// candidate solutions from original placement. CandidateSolution 
        /// only store moves and changes to the metrics. All CandidateSolution 
        /// share one Placement object
        /// There is no gurantee that a CandidateSolution doesn't violate
        /// placement constraints
        /// </summary>
        class CandidateSolution
        {
            DENY_COPY_ASSIGNMENT(CandidateSolution);

        public:

            CandidateSolution(
                Placement const * originalPlacement,
                std::vector<Movement> && creations,
                std::vector<Movement> && migrations,
                PLBSchedulerActionType::Enum currentSchedulerAction,
                SearchInsight && searchInsight = SearchInsight());

            CandidateSolution(CandidateSolution && other);
            CandidateSolution& operator=(CandidateSolution && other);

            CandidateSolution(CandidateSolution const& other);

            __declspec (property(get=get_OriginalPlacement)) Placement const * OriginalPlacement;
            Placement const * get_OriginalPlacement() const { return originalPlacement_; }

            __declspec (property(get=get_Migrations)) std::vector<Movement> const& Migrations;
            std::vector<Movement> const& get_Migrations() const { return migrations_; }

            __declspec (property(get=get_Creations)) std::vector<Movement> const& Creations;
            std::vector<Movement> const& get_Creations() const { return creations_; }

            __declspec (property(get=get_MaxNumberOfCreationAndMigration)) size_t MaxNumberOfCreationAndMigration;
            size_t get_MaxNumberOfCreationAndMigration() const { return creations_.size() + migrations_.size(); }

            __declspec (property(get=get_MaxNumberOfMigration)) size_t MaxNumberOfMigration;
            size_t get_MaxNumberOfMigration() const { return migrations_.size(); }

            __declspec (property(get=get_MaxNumberOfCreation)) size_t MaxNumberOfCreation;
            size_t get_MaxNumberOfCreation() const { return creations_.size(); }

            __declspec (property(get=get_NodeChanges)) NodeMetrics const& NodeChanges;
            NodeMetrics const& get_NodeChanges() const { return nodeChanges_; }

            __declspec (property(get = get_NodeMovingInChanges)) NodeMetrics const& NodeMovingInChanges;
            NodeMetrics const& get_NodeMovingInChanges() const { return nodeMovingInChanges_; }

            __declspec (property(get=get_MoveCost)) size_t MoveCost;
            size_t get_MoveCost() const { return moveCost_; }

            __declspec (property(get=get_ValidMoveCount)) size_t ValidMoveCount;
            size_t get_ValidMoveCount() const { return validMoveCount_; }

            __declspec(property(get = get_ValidSwapCount)) size_t ValidSwapCount;
            size_t get_ValidSwapCount() const { return validSwapCount_; }

            __declspec (property(get = get_ValidAddCount)) size_t ValidAddCount;
            size_t get_ValidAddCount() const { return validAddCount_; }

            __declspec (property(get=get_Score)) Score const& SolutionScore;
            Score const& get_Score() const { return score_; }

            __declspec (property(get=get_Energy)) double Energy;
            double get_Energy() const { return score_.Energy; }

            __declspec (property(get=get_AvgStdDev)) double AvgStdDev;
            double get_AvgStdDev() const { return score_.AvgStdDev; }

            __declspec (property(get=get_Cost)) double Cost;
            double get_Cost() const { return score_.Cost; }

            __declspec (property(get=get_ReplicaMovementPositions)) std::map<PlacementReplica const*, size_t> const& ReplicaMovementPositions;
            std::map<PlacementReplica const*, size_t> const& get_ReplicaMovementPositions() const { return replicaMovementPositions_; }

            __declspec (property(get=get_NodePlacements)) NodePlacement const& NodePlacements;
            NodePlacement const& get_NodePlacements() const { return nodePlacements_; }

            __declspec (property(get = get_NodeMovingInPlacements)) NodePlacement const& NodeMovingInPlacements;
            NodePlacement const& get_NodeMovingInPlacements() const { return nodeMovingInPlacements_; }

            __declspec (property(get=get_PartitionPlacements)) PartitionPlacement const& PartitionPlacements;
            PartitionPlacement const& get_PartitionPlacements() const { return partitionPlacements_; }

            __declspec (property(get = get_InBuildCountsPerNode)) InBuildCountPerNode const& InBuildCountsPerNode;
            InBuildCountPerNode const& get_InBuildCountsPerNode() const { return inBuildCountsPerNode_; }

            __declspec(property(get = get_ServicePackagePlacement)) ServicePackagePlacement const& ServicePackagePlacements;
            ServicePackagePlacement const& get_ServicePackagePlacement() const { return servicePackagePlacements_; }

            __declspec (property(get = get_ApplicationPlacements)) ApplicationPlacement const& ApplicationPlacements;
            ApplicationPlacement const& get_ApplicationPlacements() const { return applicationPlacements_; }

            __declspec(property(get = get_ApplicationNodeCounts)) ApplicationNodeCount const& ApplicationNodeCounts;
            ApplicationNodeCount const& get_ApplicationNodeCounts() const { return applicationNodeCounts_; }

            __declspec(property(get = get_ApplicationTotalLoad)) ApplicationTotalLoad const& ApplicationTotalLoads;
            ApplicationTotalLoad const& get_ApplicationTotalLoad() const { return applicationTotalLoad_; }

            __declspec(property(get = get_ApplicationNodeLoads)) ApplicationNodeLoads const& ApplicationNodeLoad;
            ApplicationNodeLoads const& get_ApplicationNodeLoads() const { return applicationNodeLoads_; }

            __declspec(property(get = get_ApplicationReservedLoads)) ApplicationReservedLoad const& ApplicationReservedLoads;
            ApplicationReservedLoad const& get_ApplicationReservedLoads() const { return applicationReservedLoads_; }

            __declspec (property(get=get_FaultDomainStructures)) PartitionDomainStructure const& FaultDomainStructures;
            PartitionDomainStructure const& get_FaultDomainStructures() const { return faultDomainStructures_; }

            __declspec (property(get=get_UpgradeDomainStructures)) PartitionDomainStructure const& UpgradeDomainStructures;
            PartitionDomainStructure const& get_UpgradeDomainStructures() const { return upgradeDomainStructures_; }

            __declspec (property(get = get_DynamicNodeLoads)) DynamicNodeLoadSet& DynamicNodeLoads;
            DynamicNodeLoadSet& get_DynamicNodeLoads() { return dynamicNodeLoads_; }

            __declspec (property(get = get_CurrentSchedulerAction)) PLBSchedulerActionType::Enum const& CurrentSchedulerAction;
            PLBSchedulerActionType::Enum const& get_CurrentSchedulerAction() const { return currentSchedulerAction_; }

            Movement const& GetMovement(size_t index) const { return index < creations_.size() ? creations_[index] : migrations_[index - creations_.size()]; }
 
            __declspec (property(get = get_SolutionSearchInsight, put = set_SolutionSearchInsight)) SearchInsight& SolutionSearchInsight;
            SearchInsight& get_SolutionSearchInsight() { return solutionSearchInsight_; }
            void set_SolutionSearchInsight(SearchInsight&& value) { solutionSearchInsight_ = move(value); }
            bool HasSearchInsight() const { return solutionSearchInsight_.HasInsight; }

            // Get reserved load on a node from this object.
            int64 GetApplicationReservedLoad(NodeEntry const* node, size_t capacityIndex) const
            {
                return ((LoadEntry const&)applicationReservedLoads_[node]).Values[capacityIndex];
            }

            Movement & GetMovement(size_t index)  { return index < creations_.size() ? creations_[index] : migrations_[index - creations_.size()]; }

            size_t GetReplicaMovementPosition(PlacementReplica const* replica) const;

            NodeEntry const* GetReplicaLocation(PlacementReplica const* replica) const;

            Score TryChange(TempSolution const& tempSolution) const;
            void UndoChange(TempSolution const& tempSolution) const;

            void ApplyChange(TempSolution const& tempSolution, Score && newScore);

            void ApplyChange(TempSolution const& tempSolution);

            void ApplyChange(std::map<size_t, Movement> const& moveChanges);

            void ApplyMoveChange(TempSolution const& tempSolution);

            BoundedSet<PlacementReplica const*> GetUnplacedReplicas() const;
            bool IsAnyReplicaPlaced() const { return validAddCount_ > 0; }

            bool AllReplicasPlaced() const { return validAddCount_ >= MaxNumberOfCreation; }

        private:

            void ModifyAll(Movement const & oldMove, Movement const & newMove, size_t index);
            void ModifyReplicaMovementPositions(Movement const & oldMove, Movement const & newMove, size_t index);
            void ModifyValidMoveCount(Movement const & oldMove, Movement const & newMove);

            Placement const * originalPlacement_;

            std::vector<Movement> creations_;

            std::vector<Movement> migrations_;

            std::map<PlacementReplica const*, size_t> replicaMovementPositions_;

            // node entry mapping to replicas
            NodePlacement nodePlacements_;

            // node entry mapping to replicas that are meved into the node during simulated annealing run
            NodePlacement nodeMovingInPlacements_;

            // partition entry mapping to replica placements
            PartitionPlacement partitionPlacements_;

            // Node to InBuild replica count mapping
            InBuildCountPerNode inBuildCountsPerNode_;

            // partition entry mapping to replica placements
            ApplicationPlacement applicationPlacements_;

            // Application -> Node -> replica count mapping
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

            DynamicNodeLoadSet dynamicNodeLoads_;

            NodeMetrics nodeChanges_;

            NodeMetrics nodeMovingInChanges_;

            // Number of swaps in this solution
            size_t validSwapCount_;

            // Move cost for all moves present in this solution
            size_t moveCost_;

            size_t validMoveCount_;

            size_t validAddCount_;

            Score score_;

            PLBSchedulerActionType::Enum currentSchedulerAction_;

            // NodeEntry -> ServicePackageEntry -> pair<ReplicaSet, count of replicas>
            ServicePackagePlacement servicePackagePlacements_;

            // Solution insight for search engine runs
            SearchInsight solutionSearchInsight_;
        };
    }
}
