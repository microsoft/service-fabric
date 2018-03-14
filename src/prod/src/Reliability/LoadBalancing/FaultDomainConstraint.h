// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "IConstraint.h"
#include "BalanceChecker.h"
#include "PlacementReplicaSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FaultDomainSubspace : public ISubspace
        {
            DENY_COPY(FaultDomainSubspace);
        public:
            FaultDomainSubspace(bool isFaultDomain, bool relaxed);

            virtual void GetTargetNodes(
                TempSolution const& tempSolution,
                PlacementReplica const* replica,
                NodeSet & candidateNodes,
                bool useNodeBufferCapacity,
                NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr = nullptr) const;

            virtual bool PromoteSecondary(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;

            virtual void PrimarySwapback(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                NodeSet & candidateNodes) const;

            virtual void GetNodesForReplicaDrop(
                TempSolution const& tempSolution,
                PartitionEntry const& partition,
                NodeSet & candidateNodes) const;

            virtual IConstraint::Enum get_Type() const { return IConstraint::FaultDomain; }

            virtual ~FaultDomainSubspace() {}

            static bool IsQuorumBasedDomain(TempSolution const& tempSolution, bool isFaultDomain, ServiceEntry const* service);

        private:

            bool IsValidDomain(
                Common::TreeNodeIndex const& candidateNodeIndex,
                PartitionDomainTree::Node const& replicaTree,
                BalanceChecker::DomainTree::Node const& nodeTree,
                Common::TreeNodeIndex const& currentDomain,
                bool isUnplacedReplica,
                bool nonPacking,
                bool & focusSourceDomain,
                Common::TreeNodeIndex const ** sourceDomainIndex,
                size_t & sourceDomainParentLevel) const;

            void FilterCandidateNodes(
                PartitionDomainTree::Node const& replicaTree,
                BalanceChecker::DomainTree::Node const& nodeTree,
                Common::TreeNodeIndex && currentDomain,
                NodeSet & candidateNodes,
                bool nonPacking) const;

            void FilterCandidateDomains(
                std::map<Common::TreeNodeIndex, NodeSet> const& treeIndexMapWithNodeSet,
                PartitionDomainTree::Node const& replicaTree,
                BalanceChecker::DomainTree::Node const& nodeTree,
                Common::TreeNodeIndex && currentDomain,
                NodeSet & candidateNodes,
                bool nonPacking) const;

            void FilterCandidateNodesForDrop(
                PartitionDomainTree::Node const& replicaTree,
                BalanceChecker::DomainTree::Node const& nodeTree,
                NodeSet & candidateNodes,
                PartitionEntry const& partition) const;

            bool IsValidQuorumDomain(
                Common::TreeNodeIndex const& candidateNodeIndex,
                PartitionDomainTree const* replicaTreeBase,
                PartitionDomainTree const* replicaTreeTemp,
                BalanceChecker::DomainTree const* nodeTree,
                Common::TreeNodeIndex const& currentDomain,
                PartitionEntry const* partition) const;

            void FilterQuorumCandidateDomains(
                std::map<Common::TreeNodeIndex, NodeSet> const& treeIndexMapWithNodeSet,
                PartitionDomainTree const* replicaTreeBase,
                PartitionDomainTree const* replicaTreeTemp,
                BalanceChecker::DomainTree const* nodeTree,
                Common::TreeNodeIndex && currentDomain,
                NodeSet & candidateNodes,
                PartitionEntry const* partition) const;

            void FilterQuorumCandidateNodes(
                PartitionDomainTree const* replicaTreeBase,
                PartitionDomainTree const* replicaTreeTemp,
                BalanceChecker::DomainTree const* nodeTree,
                Common::TreeNodeIndex && currentDomain,
                NodeSet & candidateNodes,
                PartitionEntry const* partition) const;

            bool isFaultDomain_;
            bool relaxed_;
        };

        // fault domain or upgrade domain are both implemented here
        class FaultDomainConstraint : public IDynamicConstraint
        {
            DENY_COPY(FaultDomainConstraint);
        public:

            FaultDomainConstraint(IConstraint::Enum name, int priority);

            __declspec (property(get = get_IsFaultDomain)) bool IsFaultDomain;
            bool get_IsFaultDomain() const { return domainName_ == IConstraint::FaultDomain; }

            static bool HasDomainViolation(
                PartitionDomainTree::Node const& replicaTree,
                BalanceChecker::DomainTree::Node const& nodeTree,
                bool nonPacking);

            static bool HasQuorumDomainViolation(
                PartitionDomainTree const* replicaTreeBase,
                PartitionDomainTree const* replicaTreeTemp,
                PartitionEntry const* partition,
                BalanceChecker::DomainTree const* nodeTree,
                bool relaxed,
                std::function<bool(std::vector<PlacementReplica const*> const&, size_t)> processor);

            static void GetInvalidReplicas(
                PartitionDomainTree::Node const& replicaTree,
                BalanceChecker::DomainTree::Node const& nodeTree,
                PlacementReplicaSet & invalidReplicas,
                size_t countToRemove,
                Common::Random & random,
                bool nonPacking,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr);

            static void GetQuorumInvalidReplicas(
                PartitionDomainTree const* replicaTreeBase,
                PartitionDomainTree const* replicaTreeTemp,
                PartitionEntry const* partition,
                BalanceChecker::DomainTree const* nodeTree,
                PlacementReplicaSet & invalidReplicas,
                bool relaxed,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr);

            virtual Enum get_Type() const { return domainName_; }

            virtual IViolationUPtr GetViolations(
                TempSolution const& solution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random& random) const;

            static bool GetViolations(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool relaxed,
                set<PartitionEntry const*> & invalidPartitions);

            virtual ConstraintCheckResult CheckSolutionAndGenerateSubspace(
                TempSolution const& tempSolution,
                bool changedOnly,
                bool relaxed,
                bool useNodeBufferCapacity,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr = nullptr) const;

            virtual ~FaultDomainConstraint() {}

            static bool GetFirstChildSubDomain(
                PartitionDomainTree const* replicaTreeBase,
                PartitionDomainTree const* replicaTreeTemp,
                BalanceChecker::DomainTree const* nodeTree,
                Common::TreeNodeIndex const& candidateNodeIndex,
                size_t & level,
                size_t & targetSegment,
                size_t & childDomainsCount,
                size_t & replicaCountTemp,
                size_t & replicaCountBase);

            static bool GetTempTrees(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool validateReplicaCount,
                PartitionDomainTree const* &replicaTree,
                BalanceChecker::DomainTree const* &nodeTree);

            static bool GetBaseTrees(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool validateReplicaCount,
                PartitionDomainTree const* &replicaTree,
                BalanceChecker::DomainTree const* &nodeTree);

            static bool GetTempAndBaseTrees(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool validateReplicaCount,
                PartitionDomainTree const* &replicaTempTree,
                PartitionDomainTree const* &replicaBaseTree,
                BalanceChecker::DomainTree const* &nodeTree);

        private:

            static bool GetTrees(
                PartitionDomainStructure const& faultDomainStructures,
                PartitionDomainStructure const& upgradeDomainStructures,
                Placement const * originalPlacement,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool validateReplicaCount,
                PartitionDomainTree const* &replicaTree,
                BalanceChecker::DomainTree const* &nodeTree);

            static bool GetNodeTrees(
                Placement const * originalPlacement,
                PartitionEntry const* partition,
                bool isFaultDomain,
                BalanceChecker::DomainTree const* &nodeTree);

            static bool GetReplicaTrees(
                PartitionDomainStructure const& faultDomainStructures,
                PartitionDomainStructure const& upgradeDomainStructures,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool validateReplicaCount,
                PartitionDomainTree const* &replicaTree);

            static void GetReplicas(
                std::vector<PlacementReplica const*> const& replicas,
                PlacementReplicaSet & invalidReplicas,
                size_t countToRemove,
                Common::Random & random);

            static void GetInvalidReplicas(
                TempSolution const& tempSolution,
                PartitionEntry const* partition,
                bool isFaultDomain,
                bool relaxed,
                Common::Random & random,
                std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr,
                PlacementReplicaSet & invalidReplicas);

            Enum domainName_;
        };

    }
}
