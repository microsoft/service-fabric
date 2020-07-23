// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "IConstraint.h"
#include "BoundedSet.h"
#include "NodeSet.h"
#include "PartitionEntry.h"
#include "PLBEventSource.h"
#include "client/ClientPointers.h"
#include "SearcherSettings.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;

        class Checker;
        typedef std::unique_ptr<Checker> CheckerUPtr;

        class PLBDiagnostics;
        typedef std::shared_ptr<PLBDiagnostics> PLBDiagnosticsSPtr;

        class ViolationList;

        class DiagnosticSubspace;

        class Checker
        {
            DENY_COPY(Checker);

        public:
            enum DiagnosticsOption
            {
                None = 0,
                ConstraintCheck = 1,
                ConstraintFixUnsuccessful = 2,
            };

            explicit Checker(Placement const* pl, PLBEventSource const& trace, PLBDiagnosticsSPtr const& plbDiagnosticsSPtr_);

            // this function assign a target node for all new replicas in the solution
            void PlaceNewReplicas(
                TempSolution & solution,
                Common::Random & random,
                int maxConstraintPriority,
                bool enableVerboseTracing = false,
                bool relaxAffinity = false,
                bool modifySolutionWhenTracing = true) const;

            __declspec (property(get = get_BatchIndex, put = set_BatchIndex)) size_t BatchIndex;
            size_t get_BatchIndex() const { return batchIndex_; }
            void set_BatchIndex(size_t index) { batchIndex_ = index; }

            __declspec(property(get=get_MaxMovementSlots)) size_t MaxMovementSlots;
            size_t get_MaxMovementSlots() const { return maxMovementSlots_; }

            // This function will drop all extra replicas
            void DropExtraReplicas(
                TempSolution & solution,
                Common::Random & random,
                int maxConstraintPriority) const;

            // this function find a target for all replicas in upgrade
            void CheckUpgradePartitionsToBePlaced(
                TempSolution & solution,
                std::vector<PartitionEntry const*> const & partitionsToBePlaced,
                Common::Random & random,
                int maxConstraintPriority,
                std::vector<PartitionEntry const*> & unplacedPartitions) const;

            void CheckSwapPrimaryUpgradePartition(
                TempSolution & tempSolution,
                std::vector<PartitionEntry const*> const & partitions,
                Common::Random & random,
                std::vector<PartitionEntry const*> & unplacedPartitions,
                bool relaxCapacity) const;

            void TraceCheckSwapPrimaryUpgradePartition(
                TempSolution & tempSolution,
                std::vector<PartitionEntry const*> const & partitions,
                Common::Random & random,
                std::vector<PartitionEntry const*> & unplacedPartitions,
                bool relaxCapacity) const;

            // correct the solution if there are constraint violations
            // return whether the correction succeeded
            // this method make sure the result is not worse than base solution
            bool CorrectSolution(
                TempSolution & tempSolution,
                Common::Random & random,
                int maxConstraintPriority,
                bool constraintFixLight,
                size_t currentTransition,
                size_t transitionsPerRound);

            bool DiagnoseCorrectSolution(
                TempSolution & tempSolution,
                Common::Random & random);

            bool MoveSolutionRandomly(
                TempSolution & solution,
                bool swapOnly,
                bool useNodeLoadAsHeuristic,
                int maxConstraintPriority,
                bool useRestrictedDefrag,
                Common::Random & random,
                bool forPlacement = false,
                bool usePartialClosure = false) const;

            int GetMaxPriority() const;

            int GetPriority(IConstraint::Enum type) const;

            void SetMaxPriorityToUse(int maxPriorityToUse = -1);

            ViolationList CheckSolution(
                TempSolution const& tempSolution,
                int maxConstraintPriority,
                bool useNodeBufferCapacity,
                Common::Random& random) const;

            std::set<Common::Guid> CheckSolutionForInvalidPartitions(
                TempSolution const& tempSolution,
                DiagnosticsOption option,
                __out std::vector<std::shared_ptr<IConstraintDiagnosticsData>> & constraintsDiagnosticsData,
                Common::Random& random) const;

            bool CheckSolutionForTriggerMigrateReplicaValidity(TempSolution const& tempSolution, PlacementReplica const* candidateReplica, std::wstring& errMsg);
            std::vector<Federation::NodeId> NodesAvailableForTriggerRandomMove(TempSolution const& tempSolution, PlacementReplica const* candidateReplica, Common::Random & random, bool force);

        private:

            // move solution to correct constraints
            // guarantee to not worse than base solution
            // return false if no such movement exist
            bool MoveSolution(
                TempSolution & tempSolution,
                Common::Random & random,
                int maxConstraintPriority) const;

            bool TraceMoveSolution(
                TempSolution & tempSolution,
                Common::Random & random) const;

            std::vector<ISubspaceUPtr> GenerateStaticSubspacesForRandomMove(
                TempSolution & solution,
                Common::Random & random,
                int maxConstraintPriority) const;

            void InnerPlaceNewReplicas(
                TempSolution & solution,
                Common::Random & random,
                int maxConstraintPriority,
                bool relaxAffinity = false) const;

            void SingletonReplicaUpgrade(
                PartitionEntry const& partition,
                std::set<PlacementReplica const*>& processedParentReplicas,
                std::set<ApplicationEntry const*>& processedScaleoutOneApplications,
                TempSolution & solution,
                NodeSet & candidateNodes,
                NodeSet* differenceNodes,
                std::vector<ISubspaceUPtr>* subspaces,
                std::vector<DiagnosticSubspace>* traceSubspace,
                std::vector<size_t>& newReplicaIndex,
                Common::Random & random,
                int maxConstraintPriority,
                bool isDummyPLB,
                bool isInTraceMode = false,
                bool modifySolutionWhenTracing = true) const;

            void AddOrMoveSingletonRelatedReplicasDuringUpgrade(
                NodeEntry const* node,
                std::vector<PlacementReplica const*> const& replicas,
                std::vector<size_t>& newReplicaIndex,
                TempSolution & solution,
                Common::Random & random,
                std::vector<Common::Guid>& nonCreatedMovements) const;

            void TraceInnerPlaceNewReplicas(
                TempSolution & solution,
                Common::Random & random,
                int maxConstraintPriority,
                bool relaxAffinity = false,
                bool modifySolutionWhenTracing = true) const;

            bool InnerCorrectSolution(
                TempSolution & tempSolution,
                Common::Random & random,
                int maxConstraintPriority,
                bool constraintFixLight);

            bool InnerMoveSolutionRandomly(
                TempSolution & solution,
                bool swapOnly,
                bool useNodeLoadAsHeuristic,
                int maxConstraintPriority,
                bool useRestrictedDefrag,
                Common::Random & random,
                bool forPlacement = false,
                bool usePartialClosure = false) const;

            void GetTargetNodesForPlacedReplica(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                std::vector<ISubspaceUPtr> const& subspaces,
                NodeSet & candidateNodes,
                bool useNodeLoadAsHeuristic,
                bool useNodeBufferCapacity) const;

            void GetTargetNodesForReplicaSwapBack(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                std::vector<ISubspaceUPtr> const& subspaces,
                NodeSet & candidateNodes) const;

            void GetSourceNodesForReplicaDrop(
                TempSolution const& tempSolution,
                PartitionEntry const& partition,
                std::vector<ISubspaceUPtr> const& subspaces,
                NodeSet & candidateNodes) const;

            void CorrectNonPartiallyPlacement(
                TempSolution & tempSolution,
                PartitionEntry const& partition,
                bool forTracing = false) const;

            bool CheckPrimaryToBePlacedUpgradePartition(
                PartitionEntry const& partition,
                TempSolution & tempSolution,
                std::vector<ISubspaceUPtr> const& subspaces,
                Common::Random & random) const;

            void CheckSecondaryToBePlacedUpgradePartition(
                PartitionEntry const& partition,
                size_t newReplicaId,
                std::vector<NodeEntry const*>& remainingSecondaryUpgradeLocations,
                TempSolution & tempSolution,
                std::vector<ISubspaceUPtr> const& subspaces,
                Common::Random & random) const;

            const NodeEntry* ChooseNodeForPlacement(
                Placement const* placement,
                PlacementReplica const* replica,
                NodeSet& candidateNodes,
                Common::Random& random,
                bool isDummyPLB) const;

            const NodeEntry* ChooseNodeForPlacement(
                Placement const* placement,
                std::vector<PlacementReplica const*> const& replicas,
                NodeSet& candidateNodes,
                Common::Random& random,
                bool isDummyPLB) const;

            void GetPartitionStartIndexes(Placement const* placement, size_t & startIndex, size_t & endIndex) const;

            std::vector<IConstraintUPtr> constraints_;

            std::map<IConstraint::Enum, int> constraintPriorities_;

            int affinityConstraintIndex_;

            // the maximal priority of constraints, constraints with 1, 2, ... max are relaxable
            int maxPriority_;

            // The max priority that can be set outside to control what level of constrain to enforce
            int maxPriorityToUse_;

            PLBEventSource const& trace_;
            PLBDiagnosticsSPtr plbDiagnosticsSPtr_;

            // Static constraints used for elimination
            std::vector<size_t> staticConstraintIndexes_;

            SearcherSettings const & settings_;

            size_t batchIndex_;

            // Maximum number of movements due to throttling.
            size_t maxMovementSlots_;
        };
    }
}
