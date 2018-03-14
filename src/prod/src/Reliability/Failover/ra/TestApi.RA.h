// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Failover/common/FailoverConfig.h"
#include "Reliability/Failover/ra/FailoverUnitStates.h"
#include "Reliability/Failover/common/ReplicaRole.h"
#include "Reliability/Failover/common/ReplicaStates.h"
#include "Reliability/Failover/ra/ReplicaStates.h"
#include "Reliability/Failover/ra/FMMessageStage.h"
#include "Reliability/Failover/ra/FailoverUnitReconfigurationStage.h"
#include "Reliability/Failover/ra/ReplicaMessageStage.h"
#include "Reliability/Failover/ra/Upgrade.UpgradeStateName.h"
#include "Reliability/Failover/TestApi.FailoverPointers.h"
#include "Reliability/Failover/common/ReplicaDeactivationInfo.h"

namespace ReliabilityTestApi
{
    namespace ReconfigurationAgentComponentTestApi
    {
        Common::ErrorCode IsLeaseExpired(Common::ComPointer<IFabricStatefulServicePartition> const& partition, __out bool & isLeaseExpired);

        class ReconfigurationAgentTestHelper
        {
            DENY_COPY(ReconfigurationAgentTestHelper);

        public:
            ReconfigurationAgentTestHelper(Reliability::ReconfigurationAgentComponent::IReconfigurationAgent &);

            __declspec(property(get=get_IsOpen)) bool IsOpen;
            bool get_IsOpen() const;

            __declspec(property(get=get_NodeId)) Federation::NodeId NodeId;
            Federation::NodeId get_NodeId() const;

            void EnableServiceType(ServiceModel::ServiceTypeIdentifier const & serviceTypeId, uint64 sequenceNumber);
            void DisableServiceType(ServiceModel::ServiceTypeIdentifier const & serviceTypeId, uint64 sequenceNumber);

            FailoverUnitSnapshotUPtr FindFTByPartitionId(Common::Guid const & fuid);
            FailoverUnitSnapshotUPtr FindFTByPartitionId(Reliability::FailoverUnitId const & fuid);
            std::vector<FailoverUnitSnapshot> SnapshotLfum();

            void GetNodeActivationStatus(bool isFmm, bool & isActivatedOut, int64 & currentSequenceNumberOut);
            bool IsLastReplicaUpAcknowledged(bool isFmm);

            Reliability::ReconfigurationAgentComponent::Upgrade::UpgradeStateName::Enum GetUpgradeStateName(std::wstring const & identifier);
        private:
            Reliability::ReconfigurationAgentComponent::ReconfigurationAgent & ra_;
        };

        class FailoverUnitSnapshot
        {
        public:
            FailoverUnitSnapshot(Reliability::ReconfigurationAgentComponent::FailoverUnitSPtr const &);

            __declspec(property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
            Reliability::ServiceDescription const & get_ServiceDescription() const;
            
            __declspec(property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const;

            __declspec(property(get=get_State)) Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Enum State;
            Reliability::ReconfigurationAgentComponent::FailoverUnitStates::Enum get_State() const;

            __declspec(property(get=get_LocalReplicaRole)) Reliability::ReplicaRole::Enum LocalReplicaRole;
            Reliability::ReplicaRole::Enum get_LocalReplicaRole() const;

            __declspec(property(get=get_LocalReplicaOpen)) bool LocalReplicaOpen;
            bool get_LocalReplicaOpen() const;

            __declspec(property(get=get_IsServiceDescriptionUpdatePending)) bool IsServiceDescriptionUpdatePending;
            bool get_IsServiceDescriptionUpdatePending() const;

            __declspec(property(get = get_FMMessageStage)) Reliability::ReconfigurationAgentComponent::FMMessageStage::Enum FMMessageStage;
            Reliability::ReconfigurationAgentComponent::FMMessageStage::Enum get_FMMessageStage() const;

            __declspec(property(get=get_CurrentConfigurationEpoch)) Reliability::Epoch const & CurrentConfigurationEpoch;
            Reliability::Epoch const & get_CurrentConfigurationEpoch() const;

            __declspec(property(get=get_ServiceTypeRegistration)) Hosting2::ServiceTypeRegistrationSPtr const & ServiceTypeRegistration;
            Hosting2::ServiceTypeRegistrationSPtr const & get_ServiceTypeRegistration() const;

            __declspec(property(get=get_LastUpdatedTime)) Common::DateTime LastUpdatedTime;
            Common::DateTime get_LastUpdatedTime() const;

            __declspec(property(get=get_LocalReplicaDeleted)) bool LocalReplicaDeleted;
            bool get_LocalReplicaDeleted() const;

            __declspec(property(get=get_ReconfigurationStage)) Reliability::ReconfigurationAgentComponent::FailoverUnitReconfigurationStage::Enum ReconfigurationStage;
            Reliability::ReconfigurationAgentComponent::FailoverUnitReconfigurationStage::Enum get_ReconfigurationStage() const;

            __declspec(property(get=get_LocalReplicaDroppedReplyPending)) bool LocalReplicaDroppedReplyPending;
            bool get_LocalReplicaDroppedReplyPending() const;

            __declspec(property(get=get_LocalReplicaClosePending)) bool LocalReplicaClosePending;
            bool get_LocalReplicaClosePending() const;

            __declspec(property(get = get_IsDeactivationInfoDropped)) bool IsDeactivationInfoDropped;
            bool get_IsDeactivationInfoDropped() const;

            __declspec (property(get=get_IsReconfiguring)) bool IsReconfiguring;
            bool get_IsReconfiguring() const;

            ReplicaSnapshotUPtr GetLocalReplica() const;
            ReplicaSnapshotUPtr GetReplicaOnNode(Federation::NodeId const & nodeId) const;
            std::vector<ReplicaSnapshot> GetReplicas() const;

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

        private:
            Reliability::ReconfigurationAgentComponent::FailoverUnitSPtr ft_;
        };

        class ReplicaSnapshot
        {
        public:
            ReplicaSnapshot(Reliability::ReconfigurationAgentComponent::Replica &);

            __declspec(property(get=get_IsUp)) bool IsUp;
            bool get_IsUp() const;

            __declspec(property(get=get_FederationNodeId)) Federation::NodeId FederationNodeId;
            Federation::NodeId get_FederationNodeId() const;

            __declspec(property(get=get_PackageVersionInstance)) ServiceModel::ServicePackageVersionInstance const & PackageVersionInstance;
            ServiceModel::ServicePackageVersionInstance const & get_PackageVersionInstance() const;

            __declspec(property(get=get_IsOffline)) bool IsOffline;
            bool get_IsOffline() const;

            __declspec(property(get=get_IsStandBy)) bool IsStandBy;
            bool get_IsStandBy() const ;

            __declspec(property(get=get_IsInPreviousConfiguration)) bool IsInPreviousConfiguration;
            bool get_IsInPreviousConfiguration() const;

            __declspec(property(get=get_MessageStage)) Reliability::ReconfigurationAgentComponent::ReplicaMessageStage::Enum MessageStage;
            Reliability::ReconfigurationAgentComponent::ReplicaMessageStage::Enum get_MessageStage() const;

            __declspec(property(get=get_LastAcknowledgedLSN)) int64 LastAcknowledgedLSN;
            int64 get_LastAcknowledgedLSN() const;
            
            __declspec(property(get=get_IsInCurrentConfiguration)) bool IsInCurrentConfiguration;
            bool get_IsInCurrentConfiguration() const;

            __declspec(property(get=get_State)) Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum State;
            Reliability::ReconfigurationAgentComponent::ReplicaStates::Enum get_State() const;

            __declspec(property(get=get_ReplicaId)) int64 ReplicaId;
            int64 get_ReplicaId() const;

            __declspec(property(get=get_InstanceId)) int64 InstanceId;
            int64 get_InstanceId() const;

            __declspec(property(get = get_ReplicaDeactivationInfo)) Reliability::ReplicaDeactivationInfo ReplicaDeactivationInfo;
            Reliability::ReplicaDeactivationInfo get_ReplicaDeactivationInfo() const;

        private:
            Reliability::ReconfigurationAgentComponent::ReplicaSPtr replica_;
        };
    }
}

