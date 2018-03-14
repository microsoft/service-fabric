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

        class ApplicationCapacitySubspace : public ISubspace
        {
            DENY_COPY(ApplicationCapacitySubspace);
        public:
            ApplicationCapacitySubspace(bool relaxed, std::map<NodeEntry const*, PlacementReplicaSet> && nodeExtraLoads)
                : relaxed_(relaxed), nodeExtraLoads_(move(nodeExtraLoads))
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

            virtual void PromoteSecondaryForPartitions(
                TempSolution const& tempSolution,
                std::vector<PartitionEntry const*> const& partitions,
                NodeSet & candidateNodes) const;

            virtual void PrimarySwapback(TempSolution const& tempSolution, 
                PartitionEntry const* partition, 
                NodeSet & candidateNodes) const;

            virtual bool FilterByNodeCapacity(
                NodeEntry const *node,
                TempSolution const& tempSolution,
                std::vector<PlacementReplica const*> const& replicas,
                int64 replicaLoadValue,
                bool useNodeBufferCapacity,
                size_t capacityIndex,
                size_t globalMetricIndex,
                IConstraintDiagnosticsDataSPtr const constraintDiagnosticsDataSPtr = nullptr) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::ApplicationCapacity; }

            virtual ~ApplicationCapacitySubspace() {}

        private:
            bool relaxed_;
            // if a node is not in this table, it either has no capacity
            // or it does't violate capacity during constraint check
            std::map<NodeEntry const*, PlacementReplicaSet> nodeExtraLoads_;

        };

        class ApplicationCapacityConstraint : public IDynamicConstraint
        {
            DENY_COPY(ApplicationCapacityConstraint);

            int64 ApplicationCapacityConstraint::CalculateNodeLoadDiff(
                TempSolution const& tempSolution,
                ApplicationEntry const* application,
                NodeEntry const* node,
                bool relaxed,
                LoadEntry & loadOverCapacity,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

        public:
            ApplicationCapacityConstraint(int priority);

            virtual Enum get_Type() const { return Enum::ApplicationCapacity; }

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

            virtual ~ApplicationCapacityConstraint(){}
        };
    }
}
