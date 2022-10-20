// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "BalanceChecker.h"
#include "LoadBalancingDomainEntry.h"
#include "PartitionEntry.h"
#include "NodeEntry.h"
#include "LoadEntry.h"
#include "PlacementReplica.h"
#include "LazyMap.h"
#include "ReplicaSet.h"
#include "PartitionPlacement.h"
#include "PartitionDomainStructure.h"
#include "ServiceEntry.h"
#include "NodeSet.h"
#include "ServiceDomainMetric.h"
#include "ApplicationEntry.h"
#include "ApplicationPlacement.h"
#include "ApplicationNodeCount.h"
#include "ApplicationTotalLoad.h"
#include "ApplicationNodeLoads.h"
#include "ApplicationReservedLoads.h"
#include "ServicePackageEntry.h"
#include "ServicePackagePlacement.h"
#include "InBuildCountPerNode.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Placement;
        typedef std::unique_ptr<Placement> PlacementUPtr;

        /// <summary>
        /// Class to store a static copy of current system configuration. It keeps
        /// the placement and metric information for all nodes_ and partitions_; and 
        /// provide interface for load balancing algorithm to access the info in 
        /// different way. It is shared by all CandidateSolution instance as the basic 
        /// placement and scores; CandidateSolution only store the changes of placement 
        /// and scores.
        /// </summary>
        class Placement
        {
            DENY_COPY(Placement);

        public:
            Placement(
                BalanceCheckerUPtr && balanceChecker,
                std::vector<ServiceEntry> && services,
                std::vector<PartitionEntry> && partitions,
                std::vector<ApplicationEntry> && applications,
                std::vector<ServicePackageEntry> && servicePackages,
                std::vector<std::unique_ptr<PlacementReplica>> && allReplicas,
                std::vector<std::unique_ptr<PlacementReplica>> && standByReplicas,
                ApplicationReservedLoad && reservationLoads,
                InBuildCountPerNode && ibCountsPerNode,
                size_t hasUpgradePartitions,
                bool isSingletonReplicaMoveAllowedDuringUpgrade,
                int randomSeed,
                PLBSchedulerAction const& schedulerAction,
                PartitionClosureType::Enum partitionClosureType,
                std::set<Common::Guid> const& partialClosureFTs,
                ServicePackagePlacement && servicePackagePlacement,
                size_t quorumBasedServicesCount,
                size_t quorumBasedPartitionsCount,
                std::set<uint64> && quorumBasedServicesTempCache);

            __declspec (property(get=get_BalanceChecker)) BalanceCheckerUPtr const& BalanceCheckerObj;
            BalanceCheckerUPtr const& get_BalanceChecker() const { return balanceChecker_; }

            __declspec (property(get=get_TotalReplicaCount)) size_t TotalReplicaCount;
            size_t get_TotalReplicaCount() const { return allReplicas_.size(); }

            __declspec (property(get=get_NewReplicaCount)) size_t NewReplicaCount;
            size_t get_NewReplicaCount() const { return newReplicas_.size(); }

            __declspec (property(get=get_ExistingReplicaCount)) size_t ExistingReplicaCount;
            size_t get_ExistingReplicaCount() const
            {
                ASSERT_IF(allReplicas_.size() < newReplicas_.size(), "All replica size should be >= new replica size");
                return (allReplicas_.size() - newReplicas_.size());
            }

            __declspec (property(get=get_NodeCount)) size_t NodeCount;
            size_t get_NodeCount() const { return balanceChecker_->Nodes.size(); }

            __declspec (property(get=get_PartitionCount)) size_t PartitionCount;
            size_t get_PartitionCount() const { return partitions_.size(); }

            __declspec (property(get = get_ApplicationCount)) size_t ApplicationCount;
            size_t get_ApplicationCount() const { return applications_.size(); }

            __declspec (property(get=get_LBDomains)) std::vector<LoadBalancingDomainEntry> const& LBDomains;
            std::vector<LoadBalancingDomainEntry> const& get_LBDomains() const { return balanceChecker_->LBDomains; }

            __declspec (property(get=get_IsBalanced)) bool IsBalanced;
            bool get_IsBalanced() const { return balanceChecker_->IsBalanced; }

            __declspec (property(get=get_Services)) std::vector<ServiceEntry> const& Services;
            std::vector<ServiceEntry> const& get_Services() const { return services_; }

            __declspec (property(get=get_ParentPartitions)) std::vector<PartitionEntry const*> const& ParentPartitions;
            std::vector<PartitionEntry const*> const& get_ParentPartitions() const { return parentPartitions_; }

            __declspec (property(get=get_TotalMetricCount)) size_t TotalMetricCount;
            size_t get_TotalMetricCount() const { return balanceChecker_->TotalMetricCount; }

            __declspec (property(get=get_GlobalMetricCount)) size_t GlobalMetricCount;
            size_t get_GlobalMetricCount() const { return balanceChecker_->LBDomains.back().MetricCount; }

            __declspec (property(get=get_NodePlacements)) NodePlacement const& NodePlacements;
            NodePlacement const& get_NodePlacements() const { return nodePlacements_; }

            __declspec (property(get = get_NodeMovingInPlacements)) NodePlacement const& NodeMovingInPlacements;
            NodePlacement const& get_NodeMovingInPlacements() const { return nodeMovingInPlacements_; }

            __declspec (property(get=get_PartitionPlacements)) PartitionPlacement const& PartitionPlacements;
            PartitionPlacement const& get_PartitionPlacements() const { return partitionPlacements_; }

            __declspec(property(get = get_ServicePackagePlacement)) ServicePackagePlacement const& ServicePackagePlacements;
            ServicePackagePlacement const& get_ServicePackagePlacement() const { return servicePackagePlacements_; }

            __declspec(property(get = get_InBuildCountsPerNode)) InBuildCountPerNode InBuildCountsPerNode;
            InBuildCountPerNode const& get_InBuildCountsPerNode() const { return inBuildCountPerNode_; }

            __declspec (property(get = get_ApplicationPlacements)) ApplicationPlacement const& ApplicationPlacements;
            ApplicationPlacement const& get_ApplicationPlacements() const { return applicationPlacements_; }

            __declspec(property(get = get_ApplicationNodeLoads)) ApplicationNodeLoads const& ApplicationNodeLoad;
            ApplicationNodeLoads const& get_ApplicationNodeLoads() const { return applicationNodeLoads_; }

            __declspec(property(get = get_ApplicationReservedLoads)) ApplicationReservedLoad const& ApplicationReservedLoads;
            ApplicationReservedLoad const& get_ApplicationReservedLoads() const { return applicationReservedLoads_; }

            __declspec (property(get = get_ApplicationNodeCount)) ApplicationNodeCount const& ApplicationNodeCounts;
            ApplicationNodeCount const& get_ApplicationNodeCount() const { return applicationNodeCount_; }

            __declspec (property(get=get_FaultDomainStructures)) PartitionDomainStructure const& FaultDomainStructures;
            PartitionDomainStructure const& get_FaultDomainStructures() const { return faultDomainStructures_; }

            __declspec (property(get=get_UpgradeDomainStructures)) PartitionDomainStructure const& UpgradeDomainStructures;
            PartitionDomainStructure const& get_UpgradeDomainStructures() const { return upgradeDomainStructures_; }

            __declspec (property(get = get_ApplicationsTotalLoad)) ApplicationTotalLoad const& ApplicationsTotalLoads;
            ApplicationTotalLoad const& get_ApplicationsTotalLoad() const { return applicationTotalLoad_; }

            PartitionEntry const& SelectPartition(size_t partitionIndex) const { return partitions_[partitionIndex]; }
            ApplicationEntry const& SelectApplication(size_t applicationIndex) const { return applications_[applicationIndex]; }

            NodeEntry const& SelectNode(size_t nodeIndex) const { return balanceChecker_->Nodes[nodeIndex]; }

            PlacementReplica const* SelectNewReplica(size_t newReplicaIndex) const { return newReplicas_[newReplicaIndex]; }

            PlacementReplica const* SelectRandomReplica(Common::Random & random, bool useNodeLoadAsHeuristic, bool useRestrictedDefrag, bool targetEmptyNodesAchieved) const;
            PlacementReplica const* SelectRandomPrimary(Common::Random & random, bool useNodeLoadAsHeuristic, bool useRestrictedDefrag, bool targetEmptyNodesAchieved) const;
            PlacementReplica const* SelectRandomExistingReplica(Common::Random & random, bool usePartialClosure) const;

            PlacementReplica const* SelectReplica(size_t replicaIndex) const { return allReplicas_[replicaIndex].get(); }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

            std::wstring GetImbalancedServices() const;

            NodeSet const* GetBeneficialTargetNodesForMetric(size_t globalMetricIndex) const;

            NodeSet const* GetBeneficialTargetNodesForPlacementForMetric(size_t globalMetricIndex) const;

            bool CanNewReplicasBePlaced() const;

            __declspec (property(get=get_ExtraReplicasCount)) size_t ExtraReplicasCount;
            size_t get_ExtraReplicasCount() const { return extraReplicasCount_; }

            __declspec (property(get=get_PartitionsInUpgradeCount)) size_t PartitionsInUpgradeCount;
            size_t get_PartitionsInUpgradeCount() const { return partitionsInUpgradeCount_; }

            __declspec(property(get = get_PartitionsInUpgradePlacementCount)) size_t PartitionsInUpgradePlacementCount;
            size_t get_PartitionsInUpgradePlacementCount() const { return partitionsInUpgradePlacementCount_; }

            __declspec (property(get = get_IsSingletonReplicaMoveAllowedDuringUpgrade)) bool IsSingletonReplicaMoveAllowedDuringUpgrade;
            bool get_IsSingletonReplicaMoveAllowedDuringUpgrade() const { return isSingletonReplicaMoveAllowedDuringUpgrade_; }

            __declspec(property(get = get_EligibleNodes)) NodeSet const& EligibleNodes;
            NodeSet const& get_EligibleNodes() const { return eligibleNodes_; }

            __declspec(property(get = get_UseHeuristicDuringPlacement)) bool UseHeuristicDuringPlacement;
            bool get_UseHeuristicDuringPlacement() const { return beneficialTargetNodesForPlacementPerMetric_.size() > 0; }

            __declspec (property(get = get_PlacementRandom)) Common::Random& PlacementRandom;
            Common::Random& get_PlacementRandom() { return random_; }

            __declspec (property(get = getSearcherSettings)) SearcherSettings const& Settings;
            SearcherSettings const& getSearcherSettings() const { return settings_; }

            __declspec (property(get = get_PartitionBatchIndexVec)) std::vector<size_t> const& PartitionBatchIndexVec;
            std::vector<size_t> const& get_PartitionBatchIndexVec() const { return partitionBatchIndexVec_; }

            static bool IsSwapBeneficial(PlacementReplica const* replica);

            __declspec (property(get = get_Random)) Common::Random& Random;
            Common::Random& get_Random() { return random_; }

            __declspec (property(get = get_QuorumBasedServicesCount)) size_t QuorumBasedServicesCount;
            size_t get_QuorumBasedServicesCount() const { return quorumBasedServicesCount_; }

            __declspec (property(get = get_QuorumBasedPartitionsCount)) size_t QuorumBasedPartitionsCount;
            size_t get_QuorumBasedPartitionsCount() const { return quorumBasedPartitionsCount_; }

            __declspec (property(get = getQuorumBasedServicesTempCache)) std::set<uint64> const& QuorumBasedServicesTempCache;
            std::set<uint64> const& getQuorumBasedServicesTempCache() const { return quorumBasedServicesTempCache_; }

            // Returns maximum number of moves that can be generated due to node throttling.
            size_t GetThrottledMoveCount() const;

            __declspec(property(get = get_Action)) PLBSchedulerActionType::Enum Action;
            PLBSchedulerActionType::Enum get_Action() const { return action_; }

            __declspec(property(get = get_IsThrottlingConstraintNeeded)) bool IsThrottlingConstraintNeeded;
            bool get_IsThrottlingConstraintNeeded() const { return throttlingConstraintPriority_ >= 0; }

            __declspec(property(get = get_ThrottlingConstraintPriority)) int ThrottlingConstraintPriority;
            int get_ThrottlingConstraintPriority() const { return throttlingConstraintPriority_; }

            // Updates the action, and updates nodes for throttling.
            void UpdateAction(PLBSchedulerActionType::Enum action, bool constructor = false);
        private:
            void PrepareServices();
            void PreparePartitions();
            void PrepareReplicas(std::set<Common::Guid>const& partialClosureFTs);
            void ComputeBeneficialTargetNodesPerMetric();
            void ComputeBeneficialTargetNodesForPlacementPerMetric();
            void CreateReplicaPlacement();
            void CreateNodePlacement();
            void PrepareApplications();

        private:
            // Functions for verification of internal structures.
            void VerifyApplicationEntries();

            BalanceCheckerUPtr balanceChecker_;

            // Dimension: #services
            std::vector<ServiceEntry> services_;

            // Dimension: #partitions
            std::vector<PartitionEntry> partitions_;

            std::vector<ApplicationEntry> applications_;

            std::vector<ServicePackageEntry> servicePackages_;

            // all partitions that are parent of other partitions
            std::vector<PartitionEntry const*> parentPartitions_;

            // Dimension: #all_replicas, including existing and new, without standBy replicas
            std::vector<std::unique_ptr<PlacementReplica>> allReplicas_;

            // All standBy replicas
            std::vector<std::unique_ptr<PlacementReplica>> standByReplicas_;

            // partialClosureReplicas used in NewReplicaPlacementWithMove phase with partial closure
            // includes new replicas expanded with affinity (parent and all its child replicas) and appGroup relations
            std::vector<PlacementReplica const*> partialClosureReplicas_;

            // Movable existing replicas
            std::vector<PlacementReplica const*> movableReplicas_;

            // Replicas among movableReplicas_ with at least one imbalanced metric whose load on the corresponding node is
            // over/under average for balancing/defrag metric, across nodes
            std::vector<PlacementReplica const*> beneficialReplicas_;

            // Replicas among beneficialReplicas_ which are on the nodes we want to free up during defrag
            // The nodes are <target_free_nodes> least loaded nodes
            std::vector<PlacementReplica const*> beneficialDefragReplicas_;

            // Replicas among beneficialReplicas_ which are on the nodes we want to free up during defrag
            // All replicas from this vector must be on N nodes, where N is number of nodes we are trying to free up
            std::vector<PlacementReplica const*> beneficialRestrictedDefragReplicas_;

            // Swappable existing primary replicas
            std::vector<PlacementReplica const*> swappablePrimaryReplicas_;

            // Replicas among movablePrimaryReplicas_ with at least one imbalanced metric whose load
            // 1. if primary load > secondary load, on the corresponding node is over/under average for balancing/defrag, across nodes
            // 2. if primary load < secondary load, on the corresponding node is under/over average for balancing/defrag, across nodes
            std::vector<PlacementReplica const*> beneficialPrimaryReplicas_;

            // Replicas among beneficialPrimaryReplicas_ which are on the nodes we want to free up during defrag
            std::vector<PlacementReplica const*> beneficialPrimaryDefragReplicas_;

            // Replicas among beneficialPrimaryReplicas_ which are on the nodes we want to free up during defrag
            // All replicas from this vector must be on N nodes, where N is number of nodes we are trying to free up
            std::vector<PlacementReplica const*> beneficialRestrictedDefragPrimaryReplicas_;

            // New replicas
            std::vector<PlacementReplica const*> newReplicas_;

            // node entry mapping to replicas
            NodePlacement nodePlacements_;

            // node entry mapping to replicas that are meved into the node during simulated annealing run
            NodePlacement nodeMovingInPlacements_;

            // partition entry mapping to replica placements
            PartitionPlacement partitionPlacements_;

            // application entry mapping to replica placements (only live replicas)
            ApplicationPlacement applicationPlacements_;

            // ApplicationEntry -> NodeEntry -> count of replicas on node (used for scaleout constraint).
            ApplicationNodeCount applicationNodeCount_;

            // ApplicationEntry -> NodeEntry -> Load on node (used for application capacity constraint).
            ApplicationNodeLoads applicationNodeLoads_;

            // NodeEntry -> LoadEntry for reserved load on node for application (reserved, but not used by replicas).
            ApplicationReservedLoad applicationReservedLoads_;

            // Application total load
            ApplicationTotalLoad applicationTotalLoad_;

            // partition entry mapping to fault domain structures, empty if no need to create fd structure
            // if the partition's service is on every node or the partition's service's fd policy is none, it's not in the list
            PartitionDomainStructure faultDomainStructures_;

            // partition entry mapping to upgrade domain structures, empty if no need to create ud structure
            // if the partition's service is on every node it's not in the list
            PartitionDomainStructure upgradeDomainStructures_;

            // map from global metric index to the nodes which have under/over average load value for that balancing/defrag metric
            std::map<size_t, NodeSet> beneficialTargetNodesPerMetric_;

            // map from global metric index to the nodes which are good candidates for placement
            std::map<size_t, NodeSet> beneficialTargetNodesForPlacementPerMetric_;

            size_t partitionsInUpgradeCount_;

            size_t partitionsInUpgradePlacementCount_;

            size_t extraReplicasCount_;

            bool isSingletonReplicaMoveAllowedDuringUpgrade_;

            // Nodes that are eligible for moving and placement (excluding down nodes).
            NodeSet eligibleNodes_;

            Common::Random random_;

            SearcherSettings const & settings_;

            PartitionClosureType::Enum partitionClosureType_;

            // NodeEntry -> ServicePackageEntry -> pair<ReplicaSet, count of replicas>
            ServicePackagePlacement servicePackagePlacements_;

            // NodeEntry -> Number of InBuild replicas (used for throttling)
            InBuildCountPerNode inBuildCountPerNode_;

            std::vector<size_t> partitionBatchIndexVec_;

            size_t quorumBasedServicesCount_;
            size_t quorumBasedPartitionsCount_;

            // Makes temporary cache of services that use quorum based logic.
            // Takes into account only services that have a partition in closure.
            std::set<uint64> quorumBasedServicesTempCache_;

            // Action that this placement object was made for
            PLBSchedulerActionType::Enum action_;

            // If greater than zero, then there are nodes that are throttled.
            // Checker should use ThrottlingConstraint for this placement in that case.
            int throttlingConstraintPriority_;
        };
    }
}
