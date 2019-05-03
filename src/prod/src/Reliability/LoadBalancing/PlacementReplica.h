// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "ReplicaDescription.h"
#include "NodeEntry.h"
#include "SearcherSettings.h"
#include "DynamicNodeLoadSet.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class PartitionEntry;
        class NodeEntry;
        class LoadEntry;
        class Metric;
        class LoadBalancingDomainEntry;
        class PlacementReplica
        {
            friend class PartitionEntry;

            public:
                // constructor for a new replica
                PlacementReplica(size_t index,
                    ReplicaRole::Enum role,
                    bool canBeThrottled);

                // constructor for an existing replica
                PlacementReplica(
                    size_t index,
                    ReplicaRole::Enum role,
                    bool isMovable,
                    bool isDropable,
                    bool isInTransition,
                    NodeEntry const* nodeEntry,
                    bool isMoveInProgress,
                    bool isToBeDropped,
                    bool isInUpgrade,
                    bool isSingletonReplicaMovableDuringUpgrade,
                    bool canBeThrottled);

                __declspec (property(get=get_ReplicaIndex)) size_t Index;
                size_t get_ReplicaIndex() const { return index_; }

                __declspec (property(get=get_PartitionEntry)) PartitionEntry const* Partition;
                PartitionEntry const* get_PartitionEntry() const { return partitionEntry_; }

                __declspec (property(get=get_Role, put=set_Role)) ReplicaRole::Enum Role;
                ReplicaRole::Enum get_Role() const { return role_; }
                void set_Role(ReplicaRole::Enum role) { role_ = role; }

                __declspec (property(get=get_IsPrimary)) bool IsPrimary;
                bool get_IsPrimary() const { return role_ == ReplicaRole::Primary; }

                __declspec (property(get=get_IsSecondary)) bool IsSecondary;
                bool get_IsSecondary() const { return role_ == ReplicaRole::Secondary; }

                __declspec (property(get=get_IsStandBy)) bool IsStandBy;
                bool get_IsStandBy() const { return role_ == ReplicaRole::StandBy; }

                __declspec (property(get=get_IsNone)) bool IsNone;
                bool get_IsNone() const { return role_ == ReplicaRole::None; }

                __declspec (property(get=get_NewReplicaIndex)) size_t NewReplicaIndex;
                size_t get_NewReplicaIndex() const { return newReplicaIndex_; }

                __declspec (property(get = get_IsMovable, put = set_IsMovable)) bool IsMovable;
                bool get_IsMovable() const { return isMovable_; }
                void set_IsMovable(bool isMovable) { isMovable_ = isMovable; }

                __declspec (property(get = get_IsDropable)) bool IsDropable;
                bool get_IsDropable() const { return isDropable_; }

                __declspec (property(get = get_IsInTransition)) bool IsInTransition;
                bool get_IsInTransition() const { return isInTransition_; }

                __declspec (property(get = get_IsOnDeactivatedNode)) bool IsOnDeactivatedNode;
                bool get_IsOnDeactivatedNode() const { return nodeEntry_->IsDeactivated; }

                // whether a replica is to be created
                __declspec (property(get=get_IsNew)) bool IsNew;
                bool get_IsNew() const { return nodeEntry_ == nullptr; }

                __declspec (property(get=get_NodeEntry)) NodeEntry const* Node;
                NodeEntry const* get_NodeEntry() const { return nodeEntry_; }

                __declspec (property(get = get_IsMoveInProgress)) bool IsMoveInProgress;
                bool get_IsMoveInProgress() const { return isMoveInProgress_; }

                __declspec (property(get = get_IsToBeDropped)) bool IsToBeDropped;
                bool get_IsToBeDropped() const { return isToBeDropped_; }

                __declspec (property(get = get_ShouldDisappear)) bool ShouldDisappear;
                bool get_ShouldDisappear() const { return isMoveInProgress_ || isToBeDropped_; }

                __declspec (property(get=get_ReplicaEntry)) LoadEntry const* ReplicaEntry;
                LoadEntry const* get_ReplicaEntry() const;

                __declspec (property(get = get_IsSingletonReplicaMovableDuringUpgrade)) bool IsSingletonReplicaMovableDuringUpgrade;
                bool get_IsSingletonReplicaMovableDuringUpgrade() const { return isSingletonReplicaMovableDuringUpgrade_; }

                __declspec (property(get = get_IsInUpgrade)) bool IsInUpgrade;
                bool get_IsInUpgrade() const { return isInUpgrade_; }

                __declspec (property(get=get_CanBeThrottled)) bool CanBeThrottled;
                bool get_CanBeThrottled() const { return canBeThrottled_; }

                std::wstring ReplicaHash(bool alreadyExists = false) const;

                void SetPartitionEntry(PartitionEntry const* partitionEntry);
                void SetNewReplicaIndex(size_t newReplicaIndex);

                // Executes specified processor for every replica's metric which node load is
                // over average for balancing and under average for defrag metric.
                void ForEachBeneficialMetric(std::function<bool(size_t, bool forDefrag)> processor) const;

                // Executes specified processor for every weighted replica's metric
                void ForEachWeightedDefragMetric(std::function<bool(size_t)> processor, SearcherSettings const& settings) const;

                bool UseSecondaryLoad() const
                {
                    return role_ == ReplicaRole::Secondary || role_ == ReplicaRole::StandBy;
                }

                static int64 GetReplicaLoadValue(
                    PartitionEntry const* partition,
                    LoadEntry const& replicaLocalLoad,
                    size_t globalMetricIndex,
                    size_t globalMetricIndexStart);

                // Replica is movable if it is primary or secondary, and it is ready and up,
                // and specific set of flags is not set:
                // ToBeDroppedByPLB('R'), ToBePromoted('P'), PendingRemove('N'), Deleted('Z'),
                // PrimaryToBePlaced('J'), ReplicaToBePlaced('K'), MoveInProgress('V')
                static bool CheckIfSingletonReplicaIsMovableDuringUpgrade(ReplicaDescription const& replica);

                void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

                void WriteToEtw(uint16 contextSequenceId) const;

        private:
                PartitionEntry const* partitionEntry_;
                ReplicaRole::Enum role_;
                size_t newReplicaIndex_;
                bool isMovable_;
                bool isDropable_;
                bool isInTransition_;
                NodeEntry const* nodeEntry_;
                bool isMoveInProgress_;
                bool isToBeDropped_;
                size_t index_;

                // Whether primary replica is in upgrade
                // i.e. IsPrimaryToBeSwappedOut('I')
                bool isInUpgrade_;

                // Whether the replica is movable during singleton replica upgrade
                bool isSingletonReplicaMovableDuringUpgrade_;

                // True if this replica can be throttled.
                bool canBeThrottled_;
        };
    }
}
