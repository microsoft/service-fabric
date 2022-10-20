// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "ReplicaRole.h"
#include "LoadEntry.h"
#include "NodeSet.h"
#include "ServiceEntry.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceEntry;
        class ApplicationEntry;
        class PlacementReplica;
        class FailoverUnitDescription;
        class NodeEntry;
        class SearcherSettings;

        class PartitionEntry
        {
            DENY_COPY(PartitionEntry);

        public:
            static Common::GlobalWString const TraceDescription;

            enum SingletonReplicaUpgradeOptimizationType
            {
                None = 0,
                CheckAffinityDuringUpgrade = 1,
                RelaxScaleoutDuringUpgrade = 2
            };

            PartitionEntry(
                Common::Guid partitionId,
                int64 version,
                bool isInTransition,
                bool movable,
                ServiceEntry const* service,
                std::vector<PlacementReplica *> && replicas,
                LoadEntry && primary,
                LoadEntry && secondary,
                uint primaryMoveCost,
                uint secondaryMoveCost,
                std::vector<NodeEntry const*> && standByLocations,
                NodeEntry const* primarySwapOutLocation,
                NodeEntry const* primaryUpgradeLocation,
                std::vector<NodeEntry const*> && secondaryUpgradeLocation,
                bool isInUpgrade,
                bool isFUInUpgrade,
                bool isInSingleReplicaUpgrade,
                SingletonReplicaUpgradeOptimizationType upgradeOptimization,
                int order,
                int extraReplicas,
                std::map<Federation::NodeId, std::vector<uint>> const& ftSecondaryMap,
                std::vector<PlacementReplica *> && standbyReplicas,
                SearcherSettings const & settings,
                int targetReplicaSetSize);

            PartitionEntry(PartitionEntry && other);

            PartitionEntry & operator = (PartitionEntry && other);

            // True if this partition has existing replica on the node.
            bool HasReplicaOnNode(NodeEntry const* node) const;

            PlacementReplica const* GetReplica(NodeEntry const* node) const;

            PlacementReplica const* SelectSecondary(Common::Random & rand) const;
            PlacementReplica const* SelectNewReplica(size_t id) const;
            PlacementReplica * NewReplica(size_t index = 0) const;
            PlacementReplica const* SelectExistingReplica(size_t id) const;

            __declspec (property(get=get_PartitionId)) Common::Guid PartitionId;
            Common::Guid get_PartitionId() const { return partitionId_; }

            __declspec (property(get=get_Version)) int64 Version;
            int64 get_Version() const { return version_; }

            __declspec (property(get = get_IsInTransition)) bool IsInTransition;
            bool get_IsInTransition() const { return isInTransition_; }

            __declspec (property(get=get_IsMovable)) bool IsMovable;
            bool get_IsMovable() const { return movable_; }

            __declspec (property(get=get_IsInUpgrade)) bool IsInUpgrade;
            bool get_IsInUpgrade() const { return inUpgrade_; }

            __declspec (property(get = get_IsFUInUpgrade)) bool IsFUInUpgrade;
            bool get_IsFUInUpgrade() const { return isFUInUpgrade_; }

            __declspec (property(get = get_IsInSingleReplicaUpgrade)) bool IsInSingleReplicaUpgrade;
            bool get_IsInSingleReplicaUpgrade() const { return isInSingleReplicaUpgrade_; }

            __declspec (property(get = get_SingletonReplicaUpgradeOptimization, put = set_SingletonReplicaUpgradeOptimization)) SingletonReplicaUpgradeOptimizationType SingletonReplicaUpgradeOptimization;
            SingletonReplicaUpgradeOptimizationType get_SingletonReplicaUpgradeOptimization() const { return singletonReplicaUpgradeOptimization_; }
            void set_SingletonReplicaUpgradeOptimization(SingletonReplicaUpgradeOptimizationType upgradeOptimization) { singletonReplicaUpgradeOptimization_ = upgradeOptimization; }

            bool IsInCheckAffinityOptimization() const { return singletonReplicaUpgradeOptimization_ == SingletonReplicaUpgradeOptimizationType::CheckAffinityDuringUpgrade; }
            bool IsInRelaxedScaleoutOptimization() const { return singletonReplicaUpgradeOptimization_ == SingletonReplicaUpgradeOptimizationType::RelaxScaleoutDuringUpgrade; }
            bool IsInSingletonReplicaUpgradeOptimization() const;

            __declspec (property(get = get_ExistsSwapOutLocation)) bool ExistsSwapOutLocation;
            bool get_ExistsSwapOutLocation() const { return primarySwapOutLocation_ != nullptr; }

            __declspec (property(get=get_Service)) ServiceEntry const* Service;
            ServiceEntry const* get_Service() const { return service_; }

            __declspec (property(get=get_StandByLocations)) std::vector<NodeEntry const*> const& StandByLocations;
            std::vector<NodeEntry const*> const& get_StandByLocations() const { return standByLocations_; }

            __declspec (property(get = get_PrimarySwapOutLocation)) NodeEntry const* PrimarySwapOutLocation;
            NodeEntry const* get_PrimarySwapOutLocation() const { return primarySwapOutLocation_; }

            __declspec (property(get = get_PrimaryUpgradeLocations, put = set_PrimaryUpgradeLocations)) NodeEntry const* PrimaryUpgradeLocation;
            NodeEntry const* get_PrimaryUpgradeLocations() const { return primaryUpgradeLocation_; }
            void set_PrimaryUpgradeLocations(NodeEntry const* value) { primaryUpgradeLocation_ = value; }

            __declspec (property(get = get_SecondaryUpgradeLocations)) std::vector<NodeEntry const*> const& SecondaryUpgradeLocations;
            std::vector<NodeEntry const*> const& get_SecondaryUpgradeLocations() const { return secondaryUpgradeLocations_; }

            __declspec (property(get = get_ExistsUpgradeLocation)) bool ExistsUpgradeLocation;
            bool get_ExistsUpgradeLocation() const { return primaryUpgradeLocation_ != nullptr || !secondaryUpgradeLocations_.empty(); }

            __declspec (property(get=get_Order)) int Order;
            int get_Order() const { return order_; }

            __declspec (property(get = get_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }

            __declspec (property(get = get_IsTargetOne)) bool IsTargetOne;
            bool get_IsTargetOne() const { return TargetReplicaSetSize == 1; }

            __declspec (property(get=get_NumberOfExtraReplicas)) int NumberOfExtraReplicas;
            int get_NumberOfExtraReplicas() const { return numberOfExtraReplicas_; }

            __declspec (property(get=get_PrimaryReplica)) PlacementReplica const* PrimaryReplica;
            PlacementReplica const* get_PrimaryReplica() const { return primaryReplica_; }

            __declspec (property(get=get_TotalReplicaCount)) size_t TotalReplicaCount;
            size_t get_TotalReplicaCount() const { return existingReplicas_.size() + newReplicas_.size(); }

            __declspec (property(get = get_ActiveReplicaCount)) size_t ActiveReplicaCount;
            size_t get_ActiveReplicaCount() const { return (primaryReplica_ == nullptr ? 0 : 1) + secondaryReplicas_.size(); }
            
            __declspec (property(get=get_ExistingReplicaCount)) size_t ExistingReplicaCount;
            size_t get_ExistingReplicaCount() const { return existingReplicas_.size(); }

            __declspec (property(get=get_NewReplicaCount)) size_t NewReplicaCount;
            size_t get_NewReplicaCount() const { return newReplicas_.size(); }

            __declspec (property(get=get_SecondaryReplicaCount)) size_t SecondaryReplicaCount;
            size_t get_SecondaryReplicaCount() const { return secondaryReplicas_.size(); }

            __declspec (property(get=get_UpgradeIndex)) size_t UpgradeIndex;
            size_t get_UpgradeIndex() const { return upgradeIndex_; }

            __declspec (property(get = get_WriteQuorumSize)) size_t WriteQuorumSize;
            size_t get_WriteQuorumSize() const { return TargetReplicaSetSize / 2 + 1; }

            __declspec (property(get = get_MaxReplicasPerDomain)) size_t MaxReplicasPerDomain;
            size_t get_MaxReplicasPerDomain() const { return std::max(TargetReplicaSetSize - WriteQuorumSize, size_t(1)); }

            void ConstructReplicas();

            void ForEachReplica(
                std::function<void(PlacementReplica *)> processor,
                bool includeNone = false,
                bool includeDisappearingReplicas = false);
            void ForEachReplica(
                std::function<void(PlacementReplica const *)> processor,
                bool includeNone = false,
                bool includeDisappearingReplicas = false) const;

            void ForEachExistingReplica(
                std::function<void(PlacementReplica *)> processor,
                bool includeNone = false,
                bool includeDisappearingReplicas = false);
            void ForEachExistingReplica(
                std::function<void(PlacementReplica const *)> processor,
                bool includeNone = false,
                bool includeDisappearingReplicas = false) const;

            void ForEachNewReplica(std::function<void(PlacementReplica *)> processor);
            void ForEachNewReplica(std::function<void(PlacementReplica const *)> processor) const;
            void ForEachStandbyReplica(std::function<void(PlacementReplica *)> processor);
            void ForEachStandbyReplica(std::function<void(PlacementReplica const *)> processor) const;

            LoadEntry const& GetReplicaEntry(ReplicaRole::Enum role, bool useNodeId = false, Federation::NodeId const & nodeId = Federation::NodeId()) const;

            uint GetMoveCost(ReplicaRole::Enum role) const;

            void SetUpgradeIndex(size_t upgradeIndex);

            PlacementReplica const* FindMovableReplicaForSingletonReplicaUpgrade() const;

            size_t GetExistingSecondaryCount() const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:

            Common::Guid partitionId_;
            int64 version_;
            bool isInTransition_;

            bool movable_;
            bool inUpgrade_;
            bool isFUInUpgrade_;

            // Is partition singleton replica upgrade
            bool isInSingleReplicaUpgrade_;
            // Type of the upgrade optimization mechanism that should be performed
            SingletonReplicaUpgradeOptimizationType singletonReplicaUpgradeOptimization_;

            ServiceEntry const* service_;
            LoadEntry primary_;
            LoadEntry secondary_;
            std::map<Federation::NodeId, LoadEntry> secondaryMap_;
            LoadEntry secondaryAverage_;

            uint primaryMoveCost_;
            uint secondaryMoveCost_;

            // Number of extra replicas for this partition
            int numberOfExtraReplicas_;

            // the order of the partition when there are dependencies
            int order_;

            // existing replicas, including those roles are None
            std::vector<PlacementReplica *> existingReplicas_;

            // primary replica, including the new replica
            PlacementReplica * primaryReplica_;

            // secondary replicas, including new replicas
            std::vector<PlacementReplica *> secondaryReplicas_;

            std::vector<PlacementReplica *> newReplicas_;

            std::vector<NodeEntry const*> standByLocations_;
            NodeEntry const* primarySwapOutLocation_;
            NodeEntry const* primaryUpgradeLocation_;
            std::vector<NodeEntry const*> secondaryUpgradeLocations_;
            size_t upgradeIndex_;

            std::vector<PlacementReplica *> standbyReplicas_;

            SearcherSettings const & settings_;

            int targetReplicaSetSize_;

            void SetSecondaryLoadMap(std::map<Federation::NodeId, std::vector<uint>> const& inputMap, Federation::NodeId const& node);
        };
    }
}
