// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "../TestApi.Failover.Internal.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReliabilityTestApi;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponentTestApi;
using namespace Infrastructure;

ErrorCode ReliabilityTestApi::ReconfigurationAgentComponentTestApi::IsLeaseExpired(
    Common::ComPointer<IFabricStatefulServicePartition> const& partition,
    __out bool & isLeaseExpired)
{
    ASSERT_IF(partition.GetRawPointer() == nullptr, "Partition cannot be null");
    auto & casted = dynamic_cast<ComStatefulServicePartition &>(*partition.GetRawPointer());

    return casted.Test_IsLeaseExpired(isLeaseExpired);
}

ReconfigurationAgentTestHelper::ReconfigurationAgentTestHelper(IReconfigurationAgent & ra)
: ra_(dynamic_cast<ReconfigurationAgent&>(ra))
{
}

bool ReconfigurationAgentTestHelper::get_IsOpen() const
{
    return ra_.IsOpen;
}

Federation::NodeId ReconfigurationAgentTestHelper::get_NodeId() const
{
    return ra_.NodeId;
}

void ReconfigurationAgentTestHelper::EnableServiceType(ServiceModel::ServiceTypeIdentifier const & serviceTypeId, uint64 sequenceNumber)
{
    // TODO(csu): Add exclusive activation mode
    return ra_.ServiceTypeUpdateProcessorObj.ProcessServiceTypeEnabled(
        serviceTypeId,
        sequenceNumber,
        ServiceModel::ServicePackageActivationContext());
}

void ReconfigurationAgentTestHelper::DisableServiceType(ServiceModel::ServiceTypeIdentifier const & serviceTypeId, uint64 sequenceNumber)
{
    // TODO(csu): Add exclusive activation mode
    return ra_.ServiceTypeUpdateProcessorObj.ProcessServiceTypeDisabled(
        serviceTypeId,
        sequenceNumber,
        ServiceModel::ServicePackageActivationContext());
}

Upgrade::UpgradeStateName::Enum ReconfigurationAgentTestHelper::GetUpgradeStateName(std::wstring const & identifier)
{
    return ra_.UpgradeMessageProcessorObj.Test_GetUpgradeState(identifier);
}

vector<FailoverUnitSnapshot> ReconfigurationAgentTestHelper::SnapshotLfum()
{
    if (!ra_.IsOpen)
    {
        return vector<FailoverUnitSnapshot>();
    }

    vector<FailoverUnitSnapshot> rv;

    auto ftIds = ra_.LocalFailoverUnitMapObj.GetAllFailoverUnitEntries(false);
    ExecuteUnderReadLockOverMap<FailoverUnit>(
        ra_.LocalFailoverUnitMapObj,
        [&](ReadOnlyLockedFailoverUnitPtr & lockedFT)
        {
            if (lockedFT.IsEntityDeleted || lockedFT.get_Current() == nullptr)
            {
                return;
            }

            auto copy = make_shared<FailoverUnit>(*lockedFT);
            rv.push_back(FailoverUnitSnapshot(copy));
        });
    return rv;
}

FailoverUnitSnapshotUPtr ReconfigurationAgentTestHelper::FindFTByPartitionId(Guid const & fuid)
{
    return FindFTByPartitionId(FailoverUnitId(fuid));
}

FailoverUnitSnapshotUPtr ReconfigurationAgentTestHelper::FindFTByPartitionId(FailoverUnitId const & fuid)
{
    auto lfum = SnapshotLfum();

    auto ft = find_if(lfum.begin(), lfum.end(), [fuid](FailoverUnitSnapshot const & inner)
    {
        return inner.FailoverUnitDescription.FailoverUnitId == fuid;
    });

    if (ft == lfum.end())
    {
        return FailoverUnitSnapshotUPtr();
    }

    return make_unique<FailoverUnitSnapshot>(*ft);
}

void ReconfigurationAgentTestHelper::GetNodeActivationStatus(bool isFmm, bool & isActivatedOut, int64 & currentSequenceNumberOut)
{
    auto info = ra_.NodeStateObj.GetNodeDeactivationState(FailoverManagerId(isFmm)).DeactivationInfo;
    isActivatedOut = info.IsActivated;
    currentSequenceNumberOut = info.SequenceNumber;
}

bool ReconfigurationAgentTestHelper::IsLastReplicaUpAcknowledged(bool isFmm)
{
    return ra_.NodeStateObj.GetPendingReplicaUploadStateProcessor(FailoverManagerId(isFmm)).IsComplete;
}

FailoverUnitSnapshot::FailoverUnitSnapshot(FailoverUnitSPtr const & ft)
: ft_(ft)
{
}

ServiceDescription const & FailoverUnitSnapshot::get_ServiceDescription() const
{
    return ft_->ServiceDescription;
}

FailoverUnitDescription const & FailoverUnitSnapshot::get_FailoverUnitDescription() const
{
    return ft_->FailoverUnitDescription;
}

FailoverUnitStates::Enum FailoverUnitSnapshot::get_State() const
{
    return ft_->State;
}

ReplicaRole::Enum FailoverUnitSnapshot::get_LocalReplicaRole() const
{
    return ft_->LocalReplicaRole;
}

bool FailoverUnitSnapshot::get_LocalReplicaOpen() const
{
    return ft_->LocalReplicaOpen;
}

FMMessageStage::Enum FailoverUnitSnapshot::get_FMMessageStage() const
{
    return ft_->FMMessageStage;
}

bool FailoverUnitSnapshot::get_IsServiceDescriptionUpdatePending() const
{
    return ft_->LocalReplicaServiceDescriptionUpdatePending.IsSet;
}

Epoch const & FailoverUnitSnapshot::get_CurrentConfigurationEpoch() const
{
    return ft_->CurrentConfigurationEpoch;
}

Hosting2::ServiceTypeRegistrationSPtr const & FailoverUnitSnapshot::get_ServiceTypeRegistration() const
{
    return ft_->ServiceTypeRegistration;
}

DateTime FailoverUnitSnapshot::get_LastUpdatedTime() const
{
    return ft_->LastUpdatedTime;
}

bool FailoverUnitSnapshot::get_LocalReplicaDeleted() const
{
    return ft_->LocalReplicaDeleted;
}

FailoverUnitReconfigurationStage::Enum FailoverUnitSnapshot::get_ReconfigurationStage() const
{
    return ft_->ReconfigurationStage;
}

bool FailoverUnitSnapshot::get_LocalReplicaDroppedReplyPending() const
{
    return ft_->FMMessageStage == FMMessageStage::ReplicaDropped;
}

bool FailoverUnitSnapshot::get_LocalReplicaClosePending() const
{
    return ft_->LocalReplicaClosePending.IsSet;
}

bool FailoverUnitSnapshot::get_IsDeactivationInfoDropped() const
{
    return ft_->DeactivationInfo.IsDropped;
}

bool FailoverUnitSnapshot::get_IsReconfiguring() const
{
    return ft_->IsReconfiguring;
}

ReplicaSnapshotUPtr FailoverUnitSnapshot::GetLocalReplica() const
{
    if (ft_->IsOpen)
    {
        return make_unique<ReplicaSnapshot>(ft_->LocalReplica);
    }

    return ReplicaSnapshotUPtr();
}

ReplicaSnapshotUPtr FailoverUnitSnapshot::GetReplicaOnNode(Federation::NodeId const & nodeId) const
{
    auto replicas = GetReplicas();
    auto iter = find_if(replicas.begin(), replicas.end(), [nodeId] (ReplicaSnapshot const & inner)
    {
        return inner.FederationNodeId == nodeId;
    });

    if (iter != replicas.end())
    {
        return make_unique<ReplicaSnapshot>(*iter);
    }

    return ReplicaSnapshotUPtr();
}

vector<ReplicaSnapshot> FailoverUnitSnapshot::GetReplicas() const
{
    auto replicas = ft_->Replicas;
    return ConstructSnapshotObjectList<Replica, ReplicaSnapshot>(replicas);
}

void FailoverUnitSnapshot::WriteTo(TextWriter& writer, FormatOptions const & options) const
{
    ft_->WriteTo(writer, options);
}

ReplicaSnapshot::ReplicaSnapshot(Replica & replica)
: replica_(make_shared<Replica>(replica))
{
}

bool ReplicaSnapshot::get_IsUp() const
{
    return replica_->IsUp;
}

Federation::NodeId ReplicaSnapshot::get_FederationNodeId() const
{
    return replica_->FederationNodeId;
}

ServiceModel::ServicePackageVersionInstance const & ReplicaSnapshot::get_PackageVersionInstance() const
{
    return replica_->PackageVersionInstance;
}

bool ReplicaSnapshot::get_IsOffline() const
{
    return replica_->IsOffline;
}

bool ReplicaSnapshot::get_IsStandBy() const 
{
    return replica_->IsStandBy;
}

bool ReplicaSnapshot::get_IsInPreviousConfiguration() const
{
    return replica_->IsInPreviousConfiguration;
}

ReplicaMessageStage::Enum ReplicaSnapshot::get_MessageStage() const
{
    return replica_->MessageStage;
}

int64 ReplicaSnapshot::get_LastAcknowledgedLSN() const
{
    return replica_->GetLastAcknowledgedLSN();
}
 
bool ReplicaSnapshot::get_IsInCurrentConfiguration() const
{
    return replica_->IsInCurrentConfiguration;
}

ReconfigurationAgentComponent::ReplicaStates::Enum ReplicaSnapshot::get_State() const
{
    return replica_->State;
}

int64 ReplicaSnapshot::get_ReplicaId() const
{
    return replica_->ReplicaId;
}

int64 ReplicaSnapshot::get_InstanceId() const
{
    return replica_->InstanceId;
}

Reliability::ReplicaDeactivationInfo ReplicaSnapshot::get_ReplicaDeactivationInfo() const
{
    return replica_->ReplicaDeactivationInfo;
}
