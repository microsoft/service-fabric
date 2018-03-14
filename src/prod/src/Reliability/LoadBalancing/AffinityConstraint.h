// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IConstraint.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceEntry;
        class NodeEntry;
        class PartitionEntry;
        class CandidateSolution;
        class ReplicaPlacement;

        class AffinitySubspace : public ISubspace
        {
            DENY_COPY(AffinitySubspace);
        public:
            AffinitySubspace(bool relaxed);

            static int const GetColocatedReplicaCount(
                TempSolution const& tempSolution,
                bool colocatePrimary,
                PartitionEntry const* parentPartition,
                ReplicaPlacement const& childPlacement,
                PlacementReplica const* replicaToExclude = nullptr,
                bool useBaseSolution = false);

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual void GetTargetNodesForReplicas(
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual void GetNodesForReplicaDrop(TempSolution const& tempSolution, PartitionEntry const& partition, NodeSet & candidateNodes) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::Affinity; }

            virtual ~AffinitySubspace() {}

        private:
            bool relaxed_;
        };

        class AffinityConstraint : public IDynamicConstraint
        {
            DENY_COPY(AffinityConstraint);

        public:
            AffinityConstraint(int priority);

            virtual Enum get_Type() const { return Enum::Affinity; }

            virtual IViolationUPtr GetViolations(
                TempSolution const& solution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random& random) const;

            virtual void CorrectViolations(TempSolution & solution, std::vector<ISubspaceUPtr> const& subspaces, Common::Random & random) const;

            virtual ConstraintCheckResult CheckSolutionAndGenerateSubspace(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

            virtual ~AffinityConstraint(){}

            __declspec (property(get = get_AllowParentToMove, put = set_AllowParentToMove)) bool AllowParentToMove;
            bool get_AllowParentToMove() const { return allowParentToMove_; }
            void set_AllowParentToMove(bool allowParentToMove) { allowParentToMove_ = allowParentToMove; }

            // During singleton replica upgrade new secondary replica is created for partitions in upgrade.
            // If some optimization is set (check affinity or relaxed scaleout) move primary is generated,
            // for stateful replicas which are affinitized, but not in upgrade (or if there is no optimization,
            // replica in upgrade needs to be promoted on other node), which will violate alignment.
            // In order to accept solution and avoid stuck in upgrade, alignment is relaxed during upgrade.
            static bool IsRelaxAlignAffinityDuringSingletonReplicaUpgrade(
                PartitionEntry const* parentPartition,
                PartitionEntry const* ChildPartition,
                TempSolution const& tempSolution
            );

        private:

            PlacementReplicaSet GetInvalidReplicas(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

            void GetInvalidReplicas(
                TempSolution const& tempSolution,
                PartitionEntry const* parentPartition,
                PlacementReplicaSet & invalidReplicas,
                bool relaxed,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

            bool allowParentToMove_;
        };
    }
}
