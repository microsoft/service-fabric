// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "ReplicaDescription.h"
#include "PLBFailoverUnitDescriptionFlags.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class FailoverUnitDescription
        {
            DENY_COPY_ASSIGNMENT(FailoverUnitDescription);

        public:
            // In case of auto-scaling, we will pass target replica set size.
            // If there is no scaling policy it will be picked up from the service description.
            FailoverUnitDescription(
                Common::Guid fuId,
                std::wstring && serviceName,
                int64 version,
                std::vector<ReplicaDescription> && replicas,
                int replicaDifference,
                Reliability::FailoverUnitFlags::Flags flags = FailoverUnitFlags::Flags::None,
                bool isReconfigurationInProgress = false,
                bool isInQuorumLost = false,
                int targetReplicaSetSize = 0);

            // Creates a FailoverUnitDescription which represents a failover unit is deleted
            FailoverUnitDescription(Common::Guid fuId, std::wstring && serviceName, uint64 serviceId = 0);

            FailoverUnitDescription(FailoverUnitDescription const & other);

            FailoverUnitDescription(FailoverUnitDescription && other);

            FailoverUnitDescription & operator = (FailoverUnitDescription && other);

            __declspec (property(get=get_FailoverUnitId)) Common::Guid const& FUId;
            Common::Guid get_FailoverUnitId() const { return fuId_; }

            __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;
            std::wstring const& get_ServiceName() const { return serviceName_; }

            __declspec (property(get=get_HasPlacement)) bool HasPlacement;
            bool get_HasPlacement() const { return ReplicaDifference > 0; }

            __declspec (property(get=get_IsInTransition)) bool IsInTransition;
            bool get_IsInTransition() const { return plbFlags_.IsInTransition(); }

            __declspec (property(get=get_IsInUpgrade)) bool IsInUpgrade;
            bool get_IsInUpgrade() const { return flags_.IsUpgrading(); }

            __declspec (property(get = get_IsInQuorumLost)) bool IsInQuorumLost;
            bool get_IsInQuorumLost() const { return plbFlags_.IsInQuorumLost(); }

            __declspec (property(get = get_HasIsInUpgradeReplica)) bool HasInUpgradeReplica;
            bool get_HasIsInUpgradeReplica() const { return plbFlags_.HasInUpgradeReplica(); }

            __declspec (property(get=get_MovableReplicaCount)) size_t MovableReplicaCount;
            size_t get_MovableReplicaCount() const;

            __declspec (property(get=get_InBuildReplicaCount)) size_t InBuildReplicaCount;
            size_t get_InBuildReplicaCount() const;

            __declspec (property(get=get_SwapPrimaryReplicaCount)) size_t SwapPrimaryReplicaCount;
            size_t get_SwapPrimaryReplicaCount() const { return flags_.IsSwappingPrimary() || plbFlags_.HasToBePromotedReplica() ? 1 : 0; }

            __declspec (property(get=get_Version)) int64 Version;
            int64 get_Version() const { return version_; }

            __declspec (property(get=get_Replicas)) std::vector<ReplicaDescription> const& Replicas;
            std::vector<ReplicaDescription> const& get_Replicas() const { return replicas_; }

            __declspec (property(get = get_PrimaryReplicaIndex)) int PrimaryReplicaIndex;
            int get_PrimaryReplicaIndex() const { return primaryReplicaIndex_; }

            __declspec (property(get = get_PrimaryReplica)) const ReplicaDescription * PrimaryReplica;
            const ReplicaDescription * get_PrimaryReplica() const;

            __declspec (property(get=get_ReplicaDifference, put = set_ReplicaDifference)) int ReplicaDifference;
            int get_ReplicaDifference() const { return replicaDifference_; }
            void set_ReplicaDifference(int replicaDifference) { replicaDifference_ = replicaDifference; }

            __declspec (property(get = get_TargetReplicaSetSize, put = set_TargetReplicaSetSize)) int TargetReplicaSetSize;
            int get_TargetReplicaSetSize() const { return targetReplicaSetSize_; }
            void set_TargetReplicaSetSize(int targetReplicaSetSize) { targetReplicaSetSize_ = targetReplicaSetSize; }

            __declspec (property(get=get_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return plbFlags_.IsDeleted(); }

            __declspec (property(get = get_ServiceId, put = set_ServiceId)) uint64 ServiceId;
            uint64 get_ServiceId() const { return serviceId_; }
            void set_ServiceId(uint64 id) { serviceId_ = id; }

            bool HasAnyReplicaOnNode(Federation::NodeId nodeId) const;

            bool HasReplicaWithRoleOnNode(Federation::NodeId nodeId, ReplicaRole::Enum role) const;

            bool HasReplicaWithSecondaryLoadOnNode(Federation::NodeId nodeId) const;

            void ForEachReplica(std::function<void(ReplicaDescription const&)> processor) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            void Initialize();

            Common::Guid fuId_;
            std::wstring serviceName_;
            int64 version_;
            std::vector<ReplicaDescription> replicas_;
            int primaryReplicaIndex_;
            int replicaDifference_;
            uint64 serviceId_;
            int targetReplicaSetSize_;

            Reliability::FailoverUnitFlags::Flags flags_;
            PLBFailoverUnitDescriptionFlags::Flags plbFlags_;
        };
    }
}
