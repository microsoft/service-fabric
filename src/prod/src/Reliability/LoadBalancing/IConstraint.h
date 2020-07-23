// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "PlacementReplicaSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class IViolation;
        typedef std::unique_ptr<IViolation> IViolationUPtr;
        class IConstraint;
        typedef std::unique_ptr<IConstraint> IConstraintUPtr;
        class IStaticConstraint;
        typedef std::unique_ptr<IStaticConstraint> IStaticConstraintUPtr;
        class ISubspace;
        typedef std::unique_ptr<ISubspace> ISubspaceUPtr;
        class IDynamicConstraint;
        typedef std::unique_ptr<IDynamicConstraint> IDynamicConstraintUPtr;

        class NodeSet;
        class PlacementReplica;
        class NodeEntry;
        class Checker;
        class TempSolution;
        class PartitionEntry;

        class IConstraintDiagnosticsData;
        class NodeCapacityViolationDiagnosticsData;
        typedef std::shared_ptr<IConstraintDiagnosticsData> IConstraintDiagnosticsDataSPtr;
        typedef std::shared_ptr<NodeCapacityViolationDiagnosticsData> NodeCapacityViolationDiagnosticsDataSPtr;
        typedef std::map<Federation::NodeId, IConstraintDiagnosticsDataSPtr> NodeToConstraintDiagnosticsDataMap;
        typedef std::shared_ptr<NodeToConstraintDiagnosticsDataMap> NodeToConstraintDiagnosticsDataMapSPtr;

        struct ConstraintCheckResult
        {
            ConstraintCheckResult(ISubspaceUPtr && subspace, PlacementReplicaSet && invalidReplicas)
                : Subspace(move(subspace)), InvalidReplicas(move(invalidReplicas))
            {
            }

            __declspec (property(get=get_IsValid)) bool IsValid;
            bool get_IsValid() const { return InvalidReplicas.empty(); }

            ISubspaceUPtr Subspace;
            PlacementReplicaSet InvalidReplicas;
        };

        class IConstraint
        {
        public:
            enum Enum
            {
                ReplicaExclusionStatic = 0,
                ReplicaExclusionDynamic = 1,
                PlacementConstraint = 2,
                NodeCapacity = 3,
                Affinity = 4,
                FaultDomain = 5,
                UpgradeDomain = 6,
                PreferredLocation = 7,
                ScaleoutCount = 8,
                ApplicationCapacity = 9,
                Throttling = 10
            };

        public:
            IConstraint(int priority) : priority_(priority)
            {
            }

            __declspec (property(get=get_Priority)) int Priority;
            int get_Priority() const { return priority_; }

            __declspec (property(get=get_IsStatic)) bool IsStatic;
            virtual bool get_IsStatic() const = 0;

            __declspec (property(get=get_Type)) Enum Type;
            virtual Enum get_Type() const = 0;

            // this method return violations of the current solution
            virtual IViolationUPtr GetViolations(
                TempSolution const& solution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random& random) const = 0;

            // this method correct all violations found in the solution and move the replicas to other nodes without considering other constraints
            // this method only correct a single replica/partition/node, the result can be worse than the base solution
            // it's expected every violations are moved, so the MoveSolution later can only look at changed ones
            virtual void CorrectViolations(TempSolution & solution, std::vector<ISubspaceUPtr> const& subspaces, Common::Random & random) const;

            // check the solution to see if it satisfies the constraint, output all replicas that don't satisfy the constraint,
            // also output the corresponding subspace
            // Only changed partitions/replicas/nodes need to be checked
            // This function will make sure the output is not worse than base solution
            virtual ConstraintCheckResult CheckSolutionAndGenerateSubspace(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const = 0;

            virtual ~IConstraint() = 0 {}

        private:
            int priority_;
        };


        class ISubspace
        {
        public:
            enum Enum
            {
                OnlySecondaries = 0,
                IsInTransition = 1,
                NodeBlockList = 2,
            };
            // get the target node of a replica within the current subspace based on the specified candidate nodes

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const = 0;

            virtual void GetTargetNodesForReplicas(
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            // returns bool indicator that represents should PLB send Void movement to the FM
            virtual bool PromoteSecondary(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;

            virtual void PromoteSecondaryForPartitions(
                TempSolution const& tempSolution,
                std::vector<PartitionEntry const*> const& partitions,
                NodeSet & candidateNodes) const;

            virtual void PromoteSecondary(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes,
                ISubspace::Enum filter) const;

            virtual void PrimarySwapback(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;

            // Get the node from which replicas of a partition can be dropped within the current subspace
            virtual void GetNodesForReplicaDrop(
                TempSolution const& tempSolution,
                PartitionEntry const& partition,
                NodeSet & candidateNodes) const;

            virtual bool FilterCandidateNodesForCapacityConstraints(
                NodeEntry const *node,
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                bool useNodeBufferCapacity,
                bool forApplicationCapacity) const;

            virtual bool FilterCandidateNodesForCapacityConstraints(
                NodeEntry const *node,
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                bool useNodeBufferCapacity,
                IConstraintDiagnosticsDataSPtr const & constraintDiagnosticsDataSPtr,
                bool forApplicationCapacity) const;

            virtual bool FilterByNodeCapacity(
                NodeEntry const *node,
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                int64 replicaLoadValue,
                bool useNodeBufferCapacity,
                size_t capacityIndex,
                size_t globalMetricIndex,
                IConstraintDiagnosticsDataSPtr const constraintDiagnosticsDataSPtr = nullptr) const;



            __declspec (property(get=get_Type)) IConstraint::Enum Type;
            virtual IConstraint::Enum get_Type() const = 0;

            virtual ~ISubspace() = 0 {}
        };

        void WriteToTextWriter(Common::TextWriter & w, IConstraint::Enum const & val);

        class IStaticConstraint: public IConstraint
        {
        public:
            IStaticConstraint(int priority) : IConstraint(priority)
            {
            }

            virtual bool get_IsStatic() const { return true; }

            virtual IViolationUPtr GetViolations(
                TempSolution const& solution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random& random) const;

            virtual ConstraintCheckResult CheckSolutionAndGenerateSubspace(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const = 0;

            // static part of a constraint, which only check if a replica can be placed on a target
            // no matter where it is now
            virtual bool CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const = 0;

            virtual ~IStaticConstraint() = 0 {}

        protected:

            PlacementReplicaSet GetInvalidReplicas(
            TempSolution const& solution,
            bool changedOnly,
            bool relaxed,
            std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

        private:
            bool AllowedPlacementOnDeactivatedNode(
                TempSolution const& solution,
                NodeEntry const* currentNode,
                PlacementReplica const* replica) const;
        };

        class StaticSubspace : public ISubspace
        {
        public:
            StaticSubspace(IStaticConstraint const* constraint, bool relaxed);

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual void PrimarySwapback(TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;

            virtual bool PromoteSecondary(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;

            virtual void PromoteSecondary(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes,
                ISubspace::Enum filter) const;

            virtual IConstraint::Enum get_Type() const { return constraint_->Type; }
        private:
            IStaticConstraint const* constraint_;
            bool relaxed_;
        };

        class IDynamicConstraint: public IConstraint
        {
        public:

            IDynamicConstraint(int priority) : IConstraint(priority)
            {
            }

            virtual bool get_IsStatic() const { return false; }

            virtual ~IDynamicConstraint() = 0 {}
        };
    }
}
