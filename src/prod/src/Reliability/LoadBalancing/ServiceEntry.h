// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "BalanceChecker.h"
#include "Service.h"
#include "DiagnosticsDataStructures.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class NodeEntry;
        class PartitionEntry;
        class ApplicationEntry;
        class PlacementReplica;
        class LoadBalancingDomainEntry;
        class ServicePackageEntry;

        class ServiceEntry
        {
            DENY_COPY(ServiceEntry);
        public:

            static Common::GlobalWString const TraceDescription;

            ServiceEntry(
                std::wstring && name,
                ServicePackageEntry const* servicePackage,
                bool isStateful,
                bool hasPersistedState,
                int targetReplicaSetSize,
                DynamicBitSet && blockList,
                DynamicBitSet && primaryBlockList,
                size_t serviceBlockListCount,
                std::vector<size_t> && globalMetricIndices,
                bool onEveryNode,
                Service::Type::Enum distribution,
                BalanceChecker::DomainTree && faultDomainStructure,
                BalanceChecker::DomainTree && upgradeDomainStructure,
                ServiceEntryDiagnosticsData && diagnosticsData,
                bool partiallyPlace,
                ApplicationEntry const* application,
                bool metricHasCapacity,
                bool autoSwitchToQuorumBasedLogic
                );

            ServiceEntry(ServiceEntry && other);

            ServiceEntry & operator = (ServiceEntry && other);

            PartitionEntry const& SelectPartition(size_t partitionIndex) const { return *(partitions_[partitionIndex]); }

            void SetLBDomain(LoadBalancingDomainEntry const* lbDomain, LoadBalancingDomainEntry const* globalLBDomain);

            void SetDependedService(ServiceEntry const* service, bool alignedAffinity);
            void AddDependentService(ServiceEntry const* service);

            bool IsNodeInBlockList(NodeEntry const* node) const { return blockList_.Check(node->NodeIndex); }
            bool IsNodeInPrimaryReplicaBlockList(NodeEntry const* node) const { return primaryReplicaBlockList_.Check(node->NodeIndex); }

            void AddPartition(PartitionEntry const* partition);

            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const { return isStateful_; }

            __declspec (property(get = get_HasPersistedState)) bool HasPersistedState;
            bool get_HasPersistedState() const { return hasPersistedState_; }

            __declspec(property(get = get_IsVolatile)) bool IsVolatile;
            bool get_IsVolatile() const { return (IsStateful && !HasPersistedState); }

            __declspec (property(get = get_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

            __declspec (property(get = get_IsTargetOne)) bool IsTargetOne;
            bool get_IsTargetOne() const { return TargetReplicaSetSize == 1; }

            __declspec(property(get = get_ServicePackage)) ServicePackageEntry const* ServicePackage;
            ServicePackageEntry const* get_ServicePackage() const { return servicePackage_; }

            __declspec (property(get=get_ZeroLoadEntry)) LoadEntry const& ZeroLoadEntry;
            LoadEntry const& get_ZeroLoadEntry() const { return zero_; }

            __declspec (property(get=get_FaultDomainStructure)) BalanceChecker::DomainTree const& FaultDomainStructure;
            BalanceChecker::DomainTree const& get_FaultDomainStructure() const { return faultDomainStructure_; }

            __declspec (property(get=get_UpgradeDomainStructure)) BalanceChecker::DomainTree const& UpgradeDomainStructure;
            BalanceChecker::DomainTree const& get_UpgradeDomainStructure() const { return upgradeDomainStructure_; }

            __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
            size_t get_ReplicaCount() const { return replicaCount_; }

            __declspec (property(get=get_PartitionCount)) size_t PartitionCount;
            size_t get_PartitionCount() const { return partitions_.size(); }

            __declspec (property(get = get_Partitions)) std::vector<PartitionEntry const*> const& Partitions;
            std::vector<PartitionEntry const*> const& get_Partitions() const { return partitions_; }

            __declspec (property(get=get_LBDomain)) LoadBalancingDomainEntry const* LBDomain;
            LoadBalancingDomainEntry const* get_LBDomain() const { return lbDomain_; }

            __declspec (property(get=get_GlobalLBDomain)) LoadBalancingDomainEntry const* GlobalLBDomain;
            LoadBalancingDomainEntry const* get_GlobalLBDomain() const { return globalLBDomain_; }

            __declspec (property(get=get_AffinitizedService)) ServiceEntry const* DependedService;
            ServiceEntry const* get_AffinitizedService() const { return dependedService_; }

            __declspec (property(get=get_AlignedAffinity)) bool AlignedAffinity;
            bool get_AlignedAffinity() const { return alignedAffinity_; }

            __declspec (property(get = get_DiagnosticsData)) ServiceEntryDiagnosticsData const& DiagnosticsData;
            ServiceEntryDiagnosticsData const& get_DiagnosticsData() const { return diagnosticsData_; }

            __declspec (property(get=get_DependentServices)) std::vector<ServiceEntry const*> const& DependentServices;
            std::vector<ServiceEntry const*> const& get_DependentServices() const { return dependentServices_; }

            __declspec (property(get=get_BlockListCount)) size_t BlockListCount;
            size_t get_BlockListCount() const { return blockList_.Count; }

            __declspec (property(get=get_BlockList)) DynamicBitSet const& BlockList;
            DynamicBitSet const& get_BlockList() const { return blockList_; }

            __declspec (property(get=get_PrimaryReplicaBlockList)) DynamicBitSet const& PrimaryReplicaBlockList;
            DynamicBitSet const& get_PrimaryReplicaBlockList() const { return primaryReplicaBlockList_; }

            __declspec (property(get=get_IsBalanced)) bool IsBalanced;
            bool get_IsBalanced() const { return isBalanced_; }

            __declspec (property(get=get_MetricCount)) size_t MetricCount;
            size_t get_MetricCount() const { return globalMetricIndices_.size(); }

            __declspec (property(get=get_ServiceBlockListCount)) size_t ServiceBlockListCount;
            size_t get_ServiceBlockListCount() const { return serviceBlockListCount_; }

            // Convert from service metric to total metric index
            __declspec (property(get=get_GlobalMetricIndices)) std::vector<size_t> const& GlobalMetricIndices;
            std::vector<size_t> const& get_GlobalMetricIndices() const { return globalMetricIndices_; }

            __declspec (property(get=get_OnEveryNode)) bool OnEveryNode;
            bool get_OnEveryNode() const { return onEveryNode_; }

            __declspec (property(get=get_FDDistribution)) Service::Type::Enum FDDistribution;
            Service::Type::Enum get_FDDistribution() const { return FDDistribution_; }

            __declspec (property(get = get_HasAffinity, put = set_HasAffinity)) bool HasAffinity;
            bool get_HasAffinity() const { return hasAffinity_; }
            void set_HasAffinity(bool hasAffinity) { hasAffinity_ = hasAffinity; }

            __declspec (property(get = get_HasAlignedAffinity)) bool IsAlignedChild;
            bool get_HasAlignedAffinity() const { return dependedService_ != nullptr && alignedAffinity_; }

            __declspec (property(get = get_PartiallyPlace)) bool PartiallyPlace;
            bool get_PartiallyPlace() const { return partiallyPlace_; }

            __declspec (property(get = get_Application)) ApplicationEntry const* Application;
            ApplicationEntry const* get_Application() const { return application_;}

            __declspec (property(get = get_HasInUpgradePartition, put = set_HasInUpgradePartition)) bool HasInUpgradePartition;
            bool get_HasInUpgradePartition() const { return hasInUpgradePartition_; }
            void set_HasInUpgradePartition(bool hasInUpgradePartition) { hasInUpgradePartition_ = hasInUpgradePartition; }

            __declspec (property(get = get_HasAffinityAssociatedSingletonReplicaInUpgrade,
                put = set_HasAffinityAssociatedSingletonReplicaInUpgrade)) bool HasAffinityAssociatedSingletonReplicaInUpgrade;
            bool get_HasAffinityAssociatedSingletonReplicaInUpgrade() const { return hasAffinityAssociatedSingletonReplicaInUpgrade_; }
            void set_HasAffinityAssociatedSingletonReplicaInUpgrade(bool hasAffinityAssociatedSingletonReplicaInUpgrade)
            {
                hasAffinityAssociatedSingletonReplicaInUpgrade_ = hasAffinityAssociatedSingletonReplicaInUpgrade;
            }

            __declspec (property(get = get_MetricHasCapacity)) bool MetricHasCapacity;
            bool get_MetricHasCapacity() const { return metricHasCapacity_; }

            _declspec (property(get = get_AutoSwitchToQuorumBasedLogic)) bool AutoSwitchToQuorumBasedLogic;
            bool get_AutoSwitchToQuorumBasedLogic() const { return autoSwitchToQuorumBasedLogic_; }

            void RefreshIsBalanced();

            void AddAlignedAffinityPartitions();
            std::vector<PartitionEntry const*> const& AffinityAssociatedPartitions() const;

            std::vector<PlacementReplica const*> AllAffinityAssociatedReplicasWithTarget1() const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring name_;
            ServiceEntryDiagnosticsData diagnosticsData_;
            bool isStateful_;
            bool hasPersistedState_;
            int targetReplicaSetSize_;

            ServicePackageEntry const* servicePackage_;

            LoadEntry zero_; // for the zero entry for all the partitions of this service

            DynamicBitSet blockList_;
            DynamicBitSet primaryReplicaBlockList_;

            size_t replicaCount_;

            //Used to store the number of nodes that are blocklisted by placementconstraint only. Avoiding passing the entire ServiceBlocklist to avoid an unnecessary copy
            size_t serviceBlockListCount_;

            std::vector<PartitionEntry const*> partitions_;

            LoadBalancingDomainEntry const* lbDomain_;
            LoadBalancingDomainEntry const* globalLBDomain_;

            ServiceEntry const* dependedService_; // the service that the current service affinitize to
            bool alignedAffinity_; //whehther the primary need to be aligned to parent, only applicable for stateful service
            std::vector<ServiceEntry const*> dependentServices_; // the services that are affinitized to the current service

            bool isBalanced_; // whether this service is already balanced or its balancing cannot be improved

            // gives the total index for each service metric
            std::vector<size_t> globalMetricIndices_;

            bool onEveryNode_; // whether to place on instance on every node
            Service::Type::Enum FDDistribution_;
            bool partiallyPlace_;

            // empty if no need to create fd structure, or we use placement's fd structure, or the service is on every node, or the service's FD policy is none
            BalanceChecker::DomainTree faultDomainStructure_;

            // empty if no need to create ud structure, or we use placement's ud structure, or the service is on every node
            BalanceChecker::DomainTree upgradeDomainStructure_;

            bool hasAffinity_; // whether the service has parent service or not

            ApplicationEntry const* application_;

            bool hasInUpgradePartition_;
            std::vector<PartitionEntry const*> alignedAffinityAssociatedPartitions_;

            bool metricHasCapacity_;

            // Service has singleton replica in upgrade or
            // it is in affinity correlation with service which has
            bool hasAffinityAssociatedSingletonReplicaInUpgrade_;

            // Has service automaticlly switched from '+1' to quorum based logic for its partitons replica distribution across FD/UD?
            bool autoSwitchToQuorumBasedLogic_;
        };
    }
}
