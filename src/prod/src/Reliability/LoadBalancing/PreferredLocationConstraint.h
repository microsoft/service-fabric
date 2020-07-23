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
        /////////////////////////////////
        // classes for preferred location
        // replicas of a service should be placed on certain nodes
        /////////////////////////////////
        class PreferredLocationConstraint : public IStaticConstraint
        {
            DENY_COPY(PreferredLocationConstraint);
        public:

            PreferredLocationConstraint(int priority)
                : IStaticConstraint(priority)
            {
            }

            virtual Enum get_Type() const { return Enum::PreferredLocation; }

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
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual bool CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const;

            static std::set<Common::TreeNodeIndex> const& GetUpgradedUDs(TempSolution const& tempSolution, PartitionEntry const* partition);
            static bool CheckUpgradedUDs(std::set<Common::TreeNodeIndex> const& upgradedUDsToUse, NodeEntry const* target);

            virtual ~PreferredLocationConstraint() {}

            void FilterPreferredNodesWithImages(
                NodeSet & nodesPreferredContainerPlacement,
                PlacementReplica const* replica) const;

        private:
            static std::set<Common::TreeNodeIndex> const& EmptyUpgradedUDs;

        };

        class PreferredLocationSubspace : public StaticSubspace
        {
            DENY_COPY(PreferredLocationSubspace);
        public:
            PreferredLocationSubspace(PreferredLocationConstraint const* constraint);

            virtual void GetNodesForReplicaDrop(
                TempSolution const& tempSolution,
                PartitionEntry const& partition,
                NodeSet & candidateNodes) const;

            virtual bool PromoteSecondary(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const;

            virtual ~PreferredLocationSubspace() {}

        private:

            bool IsNodeInOverloadedDomain(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeEntry const* node,
                bool isFaultDomain) const;
        };
    }
}
