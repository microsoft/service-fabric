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
        class LoadEntry;

        class NodeCapacitySubspace : public ISubspace
        {
            DENY_COPY(NodeCapacitySubspace);
        public:

            NodeCapacitySubspace(bool relaxed,
                std::map<NodeEntry const*, PlacementReplicaSet> && nodeExtraLoads,
                std::map<NodeEntry const*, std::set<ApplicationEntry const*>> && nodeExtraApplications,
                std::map<NodeEntry const*, std::set<ServicePackageEntry const*>> && nodeExtraServicePackages)
                : relaxed_(relaxed),
                  nodeExtraLoads_(move(nodeExtraLoads)),
                  nodeExtraApplications_(move(nodeExtraApplications)),
                  nodeExtraServicePackages_(move(nodeExtraServicePackages))
            {
            }

            __declspec (property(get = get_NodeExtraLoads)) std::map<NodeEntry const*, PlacementReplicaSet> const& NodeExtraLoads;
            std::map<NodeEntry const*, PlacementReplicaSet> const& get_NodeExtraLoads() const { return nodeExtraLoads_; }

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

            virtual bool PromoteSecondary(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;
            virtual void PrimarySwapback(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const;

            virtual void PromoteSecondaryForPartitions(
                TempSolution const& tempSolution,
                std::vector<PartitionEntry const*> const& partitions,
                NodeSet & candidateNodes) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::NodeCapacity; }

            virtual bool FilterByNodeCapacity(
                NodeEntry const *node,
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                int64 replicaLoadValue,
                bool useNodeBufferCapacity,
                size_t capacityIndex,
                size_t globalMetricIndex,
                IConstraintDiagnosticsDataSPtr const constraintDiagnosticsDataSPtr = nullptr) const;

            virtual ~NodeCapacitySubspace() {}

        private:
            bool relaxed_;
            // if a node is not in this table, it either has no capacity
            // or it doesn't violate capacity during constraint check
            std::map<NodeEntry const*, PlacementReplicaSet> nodeExtraLoads_;

            // Because of reserved load we may have to move entire application from the node.
            // If application is in this set, then it needs to be moved.
            std::map<NodeEntry const*, std::set<ApplicationEntry const*>> nodeExtraApplications_;

            // We may need to move the entire Service Package off the node if it is in violation
            std::map<NodeEntry const*, std::set<ServicePackageEntry const*>> nodeExtraServicePackages_;
        };

        class NodeCapacityConstraint : public IDynamicConstraint
        {
            DENY_COPY(NodeCapacityConstraint);

        public:
            NodeCapacityConstraint(int priority);

            static bool FindAllInvalidReplicas(
                TempSolution const& tempSolution,
                PlacementReplicaSet const& candidatesToMoveOut,
                size_t const globalMetricStartIndex,
                Common::Random & random,
                LoadEntry & diff,
                PlacementReplicaSet & invalidReplicas,
                PlacementReplicaSet & nodeInvalidReplicas,
                std::set<ApplicationEntry const*> & nodeInvalidApplications,
                std::set<ServicePackageEntry const*>& nodeInvalidServicePackages,
                NodeEntry const* node,
                bool forApplicationCapacity = false);

            virtual Enum get_Type() const { return Enum::NodeCapacity; }

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

            virtual ~NodeCapacityConstraint(){}
        };
    }
}
