// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "Diagnostics.FailoverUnitEventData.h"
#include "Diagnostics.TraceEventStateMachineAction.h"

using namespace std;

using namespace Common;
using namespace Federation;
using namespace Hosting2;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Hosting;
using namespace ServiceModel;
using namespace Infrastructure;
using namespace Health;
using namespace ReconfigurationAgentComponent::Communication;
using namespace Diagnostics;

namespace
{
    Reliability::FailoverUnitId EmptyFailoverUnitId(Guid::Empty());
}

FailoverUnit::FailoverUnit() :
    messageRetryActiveFlag_(EntitySetName::ReconfigurationMessageRetry),
    cleanupPendingFlag_(EntitySetName::StateCleanup),
    localReplicaClosePending_(EntitySetName::ReplicaCloseMessageRetry),
    localReplicaOpenPending_(EntitySetName::ReplicaOpenMessageRetry),
    localReplicaServiceDescriptionUpdatePendingFlag_(EntitySetName::UpdateServiceDescriptionMessageRetry),
    tracer_(*this),
    reconfigState_(&failoverUnitDesc_, &serviceDesc_),
    replicaStore_(&replicas_),
    serviceTypeRegistration_(failoverUnitDesc_, serviceDesc_),
    fmMessageState_(&localReplicaDeleted_, *FailoverManagerId::Fmm, Infrastructure::IRetryPolicySPtr()), // will be fixed up in final construct
    replicaUploadState_(&fmMessageState_, *FailoverManagerId::Fmm), // will be fixed up in final construct
    endpointPublishState_(nullptr, &reconfigState_, &fmMessageState_), // will be fixed up in final construct
    reconfigHealthState_(&FailoverUnitId, &serviceDesc_, &localReplicaId_, &localReplicaInstanceId_),
    reconfigHealthDescriptorFactory_()
{
    Initialize();
}

// This constructor is called when the FT is created to be inserted into the LFUM
FailoverUnit::FailoverUnit(
    FailoverConfig const & config,
    Reliability::FailoverUnitId const & ftId,
    Reliability::ConsistencyUnitDescription const & cuid,
    Reliability::ServiceDescription const & serviceDescription) : 
    failoverUnitDesc_(ftId, cuid),
    serviceDesc_(serviceDescription),
    messageRetryActiveFlag_(EntitySetName::ReconfigurationMessageRetry),
    cleanupPendingFlag_(EntitySetName::StateCleanup),
    localReplicaClosePending_(EntitySetName::ReplicaCloseMessageRetry),
    localReplicaOpenPending_(EntitySetName::ReplicaOpenMessageRetry),
    localReplicaServiceDescriptionUpdatePendingFlag_(EntitySetName::UpdateServiceDescriptionMessageRetry),
    tracer_(*this),
    reconfigState_(&failoverUnitDesc_, &serviceDesc_),
    replicaStore_(&replicas_),
    serviceTypeRegistration_(failoverUnitDesc_, serviceDesc_),
    fmMessageState_(&localReplicaDeleted_, *FailoverManagerId::Fmm, Infrastructure::IRetryPolicySPtr()), // will be fixed up in final construct
    replicaUploadState_(&fmMessageState_, *FailoverManagerId::Fmm), // will be fixed up in final construct
    endpointPublishState_(nullptr, &reconfigState_, &fmMessageState_), // will be fixed up in final construct
    reconfigHealthState_(&FailoverUnitId, &serviceDesc_, &localReplicaId_, &localReplicaInstanceId_),
    reconfigHealthDescriptorFactory_()
{
    Initialize();

    FinalConstruct(config);
}

FailoverUnit::FailoverUnit(FailoverUnit const & other) : 
    failoverUnitDesc_(other.failoverUnitDesc_),
    lastStableEpoch_(other.lastStableEpoch_),
    serviceDesc_(other.serviceDesc_),
    icEpoch_(other.icEpoch_),
    dataLossVersionToReport_(other.dataLossVersionToReport_),
    localReplicaId_(other.localReplicaId_),
    localReplicaInstanceId_(other.localReplicaInstanceId_),
    senderNode_(other.senderNode_),
    state_(other.state_),
    changeConfigFailoverUnitDesc_(other.changeConfigFailoverUnitDesc_),
    changeConfigReplicaDescList_(other.changeConfigReplicaDescList_),
    localReplicaClosePending_(other.localReplicaClosePending_),
    localReplicaOpenPending_(other.localReplicaOpenPending_),
    localReplicaServiceDescriptionUpdatePendingFlag_(other.localReplicaServiceDescriptionUpdatePendingFlag_),
    localReplicaOpen_(other.localReplicaOpen_),
    localReplicaDeleted_(other.localReplicaDeleted_),
    updateReplicatorConfiguration_(other.updateReplicatorConfiguration_),
    lastUpdated_(other.lastUpdated_),
    messageRetryActiveFlag_(other.messageRetryActiveFlag_),
    cleanupPendingFlag_(other.cleanupPendingFlag_),
    retryableErrorState_(other.retryableErrorState_),
    tracer_(*this, other.tracer_),
    replicas_(other.replicas_),
    replicaCloseMode_(other.replicaCloseMode_),
    deactivationInfo_(other.deactivationInfo_),
    reconfigState_(&failoverUnitDesc_, &serviceDesc_, other.reconfigState_),
    replicaStore_(&replicas_),
    serviceTypeRegistration_(failoverUnitDesc_, serviceDesc_, other.serviceTypeRegistration_),
    fmMessageState_(&localReplicaDeleted_, other.fmMessageState_),
    replicaUploadState_(&fmMessageState_, other.replicaUploadState_),
    endpointPublishState_(&reconfigState_, &fmMessageState_, other.endpointPublishState_),
    reconfigHealthState_(&FailoverUnitId, &serviceDesc_, &localReplicaId_, &localReplicaInstanceId_, other.reconfigHealthState_),
    reconfigHealthDescriptorFactory_(other.reconfigHealthDescriptorFactory_)
{
}

void FailoverUnit::Initialize()
{
    // Called when an FT object is created from the constructor
    // This must default initialize every data member
    // NOTE: This is called before deserialization so the data must be filled in
    // The FT constructor on a new create will assign the FTDesc and the ServiceDesc
    // Anything not being updated here is fixed up correctly in ResetNonFlagState
    ResetNonFlagState();

    tracer_.Initialize();
    lastUpdated_ = DateTime::Zero;

    state_ = FailoverUnitStates::Closed;
    localReplicaId_ = -1;
    localReplicaInstanceId_ = -1;
    reconfigState_.Reset();
    localReplicaDeleted_ = false;
    icEpoch_ = Epoch::InvalidEpoch();
    lastUpdated_ = DateTime::Now();
    lastStableEpoch_ = Epoch::InvalidEpoch();
    deactivationInfo_ = ReplicaDeactivationInfo::Dropped;
}

void FailoverUnit::FinalConstruct(FailoverConfig const & config)
{
    /*
        Called after the FT object's persisted state has been set
        This is used to initialize any in-memory state which is dependant on persisted state or other objects

        There are three scenarios:
        1. When an FT is constructed to be inserted into the LFUM as part of normal processing
        this is called from the constructor after state has been initialized
        2. When an FT is loaded from disk when the node opens, first the empty constructor is called
        and then the state is deserialized. WHen it is inserted, UpdateStateOnLFUMLoad calls this method
        3. when an FT is created in test code then this is called after the ft id, service desc and such stuff is loaded
    */
    auto fmMessageRetryPolicy = make_shared<Infrastructure::LinearRetryPolicy>(config.PerReplicaMinimumIntervalBetweenMessageToFMEntry);
    fmMessageState_ = FMMessageState(&localReplicaDeleted_, Owner, fmMessageRetryPolicy);
    replicaUploadState_ = ReplicaUploadState(&fmMessageState_, Owner);
    endpointPublishState_ = EndpointPublishState(&config.MaxWaitBeforePublishEndpointDurationEntry, &reconfigState_, &fmMessageState_);
}

bool FailoverUnit::get_AllowBuildDuringSwapPrimary() const
{
    return ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote;
}

RAReplicaOpenMode::Enum FailoverUnit::get_OpenMode() const
{
    if (!localReplicaOpenPending_.IsSet)
    {
        return RAReplicaOpenMode::None;
    }

    switch (LocalReplica.State)
    {
    case ReplicaStates::InCreate:
        return localReplicaOpen_ ? RAReplicaOpenMode::ChangeRole : RAReplicaOpenMode::Open;
    case ReplicaStates::StandBy:
        return RAReplicaOpenMode::Reopen;
    default:
        return RAReplicaOpenMode::None;
    }
}

Replica const & FailoverUnit::GetReplica(Federation::NodeId id) const
{
    return replicaStore_.GetReplica(id);
}

Replica & FailoverUnit::GetReplica(Federation::NodeId id)
{
    return replicaStore_.GetReplica(id);
}

// Start creating local replica
Replica & FailoverUnit::StartCreateLocalReplica(
    Reliability::FailoverUnitDescription const & fuDesc,
    Reliability::ReplicaDescription const & replicaDesc,
    Reliability::ServiceDescription const & serviceDesc,
    ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
    Reliability::ReplicaRole::Enum localReplicaRole,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo,
    bool isRebuild,
    FailoverUnitEntityExecutionContext & executionContext)
{
    auto & queue = executionContext.Queue;
    TESTASSERT_IF(LocalReplicaClosePending.IsSet, "Replica cannot be closing and creating at the same time {0}", *this);

    localReplicaDeleted_ = false;
    fmMessageState_.Reset(executionContext.StateUpdateContextObj);
    cleanupPendingFlag_.SetValue(false, queue);

    if (state_ == FailoverUnitStates::Open &&
        PreviousConfigurationEpoch != Epoch::InvalidEpoch() &&
        replicaDesc.CurrentConfigurationRole == ReplicaRole::Secondary)
    {
        TESTASSERT_IF(!LocalReplicaOpen, "During add replica must be open already {0}", *this);
        Replica & localReplica = LocalReplica;

        localReplica.CurrentConfigurationRole = ReplicaRole::Idle;
        localReplica.State = ReplicaStates::InCreate;

        InternalUpdateLocalReplicaPackageVersionInstance(packageVersionInstance);

        localReplicaOpenPending_.SetValue(true, queue);
        retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaReopen);

        CurrentConfigurationEpoch = fuDesc.CurrentConfigurationEpoch;

        return localReplica;
    }

    // Reset IC epoch
    IntermediateConfigurationEpoch = Epoch::InvalidEpoch();

    InternalSetFailoverUnitDescription(fuDesc);
    InternalSetServiceDescription(serviceDesc);

    if (PreviousConfigurationEpoch != Epoch::InvalidEpoch() &&
        replicaDesc.CurrentConfigurationRole != ReplicaRole::Primary)
    {
        PreviousConfigurationEpoch = Epoch::InvalidEpoch();
    }

    if (isRebuild)
    {
        replicaStore_.Clear();
    }

    localReplicaId_ = replicaDesc.ReplicaId;
    localReplicaInstanceId_ = replicaDesc.InstanceId;

    Replica & replica = replicaStore_.AddReplica(replicaDesc, ReplicaRole::None, localReplicaRole);

    InternalUpdateLocalReplicaPackageVersionInstance(packageVersionInstance);

    localReplicaOpenPending_.SetValue(true, queue);    

    // Report health only if not open to avoid duplicate health reports
    // This needs to be done after the local replica id and instance are set
    if (IsClosed)
    {
        retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaOpen);
        serviceTypeRegistration_.Start(LocalReplica.State);
        state_ = FailoverUnitStates::Open;
        EnqueueHealthEvent(ReplicaHealthEvent::OK, queue, make_unique<PartitionHealthEventDescriptor>());
    }
    else
    {
        TESTASSERT_IF(!LocalReplicaOpen, "Replica must be open prior to handling create replica for role transition {0}", *this);
        retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaReopen);
        serviceTypeRegistration_.Start(LocalReplica.State);
    }

    if (localReplicaRole == ReplicaRole::Primary)
    {
        deactivationInfo_ = ReplicaDeactivationInfo(CurrentConfigurationEpoch, 0);
    }
    else if (localReplicaRole == ReplicaRole::Idle)
    {
        /*
            FOR NEW IDLE REPLICAS
                For a new idle replica the deactivation info is set to the info of the primary
                It will only become a part of the replica set after build completes and the FM
                performs a reconfiguration

                It is important to do this for every idle replica that is built and not just on the first
                attempt to build

                Thus, if there is a brand new replica being built by a primary in epoch e1
                and if before the build completes the primary goes down
                and the build starts again by a primary in epoch e2

                The deactivation epoch needs to be updated to e2

            FOR NEW S/S IB REPLICA
                For these replicas the deactivation info is not updated until after the build completes

                This allows GetLSN for these replicas to return a special value indicating that the build never completed
                and the LSN from this replica cannot be used
        */
        if (replicaDesc.PreviousConfigurationRole == ReplicaRole::None)
        {
            deactivationInfo_ = deactivationInfo;
        }
    }
    else if (localReplicaRole == ReplicaRole::None)
    {
        // Stateless
        TESTASSERT_IF(IsStateful, "ReplicaRole None cant be stateful {0}", *this);
        deactivationInfo_ = ReplicaDeactivationInfo();
    }
    else
    {
        Assert::TestAssert("Unknown local replica role {0}", *this);
    }

    return replica;
}

// Start process of adding Idle replica on Primary
Replica & FailoverUnit::StartAddReplicaForBuild(Reliability::ReplicaDescription const & replicaDesc)
{
    // Add the new replica
    ASSERT_IFNOT(
        replicaDesc.CurrentConfigurationRole == ReplicaRole::Idle ||
        replicaDesc.CurrentConfigurationRole == ReplicaRole::Secondary,
        "Invalid role to build: {0} for {1}",
        replicaDesc, *this);

    Replica & replica = replicaStore_.AddReplica(replicaDesc, ReplicaRole::None, replicaDesc.CurrentConfigurationRole);

    return replica;
}

// Finish adding Idle replica on Primary
void FailoverUnit::FinishAddIdleReplica(
    Replica & replica,
    Reliability::ReplicaDescription const & replicaDesc,
    FailoverUnitEntityExecutionContext & executionContext)
{
    // Update version if remote replica was created in an different version than requested
    if (replica.PackageVersionInstance != replicaDesc.PackageVersionInstance)
    {
        replica.PackageVersionInstance = replicaDesc.PackageVersionInstance;
    }

    // Start build of replica now
    StartBuildIdleReplica(replica);

    // Update the endpoints
    UpdateEndpoints(replica, replicaDesc, executionContext);
}

// Abort adding Idle replica on Primary
void FailoverUnit::AbortAddIdleReplica(Replica & replica)
{
    replicaStore_.RemoveReplica(replica.ReplicaId);
}

void FailoverUnit::StartRemoveIdleReplica(Replica & replica)
{
    // Update replica state
    replica.State = ReplicaStates::InDrop;
}

// Remove the given Idle replica on Primary
void FailoverUnit::FinishRemoveIdleReplica(Replica & replica)
{
    replicaStore_.RemoveReplica(replica.ReplicaId);
}

void FailoverUnit::UpdateInstanceForClosedReplica(
    Reliability::FailoverUnitDescription const & incomingFailoverUnitDescription,
    Reliability::ReplicaDescription const & incomingReplicaDescription,
    bool clearPreviousInstanceDeleteFlag,
    FailoverUnitEntityExecutionContext & context)
{
    TESTASSERT_IF(!IsClosed, "FT must be closed {0}", *this);
    if (CurrentConfigurationEpoch < incomingFailoverUnitDescription.CurrentConfigurationEpoch)
    {
        context.UpdateContextObj.EnableUpdate();
        CurrentConfigurationEpoch = incomingFailoverUnitDescription.CurrentConfigurationEpoch;
    }

    if (LocalReplicaId < incomingReplicaDescription.ReplicaId)
    {
        context.UpdateContextObj.EnableUpdate();
        LocalReplicaId = incomingReplicaDescription.ReplicaId;
        LocalReplicaInstanceId = incomingReplicaDescription.InstanceId;

        if (clearPreviousInstanceDeleteFlag)
        {
            localReplicaDeleted_ = false;
            cleanupPendingFlag_.SetValue(false, context.Queue);
        }
    }
}

// Start building Idle replica
void FailoverUnit::StartBuildIdleReplica(Replica & replica)
{
    // Mark the replica as in build
    replica.State = ReplicaStates::InBuild;
}

void FailoverUnit::FinishDeactivationInfoUpdate(Replica & replica, FailoverUnitEntityExecutionContext & executionContext)
{
    auto & actionQueue = executionContext.Queue;
    executionContext.UpdateContextObj.EnableUpdate();

    TESTASSERT_IF(!replica.IsInBuild, "Replica must be IB {0} {1}", replica, *this);

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote ||
        ReconfigurationStage == FailoverUnitReconfigurationStage::Phase2_Catchup ||
        ReconfigurationStage == FailoverUnitReconfigurationStage::Phase3_Deactivate)
    {
        TESTASSERT_IF(!replica.ToBeDeactivated, "Replica must be to be deactivated {0} {1}", replica, *this);
        replica.ToBeDeactivated = false;
    }

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase4_Activate ||
        ReconfigurationStage == FailoverUnitReconfigurationStage::None)
    {
        TESTASSERT_IF(!replica.ToBeActivated, "Replica must be to be deactivated {0} {1}", replica, *this);
        replica.ToBeActivated = false;
    }

    replica.State = Reliability::ReconfigurationAgentComponent::ReplicaStates::Ready;
    MarkReplicationConfigurationUpdatePendingAndSendMessage(actionQueue);
}

// Finish building Idle replica
void FailoverUnit::FinishBuildIdleReplica(
    Replica & replica,
    FailoverUnitEntityExecutionContext & executionContext,
    __out bool & sendToFM)
{
    auto & queue = executionContext.Queue;

    // Depending on the reconfiguration stage perform the appropriate work
    bool transitionToReady = false;
    bool toBeActivated = false;
    bool toBeDeactivated = false;
    bool setMessageStage = false;
    bool performMessageResend = false;
    bool sendActivateMessage = false;
    sendToFM = false;

    if (replica.CurrentConfigurationRole == ReplicaRole::Idle)
    {
        // Handle both S/I IB and N/I IB
        TESTASSERT_IF(!replica.IsInBuild, "Replica must be IB {0} {1}", replica, *this);
        transitionToReady = true;

        if (replica.PreviousConfigurationRole == ReplicaRole::None)
        {
            // N/I IB on build completion should send add replica reply
            sendToFM = true;
        }
    }
    else
    {
        TESTASSERT_IF(replica.CurrentConfigurationRole != ReplicaRole::Secondary, "Must be secondary (cannot have P IB) {0} {1}", replica, *this);
        switch (ReconfigurationStage)
        {
        case FailoverUnitReconfigurationStage::Phase0_Demote:
        case FailoverUnitReconfigurationStage::Phase2_Catchup:
            toBeDeactivated = true;
            performMessageResend = true;
            break;

        case FailoverUnitReconfigurationStage::Phase3_Deactivate:
            toBeDeactivated = true;
            setMessageStage = true;
            performMessageResend = true;
            break;

        case FailoverUnitReconfigurationStage::Phase4_Activate:
            toBeActivated = true;
            setMessageStage = true;
            performMessageResend = true;
            break;

        case FailoverUnitReconfigurationStage::None:
            toBeActivated = true;
            setMessageStage = true;
            sendActivateMessage = true;
            break;

        case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
            Assert::TestAssert("Unknown reconfig stage at FinishBuildReplica {0}", static_cast<int>(ReconfigurationStage));
            break;
        }
    }

    if (transitionToReady)
    {
        replica.State = ReplicaStates::Ready;
    }
    else
    {
        TESTASSERT_IF(!replica.IsInBuild, "Must stay IB {0} {1}", replica, *this);
    }

    replica.ToBeActivated = toBeActivated;
    replica.ToBeDeactivated = toBeDeactivated;

    if (setMessageStage)
    {
        replica.MessageStage = ReplicaMessageStage::RAReplyPending;
    }

    if (performMessageResend)
    {
        ProcessMsgResends(executionContext);
    }

    if (sendActivateMessage)
    {
        SendActivateMessage(replica, queue);
    }
}

bool FailoverUnit::CanProcessGetLSNReply(
    Reliability::FailoverUnitDescription const & fuDesc,
    Reliability::ReplicaDescription const & replicaDesc,
    Common::ErrorCodeValue::Enum errCodeValue) const
{
    if (errCodeValue != ErrorCodeValue::Success && errCodeValue != ErrorCodeValue::NotFound)
    {
        return false;
    }

    if (!LocalReplicaOpen || LocalReplicaClosePending.IsSet)
    {
        // FailoverUnit is closed or local replica is down before the completed message was received,
        // just ignore reply

        return false;
    }

    if (ReconfigurationStage != FailoverUnitReconfigurationStage::Phase1_GetLSN)
    {
        return false;
    }

    auto const & replica = GetReplica(replicaDesc.FederationNodeId);
    if (replica.IsInvalid)
    {
        return false;
    }

    if (replica.MessageStage != ReplicaMessageStage::RAReplyPending)
    {
        return false;
    }

    if (replica.InstanceId != replicaDesc.InstanceId)
    {
        /*
            This check is correct because the GetLSN contract is that if a remote replica
            has a higher instance than the primary and it is DD then it will echo back the instance the primary has sent

            If the replica is not DD on the remote node then the remote node wills end replica up and the FM will send
            the DoReconfiguration with higher instance which will allow GetLSN to proceed
        */
        return false;
    }

    TESTASSERT_IF(
        CurrentConfigurationEpoch.ToPrimaryEpoch() != fuDesc.CurrentConfigurationEpoch.ToPrimaryEpoch(),
        "Epoch mismatch at get lsn reply - should be guarded by message context staleness check {0} {1} \r\n {2}",
        fuDesc,
        replicaDesc,
        *this);

    return true;
}

void FailoverUnit::ProcessGetLSNReply(
    Reliability::FailoverUnitDescription const & fuDesc,
    Reliability::ReplicaDescription const & replicaDesc,
    Reliability::ReplicaDeactivationInfo const & deactivationInfo,
    Common::ErrorCodeValue::Enum errCodeValue,
    FailoverUnitEntityExecutionContext & executionContext)
{
    if (!CanProcessGetLSNReply(fuDesc, replicaDesc, errCodeValue))
    {
        return;
    }

    executionContext.UpdateContextObj.EnableInMemoryUpdate();

    auto & replica = GetReplica(replicaDesc.FederationNodeId);

    replica.MessageStage = ReplicaMessageStage::None;

    if (errCodeValue == ErrorCodeValue::NotFound)
    {
        /*
            The replica does not know its LSN
            This can happen if it was a brand new replica being built and the primary goes down
            Example: S/S IB for a brand new replica

            This replica needs to be treated as a DD replica for the purposes of FinishPhase1GetLSNProcessing

            However, in case there is data loss consider the LSN from this replica to pick the replica with least data loss
        */
        replica.SetLSNToUnknown(replicaDesc);
    }
    else if (replicaDesc.IsDropped)
    {
        // Replica has been dropped, clean up state
        executionContext.UpdateContextObj.EnableUpdate();
        replica.MarkAsDropped();
    }
    else
    {
        // This must happen after the replicaDesc.IsDropped check
        // If the replica on the remote node is dropped then the incoming reply
        // will not contain epoch information so it is invalid to call set progress
        TESTASSERT_IF(deactivationInfo.IsDropped, "Cannot have dropped deactivation info and error code is success {0} {1}\r\n{2}", replicaDesc, deactivationInfo, *this);
        replica.SetProgress(replicaDesc, deactivationInfo);
    }

    CheckReconfigurationProgressAndHealthReportStatus(executionContext);
}

void FailoverUnit::ProcessGetLSNMessage(
    Federation::NodeInstance const & from,
    Reliability::ReplicaMessageBody const & msgBody,
    FailoverUnitEntityExecutionContext & executionContext)
{
    executionContext.UpdateContextObj.EnableInMemoryUpdate();

    TESTASSERT_IF(msgBody.FailoverUnitDescription.CurrentConfigurationEpoch < CurrentConfigurationEpoch, "incoming epoch cannot be lower - should be checked by message context {0} {1}", *this, msgBody);

    if (CurrentConfigurationEpoch != msgBody.FailoverUnitDescription.CurrentConfigurationEpoch)
    { 
        executionContext.UpdateContextObj.EnableUpdate();

        CopyCCToPC();
        
        CurrentConfigurationEpoch = msgBody.FailoverUnitDescription.CurrentConfigurationEpoch;
    }

    SenderNode = from;

    SendReplicatorGetStatusMessage(executionContext.Queue);
}

bool FailoverUnit::SendReplicatorGetStatusMessage(Infrastructure::StateMachineActionQueue & queue) const
{
    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        queue,
        LocalReplica.IsAvailable ? RAMessage::GetReplicatorUpdateEpochAndGetStatus() : RAMessage::GetReplicatorGetStatus(),
        ServiceTypeRegistration,
        FailoverUnitDescription,
        LocalReplica.ReplicaDescription);

    return true;
}

void FailoverUnit::GetLocalReplicaLSNCompleted(
    Reliability::ReplicaDescription const & replicaDescription,
    FailoverUnitEntityExecutionContext & executionContext)
{
    Replica & localReplica = LocalReplica;

    localReplica.MessageStage = ReplicaMessageStage::None;

    localReplica.SetProgress(replicaDescription, deactivationInfo_);

    CheckReconfigurationProgressAndHealthReportStatus(executionContext);
}

void FailoverUnit::ProcessReplicatorGetStatusReply(
    Reliability::ReplicaDescription const & replicaDescription,
    FailoverUnitEntityExecutionContext & executionContext)
{
    // Process for the local replica
    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase1_GetLSN)
    {
        GetLocalReplicaLSNCompleted(replicaDescription, executionContext);
    }
    else if (SenderNode != ReconfigurationAgent::InvalidNode)
    {
        executionContext.UpdateContextObj.EnableInMemoryUpdate();

        // Send the LSN even if the deactivation info is dropped
        // This will allow us to pick the most advanced replica even in the case of data loss
        // Right now it is not possible to detect a stale replicator get status reply 
        // The replica that is receiving this message will detect it
        auto error = deactivationInfo_.IsDropped ? ErrorCodeValue::NotFound : ErrorCodeValue::Success;
        ReconfigurationAgent::AddActionSendGetLsnMessageReplyToRA(
            executionContext.Queue,
            SenderNode,
            FailoverUnitDescription,
            replicaDescription,
            deactivationInfo_,
            error);

        /*
            Clear the sender node at this point because it may interfere with state later on
        */
        senderNode_ = ReconfigurationAgent::InvalidNode;
    }
}

void FailoverUnit::ProcessCatchupCompletedReply(
    ProxyErrorCode const & err,
    Reliability::ReplicaDescription const & localReplica,
    FailoverUnitEntityExecutionContext & context)
{
    if (!reconfigState_.IsCatchupReconfigurationStage)
    {
        return;
    }

    ProcessHealthOnRoleTransition(err, context);

    CatchupCompleted(err, localReplica, context);
}

void FailoverUnit::ProcessCancelCatchupReply(
    ProxyErrorCode const & err,
    Reliability::FailoverUnitDescription const & ftDesc,
    FailoverUnitEntityExecutionContext & context)
{
    if (ReconfigurationStage != FailoverUnitReconfigurationStage::Abort_Phase0_Demote ||
        ftDesc.CurrentConfigurationEpoch != CurrentConfigurationEpoch)
    {
        return;
    }

    ProcessHealthOnRoleTransition(err, context);

    CancelCatchupCompleted(err, context);
}

void FailoverUnit::CatchupCompleted(
    ProxyErrorCode const & err,
    Reliability::ReplicaDescription const & msgReplicaDesc,
    FailoverUnitEntityExecutionContext & executionContext)
{
    bool wasStateChanged = err.IsError(ErrorCodeValue::RAProxyStateChangedOnDataLoss);
    if (!err.IsSuccess() && !wasStateChanged)
    {
        TESTASSERT_IF(!err.IsUserApiFailure, "Unknown error from RAP at catchup completion {0} {1}", err, *this);
        return;
    }

    Replica & localReplica = LocalReplica;
    TESTASSERT_IF(!reconfigState_.IsCatchupReconfigurationStage, "Invalid reconfig stage {0}", *this);
    TESTASSERT_IF(wasStateChanged && !FailoverUnitDescription.IsDataLossBetweenPCAndCC, "Cannot have state changed if no data loss between PC and CC {0}", *this);

    executionContext.UpdateContextObj.EnableUpdate();

    /*
        Endpoints will be updated only if there is a change role 
        If there is no primary change between PC and CC endpoints cannot change
    */
    if (FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC)
    {
        UpdateEndpoints(localReplica, msgReplicaDesc, executionContext);
        TESTASSERT_IF(!localReplica.IsInBuild && !localReplica.IsReady, "Local replica must be IB or RD when catchup completes {0}", *this);
        TESTASSERT_IF(reconfigState_.IsSwapPrimary && !localReplica.IsReady, "Local replica must be RD in swap when catchup completes {0}", *this);
        localReplica.State = ReplicaStates::Ready;
    }
    else
    {
        TESTASSERT_IF(localReplica.State != ReplicaStates::Ready, "Local replica must be RD if catchup completes and there is no primary change {0}", *this);
    }

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote)
    {
        OnPhaseChanged(executionContext.Queue);

        FinishSwapPrimary(executionContext);

        SendContinueSwapPrimaryMessage(nullptr, executionContext);

        return;
    }

    /*
        There are three scenarios here:
        - GetLSN was conducted and the RA already has the LSN set            
        - It was UC reply after swap primary was cancelled in which case there is no need for deactivation epoch
        to be updated
        - There was data loss in which case RAP will return the last ack LSN

        In the first scenario RAP will return back the LastAckLSN on the replica so that can be used as the catchup LSN
    */
    if (failoverUnitDesc_.IsPrimaryChangeBetweenPCAndCC)
    {
        TESTASSERT_IF(wasStateChanged && msgReplicaDesc.LastAcknowledgedLSN == FABRIC_INVALID_SEQUENCE_NUMBER, "There was state change and RAP did not return last ack LSN {0} {1}", msgReplicaDesc, *this);

        if (msgReplicaDesc.LastAcknowledgedLSN != FABRIC_INVALID_SEQUENCE_NUMBER)
        {
            TESTASSERT_IF(LocalReplica.GetLastAcknowledgedLSN() != FABRIC_INVALID_SEQUENCE_NUMBER && msgReplicaDesc.LastAcknowledgedLSN != LocalReplica.GetLastAcknowledgedLSN() && !wasStateChanged, "LastAckLSN from RAP for promote to primary must match RA {0} {1}", msgReplicaDesc, *this);
            deactivationInfo_ = ReplicaDeactivationInfo(CurrentConfigurationEpoch, msgReplicaDesc.LastAcknowledgedLSN);
        }
    }

    if (wasStateChanged)
    {
        /*
            Restart remote replicas which are up, in the config and in the ready state to take them through build as the primary has changed LSN after data loss
        */
        for (auto & replica : replicaStore_.ConfigurationRemoteReplicas)
        {
            if (replica.IsUp &&
                replica.IsInConfiguration &&
                replica.IsReady)
            {
                replica.ToBeRestarted = true;
            }
        }
    }

    OnPhaseChanged(executionContext.Queue);
    StartPhase3Deactivate(executionContext);
}

void FailoverUnit::CancelCatchupCompleted(
    ProxyErrorCode const & result,
    FailoverUnitEntityExecutionContext & executionContext)
{
    // If endpoint update or something is pending
    fmMessageState_.Reset(executionContext.StateUpdateContextObj);

    // Since catchup was cancelled, the replicator configuration doesn't need to be
    // updated for this epoch during swap primary, subsequent reconfiguration message
    // with an higher epoch update the configuration appropriately
    updateReplicatorConfiguration_ = false;

    if (result.IsSuccess())
    {
        reconfigState_.FinishAbortSwapPrimary(executionContext.NodeInstance, executionContext.Clock, executionContext.Queue);

        // Revert configuration to PC
        RevertConfiguration();
    }
    else if (result.IsError(ErrorCodeValue::RAProxyDemoteCompleted))
    {
        reconfigState_.FinishDemote(executionContext.Clock);
    }
}

void FailoverUnit::OnApplicationCodeUpgrade(
    ServiceModel::ApplicationUpgradeSpecification const & specification,
    FailoverUnitEntityExecutionContext & context)
{
    ServicePackageVersionInstance packageVersionInstance;
    bool found = specification.GetUpgradeVersionForServiceType(ServiceDescription.Type, packageVersionInstance);

    if (found && LocalReplica.PackageVersionInstance.InstanceId < packageVersionInstance.InstanceId)
    {
        context.UpdateContextObj.EnableUpdate();
        InternalUpdateLocalReplicaPackageVersionInstance(packageVersionInstance);
    }
}

void FailoverUnit::OnFabricCodeUpgrade(Upgrade::FabricCodeVersionProperties const & properties)
{
    /*
        Deactivation Info needs to be reset if the target version of the code does not support deactivation info

        Otherwise the deactivation info will still be present on this FT and the replica could be rebuilt and
        added back into the replica set in a higher epoch after the upgrade

        If subsequently another upgrade is performed to a version of winfab that supports deactivation info
        it will read the previously set deactivation info for this replica and the invariant that a replica
        cannot have a quorum ack'd LSN in an epoch greater than its deactivation info will break causing undetected dataloss
    */
    if (!properties.IsDeactivationInfoSupported)
    {
        deactivationInfo_ = ReplicaDeactivationInfo();
    }
}

void FailoverUnit::UpdateDeactivationInfo(
    Reliability::FailoverUnitDescription const & incomingFTDescription,
    Reliability::ReplicaDescription const & incomingReplica,
    Reliability::ReplicaDeactivationInfo const & incomingDeactivationInfo,
    FailoverUnitEntityExecutionContext & executionContext)
{
    ASSERT_IF(incomingReplica.InstanceId != LocalReplica.InstanceId, "Incoming replica must be local replica {0} {1}", incomingReplica, *this);

    if (!incomingReplica.IsReady)
    {
        return;
    }

    auto deactivationInfo = GetDeactivationInfoWithBackwardCompatibilitySupport(incomingFTDescription, incomingReplica, incomingDeactivationInfo);

    if (deactivationInfo <= deactivationInfo_)
    {
        return;
    }

    executionContext.UpdateContextObj.EnableUpdate();
    deactivationInfo_ = deactivationInfo;
}

ReplicaDeactivationInfo FailoverUnit::GetDeactivationInfoWithBackwardCompatibilitySupport(
    Reliability::FailoverUnitDescription const & incomingFTDescription,
    Reliability::ReplicaDescription const & incomingReplica,
    Reliability::ReplicaDeactivationInfo const & incomingDeactivationInfo)
{
    TESTASSERT_IF(incomingDeactivationInfo.IsDropped, "Incoming deactivation info cannot be dropped {0} {1} {2}\r\n{3}", incomingFTDescription, incomingReplica, incomingDeactivationInfo, *this);
    if (incomingDeactivationInfo.IsValid)
    {
        return incomingDeactivationInfo;
    }

    /*
        Update the deactivation info if it is a swap primary where replica that is becoming secondary (P->S)
        is an older version of fabric which does not support deactivation info

        The incoming replica will contain catchup LSN (which is the same for all replicas as catchup all is performed)
        Further, the only scenario in which the incoming replica contains LSN is swap primary

        The deactivation info in this case can be assumed as the ccEpoch in the incoming message and the LSN for this replica
        (during swap primary all replicas have the same LSN as catchup all has been performed)
    */
    if (incomingReplica.LastAcknowledgedLSN == FABRIC_INVALID_SEQUENCE_NUMBER)
    {
        return ReplicaDeactivationInfo();
    }

    return ReplicaDeactivationInfo(incomingFTDescription.CurrentConfigurationEpoch, incomingReplica.LastAcknowledgedLSN);
}

void FailoverUnit::StartDeactivate(
    Reliability::ConfigurationMessageBody const & deactivateMessageBody,
    FailoverUnitEntityExecutionContext & executionContext)
{
    executionContext.UpdateContextObj.EnableUpdate();

    IntermediateConfigurationEpoch = deactivateMessageBody.FailoverUnitDescription.CurrentConfigurationEpoch;

    RefreshConfiguration(deactivateMessageBody, false /*resetIC*/, executionContext);
}

void FailoverUnit::FinishDeactivate(ReplicaDescription const & ccReplica)
{
    if (!ccReplica.IsInCurrentConfiguration)
    {
        TESTASSERT_IF(ccReplica.CurrentConfigurationRole != ReplicaRole::Idle, "Role must be I or N. N should be handled in RA {0} {1}", *this, ccReplica);
        PreviousConfigurationEpoch = Epoch::InvalidEpoch();
        IntermediateConfigurationEpoch = Epoch::InvalidEpoch();

        LocalReplica.PreviousConfigurationRole = ReplicaRole::None;
        LocalReplica.IntermediateConfigurationRole = ReplicaRole::None;

        reconfigState_.Reset();

        replicaStore_.ConfigurationRemoteReplicas.Clear();
    }
}

void FailoverUnit::StartActivate(
    Reliability::ActivateMessageBody const & activateMessageBody,
    FailoverUnitEntityExecutionContext & executionContext)
{
    executionContext.UpdateContextObj.EnableUpdate();

    // Update the configuration
    RefreshConfiguration(activateMessageBody, true /*resetIC*/, executionContext);

    // Note: IC epoch should be updated after the configuration has been refreshed
    IntermediateConfigurationEpoch = Epoch::InvalidEpoch();
}

void FailoverUnit::FinishActivate(
    Reliability::ReplicaDescription const & localReplicaDesc, 
    bool updateEndpoints,
    FailoverUnitEntityExecutionContext & executionContext)
{
    Replica & localReplica = LocalReplica;

    localReplica.MessageStage = ReplicaMessageStage::None;

    if (updateEndpoints)
    {
        // Update endpoints since they could have changed on role changes
        UpdateEndpoints(localReplica, localReplicaDesc, executionContext);
    }

    ResetReconfigurationStates();

    /*
        Important to clear the sender node here
        This is because the sender node is set when activate processing starts and 
        UC is needed so that the UC handler can send the reply

        After UC reply comes in FinishActivate fixes up the state and the caller is supposed
        to snapshot sendernode so that it can be cleared

        Otherwise sendernode can be set and be stale for a long time
    */
    senderNode_ = ReconfigurationAgent::InvalidNode;
}

bool FailoverUnit::IsConfigurationMessageBodyStale(Reliability::ConfigurationMessageBody const & configBody)
{
    vector<Reliability::ReplicaDescription> const & replicaDescList = configBody.ReplicaDescriptions;

    for (auto const & replicaInMessage : replicaDescList)
    {
        auto const & replicaOnNode = GetReplica(replicaInMessage.FederationNodeId);

        // If the replica does not exist or replica id is different then this replica is not stale
        if (replicaOnNode.IsInvalid || replicaOnNode.ReplicaId != replicaInMessage.ReplicaId)
        {
            continue;
        }

        if (replicaOnNode.IsStale(replicaInMessage))
        {
            return true;
        }
    }

    return false;
}

void FailoverUnit::RefreshConfiguration(
    Reliability::ConfigurationMessageBody const & configBody, 
    bool resetIC,
    FailoverUnitEntityExecutionContext & executionContext)
{
    bool useIncomingCCAsPC = configBody.FailoverUnitDescription.PreviousConfigurationEpoch.IsInvalid();

    replicaStore_.ConfigurationRemoteReplicas.Clear();
    PreviousConfigurationEpoch = useIncomingCCAsPC ? configBody.FailoverUnitDescription.CurrentConfigurationEpoch : configBody.FailoverUnitDescription.PreviousConfigurationEpoch;
    CurrentConfigurationEpoch = configBody.FailoverUnitDescription.CurrentConfigurationEpoch;
    
    /*
        Update remote replicas
    */
    auto & replicaDescList = configBody.ReplicaDescriptions;
    for (auto rdit = replicaDescList.begin(); rdit != replicaDescList.end(); ++rdit)
    {
        auto const & rit = GetReplica(rdit->FederationNodeId);

        if (!rit.IsInvalid)
        {
            continue;
        }

        TESTASSERT_IF(!rdit->IsInPreviousConfiguration && !rdit->IsInCurrentConfiguration, "Incoming replica {0} is not in PC or CC {1}\r\n{2}", *rdit, configBody, *this);
        if (!rdit->IsInPreviousConfiguration && !rdit->IsInCurrentConfiguration)
        {
            // No need to add replicas not in configuration
            continue;
        }

        auto pcRole = useIncomingCCAsPC ? rdit->CurrentConfigurationRole : rdit->PreviousConfigurationRole;
        Replica & newReplica = replicaStore_.AddReplica(*(rdit), pcRole, rdit->CurrentConfigurationRole);

        /*
            Keep IB as IB (FromReplicaDescriptionState translates IB to IC and it is not useful to have remote replica as IC
        */
        if (rdit->State == Reliability::ReplicaStates::InBuild)
        {
            newReplica.State = ReplicaStates::InBuild;
        }
        else
        {
            newReplica.State = ReplicaStates::FromReplicaDescriptionState(rdit->State);
        }

        newReplica.IntermediateConfigurationRole = resetIC ? ReplicaRole::None : rdit->CurrentConfigurationRole;
        
        newReplica.IsUp = rdit->IsUp;

        UpdateEndpoints(newReplica, *(rdit), executionContext);

        newReplica.LastUpdated = DateTime::Now();
    }

    auto & incomingLocalReplica = *configBody.GetReplica(LocalReplica.FederationNodeId);
    LocalReplica.PreviousConfigurationRole = useIncomingCCAsPC ? incomingLocalReplica.CurrentConfigurationRole : incomingLocalReplica.PreviousConfigurationRole;
    TESTASSERT_IF(incomingLocalReplica.IsDropped || incomingLocalReplica.IsInBuild, "Invalid incoming local replica state {0}\r\n{1}", incomingLocalReplica, *this);
    LocalReplica.State = ReplicaStates::FromReplicaDescriptionState(incomingLocalReplica.State);

    LocalReplica.IntermediateConfigurationRole = resetIC ? ReplicaRole::None : incomingLocalReplica.CurrentConfigurationRole;

    /*
        The only scenario in which the CC role should not become the incoming CC role is 
        when a S/N/I IB or P/N/I IB replica processes Deactivate [S/S] and updates its view of the configuration

        At this time, it should stay as S/S/I OR P/S/I so that activate knows there is a role change needed
    */
    if (!(LocalReplica.CurrentConfigurationRole == ReplicaRole::Idle && LocalReplica.IsInPreviousConfiguration && !resetIC))
    {
        LocalReplica.CurrentConfigurationRole = incomingLocalReplica.CurrentConfigurationRole;
    }

    LocalReplica.LastUpdated = DateTime::Now();
}

void FailoverUnit::DeactivateCompleted(
    Reliability::ReplicaDescription const & replicaDesc, 
    FailoverUnitEntityExecutionContext & executionContext)
{
    executionContext.UpdateContextObj.EnableInMemoryUpdate();

    // Get the replica who sent the completed message
    Replica & replica = GetReplica(replicaDesc.FederationNodeId);

    if (replica.IsInvalid || replica.InstanceId != replicaDesc.InstanceId)
    {
        return;
    }

    /*
        There are two scenarios here:
        a) Deactivate was sent as a request to restart this replica (i.e. with force)
        b) Deactivate was sent to update the deactivation info after build completion
        c) Deactivate was sent as Phase3 Deactivate

        Deactivate Reply could be received in either of these cases and staleness needs to be detected.

        Deactivate Reply for (a) will only be received for volatile services. Persisted services will never
        send Deactivate reply upon restart or close completion - it will round trip via the FM in a 
        subsequent DoReconfiguration

        Considering volatile and persisted services separately:

        Volatile services:
        If restart is pending and deactivate reply is received then it has to be for the restart. This
        is because Restart would always be performed prior to Build or Phase3 Deactivate (GetLSN/Phase1/Phase2Deactivate)

        If restart is not pending and deactivate reply is received then it has to be for either (b) or (c). This 
        is because Restart pending with DeactivateReply for volatile will transition the replica to DD and such a replica
        cannot be built or further deactivated in the same reconfiguration. 

        Persisted services:
        Deactivate reply can never be for (a) EXCEPT if the primary sends Deactivate with Force message and the remote 
        replica faults permanent at the same time. In this case if the DoReconfiguration from FM with ReplicaDropped is delayed
        and the Deactivate with Force is retried the secondary RA will send DeactivateReply. Unfortunately, it will send
        the DeactivateReply with the replica marked as Ready.

        Rather than use this the RA for persisted services will wait for a DoReconfiguration from FM

        Thus, the first case can be handled here directly
    */

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote || 
        ReconfigurationStage == FailoverUnitReconfigurationStage::Phase2_Catchup ||
        ReconfigurationStage == FailoverUnitReconfigurationStage::Phase3_Deactivate ||
        ReconfigurationStage == FailoverUnitReconfigurationStage::Phase4_Activate)
    {
        if (replica.ToBeRestarted)
        {
            if (HasPersistedState)
            {
                /*
                    For persisted services, as indicated above, do not use DeactivateReply
                    Instead wait for a message from the FM with the updated state
                */
                return;
            }

            executionContext.UpdateContextObj.EnableUpdate();

            replica.MarkAsDropped();

            CheckReconfigurationProgressAndHealthReportStatus(executionContext);

            return;
        }
    }

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote ||
        ReconfigurationStage == FailoverUnitReconfigurationStage::Phase2_Catchup)
    {
        /*
            Reply received for update deactivation epoch message
            This replica can be transitioned to ready and added to the replication configuration for catchup
            Additional staleness checks need to be performed here
        */
        if (!replica.IsInBuild)
        {
            return;
        }

        if (!replica.ToBeDeactivated)
        {
            return;
        }

        FinishDeactivationInfoUpdate(replica, executionContext);

        CheckReconfigurationProgressAndHealthReportStatus(executionContext);
        return;
    }

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase3_Deactivate)
    {
        /*
            Reply received for phase 3 deactivate message
            The replica can either be SB or IB+ToBeDeactivated or RD. Any other state is not allowed
        */
        bool isInBuildAndToBeDeactivated = replica.IsInBuild && replica.ToBeDeactivated;
        if (!replica.IsStandBy && !isInBuildAndToBeDeactivated && !replica.IsReady)
        {
            return;
        }

        if (replica.MessageStage != ReplicaMessageStage::RAReplyPending)
        {
            return;
        }

        // Special case handling for the IB replica
        if (isInBuildAndToBeDeactivated)
        {
            FinishDeactivationInfoUpdate(replica, executionContext);
        }

        executionContext.UpdateContextObj.EnableInMemoryUpdate();

        replica.MessageStage = ReplicaMessageStage::None;

        CheckReconfigurationProgressAndHealthReportStatus(executionContext);
        return;
    }
}

void FailoverUnit::ActivateCompleted(
    Reliability::ReplicaDescription const & incomingReplica, 
    FailoverUnitEntityExecutionContext & executionContext)
{
    auto & actionQueue = executionContext.Queue;

    // Get the replica who sent the completed message
    Replica & replica = GetReplica(incomingReplica.FederationNodeId);

    if (replica.IsInvalid ||
        replica.InstanceId != incomingReplica.InstanceId)
    {
        return;
    }

    // A Replica being activated must have the message stage at RAReplyPending
    // And either the primary is performing Phase4_Activate
    // Or it is an AddSecondary
    if (replica.MessageStage != ReplicaMessageStage::RAReplyPending)
    {
        return;
    }

    if (ReconfigurationStage != FailoverUnitReconfigurationStage::Phase4_Activate && ReconfigurationStage != FailoverUnitReconfigurationStage::None)
    {
        return;
    }

    // The staleness checks above are not sufficient
    // The comment below describes staleness with regards to the state
    //
    // Activate Reply cannot be processed for a remote replica that is DD on the primary. It must be stale
    // Activate Reply cannot be processed for a remote replica that is ID on the primary
    // Activate Reply for a remote replica that is IC on the primary should be processed once it is ready.
    //
    // A reconfiguration may cause a replica to become SB
    // This replica will then send an ActivateReply with itself as SB
    // Thus ActivateReply with SB when the local Replica is in SB is not stale
    // A reconfiguration may also cause no change in the state of the replica
    // i.e. the replica was RD on the primary and on the remote replica
    // Thus ActivateReply with RD on the local and incoming is not stale
    // From the above it is possible to conclude that ActivateReply
    // when the incoming state matches the current state is not stale
    //
    // If the local state of the replica is RD then the incoming state must be RD
    // Otherwise it is not possible for activate to be complete and the message is stale
    //
    // If the local state of the replica is SB then it is possible for the incoming state
    // to be IB and the ActivateReply to not be stale
    // Consider SwapPrimary where a replica restarts after Phase2_Catchup is complete and
    // the FM marks the replica as IB. In this case it is possible that the RA has created the remote replica
    // (CreateReplica is complete) and the remote replica is IB
    // The Activate message will contain the replica as SB but the ActivateReply will contain the replica as IB
    // as the remote node has the remote replica as IB
    // The primary must handle this case for backward compatibility
    //
    // Finally, for the IB case, if the replica is not marked as toBeActivated then the reply is stale
    // Also, for the IB case the incoming replica must be RD else it is stale
    if (replica.State == ReplicaStates::Dropped ||
        replica.State == ReplicaStates::InCreate||
        replica.State == ReplicaStates::InDrop)
    {
        return;
    }

    if (replica.State == ReplicaStates::Ready && incomingReplica.State != Reliability::ReplicaStates::Ready)
    {
        return;
    }

    if (replica.State == ReplicaStates::StandBy && (incomingReplica.State != Reliability::ReplicaStates::StandBy && incomingReplica.State != Reliability::ReplicaStates::InBuild))
    {
        return;
    }

    if (replica.IsInBuild && !replica.ToBeActivated)
    {
        return;
    }

    if (replica.IsInBuild && !incomingReplica.IsReady)
    {
        return;
    }

    executionContext.UpdateContextObj.EnableInMemoryUpdate();

    if (incomingReplica.CurrentConfigurationRole != Reliability::ReplicaRole::None)
    {
        // Update endpoints since they could have changed on role changes
        UpdateEndpoints(replica, incomingReplica, executionContext);
    }

    replica.MessageStage = ReplicaMessageStage::None;

    if (replica.IsInBuild)
    {
        // Transition to ready - deactivation epoch is updated for S/S IB replica
        FinishDeactivationInfoUpdate(replica, executionContext);
    }

    // see #5790069
    // temporary work around to handle the case when a replica transitions IB->RD
    // with ToBeActivated not being cleared because FM finds out about RD S due to rebuild
    // Retry from RA should fix this
    if (replica.ToBeActivated)
    {
        return;
    }

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase4_Activate)
    {
        CheckReconfigurationProgressAndHealthReportStatus(executionContext);
    }
    else
    {
        ReplicaReplyMessageBody body(FailoverUnitDescription, replica.ReplicaDescription, ErrorCode::Success());

        TransportUtility.AddSendMessageToFMAction(
            actionQueue,
            RSMessage::GetAddReplicaReply(),
            move(body));
    }
}

bool FailoverUnit::CanProcessDoReconfiguration(
    Reliability::DoReconfigurationMessageBody const & msgBody,
    FailoverUnitEntityExecutionContext & executionContext) const
{
    if (IsReconfiguring)
    {
        return true;
    }

    if (CurrentConfigurationEpoch == msgBody.FailoverUnitDescription.CurrentConfigurationEpoch)
    {
        if (reconfigState_.Result == ReconfigurationResult::DemoteCompleted)
        {
            SendContinueSwapPrimaryMessage(&msgBody, executionContext);
        }
        else if (reconfigState_.Result == ReconfigurationResult::Completed)
        {
            SendDoReconfigurationReply(executionContext);
        }
        else if (reconfigState_.Result == ReconfigurationResult::ChangeConfiguration)
        {
            SendChangeConfiguration(executionContext.Queue);
        }

        return false;
    }

    return true;
}

// Implement the reconfiguration protocol
void FailoverUnit::DoReconfiguration(
    DoReconfigurationMessageBody const & reconfigBody, 
    FailoverUnitEntityExecutionContext & executionContext)
{
    if (!CanProcessDoReconfiguration(reconfigBody, executionContext))
    {
        return;
    }

    fmMessageState_.OnReplicaUpAcknowledged(executionContext.StateUpdateContextObj);

    bool updateReplicationConfiguration = false;

    if (TryAbortReconfiguration(reconfigBody.FailoverUnitDescription, executionContext))
    {
        return;
    }

    executionContext.UpdateContextObj.EnableInMemoryUpdate();

    // Note: Identifying the reconfiguration type needs to happen before pc or cc epoch is updated
    auto newReplicaDescPtr = reconfigBody.GetReplica(LocalReplica.FederationNodeId);
    ASSERT_IF(newReplicaDescPtr == nullptr, "DoReconfig body does not contain local replica {0} {1}", reconfigBody, *this);

    CurrentConfigurationEpoch = reconfigBody.FailoverUnitDescription.CurrentConfigurationEpoch;

    // Update the last update time
    lastUpdated_ = DateTime::Now();

    if (!IsReconfiguring)
    {
        executionContext.UpdateContextObj.EnableUpdate();

        UpdateReconfigurationEpochs(reconfigBody);

        auto reconfigType = IdentifyReconfigurationType(reconfigBody, *newReplicaDescPtr);

        UpdateReconfigurationState(reconfigType, reconfigBody.Phase0Duration, executionContext);

        UpdateLocalReplicaStatesAndRoles(*newReplicaDescPtr);

        UpdateRemoteReplicaRoles(reconfigBody, executionContext);

        UpdateRemoteReplicaStates(reconfigBody, true, updateReplicationConfiguration, executionContext);

        StartReconfiguration(executionContext);

        /*
            Clear the sender node if it is set. 
            Consider: 
            1. Replica received GetLSN and set the sender node
            2. Primary-Elect performed change config to this replica
            3. This replica now got doreconfig

            At this time sender node is set even though it should not be
        */
        senderNode_ = ReconfigurationAgent::InvalidNode;
    }
    else
    {
        UpdateRemoteReplicaStates(reconfigBody, false, updateReplicationConfiguration, executionContext);

        // TODO: Refactor - updating remote replica state should be intelligent enough to not require the function below
        if (updateReplicationConfiguration)
        {
            MarkReplicationConfigurationUpdatePending();
        }

        CheckReconfigurationProgressAndHealthReportStatus(executionContext);

        if (updateReplicationConfiguration && !LocalReplicaClosePending.IsSet)
        {
            // If the state of any replica has changed, then a resend might be needed
            // It is needed to check local replica close pending because the partition could 
            // be closing (the replica may be undergoing restart) because getlsn is stuck
            // or there have been too many failures during changerole etc
            // at this point do not try to resend messages
            ProcessMsgResends(executionContext);
        }
    }
}

void FailoverUnit::MarkReplicationConfigurationUpdatePending()
{
    // If reconfiguration is already in progress and replicas go down, then the replicator configuration needs to be updated

    // This only needs to happen if we are past Phase1_GetLSN
    // Also, if we are past Phase2_Catchup and it is a swap primary (Local Replica Role = S) then the replicator has changed role
    // And there is no need to update the replicator configuration

    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        // Further stages will set the flag as appropriate
        return;

    case FailoverUnitReconfigurationStage::Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        // The replicator could still be primary so it is correct to set this
        updateReplicatorConfiguration_ = true;
        return;

    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
    case FailoverUnitReconfigurationStage::Phase4_Activate:
        if (LocalReplicaRole == ReplicaRole::Primary)
        {
            updateReplicatorConfiguration_ = true;
        }

        return;

    case FailoverUnitReconfigurationStage::None:
        updateReplicatorConfiguration_ = true;
        return;

    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
    default:
        Assert::CodingError("Invalid call for reconfig stage {0} {1}", ReconfigurationStage, *this);
    }
}

bool FailoverUnit::TryAbortReconfiguration(
    Reliability::FailoverUnitDescription const & incomingFTDescription,
    FailoverUnitEntityExecutionContext & context)
{
    /*
        Consider the following scenarios:
        1. This is a secondary and receives a DoReconfiguration message: At this point nothing is to be aborted

        2. This is a primary and is already reconfiguring (promote OR P/P): At this point nothing is to be aborted

        3. This is a primary and is demoting: At this point transition to Abort_Phase0

        4. This is a primary and is already aborting: Wait for abort to complete

        5. This was a primary and has already demoted itself: At this point nothing to be aborted - start the promote process
    */
    if (!IsReconfiguring)
    {
        return false;
    }
    
    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Abort_Phase0_Demote)
    {
        return true;
    }

    if (incomingFTDescription.CurrentConfigurationEpoch.ToPrimaryEpoch() <= CurrentConfigurationEpoch.ToPrimaryEpoch())
    {
        // Retry 
        return false;
    }

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote)
    {
        StartAbortPhase0Demote(context);

        return true;
    }
    else
    {
        return false;
    }
}

void FailoverUnit::UpdateRemoteReplicaStates(
    ConfigurationMessageBody const & reconfigBody,
    bool isReconfigurationStarting,
    bool & updateReplicationConfiguration,
    FailoverUnitEntityExecutionContext & executionContext)
{
    /*
        If an IB or Idle replica goes down it needs to be removed from the replicator view
        This is done by setting the replicatorRemovePending flag on the replica

        However, this should only be done if it is possible to remove the replica from the replicator view
    */
    bool isReplicatorRemovePendingPossible = true;

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase1_GetLSN)
    {
        isReplicatorRemovePendingPossible = false;
    }
    else if (LocalReplica.CurrentConfigurationRole != ReplicaRole::Primary && !AllowBuildDuringSwapPrimary)
    {
        isReplicatorRemovePendingPossible = false;
    }

    // Now update state of the rest of the replicas
    for (auto rdit = reconfigBody.BeginIterator; rdit != reconfigBody.EndIterator; ++rdit)
    {
        ReplicaDescription const & newReplicaDesc = *(rdit);

        // Get the replica undergoing potential reconfiguration
        auto & replica = GetReplica(newReplicaDesc.FederationNodeId);
        ASSERT_IF(replica.IsInvalid, "Replica must exist - DoReconfiguration body cannot have a replica RA does not have {0}\r\n {1}", reconfigBody, *this);

        if (replicaStore_.IsLocalReplica(replica))
        {
           // Local replica, continue since it has already been updated
            continue;
        }

        if (replica.ReplicatorRemovePending)
        {
            continue;
        }

        /*
            Temporary change. Eventually refactor this function to be clearer and have consistent state transitions
            For now wait until ToBeRestarted flag gets cleared prior to processing any further updates
        */
        if (replica.ToBeRestarted)
        {
            if (replica.InstanceId < newReplicaDesc.InstanceId)
            {
                replica.ToBeRestarted = false;
            }
            else if (replica.IsUp && !newReplicaDesc.IsUp)
            {
                replica.ToBeRestarted = false;
            }
            else
            {
                continue;
            }
        }

        // Update states
        if (replica.InstanceId < newReplicaDesc.InstanceId)
        {
            if (replica.IsUp && (replica.IsInBuild || !replica.IsInCurrentConfiguration))
            {
                replica.IsUp = false;

                if (replica.IsInBuild && isReplicatorRemovePendingPossible)
                {
                    replica.ReplicatorRemovePending = true;
                    replica.ToBeActivated = false;
                    replica.ToBeDeactivated = false;

                    /*
                        Only reset the message stage if the FT is not reconfiguring as it could be N/S IB and this is the first DoReconfig
                        If the FT is already reconfiguring then the message stage indicates whether Deactivate/Activate is pending
                        and should not be reset
                    */
                    if (isReconfigurationStarting)
                    {
                        replica.MessageStage = ReplicaMessageStage::None;
                    }
                }
            }
            else
            {
                replica.UpdateInstance(newReplicaDesc, false /*resetLSN*/);

                replica.State = ReplicaStates::StandBy;

                updateReplicationConfiguration = true;
            }
        }

        if (replica.InstanceId == newReplicaDesc.InstanceId)
        {
            if (newReplicaDesc.IsUp)
            {
                if (replica.IsUp &&
                    replica.IsStandBy &&
                    newReplicaDesc.State == Reliability::ReplicaStates::InBuild)
                {
                    Replica & localReplica = LocalReplica;

                    if (localReplica.CurrentConfigurationRole == ReplicaRole::Primary ||
                        AllowBuildDuringSwapPrimary)
                    {
                        replica.State = ReplicaStates::InCreate;

                        if ((ReconfigurationStage == FailoverUnitReconfigurationStage::Phase3_Deactivate ||
                            ReconfigurationStage == FailoverUnitReconfigurationStage::Phase4_Activate) &&
                            replica.MessageStage == ReplicaMessageStage::None)
                        {
                            // Deactivate or Activate needs to be sent to this replica after
                            // the build is completed
                            replica.MessageStage = ReplicaMessageStage::RAReplyPending;
                        }
                    }
                }
                else if (newReplicaDesc.State == Reliability::ReplicaStates::Ready && replica.IsUp)
                {
                    if (replica.State != ReplicaStates::Ready)
                    {
                        replica.State = ReplicaStates::Ready;

                        UpdateEndpoints(replica, newReplicaDesc, executionContext);
                    }
                }
                else if (replica.IsUp &&
                         replica.IsInBuild &&
                         replica.ToBeActivated &&
                         newReplicaDesc.IsInBuild &&
                         isReconfigurationStarting)
                {
                    // handle the case where RA has N/S IB ToBeActivated
                    // and the FM starts another reconfiguration with S/S IB
                    // The ToBeActivated needs to become a ToBeDeactivated in the higher epoch
                    // The replica will then transition to [S/S IB ToBeDeactivated] in phase2 catchup
                    // And be activated when the reconfig moves to phase4 activate
                    replica.ToBeActivated = false;
                    replica.ToBeDeactivated = true;
                    replica.MessageStage = ReplicaMessageStage::None;
                }
            }
            else
            {
                if (replica.IsUp)
                {
                    replica.IsUp = false;

                    updateReplicationConfiguration = true;

                    // Mark Replicator remove pending if in a phase where remove is possible (i.e. Phase2/Phase3 etc)
                    // and replica is IB (S/S IB or S/I IB or I/S IB)
                    // OR I/S and in Phase2 because this may be the very first UC message to RAP and the replica may never 
                    // get promoted in the replicator
                    bool isInBuildReplica = replica.IsInBuild;
                    bool isIdleReplicaBeingPromotedToActiveSecondary = replica.PreviousConfigurationRole == ReplicaRole::Idle &&
                        replica.CurrentConfigurationRole == ReplicaRole::Secondary &&
                        reconfigState_.IsCatchupReconfigurationStage;

                    if (isReplicatorRemovePendingPossible && (isInBuildReplica || isIdleReplicaBeingPromotedToActiveSecondary))
                    {
                        replica.ReplicatorRemovePending = true;
                        replica.ToBeActivated = false;
                        replica.ToBeDeactivated = false;
                        updateReplicationConfiguration = false;

                        if (isReconfigurationStarting)
                        {
                            replica.MessageStage = ReplicaMessageStage::None;
                        }
                    }
                }
                else if (newReplicaDesc.IsDropped && !replica.IsDropped)
                {
                    replica.MarkAsDropped();
                }
            }
        }
    }
}

void FailoverUnit::UpdateReconfigurationEpochs(
    Reliability::DoReconfigurationMessageBody const & reconfigBody)
{
    // Reset the IC at the start of a reconfiguration
    IntermediateConfigurationEpoch = Epoch::InvalidEpoch();
    LocalReplica.IntermediateConfigurationRole = ReplicaRole::None;

    // Always take the PC from the incoming message
    PreviousConfigurationEpoch = reconfigBody.FailoverUnitDescription.PreviousConfigurationEpoch;

    bool isReadyPrimary = LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary && LocalReplica.IsReady;
    if (!isReadyPrimary)
    {
        replicaStore_.ConfigurationRemoteReplicas.Clear();
    }
}

ReconfigurationType::Enum FailoverUnit::IdentifyReconfigurationType(
    DoReconfigurationMessageBody const & reconfigBody,
    Reliability::ReplicaDescription const & newReplicaDesc) const
{
    /*
        A do reconfiguration from remote RA is always a continue swap which will always have the phase0 duration set to a valid value and not the default MaxValue        
    */
    if (reconfigBody.Phase0Duration != TimeSpan::MaxValue)
    {
        return ReconfigurationType::SwapPrimary;
    }

    /*
        If there is no primary change between PC and CC then it is an 'Other' reconfiguration        
    */
    if (!reconfigBody.FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC)
    {
        return ReconfigurationType::Other;
    }

    /*
        At this point there is a primary change between PC and CC.
        If the incoming do reconfig message has this replica marked as secondary then it has to be a swap
     */
    if (newReplicaDesc.CurrentConfigurationRole == ReplicaRole::Secondary)
    {
        // Since a swap is starting the local replica must be RD Primary
        TESTASSERT_IF(LocalReplica.CurrentConfigurationRole != ReplicaRole::Primary, "Replica must be RD Primary {0} {1}", reconfigBody, *this);
        TESTASSERT_IF(!LocalReplica.IsReady, "Replica must be RD Primary {0} {1}", reconfigBody, *this);
        return ReconfigurationType::SwapPrimary;
    }

    /*
        The reconfiguration result would only be set on a RD Primary if swap gets aborted
        As soon as the RD Primary local replica state changes the reconfig result would be reset
        In addition, the FM must send the DoReconfig back to the old primary if the new primary goes down
    */
    if (reconfigState_.Result == ReconfigurationResult::AbortSwapPrimary)
    {
        return ReconfigurationType::Other;
    }

    /*
        The only case left to handle is a failed swap where the old primary never receives the initial swap message
        Consider: Replica set is 
    
        1. 0/e0 [N/P RD U] [N/S RD U]
        2. e0/e1 [P/S RD U] [S/P RD U] -> this DoReconfiguration does not arrive at the old primary
        3. e0/e2 [P/P RD U] [S/S RD D] -> this DoReconfiguration arrives at the old primary as the new primary goes down

        At step 3 the replica is a ready primary and is transitioning to a RD primary so it must not do GetLSN
    */

    /*
        The check for reconfigState_.Result is added to fix RDBug 10730785. In following scenario, it satisfies the condistions
        described above but the ReconfigurationType should be Failover instead of Other.

        1. 0/e0 [N/P RD U n1] [N/S RD U n2] [N/S RD U n3] [N/S RD U n4]
        2. e0/e1 [P/S RD U n1] [S/P RD U n2] [S/S RD U n3] [S/S RD U n4] -> This reconfig failed in Phase4_Activate before it can activate n1 since AppHost crashed
        3. e0/e2 [P/P RD U n1] [S/S RD D n2] [S/S RD U n3] [S/S RD U n4] -> In this reconfig, RA on n1 getS all GetLSNReply and based on Deactivation Info,
            it found replica on n3 should be the new primary. It sent ChangeConfiguration to FM. This reconfig completed with result ChangeConfiguration.
        "
        2018-1-17 09:52:46.008	RA.SendFM@2aef1317-62b9-46f0-adef-8e99787d33f3	4752	RA on node n1:131606484171798747 sending message ChangeConfiguration to FM, body:
        2aef1317-62b9-46f0-adef-8e99787d33f3 e0/e2 1
        N/P RD U n1:131606484171798747 131606487555329532:131606487555329533 -1 -1 1.0:1.0:0
        N/S RD U n3:131606484171173721 131606487540203284:131606487540203287 6850 6850 1.0:1.0:0
        N/S RD U n4:131606553122365516 131606546882262452:131606546882262453 -1 -1 1.0:1.0:0
        N/S RD D n2:131606551685421848 131606551905314463:131606551905314464 -1 -1 1.0:1.0:0
        "
        After getting above message, FM send DoReconfiguration to new primary on node n3, which resulted in the next reconfiguration (e0/e3)
        "
        2018-1-17 09:52:46.071	FM.FTUpdate@2aef1317-62b9-46f0-adef-8e99787d33f3	5436	Updated: fabric:/persistedservice1 4 1 2aef1317-62b9-46f0-adef-8e99787d33f3 e0/e3 SP 164 2262 2011 8485 false 2018-01-17 09:52:46.008
        S/P RD U - n3:131606484171173721 131606487540203284:131606487540203287 1 1.0:1.0:0 2018-01-17 09:01:43.387/2018-01-17 09:52:46.008
        P/S RD U - n1:131606484171798747 131606487555329532:131606487555329533 1 1.0:1.0:0 2018-01-17 09:10:04.070/2018-01-17 09:52:46.008
        S/S RD U - n4:131606553122365516 131606546882262452:131606546882262453 1 1.0:1.0:0 2018-01-17 09:35:32.622/2018-01-17 09:52:38.867
        S/S RD D - n2:131606551685421848 131606551905314463:131606551905314464 1 1.0:1.0:0 2018-01-17 09:52:45.211/2018-01-17 09:52:45.211
        Actions: (DoReconfiguration->n3 [n3 S/P] [n1 P/S] [n4 S/S] [n2 S/S] ChangeConfigurationReply->n1 [S_OK] DoReconfiguration->n3 [n3 S/P] [n1 P/S] [n4 S/S] [n2 S/S])
        "
        4. e0/e3 [P/S RD U n1] [S/S RD D n2] [S/P RD U n3] [S/S RD U n4] -> This reconfig failed in Phase2_Catchup due to RAReportFault on node n3
        5. e0/e4 [P/P RD U n1] [S/S RD D n2] [S/S RD D n3] [N/S RD U n4] -> This DoReconfiguration on n1 should not skip Phase1_GetLSN
    
        In last reconfiguration, the type should be Failover, not Other.
        Add this new check is a short term fix. In long term, we should think about other solutions like recording the "actual" replica state during reconfiguration,
        instead using previous reconfiguration result to determine reconfiguration type.
    */
    
    if (reconfigState_.Result != ReconfigurationResult::ChangeConfiguration &&
        LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary &&
        LocalReplica.IsReady &&
        newReplicaDesc.CurrentConfigurationRole == ReplicaRole::Primary)
    {
        return ReconfigurationType::Other;
    }

    return ReconfigurationType::Failover;
}

void FailoverUnit::UpdateRemoteReplicaRoles(
    Reliability::ConfigurationMessageBody const & reconfigBody,
    FailoverUnitEntityExecutionContext & executionContext)
{
    for (auto it = reconfigBody.BeginIterator; it != reconfigBody.EndIterator; ++it)
    {
        auto & replica = GetReplica(it->FederationNodeId);
        if (replica.IsInvalid)
        {
            // Replica not found, add this new replica
            auto & innerReplica = replicaStore_.AddReplica(*it, it->PreviousConfigurationRole, it->CurrentConfigurationRole);
            
            if ((it->CurrentConfigurationRole == ReplicaRole::Primary ||
                it->CurrentConfigurationRole == ReplicaRole::Secondary) &&
                it->State == Reliability::ReplicaStates::InBuild)
            {
                innerReplica.State = ReplicaStates::StandBy;
            }
            else
            {
                innerReplica.State = ReplicaStates::FromReplicaDescriptionState(it->State);
            }

            UpdateEndpoints(innerReplica, *it, executionContext);

            continue;
        }

        if (replicaStore_.IsLocalReplica(replica))
        {
            continue;
        }

        replica.IntermediateConfigurationRole = ReplicaRole::None;
        replica.PreviousConfigurationRole = it->PreviousConfigurationRole;
        replica.CurrentConfigurationRole = it->CurrentConfigurationRole;
    }
}

void FailoverUnit::UpdateLocalReplicaStatesAndRoles(
    Reliability::ReplicaDescription const & newReplicaDesc)
{
    // Update LSN
    auto & replica = LocalReplica;
    replica.TrySetFirstAndLastLSNIfPresentInParameter(newReplicaDesc);

    LocalReplica.PreviousConfigurationRole = newReplicaDesc.PreviousConfigurationRole;
    LocalReplica.CurrentConfigurationRole = newReplicaDesc.CurrentConfigurationRole;
}

void FailoverUnit::UpdateReconfigurationState(
    ReconfigurationType::Enum reconfigType,
    TimeSpan phase0Duration,
    FailoverUnitEntityExecutionContext & executionContext)
{
    reconfigState_.Start(reconfigType, phase0Duration, executionContext.Clock);
}

void FailoverUnit::StartReconfiguration(
    FailoverUnitEntityExecutionContext & executionContext)
{
    endpointPublishState_.StartReconfiguration(executionContext.StateUpdateContextObj);

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase1_GetLSN)
    {
        StartPhase1GetLSN(executionContext);
    }
    else 
    {
        /*
            This can be either swap primary (demote) or a normal P/P reconfig    
        */
        StartPhase2Catchup(executionContext);
    }

    messageRetryActiveFlag_.SetValue(true, executionContext.Queue);
}

FailoverUnit::ReplicaPointerList FailoverUnit::GetReplicasToBeConsideredForGetLSN(bool ignoreUnknownLSN) const
{
    // Helper function to get the list of replicas to be considered during getlsn
    ReplicaPointerList eligibleReplicas;

    for (auto const & it : replicaStore_.ConfigurationReplicas)
    {
        if (it.IsDropped)
        {
            continue;
        }

        if (!it.IsLSNSet())
        {
            continue;
        }

        if (it.IsLSNUnknown() && ignoreUnknownLSN)
        {
            continue;
        }

        eligibleReplicas.push_back(&it);
    }

    return eligibleReplicas;

}

FailoverUnit::ReplicaPointerList FailoverUnit::GetReplicasToBeConsideredForGetLSN() const
{
    /*
        During GetLSN consider all replicas except those that have unknown LSN
        These replicas do not have valid progress and should not be considered unless there is dataloss detected
    */
    return GetReplicasToBeConsideredForGetLSN(true);
}

FailoverUnit::ReplicaPointerList FailoverUnit::GetReplicasToBeConsideredForGetLSNDuringDataLoss() const
{
    /*
        Once data loss has been declared RA will pick the replica with the highest LSN
        This is to reduce the amount of data that has been lost
    */
    return GetReplicasToBeConsideredForGetLSN(false);
}

bool FailoverUnit::DoAllReplicasSupportDeactivationInfo(ReplicaPointerList const & replicas) const
{
    for (auto const & replica : replicas)
    {
        TESTASSERT_IF(replica->IsLSNUnknown(), "Unknown LSN should have been filtered out prior to this check {0} {1}", *replica, *this);
        if (!replica->ReplicaDeactivationInfo.IsValid)
        {
            return false;
        }
    }

    return true;
}

void FailoverUnit::RemoveReplicasWithOldDeactivationEpoch(ReplicaPointerList & replicas) const
{
    // First consider only the replicas that have completed catchup
    vector<Reliability::Epoch> deactivationEpochs;
    for (auto const & it : replicas)
    {
        if (it->GetLastAcknowledgedLSN() >= it->ReplicaDeactivationInfo.CatchupLSN)
        {
            deactivationEpochs.push_back(it->ReplicaDeactivationInfo.DeactivationEpoch);
        }
    }

    if (deactivationEpochs.empty())
    {
        /*
            Only replicas that have not completed catchup are left at this point
        */
        TESTASSERT_IF(
            FailoverConfig::GetConfig().IsDataLossLsnCheckEnabled && !failoverUnitDesc_.IsDataLossBetweenPCAndCC, 
            "Must have data loss as no replica was found with a valid deactivation epoch {0}", 
            *this);

        return;
    }

    auto deactivationEpoch = *max_element(deactivationEpochs.cbegin(), deactivationEpochs.cend(), [](Epoch const & left, Epoch const & right)
    {
        return left.ToPrimaryEpoch() < right.ToPrimaryEpoch();
    });

    auto start = remove_if(replicas.begin(), replicas.end(), [deactivationEpoch](Replica const * replica)
    {
        return replica->ReplicaDeactivationInfo.DeactivationEpoch.ToPrimaryEpoch() < deactivationEpoch.ToPrimaryEpoch();
    });

    replicas.erase(start, replicas.end());
}

int64 FailoverUnit::FindReplicaWithHighestLastLSN(
    ReplicaPointerList const & replicas) const
{
    return (*FindReplicaIteratorWithHighestLastLSN(replicas))->ReplicaId;
}

FailoverUnit::ReplicaPointerList::const_iterator FailoverUnit::FindReplicaIteratorWithHighestLastLSN(
    ReplicaPointerList const & replicas) const
{
    ASSERT_IF(replicas.empty(), "Cannot be empty {0}", *this);

    return max_element(replicas.cbegin(), replicas.cend(), [](Replica const * left, Replica const * right)
    {
        return left->GetLastAcknowledgedLSN() < right->GetLastAcknowledgedLSN();
    });
}

int64 FailoverUnit::FindPrimaryWithBestCatchupCapability(ReplicaPointerList const & replicas) const
{
    // If the local replica is not present then it must have been filtered out because it had false progress
    // If it had false progress RA should be performing change configuration and not continuing with primary election
    // this code below relies on the local replica being the primary
    ASSERT_IF(LocalReplica.ReplicaId != (*replicas.cbegin())->ReplicaId, "Local Replica is not present in the list of replicas to find best based on catchup capability {0}", *this);

    auto candidate = replicas.cbegin();
    int64 lowestLastLSN = (*candidate)->GetLastAcknowledgedLSN();

    auto localReplicaFirstAcknowledegedLSN = LocalReplica.GetFirstAcknowledgedLSN();
    auto localReplicaLastAcknowledegedLSN = LocalReplica.GetLastAcknowledgedLSN();

    // Go throught the list of replicas and determine if there is a better Primary candidate
    for (auto replicaIter = replicas.cbegin() + 1; replicaIter != replicas.cend(); ++replicaIter)
    {
        auto replica = *replicaIter;
        auto replicaLastAcknowledegedLSN = replica->GetLastAcknowledgedLSN();
        auto replicaFirstAcknowledegedLSN = replica->GetFirstAcknowledgedLSN();
        auto candidateFirstAcknowledegedLSN = (*candidate)->GetFirstAcknowledgedLSN();

        if (replicaLastAcknowledegedLSN >= 0 &&
            replica->IsInConfiguration)
        {
            ASSERT_IF(replicaLastAcknowledegedLSN > localReplicaLastAcknowledegedLSN, "cannot have a higher progress replica");

            if (replicaLastAcknowledegedLSN == localReplicaLastAcknowledegedLSN)
            {
                if (replicaFirstAcknowledegedLSN > 0 &&
                    (candidateFirstAcknowledegedLSN == 0 || replicaFirstAcknowledegedLSN < candidateFirstAcknowledegedLSN))
                {
                    candidate = replicaIter;
                }
            }
            else if (replicaLastAcknowledegedLSN < lowestLastLSN)
            {
                lowestLastLSN = replicaLastAcknowledegedLSN;
            }
        }
    }

    bool isFound = ((candidate != replicas.begin()) &&
        (lowestLastLSN < localReplicaLastAcknowledegedLSN) &&
        ((localReplicaFirstAcknowledegedLSN > lowestLastLSN + 1) ||
        (localReplicaFirstAcknowledegedLSN == 0)));

    return isFound ? (*candidate)->ReplicaId : LocalReplicaId;
 }

bool FailoverUnit::TryFindPrimary(int64 & replicaId) const
{
    auto eligibleReplicas = GetReplicasToBeConsideredForGetLSN();
    if (eligibleReplicas.empty())
    {
        /*
            There is no eligible primary
            This can happen if all the replicas in the configuration return Unknown LSN
            For instance: they can be new S/S IB replicas that have not completed build
            and only those replicas are remaining.

            In that case we will consider only the local replica as eligible for primary promotion
            and data loss should already have been declared

            Promote the local replica as primary
        */
        TESTASSERT_IF(
            FailoverConfig::GetConfig().IsDataLossLsnCheckEnabled && !failoverUnitDesc_.IsDataLossBetweenPCAndCC, 
            "Dataloss must have been declared {0}", 
            *this);

        TESTASSERT_IF(!deactivationInfo_.IsDropped, "Deactivation info for local replica must be dropped else it should have been eligible {0}", *this);
        return TryFindPrimaryDuringDataLoss(replicaId);
    }

    if (DoAllReplicasSupportDeactivationInfo(eligibleReplicas) && FailoverConfig::GetConfig().IsDeactivationInfoEnabled)
    {
        RemoveReplicasWithOldDeactivationEpoch(eligibleReplicas);
    }

    if (eligibleReplicas.empty())
    {
        /*
            There was no replica available. This would happen if the combination of replicas left consist of:
            - Replicas that have not completed catchup (and hence are ineligible) in the deactivated epoch
            - Replicas that are new S/S IB replicas that never finished builds

            Fall back to simple LSN based comparison
        */
        return TryFindPrimaryDuringDataLoss(replicaId);
    }

    if (!TryFindPrimary(eligibleReplicas, replicaId))
    {
        /*
            Still could not find a primary
            Example: The local replica is not eligible and
            the only eligible replicas are down
        */
        return false;
    }

    /*
        The check below should only be performed if it is a failover and the current replica should be the primary. 
        If it is a swap it is possible then the replica may not have enough operations in its queue to catchup replicas that are behind
        In such a case, since it is swap and this replica should be the primary disable the optimization below
    */
    TESTASSERT_IF(reconfigState_.ReconfigType == ReconfigurationType::Other, "Cannot be other {0}", *this);
    if (reconfigState_.ReconfigType == ReconfigurationType::Failover && replicaId == LocalReplicaId)
    {
        replicaId = FindPrimaryWithBestCatchupCapability(eligibleReplicas);
    }

    return true;
}

bool FailoverUnit::TryFindPrimaryDuringDataLoss(int64 & primary) const
{
    TESTASSERT_IF(!failoverUnitDesc_.IsDataLossBetweenPCAndCC, "There must be data loss {0}", *this);

    auto eligibleReplicas = GetReplicasToBeConsideredForGetLSNDuringDataLoss();
    TESTASSERT_IF(eligibleReplicas.empty(), "Must have an eligible replica {0}", *this);

    if (eligibleReplicas.empty())
    {
        // Handle the condition anyway - do not assert in production
        primary = LocalReplicaId;
        return true;
    }

    primary = FindReplicaWithHighestLastLSN(eligibleReplicas);
    return true;
}

bool FailoverUnit::TryFindPrimary(ReplicaPointerList const & eligibleReplicas, int64 & primary) const
{
    /*
        Return the first up replica with the highest LSN
        If all replica(s) with the highest LSN are down then 
        return false to not continue and wait here in QL
    */
    auto highestLastLSN = (*FindReplicaIteratorWithHighestLastLSN(eligibleReplicas))->GetLastAcknowledgedLSN();

    auto candidate = find_if(eligibleReplicas.begin(), eligibleReplicas.end(), [highestLastLSN](Replica const * iter)
    {
        return iter->GetLastAcknowledgedLSN() == highestLastLSN && iter->IsUp;
    });

    if (candidate != eligibleReplicas.end())
    {
        primary = (*candidate)->ReplicaId;
        return true;
    }
    else
    {
        primary = -1;
        return false;
    }
}

void FailoverUnit::StartPhase1GetLSN(FailoverUnitEntityExecutionContext & executionContext)
{
    auto & actionQueue = executionContext.Queue;

    TESTASSERT_IF(ReconfigurationStage != FailoverUnitReconfigurationStage::Phase1_GetLSN, "Must be set when reconfig starts {0}", *this);

    UpdateStateAndSendGetLSNMessage(actionQueue);

    CheckReconfigurationProgressAndHealthReportStatus(executionContext);
}

void FailoverUnit::UpdateStateAndSendGetLSNMessage(Infrastructure::StateMachineActionQueue & actionQueue)
{
    bool isSent = false;

    for (auto & replica : replicaStore_.ConfigurationRemoteReplicas)
    {
        if (!replica.IsInConfiguration)
        {
            continue;
        }

        if (replica.IsDropped)
        {
            continue;
        }

        replica.MessageStage = ReplicaMessageStage::RAReplyPending;
        isSent |= SendPhase1GetLSNMessage(replica, actionQueue);
    }

    LocalReplica.MessageStage = ReplicaMessageStage::RAProxyReplyPending;
    isSent |= SendPhase1GetLSNMessage(LocalReplica, actionQueue);

    ArmMessageRetryTimerIfSent(isSent, actionQueue);
}

bool FailoverUnit::SendPhase1GetLSNMessage(Replica const & replica, StateMachineActionQueue & actionQueue) const
{
    if (replica.MessageStage == ReplicaMessageStage::None)
    {
        // GetLSN message is not pending
        return false;
    }

    if (replicaStore_.IsLocalReplica(replica))
    {
        return SendReplicatorGetStatusMessage(actionQueue);
    }
    else
    {
        if (!replica.IsUp)
        {
            return false;
        }

        ReconfigurationAgent::AddActionSendMessageToRA(
            actionQueue,
            RSMessage::GetGetLSN(),
            replica.FederationNodeInstance,
            FailoverUnitDescription,
            ReplicaDescription(replica.FederationNodeInstance, replica.ReplicaId, replica.InstanceId),
            ServiceDescription);

        return true;
    }
}

void FailoverUnit::StartPhase2CatchupOnFailover(
    FailoverUnitEntityExecutionContext & executionContext)
{
    Replica & localReplica = LocalReplica;

    ASSERT_IF(!localReplica.IsLSNSet(), "Cannot be trying to promote to primary further if the local replica did not return LSN {0}", *this);

    // Set the reconfiguration stage
    reconfigState_.StartPhase2Catchup(executionContext.Clock);

    UpdateLocalStateOnPhase2Catchup(executionContext);

    for (auto & replica : replicaStore_.ConfigurationRemoteReplicas)
    {
        replica.MessageStage = ReplicaMessageStage::None;

        TESTASSERT_IF(replica.ToBeRestarted, "ToBeRestarted cannot be true after phase1 getlsn complete {0}\r\n{1}", replica, *this);
        bool isRestartNeeded = IsRemoteReplicaRestartNeededAfterGetLsn(replica);

        // GetLSN can leave LSN in the unknown state. Fix that up
        replica.TryClearUnknownLSN();

        if (isRestartNeeded)
        {
            replica.ToBeRestarted = true;
        }        
    }

    ProcessMsgResends(executionContext);
}

bool FailoverUnit::IsRemoteReplicaRestartNeededAfterGetLsn(Replica const & remoteReplica) const
{
    Replica const & localReplica = LocalReplica;
    bool hasLsn = remoteReplica.IsLSNSet();
    bool hasUnknownLSN = remoteReplica.IsLSNUnknown();

    if (!remoteReplica.IsUp || !remoteReplica.IsInCurrentConfiguration)
    {
        return false;
    }

    if (!remoteReplica.IsReady)
    {
        /*
            Do not restart replica that is IB or SB
            Both these will be built again regardless so an extra restart is not needed
        */
        return false;
    }

    if (!hasLsn)
    {
        // Mark any replica that did not respond to get lsn as ToBeRestarted in the reconfiguration
        // This replica needs to be rebuilt
        return true;
    }

    /*
        Only Up, Ready, CC replicas are being considered for Phase1_Deactivate
        They should be restarted if:
        - They cannot be caught up by the current primary
        - They reported an unknown LSN
    */
    if (hasUnknownLSN)
    {
        return true;
    }
    else if ((localReplica.GetLastAcknowledgedLSN() > remoteReplica.GetLastAcknowledgedLSN()) &&
        ((localReplica.GetFirstAcknowledgedLSN() == 0) ||
            (remoteReplica.GetLastAcknowledgedLSN() < localReplica.GetFirstAcknowledgedLSN() - 1)))
    {
        return true;
    }

    return false;
}

void FailoverUnit::StartPhase2Catchup(
    FailoverUnitEntityExecutionContext & executionContext)
{
    UpdateLocalStateOnPhase2Catchup(executionContext);

    // The state updates have already been performed
    // Simply resend all messages
    ProcessMsgResends(executionContext);
}

void FailoverUnit::UpdateLocalStateOnPhase2Catchup(
    FailoverUnitEntityExecutionContext & executionContext)
{
    Replica & localReplica = LocalReplica;
    retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaChangeRoleAtCatchup);

    localReplica.MessageStage = ReplicaMessageStage::None;
    
    if (failoverUnitDesc_.IsPrimaryChangeBetweenPCAndCC && LocalReplica.IsLSNSet())
    {
        /*
            If the local replica is being promoted to primary and has no deactivation info OR
            If the local replica is being promoted to primary and it's LSN is less than the catchup LSN it has THEN
            Set the deactivation info to the CC Epoch and this replica's LSN

            At this time there is no deactivation info set on any replica and is effectively a new primary being created

            As replicas get built the new deactivation info will be propagated

            Since there was data loss, the deactivation info will get updated to the CCEpoch, LSN returned by the replicator
            In the StateChangedOnDataLoss handler. No build would have been started in the cc epoch as ondataloss was running
            and the replica will go into phase2 deactivate which will rebuild all replicas with the correct deactivation info
        */
        bool isNewReplica = deactivationInfo_.IsDropped;
        bool wasNotCaughtUpAndIsBeingPromotedToPrimary = LocalReplica.GetLastAcknowledgedLSN() < deactivationInfo_.CatchupLSN;

        if (isNewReplica || wasNotCaughtUpAndIsBeingPromotedToPrimary)
        {
            TESTASSERT_IF(
                FailoverConfig::GetConfig().IsDataLossLsnCheckEnabled && !failoverUnitDesc_.IsDataLossBetweenPCAndCC, 
                "There must be data loss {0}", 
                *this);

            deactivationInfo_ = ReplicaDeactivationInfo(CurrentConfigurationEpoch, LocalReplica.GetLastAcknowledgedLSN());
        }
    }

    // Replicator configuration will be updated with the UC message for catchup
    updateReplicatorConfiguration_ = false;

    if (localReplica.IsStandBy)
    {
        // Transition standby replica to inbuild
        // The replica up may be pending and this is equivalent to receiving an AddPrimary/CreateReplica 
        localReplica.State = ReplicaStates::InBuild;
        fmMessageState_.Reset(executionContext.StateUpdateContextObj);
    }
}

bool FailoverUnit::SendUpdateDeactivationInfoMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const
{
    if (replicaStore_.IsLocalReplica(replica))
    {
        return false;
    }

    bool isInBuildAndToBeDeactivated = replica.IsInBuild && replica.ToBeDeactivated;
    if (!isInBuildAndToBeDeactivated)
    {
        return false;
    }

    // Mark the replica as ready (for back compat) and send the update deactivation message
    auto replicaDescList = CreateConfigurationDescriptionListForDeactivateOrActivate(replica.ReplicaId);

    ReconfigurationAgent::AddActionSendDeactivateMessageToRA(
        actionQueue,
        replica.FederationNodeInstance,
        FailoverUnitDescription,
        ServiceDescription,
        move(replicaDescList),
        deactivationInfo_,
        false /*isForce*/);

    return true;
}

bool FailoverUnit::SendReplicaLifeCycleMessage(Replica const & replica, StateMachineActionQueue & actionQueue) const
{
    if (replicaStore_.IsLocalReplica(replica))
    {
        return false;
    }

    Replica const & localReplica = LocalReplica;

    bool isPrimary = localReplica.CurrentConfigurationRole == ReplicaRole::Primary;
    bool isSwapPrimaryInProgress = localReplica.PreviousConfigurationRole == ReplicaRole::Primary &&
        localReplica.CurrentConfigurationRole == ReplicaRole::Secondary;

    if (!isPrimary && !isSwapPrimaryInProgress)
    {
        return false;
    }

    if (!replica.IsInConfiguration)
    {
        return false;
    }

    if (replica.CurrentConfigurationRole == ReplicaRole::Primary)
    {
        // TODO: This can be removed but being kept for compatibility
        return false;
    }

    if (replica.IsUp)
    {
        if (replica.IsInCreate)
        {
            ReconfigurationAgent::AddActionSendCreateReplicaMessageToRA(
                actionQueue,
                replica.FederationNodeInstance,
                FailoverUnitDescription,
                replica.ReplicaDescription,
                ServiceDescription,
                deactivationInfo_);

            return true;
        }
        else if (replica.IsInBuild && !replica.ToBeActivated && !replica.ToBeDeactivated)
        {
            ReconfigurationAgent::AddActionSendMessageToRAProxy(
                actionQueue,
                RAMessage::GetReplicatorBuildIdleReplica(),
                ServiceTypeRegistration,
                failoverUnitDesc_,
                localReplica.ReplicaDescription,
                replica.ReplicaDescription);

            return true;
        }
    }
    else if (replica.ReplicatorRemovePending)
    {
        ReconfigurationAgent::AddActionSendMessageToRAProxy(
            actionQueue,
            RAMessage::GetReplicatorRemoveIdleReplica(),
            ServiceTypeRegistration,
            failoverUnitDesc_,
            localReplica.ReplicaDescription,
            replica.ReplicaDescription);

        return true;
    }

    return false;
}

bool FailoverUnit::SendUpdateConfigurationMessage(StateMachineActionQueue & actionQueue) const
{
    return SendUpdateConfigurationMessage(actionQueue, reconfigState_.IsCatchupReconfigurationStage ? ProxyMessageFlags::Catchup : ProxyMessageFlags::None);
}

bool FailoverUnit::SendUpdateConfigurationMessage(StateMachineActionQueue & actionQueue, ProxyMessageFlags::Enum flags) const
{
    // UC Message needs to be sent for the cases below
    //  a) UpdateReplicatorConfiguration flag is set
    //  b) We are doing S->P
    //  c) We are going from P->P
    auto replicaDescList = CreateReplicationConfigurationReplicaDescriptionList();
    TESTASSERT_IF(replicaDescList.empty(), "Replica description list cannot be empty in SendMessages {0}", *this);
    TESTASSERT_IF(flags != ProxyMessageFlags::Catchup && reconfigState_.IsCatchupReconfigurationStage, "Sending message with incorrect flags {0} {1} \r\n{2}", flags, replicaDescList, *this);

    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        actionQueue,
        RAMessage::GetUpdateConfiguration(),
        ServiceTypeRegistration,
        FailoverUnitDescription,
        LocalReplica.ReplicaDescription,
        move(replicaDescList),
        flags);

    return true;
}

void FailoverUnit::MarkReplicationConfigurationUpdatePendingAndSendMessage(Infrastructure::StateMachineActionQueue & actionQueue)
{
    MarkReplicationConfigurationUpdatePending();
    SendUpdateReplicationConfigurationMessage(actionQueue);
}

bool FailoverUnit::SendUpdateReplicationConfigurationMessage(Infrastructure::StateMachineActionQueue & actionQueue) const
{
    if (!updateReplicatorConfiguration_)
    {
        return false;
    }

    return SendUpdateConfigurationMessage(actionQueue);
}

void FailoverUnit::StartAbortPhase0Demote(FailoverUnitEntityExecutionContext & context)
{
    auto & actionQueue = context.Queue;

    // Set the reconfiguration stage
    reconfigState_.StartAbortPhase0Demote(context.Clock);

    // Attempt to cancel the catchup operation
    SendCancelCatchupMessage(actionQueue);

    // Always retry this
    ArmMessageRetryTimerIfSent(true, actionQueue);
}

bool FailoverUnit::SendRestartRemoteReplicaMessage(Replica const & replica, StateMachineActionQueue & actionQueue) const
{
    if (replicaStore_.IsLocalReplica(replica))
    {
        return false;
    }

    if (!replica.ToBeRestarted)
    {
        return false;
    }

    auto replicaDescList = CreateConfigurationReplicaDescriptionListForRestartRemoteReplica();

    /*
        When using the deactivate message to restart a remote replica
        Set the deactivation info to invalid so that it does not update deactivation epoch
    */
    ReconfigurationAgent::AddActionSendDeactivateMessageToRA(
        actionQueue,
        replica.FederationNodeInstance,
        FailoverUnitDescription,
        ServiceDescription,
        move(replicaDescList),
        Reliability::ReplicaDeactivationInfo(),
        true /*isForce*/);

    return true;
}

void FailoverUnit::StartPhase3Deactivate(FailoverUnitEntityExecutionContext & executionContext)
{
    if (ShouldSkipPhase3Deactivate(executionContext))
    {
        StartPhase4Activate(executionContext);
        return;
    }

    auto & actionQueue = executionContext.Queue;

    reconfigState_.StartPhase3Deactivate(executionContext.Clock);

    IntermediateConfigurationEpoch = CurrentConfigurationEpoch;

    UpdateStateAtPhase3Deactivate(actionQueue);

    ProcessMsgResends(executionContext);

    CheckReconfigurationProgressAndHealthReportStatus(executionContext);
}

void FailoverUnit::UpdateStateAtPhase3Deactivate(Infrastructure::StateMachineActionQueue &)
{
    LocalReplica.IntermediateConfigurationRole = LocalReplica.CurrentConfigurationRole;

    for (auto & replica : replicaStore_.ConfigurationRemoteReplicas)
    {
        replica.IntermediateConfigurationRole = replica.CurrentConfigurationRole;

        if (!replica.IsInPreviousConfiguration)
        {
            continue;
        }

        if (replica.IsDropped)
        {
            continue;
        }

        replica.MessageStage = ReplicaMessageStage::RAReplyPending;
    }
}

bool FailoverUnit::ShouldSkipPhase3Deactivate(FailoverUnitEntityExecutionContext & executionContext) const
{
    if (!executionContext.Config.EnablePhase3Phase4InParallel)
    {
        return false;
    }

    return ShouldSkipPhase3Deactivate(replicaStore_.ConfigurationReplicas);
}

bool FailoverUnit::SendPhase3DeactivateMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const
{
    if (replicaStore_.IsLocalReplica(replica))
    {
        return false;
    }

    if (replica.MessageStage != ReplicaMessageStage::RAReplyPending)
    {
        return false;
    }

    if (!replica.IsUp)
    {
        return false;
    }

    if (replica.IsInCreate)
    {
        return false;
    }

    if (replica.IsInBuild && !replica.ToBeDeactivated)
    {
        return false; // Build is in progress
    }

    if (replica.ToBeRestarted)
    {
        return false;
    }

    /*
        If the replica is IB and ToBeDeactivated is true
        Then build is actually complete and the deactivation is pending
        before it can be added in the replicator view

        For backward compatibility need to send this as RD and not IB
        because the previous behavior used to be to send this replica as RD
    */
    auto replicaDescList = CreateConfigurationDescriptionListForDeactivateOrActivate(replica.ReplicaId);

    ReconfigurationAgent::AddActionSendDeactivateMessageToRA(
        actionQueue,
        replica.FederationNodeInstance,
        FailoverUnitDescription,
        ServiceDescription,
        move(replicaDescList),
        deactivationInfo_,
        false /*isForce*/);

    return true;
}

void FailoverUnit::StartPhase4Activate(FailoverUnitEntityExecutionContext & executionContext)
{
    auto & actionQueue = executionContext.Queue;

    executionContext.UpdateContextObj.EnableUpdate();
    
    /*
        On the primary IC needs to be set to CC in case deactivate is skipped
        and activate is run directly 

        Otherwise, the fm wil consider PC to be the activated epoch and not the cc
        and this can cause issues during rebuild
    */
    IntermediateConfigurationEpoch = CurrentConfigurationEpoch;

    reconfigState_.StartPhase4Activate(executionContext.Clock);

    UpdateStateAtPhase4Activate(actionQueue);

    /*
        In Phase4Active RA needs to send UpdateConfiguration with ER to RAP
        This only happens if this replica is Primary (i.e. P->S replica in swap does not do this)

        The message stage is initially set to RAProxyReplyPending. This implies that no UC with ER has been ack'd by RAP

        Once all Up+Ready replicas are activated then the flag marking this is set to true
        After this RA can send the flag with ER (which will tell RAP to grant write status)
        Once a reply with ER is received then the message stage is cleared. This implies that UC with ER has been received.

        Any changes in the replication configuration cause updateReplicatorConfiguration_ to be set

        To complete Phase4Activate:
        - the status must be complete (i.e. all remote replicas are activated)
        - the local replica message stage must be None (i.e. one UC with ER has been ack'd)
        - the replicator configuration must be updated (i.e. updateReplicatorConfig_ is false)
    */
    if (LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary)
    {
        LocalReplica.MessageStage = ReplicaMessageStage::RAProxyReplyPending;
    }

    ProcessMsgResends(executionContext);

    CheckReconfigurationProgressAndHealthReportStatus(executionContext);
}

void FailoverUnit::UpdateStateAtPhase4Activate(StateMachineActionQueue &)
{
    LocalReplica.IntermediateConfigurationRole = LocalReplica.CurrentConfigurationRole;

    for (auto & replica : replicaStore_.ConfigurationRemoteReplicas)
    {
        // reset message stage of both pc and cc
        replica.MessageStage = ReplicaMessageStage::None;
        replica.IntermediateConfigurationRole = replica.CurrentConfigurationRole;

        // Send Activate message to replicas in CC
        if (!replica.IsInCurrentConfiguration)
        {
            continue;
        }

        if (replica.IsDropped)
        {
            continue;
        }

        /*
            Transition replicas that are IB and waiting for deactivation epoch to be updated
            to replicas that are IB and waiting for activation to complete
        */
        if (replica.IsInBuild && replica.ToBeDeactivated)
        {
            replica.ToBeDeactivated = false;
            replica.ToBeActivated = true;
        }

        replica.MessageStage = ReplicaMessageStage::RAReplyPending;
    }
}

bool FailoverUnit::AreAllUpReadyReplicasActivated() const
{      
    return  CheckPhase4ActivateProgress() != ReconfigurationProgressStages::Phase4_UpReadyReplicasPending;
}

bool FailoverUnit::IsEndReconfigurationMessagePending(bool areAllUpReadyReplicasActivated) const
{
    if (LocalReplica.CurrentConfigurationRole != ReplicaRole::Primary)
    {
        return false;
    }

    /*
        Send a message if:
        - ER message has never been ack'd and all up ready replicas are activated
        - Replicator config update is pending
    */
    if (updateReplicatorConfiguration_)
    {
        return true;
    }

    if (LocalReplica.MessageStage == ReplicaMessageStage::RAProxyReplyPending &&
        areAllUpReadyReplicasActivated)
    {
        return true;
    }

    return false;
}

bool FailoverUnit::SendEndReconfigurationMessage(Infrastructure::StateMachineActionQueue & actionQueue) const
{
    auto areAllUpReadyReplicasActivated = AreAllUpReadyReplicasActivated();
    if (!IsEndReconfigurationMessagePending(areAllUpReadyReplicasActivated))
    {
        return false;
    }

    auto flags = areAllUpReadyReplicasActivated ? ProxyMessageFlags::EndReconfiguration : ProxyMessageFlags::None;
    SendUpdateConfigurationMessage(actionQueue, flags);
    return true;
}

bool FailoverUnit::SendActivateMessage(Replica const & replica, StateMachineActionQueue & queue) const
{
    if (replicaStore_.IsLocalReplica(replica))
    {
        // Can only activate remote replicas
        return false;
    }

    if (!replica.IsInCurrentConfiguration)
    {
        return false;
    }

    if (replica.MessageStage != ReplicaMessageStage::RAReplyPending)
    {
        return false;
    }

    if (!replica.IsUp)
    {
        return false;
    }

    if (replica.IsInCreate)
    {
        return false;
    }

    if (replica.IsInBuild && !replica.ToBeActivated)
    {
        return false;
    }

    if (replica.ToBeRestarted)
    {
        return false;
    }

    auto replicaDescriptionList = CreateConfigurationDescriptionListForDeactivateOrActivate(replica.ReplicaId);
    ReconfigurationAgent::AddActionSendActivateMessageToRA(
        queue,
        replica.FederationNodeInstance,
        FailoverUnitDescription,
        ServiceDescription,
        move(replicaDescriptionList),
        deactivationInfo_);

    return true;
}

void FailoverUnit::FinishPhase1GetLSN(
    int64 primaryReplicaId,
    FailoverUnitEntityExecutionContext & executionContext)
{
    ASSERT_IF(primaryReplicaId == -1, "Phase1 completed and did not find a primary {0}", *this);

    executionContext.UpdateContextObj.EnableUpdate();

    auto & actionQueue = executionContext.Queue;

    if (!LocalReplica.IsLSNSet())
    {
        /*
            GetLSN is complete so either data loss has been declared or a read quorum has replied
            If the local replica has not reported its LSN then fault it because it is unlikely that it will
            be able to become a primary
        */
        auto closeMode = HasPersistedState ? ReplicaCloseMode::Restart : ReplicaCloseMode::Drop;
        TESTASSERT_IF(!CanCloseLocalReplica(closeMode), "Must be able to restart/drop here");
        StartCloseLocalReplica(closeMode, ReconfigurationAgent::InvalidNode, executionContext, ActivityDescription::Empty);
        SendReplicaCloseMessage(executionContext.Queue);
        return;
    }

    if (primaryReplicaId != LocalReplicaId)
    {
        TESTASSERT_IF(reconfigState_.ReconfigType == ReconfigurationType::SwapPrimary, "Changing config during swap {0}", *this);

        changeConfigFailoverUnitDesc_ = FailoverUnitDescription;
        changeConfigReplicaDescList_ = CreateChangeConfigurationReplicaDescriptionList(primaryReplicaId);

        OnPhaseChanged(executionContext.Queue);

        SendChangeConfiguration(actionQueue);
        
        reconfigState_.FinishWithChangeConfiguration(executionContext.NodeInstance, executionContext.Clock, executionContext.Queue);

        // Revert the configuration
        RevertConfiguration();
    }
    else
    {
        OnPhaseChanged(executionContext.Queue);
        // If not, then start next reconfiguration stage
        StartPhase2CatchupOnFailover(executionContext);
    }
}

void FailoverUnit::FinishSwapPrimary(FailoverUnitEntityExecutionContext & executionContext)
{
    executionContext.UpdateContextObj.EnableUpdate();

    reconfigState_.FinishDemote(executionContext.Clock);

    for (auto & it : replicaStore_.ConfigurationRemoteReplicas)
    {
        TESTASSERT_IF(it.MessageStage != ReplicaMessageStage::None, "Message stage cannot be set at finish swap {0} {1}", it, *this);
        
        // ToBeActivated should not be set because FM should not have started a swap with S IB
        TESTASSERT_IF(it.ToBeActivated, "ToBeActivated should not be set during swap as FM cannot have started swap with S IB and S/S IB replicas in reconfig during phase 0 will not be activated until phase 4 {0} {1}", it, *this);

        /*
            It is possible that some replica is still ToBeDeactivated and swap completes
            Consider [P/S] [S/S IB] [S/S IB] [S/P]
            Now build completes for one replica which then gets deactivation epoch updated (TODO: No need for deactivation epoch update in swap during phase 0)
            This leads to [P/S] [S/S RD] [S/S IB] [S/P]
            Now build completes for other replica
            [P/S] [S/S RD] [S/S IB ToBeDeactivated] [S/P]
            Now if catchup quorum and swap completes the replica will stay ToBeDeactivated and the reconfig stage will transition to None which is invalid
        */

        it.ToBeDeactivated = false;
    }
}

void FailoverUnit::FinishPhase3Deactivate(
    FailoverUnitEntityExecutionContext & executionContext)
{
    OnPhaseChanged(executionContext.Queue);
    StartPhase4Activate(executionContext);
}

void FailoverUnit::ReplicatorConfigurationUpdateAfterDeactivateCompleted(
    FailoverUnitEntityExecutionContext & executionContext)
{
    LocalReplica.MessageStage = ReplicaMessageStage::None;

    CheckReconfigurationProgressAndHealthReportStatus(executionContext);
}

void FailoverUnit::UpdateReplicaSetCountsAtPhase1GetLSN(
    Replica const & replica,
    bool isInConfiguration,
    ReplicaSetCountsAtGetLsn & counter) const
{
    if (!isInConfiguration)
    {
        return;
    }

    counter.ReplicaCount++;

    // The rest of this function works on whether the replica is dropped, isUp and if the lsn has been received
    if (replica.IsDropped)
    {
        return;
    }

    
    TESTASSERT_IF(!replica.IsLSNSet() && replica.IsLSNUnknown(), "Unknown LSN on a replica does mean that the LSN has been set {0}\r\n{1}", replica, *this);
    if (replica.IsLSNSet() && !replica.IsLSNUnknown())
    {
        /* Consider all replicas for which GetLSN is complete
           The replica may have gone down after GetLSN is complete but that is ok
           It would only cause catchup to get stuck
        */
        counter.CompletedCount++;
    }
    else if (replica.IsLSNSet() && replica.IsLSNUnknown())
    {
        // replicas that report unknown LSN are considered as dropped
    }
    else
    {
        if (replica.IsUp)
        {
            counter.UpWaitingCount++;
        }
        else
        {
            counter.DownWaitingCount++;
        }
    }
}

ReconfigurationProgressStages::Enum FailoverUnit::CheckPhase1GetLSNProgress(int64 & primaryReplicaId, FailoverUnitEntityExecutionContext & executionContext)
{
    primaryReplicaId = -1;

    auto & actionQueue = executionContext.Queue;

    ReplicaSetCountsAtGetLsn pcCounter;
    ReplicaSetCountsAtGetLsn ccCounter;

    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        UpdateReplicaSetCountsAtPhase1GetLSN(replica, replica.IsInPreviousConfiguration, pcCounter);

        UpdateReplicaSetCountsAtPhase1GetLSN(replica, replica.IsInCurrentConfiguration, ccCounter);
    }

    // If getlsn is still pending from up replicas and the duration has not expired then continue to wait
    bool hasGetLsnWaitDurationExpired = reconfigState_.GetCurrentReconfigurationDuration(executionContext.Clock) >= executionContext.Config.RemoteReplicaProgressQueryWaitDuration;
    if (!hasGetLsnWaitDurationExpired && (pcCounter.UpWaitingCount > 0 || ccCounter.UpWaitingCount > 0))
    {        
        return ReconfigurationProgressStages::Enum::Phase1_WaitingForReadQuorum;
    }

    // If either pc or cc are below quorum and there is a replica for which ra is waiting then continue the wait
    if ((pcCounter.IsBelowReadQuorum && pcCounter.WaitingCount > 0) || (ccCounter.IsBelowReadQuorum && ccCounter.WaitingCount > 0))
    {
        return ReconfigurationProgressStages::Enum::Phase1_WaitingForReadQuorum;
    }

    if(CheckDataLossAtPhase1GetLSN(pcCounter, ccCounter, actionQueue))
    {
        // Data loss was detected. Wait until datalossreport exchange is complete
        return ReconfigurationProgressStages::Enum::Phase1_DataLoss;
    }

    if (TryFindPrimary(primaryReplicaId))
    {
        return ReconfigurationProgressStages::Enum::CurrentPhaseFinished;
    } 
    else
    {
        return ReconfigurationProgressStages::Enum::Phase1_WaitingForReadQuorum;
    }
}

bool FailoverUnit::CheckDataLossAtPhase1GetLSN(
    ReplicaSetCountsAtGetLsn const & pcCounter, 
    ReplicaSetCountsAtGetLsn const & ccCounter, 
    Infrastructure::StateMachineActionQueue & actionQueue)
{
    if (reconfigState_.ReconfigType == ReconfigurationType::SwapPrimary)
    {
        /*
            There can be no data loss if the DoReconfiguration message was forwarded by the previous primary
            This is because prior to forwarding the DoReconfiguration message the previous primary would
            complete Catchup Quorum and the quorum that is caught up includes this replica.

            Thus, this replica has all the data so even if a read quorum of PC is not available anymore
            it is safe to proceed
        */
        return false;
    }

    int64 ccDataLossVersion = CurrentConfigurationEpoch.DataLossVersion;
    int64 pcDataLossVersion = PreviousConfigurationEpoch.DataLossVersion;

    if ((pcCounter.IsBelowReadQuorum && (ccDataLossVersion == pcDataLossVersion)) ||
        (ccCounter.IsBelowReadQuorum && (dataLossVersionToReport_ == 0 || ccDataLossVersion <= dataLossVersionToReport_)))
    {
        dataLossVersionToReport_ = ccDataLossVersion;

        SendDataLossReportMessage(actionQueue);

        return true;
    }

    return false;
}

bool FailoverUnit::ShouldSkipPhase3Deactivate(ReplicaStore::ConfigurationReplicaStore const & replicas)
{
    /*
        Phase3 and Phase4 can be performed in parallel when all write quorums of PC overlap
        with all write quorums of CC (which guarantees that in rebuild if any configuration is deactivated 
        the deactivation will be discovered)

        The above translates to the following conditions:
        
        - PC = CC
        Example: Swap primary, replicas going down at mrss etc
    
        - |PC|=|CC|=n, n is even, |PC ∩ CC|=n −1
        Example: [P/P] [S/I] [I/S] [S/S] [S/S]
    
        - PC ⊂ CC, |CC − PC|=1
        Any upshift where one replica is being added
    
        - PC ⊂ CC, |CC − PC|=2, |PC|is even
        Any upshift where PC is even and two replicas are being added
    
        - CC ⊂ PC, |PC − CC|=1
        Any downshift where one replica is being removed
    
        - CC ⊂ PC, |PC − CC|=2, |CC|is even
        Any downshift where PC is even and two replicas are being removed

        At this time, the state machine does not support in parallel phase3 and phase4 so only consider
        the first optimization where PC and CC are exactly the same
    */
    return all_of(replicas.begin(), replicas.end(), [](Replica const & r) { return r.IsInPreviousConfiguration && r.IsInCurrentConfiguration; });
}

ReconfigurationProgressStages::Enum FailoverUnit::CheckPhase3DeactivateProgress() const
{
    int pcUpReplicaWaitingCount = 0;
    int pcCompletedCount = 0;
    int pcReplicaCount = 0;

    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        if (replica.IsInPreviousConfiguration)
        {
            pcReplicaCount++;

            if (replica.MessageStage == ReplicaMessageStage::None || replica.IsDropped)
            {
                pcCompletedCount++;
            }
            else if (replica.IsUp && !replica.IsBuildInProgress && !replica.ToBeRestarted)
            {
                pcUpReplicaWaitingCount++;
            }
        }
    }

    bool pcBelowReadQuorum = pcCompletedCount < ((pcReplicaCount + 1) / 2);

    if (pcBelowReadQuorum)
    {
        return ReconfigurationProgressStages::Phase3_PCBelowReadQuorum;
    }
    else if (pcUpReplicaWaitingCount != 0)
    {
        return ReconfigurationProgressStages::Phase3_WaitingForReplicas;
    }
    else
    {
        return ReconfigurationProgressStages::CurrentPhaseFinished;
    }
}

ReconfigurationProgressStages::Enum FailoverUnit::CheckPhase4ActivateProgress() const
{
    /*
    Local Replica will have its message stage set to RAP
    Remote replicas will have message stage set to RA

    Replicas for which it is complete will have message stage cleared

    Wait for isUpdateReplicationConfiguration as well
    */
    bool upReadyReplicaPending = false;
    bool upReplicaPending = false;
    bool replicatorRemovePending = false;
    bool restartPending = false;
    bool isInBuild = false;

    for (auto const & replica : replicaStore_.ConfigurationRemoteReplicas)
    {
        if (replica.ToBeRestarted)
        {
            restartPending = true;
            continue;
        }

        if (replica.ReplicatorRemovePending)
        {
            replicatorRemovePending = true;
            continue;
        }

        if (!replica.IsInCurrentConfiguration)
        {
            continue;
        }

        if (replica.MessageStage == ReplicaMessageStage::None)
        {
            continue;
        }

        if (replica.IsInBuild && replica.IsUp)
        {
            isInBuild = true;
        }

        if (replica.IsUp && replica.IsReady)
        {
            upReadyReplicaPending = true;
        }
        else if (replica.IsUp)
        {
            upReplicaPending = true;
        }
    }

    if (upReadyReplicaPending)
    {
        return ReconfigurationProgressStages::Phase4_UpReadyReplicasPending;
    }
    else if (isInBuild)
    {
        return ReconfigurationProgressStages::Phase4_ReplicaStuckIB;
    }
    else if (restartPending)
    {
        return ReconfigurationProgressStages::Phase4_ReplicaPendingRestart;
    }
    else if (LocalReplica.MessageStage != ReplicaMessageStage::None)
    {
        return ReconfigurationProgressStages::Phase4_LocalReplicaNotReplied;
    }
    else if (IsUpdateReplicatorConfiguration)
    {
        return ReconfigurationProgressStages::Phase4_ReplicatorConfigurationUpdatePending;
    }
    else if (upReplicaPending || replicatorRemovePending)
    {
        return ReconfigurationProgressStages::Phase4_UpReadyReplicasActivated;
    }
    else
    {
        return ReconfigurationProgressStages::CurrentPhaseFinished;
    }
}

void FailoverUnit::OnReconfigurationPhaseHealthReport(Infrastructure::StateMachineActionQueue & queue, ReconfigurationStuckDescriptorSPtr const & newReport)
{
    reconfigHealthState_.OnReconfigurationPhaseHealthReport(
        queue,
        newReport);
}

void FailoverUnit::OnPhaseChanged(Infrastructure::StateMachineActionQueue & queue)
{
    reconfigHealthState_.OnPhaseChanged(
        queue, 
        reconfigHealthDescriptorFactory_.CreateReconfigurationStuckHealthReportDescriptor());
}

void FailoverUnit::FinishPhase4Activate(FailoverUnitEntityExecutionContext & executionContext)
{
    OnPhaseChanged(executionContext.Queue);

    executionContext.UpdateContextObj.EnableUpdate();

    CompleteReconfiguration(executionContext);

    SendDoReconfigurationReply(executionContext);
}

void FailoverUnit::CheckReconfigurationProgressAndHealthReportStatus(FailoverUnitEntityExecutionContext & executionContext)
{
    auto currentStage = CheckReconfigurationProgress(executionContext);
    if (currentStage == ReconfigurationProgressStages::CurrentPhaseFinished)
    {
        return;
    }

    auto currentPhaseTime = reconfigState_.GetCurrentReconfigurationPhaseTimeElapsed(executionContext.Clock);
    bool shouldReportHealth = currentPhaseTime > executionContext.Config.ReconfigurationHealthReportThreshold;
    bool shouldTraceApiSlow = currentPhaseTime > executionContext.Config.ServiceReconfigurationApiTraceWarningThreshold;

    if (!shouldReportHealth && !shouldTraceApiSlow)
    {
        return;
    }

    auto reportDescriptor = reconfigHealthDescriptorFactory_.CreateReconfigurationStuckHealthReportDescriptor(
            currentStage,
            replicaStore_.ConfigurationReplicas,
            reconfigState_.StartTime,
            reconfigState_.PhaseStartTime);

    if (shouldReportHealth)
    {
        OnReconfigurationPhaseHealthReport(executionContext.Queue, reportDescriptor);
    }

    if (shouldTraceApiSlow)
    {
        ReconfigurationSlowEventData data(failoverUnitDesc_.FailoverUnitId.Guid, executionContext.NodeInstance, wformatString(ReconfigurationStage), reportDescriptor->GenerateReportDescriptionInternal());
        auto action = make_unique<TraceEventStateMachineAction<ReconfigurationSlowEventData>>(move(data));
        executionContext.Queue.Enqueue(move(action));
    }
}

ReconfigurationProgressStages::Enum FailoverUnit::CheckReconfigurationProgress(
    FailoverUnitEntityExecutionContext & executionContext)
{    
    // Check the reconfiguration stage has been completed
    ReconfigurationProgressStages::Enum phaseProgressStatus(ReconfigurationProgressStages::Invalid);
    auto & actionQueue = executionContext.Queue;
    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        {
            int64 primaryReplicaId = -1;
            phaseProgressStatus = CheckPhase1GetLSNProgress(primaryReplicaId, executionContext);
            if (phaseProgressStatus == ReconfigurationProgressStages::CurrentPhaseFinished)
            {
                FinishPhase1GetLSN(primaryReplicaId, executionContext);
            }
        }
        break;
    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase0_Demote:
        phaseProgressStatus = ReconfigurationProgressStages::Phase0_NoReplyFromRAP;
        break;

    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        phaseProgressStatus = ReconfigurationProgressStages::Phase2_NoReplyFromRap;
        break;

    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        phaseProgressStatus = CheckPhase3DeactivateProgress();
        if (phaseProgressStatus == ReconfigurationProgressStages::CurrentPhaseFinished)
        {
            FinishPhase3Deactivate(executionContext);
        }
        break;

    case FailoverUnitReconfigurationStage::Phase4_Activate:
        {
            phaseProgressStatus = CheckPhase4ActivateProgress();
            if (phaseProgressStatus == ReconfigurationProgressStages::CurrentPhaseFinished)
            {
                FinishPhase4Activate(executionContext);
            }
            else if (phaseProgressStatus != ReconfigurationProgressStages::Phase4_UpReadyReplicasPending)
            {
                if (LocalReplica.MessageStage == ReplicaMessageStage::RAProxyReplyPending)
                {
                    // Start sending EndReconfiguration message
                    SendEndReconfigurationMessage(actionQueue);
                }
            }
        }
        break;

    default:
        {
            Assert::CodingError(
                "Invalid reconfiguration stage during progress check: {0}", *(this));
        }
        break;
    }

    ASSERT_IF(phaseProgressStatus == ReconfigurationProgressStages::Enum::Invalid, "Invalid reconfiguration  phase progress state during progress check: {0}", *this);
        
    return phaseProgressStatus;
}

void FailoverUnit::ResetReconfigurationStates()
{
    // Reset PC version
    PreviousConfigurationEpoch = Epoch::InvalidEpoch();
    IntermediateConfigurationEpoch = Epoch::InvalidEpoch();

    // Reset state of replicas.
    vector<int64> replicasToRemove;
    for (auto & replica : replicaStore_.ConfigurationReplicas)
    {
        replica.PreviousConfigurationRole = ReplicaRole::None;
        replica.IntermediateConfigurationRole = ReplicaRole::None;

        replica.ClearLSN();
        replica.MessageStage = ReplicaMessageStage::None;

        /*
            The logic below identifies what replicas need to be removed from ra view
            At this point reconfiguration is complete     
            Remove all replicas that are not in CC
            - This means all X/N replicas 
            - Idle replicas need to be handled in a special case
              If the replica is I RD U then it needs to be moved into the Idle list (kept as N/I RD U) 

            All other replicas need to be removed
        */
        if (replicaStore_.IsLocalReplica(replica) || replica.IsInCurrentConfiguration)
        {
            continue;
        }

        if (replica.CurrentConfigurationRole == ReplicaRole::Idle && replica.IsReady && replica.IsUp)
        {
            continue;
        }

        replicasToRemove.push_back(replica.ReplicaId);
    }

    replicaStore_.Remove(replicasToRemove);
}

void FailoverUnit::CompleteReconfiguration(FailoverUnitEntityExecutionContext & executionContext)
{
    ResetReconfigurationStates();

    reconfigState_.Finish(executionContext.NodeInstance, executionContext.Clock, executionContext.Queue);

    endpointPublishState_.Clear(executionContext.StateUpdateContextObj);
}

void FailoverUnit::CopyCCToPC()
{
    if (PreviousConfigurationEpoch.IsValid() || !LocalReplica.IsInCurrentConfiguration)
    {
        return;
    }

    PreviousConfigurationEpoch = CurrentConfigurationEpoch;

    for (auto & it : replicaStore_.ConfigurationReplicas)
    {
        it.PreviousConfigurationRole = it.CurrentConfigurationRole;
    }
}

bool FailoverUnit::SendCancelCatchupMessage(StateMachineActionQueue & actionQueue) const
{
    Replica const & localReplica = LocalReplica;

    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        actionQueue,
        RAMessage::GetCancelCatchupReplicaSet(),
        ServiceTypeRegistration,
        FailoverUnitDescription,
        localReplica.ReplicaDescription);

    // We always resend this message
    return true;
}

void FailoverUnit::SendChangeConfiguration(StateMachineActionQueue & actionQueue) const
{
    ASSERT_IFNOT(
        changeConfigReplicaDescList_.size() > 0,
        "Invalid change configuration replica description list, fudesc:{0} list:{1}",
        changeConfigFailoverUnitDesc_, changeConfigReplicaDescList_);

    vector<Reliability::ReplicaDescription> tmpList = changeConfigReplicaDescList_;
    ConfigurationMessageBody body(changeConfigFailoverUnitDesc_, ServiceDescription, move(tmpList));

    TransportUtility.AddSendMessageToFMAction(actionQueue, RSMessage::GetChangeConfiguration(), move(body));
}

bool FailoverUnit::SendDataLossReportMessage(Infrastructure::StateMachineActionQueue & actionQueue) const
{
    if (CurrentConfigurationEpoch.DataLossVersion > dataLossVersionToReport_)
    {
        return false;
    }

    // Send reconfiguration completed message
    auto replicaDescList = CreateCurrentConfigurationReplicaDescriptionList();

    ConfigurationMessageBody body(FailoverUnitDescription, ServiceDescription, move(replicaDescList));

    TransportUtility.AddSendMessageToFMAction(actionQueue, RSMessage::GetDatalossReport(), move(body));

    return true;
}

void FailoverUnit::SendDoReconfigurationReply(
    FailoverUnitEntityExecutionContext const & executionContext) const
{
    auto replicaDescList = CreateCurrentConfigurationReplicaDescriptionList();

    ConfigurationReplyMessageBody body(FailoverUnitDescription, move(replicaDescList), ErrorCode::Success());

    TransportUtility.AddSendMessageToFMAction(executionContext.Queue, RSMessage::GetDoReconfigurationReply(), move(body));
}

void FailoverUnit::SendContinueSwapPrimaryMessage(
    DoReconfigurationMessageBody const * incomingMessage,
    FailoverUnitEntityExecutionContext const & context) const
{
    vector<ReplicaDescription> replicas;
    Reliability::FailoverUnitDescription const * ftDesc = nullptr;
    Reliability::ServiceDescription const * serviceDesc = nullptr;

    if (incomingMessage == nullptr)
    {
        replicas = CreateReplicationConfigurationReplicaDescriptionList();
        ftDesc = &failoverUnitDesc_;
        serviceDesc = &serviceDesc_;
    }
    else
    {
        // TODO: Merge with the RA state and send an updated DoReconfig message
        replicas = incomingMessage->ReplicaDescriptions;
        ftDesc = &incomingMessage->FailoverUnitDescription;
        serviceDesc = &incomingMessage->ServiceDescription;
    }

    auto phase0Duration = reconfigState_.Phase0Duration;
    TESTASSERT_IF(phase0Duration == TimeSpan::Zero, "Phase0 duration should be set here {0}", *this);
    if (phase0Duration == TimeSpan::Zero)
    {
        phase0Duration = TimeSpan::MaxValue;
    }

    auto it = find_if(replicas.cbegin(), replicas.cend(), [](ReplicaDescription const & r) { return r.CurrentConfigurationRole == ReplicaRole::Primary; });
    if (it == replicas.cend() || it->FederationNodeId == LocalReplica.FederationNodeId)
    {
        // Assert in test and drop the continue message
        Assert::TestAssert("Did not find appropriate replica to send message to {0} {1} {2}", replicas, ftDesc, *this);
        return;
    }

    ReconfigurationAgent::AddActionSendContinueSwapPrimaryMessageToRA(
        context.Queue, 
        it->FederationNodeInstance, 
        *ftDesc, 
        *serviceDesc, 
        move(replicas),
        phase0Duration);
}

void FailoverUnit::RevertConfiguration()
{
    /*
        At the point of revert configuration the partition is either in phase1 
        and change config has happend or it is in phase0 and swap has been aborted

        The former case is more interesting because prior to the reconfiguration starting
        this replica could have been a ready secondary and was the primary elect
        This would cause its CC role to be primary and state to be RD making it a RD Primary

        When change configuration is declared the reconfig is completed. At this time this replica
        is still RD Primary which is incorrect.

        RevertConfiguration will mark CC Role as the PC Role 
        This needs to be done for all replicas because otherwise may end up with multiple replicas

        IN addition, there could be replicas with PC role as I (I/S)
        Marking the CC Role as I would make these I/N/I which makes no sense so remove these replicas
     */
    vector<int64> replicasToRemove;
    for (auto & replica : replicaStore_.ConfigurationReplicas)
    {
        replica.CurrentConfigurationRole = replica.PreviousConfigurationRole;
        replica.ClearLSN();
        replica.MessageStage = ReplicaMessageStage::None;

        if (!replicaStore_.IsLocalReplica(replica) && replica.PreviousConfigurationRole == ReplicaRole::Idle)
        {
            replicasToRemove.push_back(replica.ReplicaId);
        }
    }

    replicaStore_.Remove(replicasToRemove);
}

void FailoverUnit::UpdateEndpoints(
    Replica & replica,
    Reliability::ReplicaDescription const & newReplicaDesc,
    FailoverUnitEntityExecutionContext & executionContext)
{
    UpdateServiceEndpoint(replica, newReplicaDesc, executionContext);
    UpdateReplicatorEndpoint(replica, newReplicaDesc);
}

void FailoverUnit::UpdateServiceEndpoint(
    Replica & replica,
    Reliability::ReplicaDescription const & newReplicaDesc,
    FailoverUnitEntityExecutionContext & executionContext)
{
    UpdateServiceEndpointInternal(replica, newReplicaDesc);

    if (!replicaStore_.IsLocalReplica(replica))
    {
        return;
    }

    /*
        mark endpoint update to FM as pending only during S->P and phase2 catchup

        there are several cases being handled here:
        - replica opens and changes role { CR(P), CR(I), Stateless open }:
        No need to send endpoint update

        - replica transitions to secondary as part of reconfig { CR(P->S) ) CR(I->S) }:
        No need to send endpoint update as FM doesn't care about secondary endpoing

        - replica transitions to primary as part of swap complete { CR(S->P) }:
        No need to send endpoint update as not much other work is pending anywhere
        and phase4 activate completion will send doreconfigreply

        - replica transitions to primary as part of promote to primary { CR(X->P) }:
        mark the endpoint update as pending and send the message if the reconfig has already taken a long time

        the other consideration is that both UC with catchup completing and the replica endpoint update reply from RAP
        must in the last case set the value to true and send the message
    */

    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase2_Catchup &&
        FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC &&
        LocalReplicaRole == ReplicaRole::Primary)
    {
        if (endpointPublishState_.OnEndpointUpdated(executionContext.Clock, executionContext.StateUpdateContextObj))
        {
            SendPublishEndpointMessage(executionContext);
        }
    }
}

void FailoverUnit::UpdateServiceEndpointInternal(
    Replica & replica,
    ReplicaDescription const & replicaDesc)
{
    if (replicaDesc.ServiceLocation != replica.ServiceLocation)
    {
        RAEventSource::Events->ServiceLocationUpdate(
            FailoverUnitId.Guid,
            LocalReplica.FederationNodeInstance,
            replica.ReplicaId,
            replica.InstanceId,
            replicaDesc.ServiceLocation,
            replica.ServiceLocation);

        replica.ServiceLocation = replicaDesc.ServiceLocation;
    }
}

void FailoverUnit::UpdateReplicatorEndpoint(
    Replica & replica,
    Reliability::ReplicaDescription const & newReplicaDesc)
{
    if (replica.ReplicationEndpoint != newReplicaDesc.ReplicationEndpoint)
    {
        RAEventSource::Events->ReplicatorLocationUpdate(
            FailoverUnitId.Guid,
            LocalReplica.FederationNodeInstance,
            replica.ReplicaId,
            replica.InstanceId,
            newReplicaDesc.ReplicationEndpoint,
            replica.ReplicationEndpoint);

        replica.ReplicationEndpoint = newReplicaDesc.ReplicationEndpoint;
    }
}

bool FailoverUnit::ProcessReplicaMessageResend(Replica const & replica, StateMachineActionQueue & actionQueue) const
{
    bool isSent = false;

    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        isSent |= SendPhase1GetLSNMessage(replica, actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        isSent |= SendReplicaLifeCycleMessage(replica, actionQueue);
        isSent |= SendUpdateDeactivationInfoMessage(replica, actionQueue);
        isSent |= SendRestartRemoteReplicaMessage(replica, actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        isSent |= SendReplicaLifeCycleMessage(replica, actionQueue);
        isSent |= SendPhase3DeactivateMessage(replica, actionQueue);
        isSent |= SendRestartRemoteReplicaMessage(replica, actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Phase4_Activate:
        isSent |= SendReplicaLifeCycleMessage(replica, actionQueue);
        isSent |= SendActivateMessage(replica, actionQueue);
        isSent |= SendRestartRemoteReplicaMessage(replica, actionQueue);
        break;

    case FailoverUnitReconfigurationStage::None:
        isSent |= SendActivateMessage(replica, actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
        break;

    default:
        Assert::CodingError("Unknown reconfig stage {0}", *this);
    }

    return isSent;
}

bool FailoverUnit::ProcessReplicaMessageResends(ReconfigurationAgentComponent::Infrastructure::StateMachineActionQueue & actionQueue) const
{
    bool isSent = false;

    for (auto const & replica : replicaStore_)
    {
        isSent |= ProcessReplicaMessageResend(replica, actionQueue);
    }

    return isSent;
}

bool FailoverUnit::ProcessPhaseMessageResends(ReconfigurationAgentComponent::Infrastructure::StateMachineActionQueue & actionQueue) const
{
    bool isSent = false;

    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        break;

    case FailoverUnitReconfigurationStage::None:
        isSent |= SendUpdateReplicationConfigurationMessage(actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        isSent |= SendUpdateConfigurationMessage(actionQueue, ProxyMessageFlags::Catchup);
        break;

    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        isSent |= SendUpdateReplicationConfigurationMessage(actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Phase4_Activate:
        isSent |= SendEndReconfigurationMessage(actionQueue);
        break;

    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
        isSent |= SendCancelCatchupMessage(actionQueue);
        break;

    default:
        Assert::CodingError("Unknown reconfig stage {0}", *this);
    }

    return isSent;
}

void FailoverUnit::SendPublishEndpointMessage(FailoverUnitEntityExecutionContext & context)
{
    TransportUtility.AddRequestFMMessageRetryAction(context.Queue);
}

void FailoverUnit::ProcessMsgResends(FailoverUnitEntityExecutionContext & executionContext)
{
    auto & actionQueue = executionContext.Queue;

    bool shouldArmTimer = false;

    shouldArmTimer |= ProcessReplicaMessageResends(actionQueue);

    shouldArmTimer |= ProcessPhaseMessageResends(actionQueue);

    shouldArmTimer |= SendDataLossReportMessage(actionQueue);
    
    {
        bool sendEndpointMessage = false, isEndpointRetryRequired = false;
        endpointPublishState_.OnReconfigurationTimer(
            executionContext.Clock, 
            executionContext.StateUpdateContextObj, 
            sendEndpointMessage, 
            isEndpointRetryRequired);

        shouldArmTimer |= isEndpointRetryRequired;

        if (sendEndpointMessage)
        {
            SendPublishEndpointMessage(executionContext);
        }
    }

    messageRetryActiveFlag_.SetValue(shouldArmTimer, actionQueue);
}

bool FailoverUnit::CanReopenDownReplica() const
{
    if (!HasPersistedState)
    {
        return false;
    }

    if (IsClosed)
    {
        return false;
    }

    if (LocalReplica.IsUp)
    {
        return false;
    }

    return true;
}

void FailoverUnit::ReopenDownReplica(FailoverUnitEntityExecutionContext & executionContext)
{
    TESTASSERT_IF(!CanReopenDownReplica(), "This should be checked before calling this method {0}", *this);
    TESTASSERT_IF(localReplicaOpen_, "A down replica can't be open {0}", *this);
    TESTASSERT_IF(IsRuntimeActive, "Cannot have runtime active for down replica {0}", *this);

    // Persisted state is being updated
    executionContext.UpdateContextObj.EnableUpdate();

    auto & queue = executionContext.Queue;

    // Update the instance
    LocalReplica.UpdateStateOnReopen(true);
    localReplicaInstanceId_ = LocalReplica.InstanceId;

    if (LocalReplica.State == ReplicaStates::InDrop)
    {
        /*
            Currently, when a replica goes down the RA does not preserve the reason
            the replica went down. This is because it assumes that whenever a replica starts to close
            with intent A any subsequent close with intent B overrides intent A.

            This is not optimal. It implies that if RA accepts delete, the replica goes down, 
            FM will keep retrying delete until the replica comes back up and gets deleted. 

            The RA cannot simply assume that because LocalReplicaDeleted can also be set to 
            true in QueuedDelete or ForceDelete scenarios where the FM has forgotten about
            this replica and asked the RA to delete it in ReplicaDroppedReply. Since the RA
            has acknowledged that intent it cannot send back a delete replica reply.

            Thus, at this point RA will restore the close mode to Drop if delete has not been accepted
            because ReplicaDropped can be sent or ForceDelete which will not send any message to FM on completion
            of the delete. When the delete does complete, if the FM had requested an explicit delete, the FM
            retry of DeleteReplica will send back the DeleteReplicaReply
        */
        localReplicaClosePending_.SetValue(true, queue);
        replicaCloseMode_ = LocalReplicaDeleted ? ReplicaCloseMode::ForceDelete : ReplicaCloseMode::Drop;
        if (LocalReplicaDeleted)
        {
            retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaDelete);
        }
        else 
        {
            retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaClose);
        } 

        serviceTypeRegistration_.Start(LocalReplica.State);
    }
    else
    {
        localReplicaOpenPending_.SetValue(true, queue);
        retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaReopen);
        serviceTypeRegistration_.Start(LocalReplica.State);
    }

    EnqueueHealthEvent(ReplicaHealthEvent::Restart, queue, make_unique<PartitionHealthEventDescriptor>());
}

void FailoverUnit::UploadReplicaForInitialDiscovery(
    FailoverUnitEntityExecutionContext & context)
{
    replicaUploadState_.UploadReplica(context.StateUpdateContextObj);
}

void FailoverUnit::ProcessNodeUpAck(
    vector<UpgradeDescription> const & upgrades,
    bool isNodeActivated,
    FailoverUnitEntityExecutionContext & context)
{
    TESTASSERT_IF(!IsClosed && LocalReplica.IsUp, "Invalid state at node up ack {0}", *this);

    bool shouldUploadReplica = false;
    bool isDeferredUpload = true;

    // The replica should be uploaded only if it is not deleted
    // Once RA has acknowedged the delete it can never send that replica to the FM
    shouldUploadReplica = !LocalReplicaDeleted;

    if (IsClosed)
    {
        // There is no state change and the replica upload can happen immediately
        isDeferredUpload = false;
    }
    else
    {
        /*  
            If the node is deactivated then the upload can happen immediately
            Otherwise start reopening the replica and optimize to send only ReplicaUp with the replica as up
            If the replica does not open within the configured ReopenSuccessWaitDuration
            The PendingReplicaUploadState will enqueue work to cause it to be uploaded
        */
        isDeferredUpload = isNodeActivated;

        /*
            It is possible that the application was upgraded while the node was down. In this case
            the FM would have sent the version of this package in the message (list of upgrades)
            
            The RA should open the app in the version requested by the FM which means perform a code upgrade

            It is also possible that the service type was deleted for this specific partition in which
            case this replica can never be opened so the replica should be dropped

            NOTE: This must happen before calling reopen down replica
        */
        bool shouldDropReplicaDueToDeletedServiceType = false;
        for (auto const & it : upgrades)
        {
            if (it.ApplicationId != ServiceDescription.ApplicationId)
            {
                continue;
            }

            OnApplicationCodeUpgrade(it.Specification, context);

            if (it.InstanceId == ServicePackageVersionInstance::Invalid.InstanceId)
            {
                shouldDropReplicaDueToDeletedServiceType = true;
            }
        }

        /*
            Start reopening the replica immediately if the node is activated
            Otherwise wait until the Activate message comes from the FM 
        */
        if (shouldDropReplicaDueToDeletedServiceType)
        {
            StartCloseLocalReplica(ReplicaCloseMode::ForceDelete, ReconfigurationAgent::InvalidNode, context, ActivityDescription::Empty);
        }
        else if (isNodeActivated)
        {
            ReopenDownReplica(context);
        }
    }

    // Order is important - call ShouldUpload prior to calling upload
    if (shouldUploadReplica)
    {
        replicaUploadState_.MarkUploadPending(context.StateUpdateContextObj);

        if (!isDeferredUpload)
        {
            replicaUploadState_.UploadReplica(context.StateUpdateContextObj);
        }
    }

}

void FailoverUnit::UpdateStateOnLFUMLoad(
    Federation::NodeInstance const & currentNodeInstance,
    FailoverUnitEntityExecutionContext & executionContext)
{
    FinalConstruct(executionContext.Config);

    auto & hosting = executionContext.Hosting;
    auto & queue = executionContext.Queue;

    UpdateLastStableEpochForBackwardCompatibility();
    FixupServiceDescriptionOnLfumLoad();

    if (IsClosed)
    {
        UpdateStateOnLocalReplicaDropped(hosting, queue);
    }
    else
    {
        ASSERT_IF(FailoverConfig::GetConfig().AssertOnNodeIdMismatchAtLfumLoad && currentNodeInstance.Id != LocalReplica.FederationNodeId, "NodeId mismatch on Load. Current: {0}\r\nLocal: {1}", currentNodeInstance, *this);

        if (HasPersistedState)
        {
            UpdateStateOnLocalReplicaDown(hosting, queue);
            LocalReplica.FederationNodeInstance = currentNodeInstance;

            // This is required so that the FM Message State is aware of which
            // instance of the replica is actually down at this point
            // When a node comes up the instance it has persisted is down
            fmMessageState_.OnLoadedFromLfum(LocalReplicaInstanceId, executionContext.StateUpdateContextObj);
        }
        else
        {
            UpdateStateOnLocalReplicaDropped(hosting, queue);
        }
    }
}

bool FailoverUnit::TryGetConfiguration(
    Federation::NodeInstance const & localNodeInstance, 
    bool shouldMarkClosingReplicaAsDown,
    FailoverUnitInfo & rv) const
{
    auto success = TryGetConfigurationInternal(localNodeInstance, shouldMarkClosingReplicaAsDown, rv);
    if (!success)
    {
        return success;
    }

    rv.AssertInvariants();

    return success;
}

bool FailoverUnit::TryGetConfigurationInternal(
    Federation::NodeInstance const & localNodeInstance, 
    bool shouldMarkClosingReplicaAsDown,
    FailoverUnitInfo & rv) const
{
    // The configuration of FT can be requested in multiple scenarios for sending to FM:
    // - LfumUpload
    // - ReplicaUp
    // - Initial Replica Upload
    // - ReplicaDown
    // - Replica Dropped

    // There are multiple states of the replica in which this can be requested
    //   Local Replica is deleted: send nothing
    //   Local FT is closed: send replica id, instance id and role as None and dropped
    //   Local replica is open: Send the local replica and the remote replica list
    //   Local replica is Up and not open (reopen is pending): Send the prereopenlocalreplica as down and remote list
    //   Local replica is Down: Send the local replica as SB and D and remote list
    //   Local replica is ID: Send the local replica as dropped
    if (localReplicaDeleted_)
    {
        return false;
    }

    vector<ReplicaInfo> replicaDescList;

    bool isReportFromPrimary = false;

    if (state_ == FailoverUnitStates::Closed || LocalReplica.IsInDrop)
    {
        replicaDescList.push_back(ReplicaInfo(CreateDroppedReplicaDescription(localNodeInstance), ReplicaRole::None));
    }
    else
    {
        for (auto const & replica : replicaStore_)
        {
            ReplicaDescription replicaDesc = replica.GetConfigurationDescriptionForRebuildAndReplicaUp();

            if (replicaStore_.IsLocalReplica(replica))
            {
                if (!localReplicaOpen_ && replica.IsUp && replica.IsStandBy)
                {
                    // If reopen of local replica is pending, consider it to be down in the report
                    replicaDesc.IsUp = false;
                    replicaDesc.InstanceId = fmMessageState_.DownReplicaInstanceId;
                }
                else if (replica.IsUp)
                {
                    if (replica.IsAvailable || replica.IsInCreate || replica.IsStandBy)
                    {
                        if (replica.PreviousConfigurationRole == ReplicaRole::Primary ||
                            replica.CurrentConfigurationRole == ReplicaRole::Primary)
                        {
                            if (IsReconfiguring ||
                                (replica.CurrentConfigurationRole == ReplicaRole::Primary && !replica.IsStandBy))
                            {
                                isReportFromPrimary = true;

                                if (replica.IsStandBy)
                                {
                                    replicaDesc.State = Reliability::ReplicaStates::InBuild;
                                }
                            }
                        }

                        if ((replica.PreviousConfigurationRole == ReplicaRole::Primary ||
                            replica.IntermediateConfigurationRole == ReplicaRole::Primary) &&
                            replica.IsReady &&
                            !isReportFromPrimary)
                        {
                            replicaDesc.State = Reliability::ReplicaStates::InBuild;
                        }
                    }
                }

                // Set the package version instance to invalid if the replica is a S/S IB replica
                // That was being created but has not completed build yet
                if (deactivationInfo_.IsDropped)
                {
                    replicaDesc.PackageVersionInstance = ServicePackageVersionInstance::Invalid;
                }

                // There are scenarios where the replica is closing and can be marked as down
                // Example: If read/write status has been revoked already - in this case
                // the replica technically is up because it is closing but can be marked
                // as down in a message to the fm
                // If the replica is being marked as down it should be sent as either SB or DD
                // the caller must specify this
                if (shouldMarkClosingReplicaAsDown && LocalReplicaClosePending.IsSet)
                {
                    replicaDesc.IsUp = false;
                    TESTASSERT_IF(replicaDesc.IsDropped, "Dropped replica should be handled at the very top of this function {0}", *this);
                    replicaDesc.State = Reliability::ReplicaStates::StandBy;
                }
            }

            replicaDescList.push_back(ReplicaInfo(move(replicaDesc), replica.IntermediateConfigurationRole));
        }
    }

    rv = FailoverUnitInfo(
        serviceDesc_,
        failoverUnitDesc_,
        icEpoch_,
        isReportFromPrimary,
        false, // local replica deleted is false right now - until fm adds support to handle it
        move(replicaDescList));

    return true;
}

vector<Reliability::ReplicaDescription> FailoverUnit::CreateReplicationConfigurationReplicaDescriptionList() const
{
    vector<Reliability::ReplicaDescription> replicaDescriptionList;

    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        replicaDescriptionList.push_back(replica.GetConfigurationDescriptionForReplicator());
    }

    return replicaDescriptionList;
}

vector<Reliability::ReplicaDescription> FailoverUnit::CreateCurrentConfigurationReplicaDescriptionList() const
{
    vector<Reliability::ReplicaDescription> replicaDescriptionList;

    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        if (replica.IsInCurrentConfiguration)
        {
            replicaDescriptionList.push_back(replica.GetCurrentConfigurationDescription());
        }
    }

    return replicaDescriptionList;
}

vector<Reliability::ReplicaDescription> FailoverUnit::CreateChangeConfigurationReplicaDescriptionList(int64 replicaIdOfBestPrimary) const
{
    vector<Reliability::ReplicaDescription> replicaDescriptionList;

    bool primaryFound = false;
    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        auto replicaDescription = replica.GetCurrentConfigurationDescription();

        /*
            The asserts below verify that LSN is only sent for the replica that RA decides should be primary
            The configuration epoch description is not transferred as on the FM there is no knowledge of epoch:LSN (it would break backward compat) so
            the FM can incorrectly pick a higher LSN replica as the new primary even if the higher LSN was false progerss
            if the RA sends all the LSN
            With this we only send the LSN of the replica that should be made primary forcing FM to send
            DoReconfiguration to that replica.

            That replica will see that it does not have LSNs and perform phase1_getlsn again and continue the reconfiguration
            */
        if (replicaDescription.ReplicaId == replicaIdOfBestPrimary)
        {
            replicaDescription.AssertLSNsAreValid();
            primaryFound = true;
        }
        else
        {
            replicaDescription.FirstAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
            replicaDescription.LastAcknowledgedLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
        }

        replicaDescriptionList.push_back(move(replicaDescription));
    }

    ASSERT_IF(!primaryFound, "Sending a ChangeConfiguration without any primary replica {0}", *this);

    return replicaDescriptionList;
}

vector<ReplicaDescription> FailoverUnit::CreateConfigurationDescriptionListForDeactivateOrActivate(
    int64 replicaIdToWhichMessageIsBeingSent) const
{
    vector<Reliability::ReplicaDescription> replicaDescriptionList;

    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        /*
            For backward compatibility
            The primary RA may have the replica as IB (ToBeDeactivated) or IB (ToBeActivated)
            And a deactivate/activate message needs to be sent to the remote replica
            At this time sending the replica as IB in the message is incorrect and will
            break backward compatibility
        */
        auto description = replica.GetConfigurationDescriptionForDeactivateOrActivate(replica.ReplicaId == replicaIdToWhichMessageIsBeingSent);

        replicaDescriptionList.push_back(move(description));
    }

    return replicaDescriptionList;
}


vector<Reliability::ReplicaDescription> FailoverUnit::CreateConfigurationReplicaDescriptionListForRestartRemoteReplica() const
{
    /*
        TODO: Replace Deactivate message with the force flag with ClientReportFault
    */
    vector<Reliability::ReplicaDescription> replicaDescriptionList;

    for (auto const & replica : replicaStore_.ConfigurationReplicas)
    {
        replicaDescriptionList.push_back(replica.GetCurrentConfigurationDescription());
    }

    return replicaDescriptionList;
}

void FailoverUnit::ArmMessageRetryTimerIfSent(bool isSent, Infrastructure::StateMachineActionQueue & queue)
{
    if (isSent)
    {
        messageRetryActiveFlag_.SetValue(true, queue);
    }
}

FMMessageState & FailoverUnit::Test_GetFMMessageState()
{
    return fmMessageState_;
}

EndpointPublishState & FailoverUnit::Test_GetEndpointPublishState()
{
    return endpointPublishState_;
}

void FailoverUnit::Test_SetMessageRetryFlag(SetMembershipFlag const & flag)
{
    messageRetryActiveFlag_ = flag;
}

void FailoverUnit::Test_SetCleanupPendingFlag(SetMembershipFlag const & flag)
{
    cleanupPendingFlag_ = flag;
}

void FailoverUnit::Test_SetLastUpdatedTime(Common::DateTime time)
{
    lastUpdated_ = time;
}

void FailoverUnit::Test_SetServiceTypeRegistration(ServiceTypeRegistrationSPtr const & registration)
{
    serviceTypeRegistration_.Test_SetServiceTypeRegistration(registration);
}

void FailoverUnit::Test_SetDeactivationInfo(Reliability::ReplicaDeactivationInfo const & deactivationInfo)
{
    deactivationInfo_ = deactivationInfo;
}

void FailoverUnit::Test_SetLastStableEpoch(Reliability::Epoch const & epoch)
{
    lastStableEpoch_ = epoch;
}

void FailoverUnit::AssertIfReplicaFlagMismatch(bool isAllowed, bool actual, wstring const & flagName, Replica const & r) const
{
    if (!isAllowed && actual)
    {
        AssertReplicaFlagMismatch(flagName, r);
    }
}

void FailoverUnit::AssertReplicaFlagMismatch(wstring const & flagName, Replica const & r) const
{
    Assert::TestAssert("Replica flag {0} is set when it is not expected to be set. Replica {1}\r\n{2}", flagName, r, *this);
}

void FailoverUnit::AssertLocalReplicaInvariants() const
{
    auto const & replica = LocalReplica;
    TESTASSERT_IF(replica.ToBeActivated || replica.ToBeRestarted || replica.ReplicatorRemovePending || replica.ToBeDeactivated, "Local replica flag is invalid {0}", *this);

    if (!IsReconfiguring)
    {
        return;
    }

    bool isMessageStageAllowed = true;
    bool validateICRole = false;

    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        isMessageStageAllowed = false;
        break;

    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        validateICRole = true;
        break;

    case FailoverUnitReconfigurationStage::Phase4_Activate:
        isMessageStageAllowed = LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary;
        validateICRole = true;
        break;
    }

    if (!isMessageStageAllowed && replica.MessageStage != ReplicaMessageStage::None)
    {
        AssertReplicaFlagMismatch(L"MessageStage", replica);
    }    

    TESTASSERT_IF(validateICRole && replica.IntermediateConfigurationRole != replica.CurrentConfigurationRole, "IC Role does not match CC Role {0}\r\n{1}", replica, *this);
}

void FailoverUnit::AssertRemoteReplicaInvariants(Replica const & replica) const
{
    bool isToBeActivatedAllowed = false;
    bool isToBeRestartedAllowed = false;
    bool isToBeDeactivatedAllowed = false;
    bool isReplicatorRemovePendingAllowed = false;
    bool isMessageStageAllowed = false;
    bool isUnknownLSNAllowed = false;
    bool validateICRole = false;

    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::None:
        isToBeActivatedAllowed = replica.IsInBuild && replica.IsUp;
        isToBeDeactivatedAllowed = false;
        isReplicatorRemovePendingAllowed = LocalReplica.PreviousConfigurationRole != ReplicaRole::None; // For abort_phase0_demote
        isMessageStageAllowed = replica.IsInBuild && replica.CurrentConfigurationRole == ReplicaRole::Secondary;
        isToBeRestartedAllowed = false;
        break;

    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
        isToBeActivatedAllowed = false;
        isToBeDeactivatedAllowed = false;
        isReplicatorRemovePendingAllowed = false;
        isMessageStageAllowed = true;
        isUnknownLSNAllowed = true;
        isToBeRestartedAllowed = false;
        break;

    case FailoverUnitReconfigurationStage::Phase0_Demote:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
        isToBeActivatedAllowed = false;
        isToBeDeactivatedAllowed = replica.IsInBuild && replica.IsUp;
        isReplicatorRemovePendingAllowed = !replica.IsUp;
        isMessageStageAllowed = false;
        isToBeRestartedAllowed = true;
        break;

    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        isToBeActivatedAllowed = false;
        isToBeDeactivatedAllowed = replica.IsInBuild && replica.IsUp;
        isReplicatorRemovePendingAllowed = !replica.IsUp;
        isMessageStageAllowed = true;
        isToBeRestartedAllowed = true;
        validateICRole = true;
        break;

    case FailoverUnitReconfigurationStage::Phase4_Activate:
        isToBeActivatedAllowed = replica.IsInBuild && replica.IsUp;
        isToBeDeactivatedAllowed = false;
        isReplicatorRemovePendingAllowed = !replica.IsUp;
        isMessageStageAllowed = true;
        isToBeRestartedAllowed = true;
        validateICRole = true; 
        break;

    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
        isToBeActivatedAllowed = false;
        isToBeDeactivatedAllowed = true;
        isReplicatorRemovePendingAllowed = true;
        isMessageStageAllowed = false;
        isToBeRestartedAllowed = false;
        break;

    default:
        Assert::CodingError("Unknown reconfig stage {0}", static_cast<int>(ReconfigurationStage));
    }

    AssertIfReplicaFlagMismatch(isToBeActivatedAllowed, replica.ToBeActivated, L"ToBeActivated", replica);
    AssertIfReplicaFlagMismatch(isToBeDeactivatedAllowed, replica.ToBeDeactivated, L"ToBeDeactivated", replica);
    AssertIfReplicaFlagMismatch(isReplicatorRemovePendingAllowed, replica.ReplicatorRemovePending, L"ReplicatorRemovePending", replica);
    AssertIfReplicaFlagMismatch(isToBeRestartedAllowed, replica.ToBeRestarted, L"ToBeRestarted", replica);

    if (!isMessageStageAllowed && replica.MessageStage != ReplicaMessageStage::None)
    {
        AssertReplicaFlagMismatch(L"MessageStage", replica);
    }

    TESTASSERT_IF(!isUnknownLSNAllowed && replica.IsLSNUnknown(), "LSN is unknown when it cannot be unknown. Replica = {0} \r\n. {1}", replica, *this);
    TESTASSERT_IF(replica.ReplicatorRemovePending && replica.ToBeActivated, "Invalid flags ReplicatorRemovePending & ToBeActivated. Replica = {0}\r\n. {1}", replica, *this);
    TESTASSERT_IF(replica.ReplicatorRemovePending && replica.ToBeDeactivated, "Invalid flags ReplicatorRemovePending & ToBeDeactivated. Replica = {0}\r\n. {1}", replica, *this);
    TESTASSERT_IF(replica.ToBeActivated && replica.ToBeDeactivated, "Invalid flags ToBeActivated & ToBeDeactivated. Replica = {0}\r\n. {1}", replica, *this);
    TESTASSERT_IF(replica.ToBeActivated && !replica.IsUp, "Replica Up and ToBeActivated. Replica = {0}\r\n {1}", replica, *this);
    TESTASSERT_IF(replica.ToBeDeactivated && !replica.IsUp, "Replica Up and ToBeDeactivated. Replica = {0}\r\n {1}", replica, *this);
    TESTASSERT_IF(replica.ToBeRestarted && !replica.IsReady, "Replica that is not ready marked to be restarted Replica = {0}\r\n{1}", replica, *this);
    TESTASSERT_IF(validateICRole && replica.IntermediateConfigurationRole != replica.CurrentConfigurationRole, "IC Role does not match CC Role {0}\r\n{1}", replica, *this);
    TESTASSERT_IF(replica.PreviousConfigurationRole == ReplicaRole::Idle && replica.CurrentConfigurationRole == ReplicaRole::Idle, "Cannot have I/N/I {0}\r\n{1}", replica, *this);
}

void FailoverUnit::AssertIdleReplicaInvariants(Replica const & replica) const
{
    TESTASSERT_IF(replica.PreviousConfigurationRole != ReplicaRole::None, "PC Role is not none for replica in idle list {0}\r\n{1}", replica, *this);
    TESTASSERT_IF(replica.IntermediateConfigurationRole != ReplicaRole::None, "IC Role is not none for replica in idle list {0}\r\n{1}", replica, *this);
}

void FailoverUnit::AssertSenderNodeIsInvalid() const
{
    TESTASSERT_IF(SenderNode != ReconfigurationAgent::InvalidNode, "Sender node is not invalid {0}", *this);
}

void FailoverUnit::AssertEpochInvariants() const
{
    TESTASSERT_IF(lastStableEpoch_ != PreviousConfigurationEpoch, "Last stable epoch must be pc epoch for backward compatibility {0} {1}", lastStableEpoch_, *this);
    TESTASSERT_IF(CurrentConfigurationEpoch == Epoch::InvalidEpoch(), "CCEpoch cannot be invalid {0}", *this);

    bool validatePC = false;
    bool validateIC = false;

    switch (ReconfigurationStage)
    {
    case FailoverUnitReconfigurationStage::Phase0_Demote: 
    case FailoverUnitReconfigurationStage::Phase1_GetLSN:
    case FailoverUnitReconfigurationStage::Phase2_Catchup:
    case FailoverUnitReconfigurationStage::Abort_Phase0_Demote:
        validatePC = true;        
        break;

    case FailoverUnitReconfigurationStage::Phase4_Activate:
    case FailoverUnitReconfigurationStage::Phase3_Deactivate:
        validatePC = true;
        validateIC = true;
        break;

    default: 
        break;
    }

    TESTASSERT_IF(validatePC && PreviousConfigurationEpoch.IsInvalid(), "PC is invalid when it should not be {0}", *this);
    TESTASSERT_IF(validateIC && IntermediateConfigurationEpoch.IsInvalid(), "IC is invalid when it should not be {0}", *this);
}

void FailoverUnit::AssertInvariants() const
{
    AssertEpochInvariants();
    serviceTypeRegistration_.AssertInvariants(*this);
    fmMessageState_.AssertInvariants(*this);
    reconfigState_.AssertInvariants(*this);
    endpointPublishState_.AssertInvariants(*this);

    TESTASSERT_IF(LocalReplicaOpenPending.IsSet && LocalReplicaClosePending.IsSet, "Replica cannot be open pending and close pending {0}", *this);
    TESTASSERT_IF(CloseMode == ReplicaCloseMode::None && LocalReplicaClosePending.IsSet, "Replica cannot be close pending with no close mode {0}", *this);
    TESTASSERT_IF(OpenMode == RAReplicaOpenMode::None && LocalReplicaOpenPending.IsSet, "Replica cannot be open pending with no open mode {0}", *this);
    
    bool nonLifecycleMessageFlagCanBeSet = true;
    if (LocalReplicaOpenPending.IsSet)
    {
        TESTASSERT_IF(RetryableErrorStateObj.CurrentState != RetryableErrorStateName::ReplicaOpen && RetryableErrorStateObj.CurrentState != RetryableErrorStateName::ReplicaReopen, "State is {0}. State should be ReplicaOpen or ReplicaReopen when OpenPending is set {1}.", RetryableErrorStateObj.CurrentState, *this);
    }
    else if (LocalReplicaClosePending.IsSet)
    {
        TESTASSERT_IF(RetryableErrorStateObj.CurrentState != RetryableErrorStateName::ReplicaClose && RetryableErrorStateObj.CurrentState != RetryableErrorStateName::ReplicaDelete, "State is {0}. State should be ReplicaClose or ReplicaDelete when ClosePending is set {1}.", RetryableErrorStateObj.CurrentState, *this);
    }

    TESTASSERT_IF(cleanupPendingFlag_.IsSet && !LocalReplicaDeleted, "cleanup pending set when not deleted {0}", *this);
    TESTASSERT_IF(cleanupPendingFlag_.IsSet && IsOpen, "cleanup pending set when open {0}", *this);
    TESTASSERT_IF(LocalReplicaDeleted && IsClosed && !cleanupPendingFlag_.IsSet, "cleanup pending not set for deleted closed {0}", *this);

    if (state_ == FailoverUnitStates::Closed)
    {
        nonLifecycleMessageFlagCanBeSet = false;
        TESTASSERT_IF(messageRetryActiveFlag_.IsSet, "Message reset flag cannot be true {0}", *this);
        TESTASSERT_IF(updateReplicatorConfiguration_, "UpdateReplicatorConfiguration cannot be true {0}", *this);
        TESTASSERT_IF(LocalReplicaOpen, "Local replica cannot be open {0}", *this);
        TESTASSERT_IF(LocalReplicaOpenPending.IsSet, "OpenPending cannot be set when closed {0}", *this);
        TESTASSERT_IF(LocalReplicaClosePending.IsSet, "ClosePending cannot be set when closed {0}", *this);
        TESTASSERT_IF(RetryableErrorStateObj.CurrentState != RetryableErrorStateName::None, "State is {0}. State must be none {1}", RetryableErrorStateObj.CurrentState, *this);
        TESTASSERT_IF(!DeactivationInfo.IsDropped, "Deactivation info must be dropped {0}", *this);
        TESTASSERT_IF(IsReconfiguring, "Cannot be closed and reconfiguring {0}", *this);
        AssertSenderNodeIsInvalid();
    }
    else
    {        
        TESTASSERT_IF(LocalReplicaDeleted && !LocalReplica.IsInDrop, "Replica must be ID for deleted {0}", *this);        

        TESTASSERT_IF(LocalReplica.PackageVersionInstance == ServicePackageVersionInstance::Invalid, "Cannot have invalid version instance {0}", *this);
        TESTASSERT_IF(LocalReplica.PackageVersionInstance != serviceDesc_.PackageVersionInstance, "Package version instance must match {0} {1}", serviceDesc_, *this);

        TESTASSERT_IF(TargetReplicaSetSize == 0, "TargetReplicaSetSize cannot be zero, ft: {0}", *this);
        TESTASSERT_IF(IsStateful && MinReplicaSetSize == 0, "MinReplicaSetSize cannot be zero, ft: {0}", *this);

        if (!IsReconfiguring)
        {
            TESTASSERT_IF(updateReplicatorConfiguration_ && LocalReplicaRole != ReplicaRole::Primary, "UpdateReplicatorConfiguration cannot be true if not primary {0}", *this);
        }
        else
        {
            TESTASSERT_IF(updateReplicatorConfiguration_ && LocalReplica.CurrentConfigurationRole != ReplicaRole::Primary && LocalReplica.PreviousConfigurationRole != ReplicaRole::Primary, "invalid update replicator config {0}", *this);            
        }

        if (LocalReplica.IsUp)
        {
            if (LocalReplicaOpen)
            {
                TESTASSERT_IF(ServiceTypeRegistration == nullptr, "Replica cannot be open without service type registration {0}", *this);
                nonLifecycleMessageFlagCanBeSet = !localReplicaClosePending_.IsSet;                
                
                if (IsReconfiguring)
                {
                    AssertSenderNodeIsInvalid();
                }
            }
            else
            {
                TESTASSERT_IF(LocalReplicaOpenPending.IsSet && !(LocalReplica.IsInCreate || LocalReplica.IsStandBy), "Replica cannot be open pending but not IC/SB {0}", *this);
            }
        }
        else
        {
            TESTASSERT_IF(LocalReplica.State != ReplicaStates::StandBy && LocalReplica.State != ReplicaStates::InDrop, "Incorrect local replica state for down replica {0}", *this);
            TESTASSERT_IF(LocalReplicaOpen, "LocalReplica cannot be open if down {0}", *this);
            TESTASSERT_IF(RetryableErrorStateObj.CurrentState != RetryableErrorStateName::None, "State is {0}. State must be none {1}", RetryableErrorStateObj.CurrentState, *this);
            TESTASSERT_IF(LocalReplicaOpenPending.IsSet, "Replica cannot be open pending and down {0}", *this);
            TESTASSERT_IF(LocalReplicaClosePending.IsSet, "Replica cannot be close pending and down {0}", *this);
            TESTASSERT_IF(IsReconfiguring, "Cannot be down and reconfiguring {0}", *this);
            nonLifecycleMessageFlagCanBeSet = false;            
        }
    }

    TESTASSERT_IF(!nonLifecycleMessageFlagCanBeSet && messageRetryActiveFlag_.IsSet, "Message retry flag {0}", *this);
    TESTASSERT_IF(!nonLifecycleMessageFlagCanBeSet && LocalReplicaServiceDescriptionUpdatePending.IsSet, "SD Update flag {0}", *this);

    vector<int64> logicalStoreList;

    for (auto const & it : replicaStore_.ConfigurationRemoteReplicas)
    {
        logicalStoreList.push_back(it.ReplicaId);
        AssertRemoteReplicaInvariants(it);
    }

    for (auto const & it : replicaStore_.IdleReplicas)
    {
        logicalStoreList.push_back(it.ReplicaId);
        AssertIdleReplicaInvariants(it);
    }

    if (IsOpen)
    {
        logicalStoreList.push_back(LocalReplica.ReplicaId);
        AssertLocalReplicaInvariants();
    }

    /*
        Verify that the set of all replica ids found by adding local replica id, 
        configuration remote replica and idle replicas from the store matches exactly
        the set of all the replicas in the replicas_ vector and ensure that
        the replica store is consistent
    */
    vector<int64> physicalStoreList;
    for (auto const & it : replicaStore_)
    {
        physicalStoreList.push_back(it.ReplicaId);
    }
    
    sort(physicalStoreList.begin(), physicalStoreList.end());
    sort(logicalStoreList.begin(), logicalStoreList.end());

    TESTASSERT_IF(physicalStoreList.size() != logicalStoreList.size(), "Size does not match of enumeration of replica store and replicas_ vector {0}", *this);

    for(size_t i = 0; i < physicalStoreList.size(); ++i)
    {
        TESTASSERT_IF(logicalStoreList[i] != physicalStoreList[i], "Mismatch of replica in logical and physical store {0}", *this);
    }

    auto pcPrimaryCount = count_if(replicaStore_.begin(), replicaStore_.end(), [](Replica const & r) { return r.PreviousConfigurationRole == ReplicaRole::Primary; });
    auto ccPrimaryCount = count_if(replicaStore_.begin(), replicaStore_.end(), [](Replica const & r) { return r.CurrentConfigurationRole == ReplicaRole::Primary; });
    auto icPrimaryCount = count_if(replicaStore_.begin(), replicaStore_.end(), [](Replica const & r) { return r.IntermediateConfigurationRole == ReplicaRole::Primary; });
    TESTASSERT_IF(pcPrimaryCount > 1, "More than one primary in PC {0}", *this);
    TESTASSERT_IF(ccPrimaryCount > 1, "More than one primary in CC {0}", *this);
    TESTASSERT_IF(icPrimaryCount > 1, "More than one primary in IC {0}", *this);
}

void FailoverUnit::ProcessProxyReplicaEndpointUpdate(
    ReplicaMessageBody const& msgBody,
    FailoverUnitEntityExecutionContext& executionContext)
{
    // Always send the reply to RAP so that it stops retrying
    if (ServiceTypeRegistration != nullptr)
    {
        ReconfigurationAgent::AddActionSendMessageToRAProxy(
            executionContext.Queue,
            RAMessage::GetReplicaEndpointUpdatedReply(),
            ServiceTypeRegistration,
            msgBody.FailoverUnitDescription,
            msgBody.ReplicaDescription);
    }

    if (!msgBody.FailoverUnitDescription.CurrentConfigurationEpoch.IsPrimaryEpochEqual(CurrentConfigurationEpoch))
    {
        return;
    }

    if (ReconfigurationStage != FailoverUnitReconfigurationStage::Phase2_Catchup)
    {
        return;
    }

    UpdateServiceEndpoint(LocalReplica, msgBody.ReplicaDescription, executionContext);
}

void FailoverUnit::ProcessReplicationConfigurationUpdate(ProxyReplyMessageBody const & ucReply)
{
    if (!ucReply.ErrorCode.IsSuccess())
    {
        return;
    }

    if (IsReplicatorConfigurationUpdated(ucReply))
    {
        updateReplicatorConfiguration_ = false;
    }
}

bool FailoverUnit::IsReplicatorConfigurationUpdated(ProxyReplyMessageBody const & ucReply)
{
    ASSERT_IF(!ucReply.ErrorCode.IsSuccess(), "Caller should check this");

    /*
        In a swap primary when UC reply is received for catchup completing then the replicator configuration is up to date
        as the replicator has now changed role to S
    */
    if (ReconfigurationStage == FailoverUnitReconfigurationStage::Phase0_Demote &&
        ucReply.Flags == ProxyMessageFlags::Catchup)
    {
        return true;
    }

    vector<ReplicaDescription> const & remoteReplicas = ucReply.RemoteReplicaDescriptions;
    for(auto replicatorReplica = remoteReplicas.cbegin(); replicatorReplica != remoteReplicas.cend(); ++replicatorReplica)
    {
        Replica const & raReplica = GetReplica(replicatorReplica->FederationNodeId);

        if (raReplica.IsInvalid)
        {
            // this is a replica that the RA has removed from its list
            // and the replicator would remove it eventually
            // Example: A DoReconfiguration message caused the replicator configuration to be updated and the reconfiguration had a S->N transition
            // Now if the reconfiguration completes the RA will forget about the S->N replica while the UCReply message from the replicator will still have this replica
            continue;
        }

        if (raReplica.ReplicaId != replicatorReplica->ReplicaId)
        {
            return false;
        }

        ASSERT_IF(raReplica.InstanceId < replicatorReplica->InstanceId, "Replicator cannot know greater instance replica. Replicator Instance = {0}. Ra Instance = {1}. {2} \r\n {3}", replicatorReplica->InstanceId, raReplica.InstanceId, *this, ucReply);

        if (raReplica.InstanceId > replicatorReplica->InstanceId)
        {
            return false;
        }

        if (!raReplica.IsUp && replicatorReplica->IsUp)
        {
            return false;
        }

        if (raReplica.IsDropped && !replicatorReplica->IsDropped)
        {
            return false;
        }

        if (raReplica.IsReady && !replicatorReplica->IsReady)
        {
            return false;
        }
    }

    return true;
}

void FailoverUnit::EnqueueHealthEvent(
    ReplicaHealthEvent::Enum ev,
    Infrastructure::StateMachineActionQueue & queue,
    IHealthDescriptorSPtr const & descriptor) const
{
    IHealthSubsystemWrapper::EnqueueReplicaHealthAction(
        ev,
        FailoverUnitId,
        IsStateful,
        LocalReplicaId,
        LocalReplicaInstanceId,
        queue,
        move(descriptor));
}

ReplicaDescription FailoverUnit::CreateDroppedReplicaDescription(NodeInstance const & nodeInstance) const
{
    TESTASSERT_IF(!IsClosed && !LocalReplica.IsInDrop, "Cannot create dropped replica description if ft is not closed or in drop {0}", *this);
    return ReplicaDescription::CreateDroppedReplicaDescription(nodeInstance, LocalReplicaId, LocalReplicaInstanceId);
}

void FailoverUnit::SendReplicaDroppedMessage(
    Federation::NodeInstance const & nodeInstance,
    StateMachineActionQueue & queue) const
{
    TESTASSERT_IF(!IsClosed, "Cannot create dropped replica description if ft is not closed {0}", *this);

    FailoverUnitInfo ftInfo;
    if (!TryGetConfigurationInternal(nodeInstance, true, ftInfo))
    {
        return;
    }

    vector<FailoverUnitInfo> droppedReplicas;
    droppedReplicas.push_back(move(ftInfo));

    ReplicaUpMessageBody body(vector<FailoverUnitInfo>(), move(droppedReplicas), false, false);
    
    TransportUtility.AddSendMessageToFMAction(
        queue,
        RSMessage::GetReplicaUp(),
        move(body));
}

void FailoverUnit::SendReadWriteStatusRevokedNotificationReply(
    ReplicaMessageBody const & incomingBody,
    Infrastructure::StateMachineActionQueue & queue) const
{
    if (!IsRuntimeActive)
    {
        return;
    }

    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        queue,
        RAMessage::GetReadWriteStatusRevokedNotificationReply(),
        ServiceTypeRegistration,
        incomingBody.FailoverUnitDescription,
        incomingBody.ReplicaDescription);
}

void FailoverUnit::SendReplicaCloseMessage(StateMachineActionQueue & queue) const
{
    TESTASSERT_IF(!IsRuntimeActive, "Runtime must be active {0}", *this);
    TESTASSERT_IF(!LocalReplicaClosePending.IsSet, "LocalReplicaClosePending should have been set {0}", *this);
    TESTASSERT_IF(CloseMode == ReplicaCloseMode::Obliterate || CloseMode == ReplicaCloseMode::QueuedDelete, "Cannot send close msg for obliterate/queueddelete {0}", *this);

    ReplicaDescription closingReplicaDescription(
        LocalReplica.FederationNodeInstance,
        LocalReplicaId,
        LocalReplicaInstanceId);

    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        queue,
        RAMessage::GetReplicaClose(),
        ServiceTypeRegistration,
        FailoverUnitDescription,
        closingReplicaDescription,
        ServiceDescription,
        CloseMode.MessageFlags);
}

bool FailoverUnit::CanCloseLocalReplica(
    ReplicaCloseMode closeMode) const
{
    TESTASSERT_IF(closeMode == ReplicaCloseMode::None, "Invalid argument {0}", *this);
    TESTASSERT_IF(closeMode == ReplicaCloseMode::Restart && !HasPersistedState, "Can only restart persisted replica {0}", *this);

    // The operations below are invalid
    if (IsClosed)
    {
        // It is not possible to issue a restart or go down if the FT is closed
        return closeMode != ReplicaCloseMode::Restart && closeMode != ReplicaCloseMode::AppHostDown;
    }

    if (!LocalReplica.IsUp)
    {
        // It is not possible to drop/abort/deactivate/delete a down replica
        // as the replica needs to be Up to call CR (None)
        switch (closeMode.Name)
        {
        case ReplicaCloseModeName::ForceAbort:
        case ReplicaCloseModeName::ForceDelete:
        case ReplicaCloseModeName::QueuedDelete:
        case ReplicaCloseModeName::Obliterate:
            return true;

        case ReplicaCloseModeName::Drop:
        case ReplicaCloseModeName::Abort:
        case ReplicaCloseModeName::Delete:
        case ReplicaCloseModeName::Deactivate:
        case ReplicaCloseModeName::AppHostDown:
        case ReplicaCloseModeName::Close:
        case ReplicaCloseModeName::DeactivateNode:
        case ReplicaCloseModeName::Restart:
            return false;

        default:
            Assert::CodingError("Unknown close mode {0}", static_cast<int>(closeMode.Name));
        }
    }

    // Cannot queue deleting an up replica - the replica should be deleted
    if (closeMode == ReplicaCloseMode::QueuedDelete)
    {
        return false;
    }

    // Already closing
    if (LocalReplicaClosePending.IsSet)
    {
        // AppHostDown can always be processed
        // If already closing for Obliterate then Obliterate is not allowed
        if (closeMode == ReplicaCloseMode::AppHostDown ||
            (closeMode == ReplicaCloseMode::Obliterate && replicaCloseMode_ != ReplicaCloseMode::Obliterate))
        {
            return true;
        }

        // The only other scenario in which Close is allowed to continue
        // Is if the close mode implies a Force and STR is not available
        // This is inelegant -> ideally this should be internal
        // and will be done once the service type registration lookup code
        // is being performed inside the FT itself
        if (closeMode.IsForceDropImplied && !IsRuntimeActive)
        {
            return true;
        }

        return false;
    }

    return true;
}

bool FailoverUnit::IsLocalReplicaClosed(
    ReplicaCloseMode closeMode) const
{
    TESTASSERT_IF(closeMode == ReplicaCloseMode::None, "Cannot check for is closed with mode = None {0}", *this);

    if (IsClosed)
    {
        // Invalid to restart a Closed FT
        // Delete is complete only if localReplicaDeleted_ is true
        switch (closeMode.Name)
        {
        case ReplicaCloseModeName::Close:
        case ReplicaCloseModeName::DeactivateNode:
        case ReplicaCloseModeName::Drop:
        case ReplicaCloseModeName::Abort:
        case ReplicaCloseModeName::Deactivate:
        case ReplicaCloseModeName::ForceAbort:
        case ReplicaCloseModeName::AppHostDown:
        case ReplicaCloseModeName::Obliterate:
            return true;

        case ReplicaCloseModeName::ForceDelete:
        case ReplicaCloseModeName::Delete:
        case ReplicaCloseModeName::QueuedDelete:
            return localReplicaDeleted_;

        case ReplicaCloseModeName::Restart:
        default:
            Assert::CodingError("Unexpected close mode {0}", static_cast<int>(closeMode.Name));
        }
    }

    if (!LocalReplica.IsUp)
    {
        switch (closeMode.Name)
        {
        case ReplicaCloseModeName::Close:
        case ReplicaCloseModeName::DeactivateNode:
        case ReplicaCloseModeName::Restart:
        case ReplicaCloseModeName::AppHostDown:
            return true;

        case ReplicaCloseModeName::ForceAbort:
        case ReplicaCloseModeName::ForceDelete:
        case ReplicaCloseModeName::Obliterate:
            return false;

        case ReplicaCloseModeName::QueuedDelete:
            return localReplicaDeleted_;

        case ReplicaCloseModeName::Drop:
        case ReplicaCloseModeName::Abort:
        case ReplicaCloseModeName::Deactivate:
        case ReplicaCloseModeName::Delete:
        default:
            return false;
        }
    }

    return false;
}

void FailoverUnit::StartCloseLocalReplica(
    ReplicaCloseMode closeMode,
    Federation::NodeInstance const & senderNode,
    FailoverUnitEntityExecutionContext & executionContext,
    ActivityDescription const & activityDescription)
{
    auto & queue = executionContext.Queue;

    TESTASSERT_IF(!CanCloseLocalReplica(closeMode), "CanClose should be true {0}", *this);

    //If the Local Replica is closing, clear any existing reconfiguration
    //related health reports
    OnPhaseChanged(queue);

    bool isReplicaDropImplied = closeMode.IsDropImplied;
    bool isDeleteReplicaImplied = closeMode.IsDeleteImplied;
    bool isForce = closeMode.IsForceDropImplied;

    // If the FT is closed
    if (IsClosed)
    {
        if (isDeleteReplicaImplied)
        {
            MarkLocalReplicaDeleted(executionContext);
        }

        return;
    }

    endpointPublishState_.Clear(executionContext.StateUpdateContextObj);
    fmMessageState_.OnClosing(executionContext.StateUpdateContextObj);

    if (closeMode == ReplicaCloseMode::AppHostDown && replicaCloseMode_ == ReplicaCloseMode::Obliterate)
    {
        // If the app host went down while the replica was closing to obliterate
        // Finish the close
        FinishCloseLocalReplica(executionContext);
        return;
    }

    // If the FT is Down then there is nothing that can be done here
    // Unless the close mode requires a force cleanup of the replica
    // i.e. leak state and move on
    if (!LocalReplica.IsUp && !isForce && closeMode != ReplicaCloseMode::QueuedDelete)
    {
        return;
    }

    Diagnostics::ReplicaStateChangeEventData data(
        executionContext.NodeInstance.ToString(),
        FailoverUnitId.Guid,
        LocalReplicaId,
        CurrentConfigurationEpoch,
        ReplicaLifeCycleState::Enum::Closing,
        LocalReplicaRole,
        activityDescription);

    auto action = make_unique<Diagnostics::TraceEventStateMachineAction<Diagnostics::ReplicaStateChangeEventData>>(move(data));
    queue.Enqueue(move(action));

    executionContext.UpdateContextObj.EnableInMemoryUpdate();

    auto preState = LocalReplica.State;

    if (isDeleteReplicaImplied)
    {
        MarkLocalReplicaDeleted(executionContext);
    }

    senderNode_ = senderNode;
    localReplicaOpenPending_.SetValue(false, queue);
    replicaCloseMode_ = closeMode;
    localReplicaClosePending_.SetValue(true, queue);
    localReplicaServiceDescriptionUpdatePendingFlag_.SetValue(false, queue);
    messageRetryActiveFlag_.SetValue(false, queue);

    if (localReplicaDeleted_)
    {
        retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaDelete);
    }
    else
    {
        retryableErrorState_.EnterState(RetryableErrorStateName::ReplicaClose);
    }
    
    if (isReplicaDropImplied)
    {
        executionContext.UpdateContextObj.EnableUpdate();
        LocalReplica.State = ReplicaStates::InDrop;
    }

    // At this time all state updates are complete

    if (closeMode == ReplicaCloseMode::AppHostDown || closeMode == ReplicaCloseMode::QueuedDelete)
    {
        // Close the replica correctly
        FinishCloseLocalReplica(executionContext);
        return;
    }

    if (ServiceTypeRegistration == nullptr && isForce)
    {
        // There is no registration and the close mode is force perform the cleanup
        FinishCloseLocalReplica(executionContext);
        return;
    }

    if (ServiceTypeRegistration == nullptr && preState == ReplicaStates::InCreate)
    {
        // The replica is being created (there is no data on the node)
        // The close can be completed immediately
        FinishCloseLocalReplica(executionContext);
        return;
    }

    if (ServiceTypeRegistration == nullptr && preState == ReplicaStates::StandBy)
    {
        // if there is no STR and there is data for the replica and the request was to CR(None) it then
        // set the retryable error state to find service type registration at drop and start looking for the registration
        // Otherwise there is no STR and the request is to close the replica then complete
        if (isReplicaDropImplied)
        {
            serviceTypeRegistration_.Start(LocalReplica.State);
            return;
        }
        else
        {
            FinishCloseLocalReplica(executionContext);
            return;
        }
    }

    if (ServiceTypeRegistration != nullptr && closeMode == ReplicaCloseMode::Obliterate)
    {
        /*
            For obliterate of replicas hosted in fabric.exe transition directly to dropped and enqueue
            the action to terminate fabric.exe (via an assert). This action will execute under the FT lock
            so there will be no possibility of any other message being processed

            For all other replicas terminate the app host and once that termination completes
            transition the replica to dropped
        */
        executionContext.Hosting.AddTerminateServiceHostAction(
            TerminateServiceHostReason::RemoveReplica,
            ServiceTypeRegistration,
            queue);

        if (Utility2::SystemServiceHelper::GetHostType(*this) == Utility2::SystemServiceHelper::HostType::Fabric)
        {
            FinishCloseLocalReplica(executionContext);
        }

        return;
    }
}

Common::ErrorCode FailoverUnit::ProcessReplicaCloseRetry(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    FailoverUnitEntityExecutionContext & executionContext)
{
    // No-OP for obliterate close mode
    if (CloseMode == ReplicaCloseMode::Obliterate)
    {
        return ErrorCode::Success();
    }

    // Trying to drop a SB replica for which drop had been started
    // But the replica went down or the node restarted
    // This replica restarted as ID U and the drop should be completed
    // RA will try to acquire the service type registration here 
    // (The standard rules apply - RA will report health if it is unable to acquire STR or there are too many failures)
    // (In which case since FM had asked to drop the replica it is fine for it to drop)
    TESTASSERT_IF(!IsRuntimeActive && LocalReplica.State != ReplicaStates::InDrop, "Replica must be in drop (earlier SB) if runtime is not active and close pending is set {0}", *this);
    auto error = TryGetAndAddServiceTypeRegistration(registration, executionContext);
    if (error.IsError(ErrorCodeValue::RAServiceTypeNotRegistered))
    {
        return ErrorCode::Success();
    }
    else if (!error.IsSuccess())
    {
        return error;
    }

    TESTASSERT_IF(!IsRuntimeActive, "must have STR or TryGetAndAddSTR could not have succeeded {0}", *this);
    SendReplicaCloseMessage(executionContext.Queue);
    return ErrorCode::Success();
}

ErrorCode FailoverUnit::TryGetAndAddServiceTypeRegistration(
    Hosting2::ServiceTypeRegistrationSPtr const & incomingRegistration,
    FailoverUnitEntityExecutionContext & executionContext)
{
    if (ServiceTypeRegistration != nullptr)
    {
        return ErrorCode::Success();
    }

    TESTASSERT_IF(RetryableErrorStateObj.CurrentState == RetryableErrorStateName::None, "retryableErrorState_ must have retryable error state here {0}.", *this);

    auto healthReportInformation = make_pair<ReplicaHealthEvent::Enum, IHealthDescriptorUPtr>(ReplicaHealthEvent::OK, make_unique<ClearWarningErrorHealthReportDescriptor>());
    auto error = serviceTypeRegistration_.TryGetAndAddServiceTypeRegistration(incomingRegistration, executionContext.Hosting, executionContext.Queue, executionContext.Config, healthReportInformation);

    if (healthReportInformation.first == ReplicaHealthEvent::ServiceTypeRegistrationWarning)
    {
        EnqueueHealthEvent(healthReportInformation.first, executionContext.Queue, move(healthReportInformation.second));
    }

    if (healthReportInformation.first == ReplicaHealthEvent::ClearServiceTypeRegistrationWarning)
    {
        EnqueueHealthEvent(healthReportInformation.first, executionContext.Queue, move(healthReportInformation.second));
    }

    if (error.IsSuccess())
    {
        InternalUpdateLocalReplicaPackageVersionInstance(ServiceTypeRegistration->servicePackageVersionInstance);
    }
    else if (!error.IsError(ErrorCodeValue::RAServiceTypeNotRegistered))
    {
        // Here the only 2 cases in which error is not RAServiceTypeNotRegistered are:
        // 1. error is retryable and action == RetryableErrorAction::Drop;
        // 2. error is not Successful and is not retryable;
        ProcessFindServiceTypeRegistrationFailure(executionContext);
    }

    return error;
}

Common::ErrorCode FailoverUnit::ProcessReplicaOpenRetry(
    Hosting2::ServiceTypeRegistrationSPtr const & registration,
    FailoverUnitEntityExecutionContext & executionContext)
{
    RAMessage const * messageType = nullptr;

    // Identify all the parameters based on whether this is an existing replica or a new replica
    // For a new replica RA will use different configurations which are much smaller
    switch (OpenMode)
    {
    case RAReplicaOpenMode::ChangeRole:
        messageType = &RAMessage::GetReplicaOpen();
        TESTASSERT_IF(!IsRuntimeActive, "Runtime must be active as the replica is open {0}", *this);
        break;

    case RAReplicaOpenMode::Open:
        messageType = &RAMessage::GetReplicaOpen();
        break;

    case RAReplicaOpenMode::Reopen:
        messageType = &RAMessage::GetStatefulServiceReopen();
        break;

    default:
        Assert::TestAssert("Unknown replica state at replica open retry {0}", *this);
    }

    auto error = TryGetAndAddServiceTypeRegistration(registration, executionContext);
    if (error.IsError(ErrorCodeValue::RAServiceTypeNotRegistered))
    {
        // Indicate to caller that everything is good and retry needs to continue
        return ErrorCode::Success();
    }
    else if (!error.IsSuccess())
    {
        // Other fatal error
        return error;
    }

    // At this point we have the service type registration so send message to RAP
    TESTASSERT_IF(ServiceTypeRegistration == nullptr, "Cannot be null {0}", *this);
    
    ProxyMessageFlags::Enum flags = RetryableErrorStateObj.IsLastRetryBeforeDrop(executionContext.Config) ? ProxyMessageFlags::Abort : ProxyMessageFlags::None;
    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        executionContext.Queue,
        *messageType,
        ServiceTypeRegistration,
        FailoverUnitDescription,
        LocalReplica.ReplicaDescription,
        ServiceDescription,
        flags);

    return ErrorCode::Success();
}

void FailoverUnit::ProcessFindServiceTypeRegistrationFailure(
    FailoverUnitEntityExecutionContext & executionContext)
{
    // FSR can fail either during open or during close
    if (LocalReplicaOpenPending.IsSet)
    {
        ProcessOpenFailure(executionContext);
    }
    else if (LocalReplicaClosePending.IsSet)
    {
        FinishCloseLocalReplica(executionContext);
    }
}

void FailoverUnit::ProcessOpenFailure(
    FailoverUnitEntityExecutionContext & executionContext)
{
    TESTASSERT_IF(!LocalReplicaOpenPending.IsSet, "Should be called if oepn is pending {0}", *this);
    auto & hosting = executionContext.Hosting;
    auto & queue = executionContext.Queue;

    // Snapshot state that can be reset
    auto openMode = OpenMode;
    if (openMode == RAReplicaOpenMode::Open || openMode == RAReplicaOpenMode::Reopen)
    {
        // The limit for continuous failures on open has been hit
        // according to the protocol RAP has closed the FT so
        // all RA needs to do is update the state to dropped
        // The reply will be sent by the caller if needed 
        UpdateStateOnLocalReplicaDropped(hosting, queue);

        EnqueueHealthEvent(ReplicaHealthEvent::Close, queue, make_unique<PartitionHealthEventDescriptor>());

        if (openMode == RAReplicaOpenMode::Reopen)
        {
            fmMessageState_.OnDropped(executionContext.StateUpdateContextObj);
        }
    }
    else if (openMode == RAReplicaOpenMode::ChangeRole)
    {
        // This replica is still open on RAP - it was a SB replica that was open
        // and then RA was trying to change role
        // CR has hit the failure threshold so the replica needs to be dropped
        // Since RAP still has the replica as open send a message to CR(None) it and drop
        StartCloseLocalReplica(ReplicaCloseMode::Abort, ReconfigurationAgent::InvalidNode, executionContext, ActivityDescription::Empty);        
    }
    else
    {
        Assert::TestAssert("Unknown open mode {0}", static_cast<int>(openMode));
    }
}

void FailoverUnit::ProcessReplicaOpenReply(
    Reliability::FailoverConfig const & config,
    Reliability::ReplicaDescription const & replicaDescription,
    TransitionErrorCode const & transitionErrorCode,
    FailoverUnitEntityExecutionContext & executionContext)
{
    StateMachineActionQueue & queue = executionContext.Queue;

    if (!transitionErrorCode.IsSuccess() && !transitionErrorCode.IsError(ErrorCodeValue::RAProxyStateChangedOnDataLoss))
    {
        auto action = retryableErrorState_.OnFailure(config);

        if (action == RetryableErrorAction::Drop)
        {
            ProcessOpenFailure(executionContext);
            return;
        }

        if (action == RetryableErrorAction::ReportHealthWarning)
        {
            auto healthDescriptor = make_unique<TransitionHealthReportDescriptor>(transitionErrorCode);
            EnqueueHealthEvent(ReplicaHealthEvent::OpenWarning, queue, move(healthDescriptor));
        }

        if (transitionErrorCode.IsError(ErrorCodeValue::ApplicationHostCrash))
        {
            serviceTypeRegistration_.ReleaseServiceTypeRegistrationAndStart(executionContext.Hosting, queue, LocalReplica.State);
            localReplicaOpen_ = false;
        }
    }
    else
    {
        ReplicaStates::Enum replicaState = ReplicaStates::Dropped;
        bool setReplicaUpPending = false;
        bool updateReplicaEndpoint = false;
        bool updateReplicatorEndpoint = false;
        bool updateDeactivationInfo = false;

        switch (OpenMode)
        {
        case RAReplicaOpenMode::ChangeRole:
        case RAReplicaOpenMode::Open:
            updateReplicaEndpoint = true;

            /*
                In open mode change role, ideally RA should have the replicator end point already
                (Open Mode ChangeRole is when a SB replica is opened to either P or I)
                However, due to code in CreateLocalReplica which resets the endpoint of the replicator
                the update replicator endpoint is also required
            */
            updateReplicatorEndpoint = true;

            if (!IsStateful || LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary)
            {
                replicaState = ReplicaStates::Ready;
            }
            else
            {
                TESTASSERT_IF(LocalReplica.CurrentConfigurationRole != ReplicaRole::Idle, "Must be either P or I when ReplicaOpen/ChangeRole is happening {0}", *this);
                replicaState = ReplicaStates::InBuild;
            }

            if (LocalReplica.CurrentConfigurationRole == ReplicaRole::Primary)
            {
                updateDeactivationInfo = true;
            }

            break;

        case RAReplicaOpenMode::Reopen:
            TESTASSERT_IF(!IsStateful, "Must be stateful for open mode reopen {0}", *this);
            replicaState = ReplicaStates::StandBy;
            setReplicaUpPending = true;
            updateReplicatorEndpoint = true;
            break;
        }

        TESTASSERT_IF(replicaState == ReplicaStates::Dropped, "Must have been set by the code above {0}", *this);

        LocalReplica.State = replicaState;
        LocalReplica.MessageStage = ReplicaMessageStage::None;
        localReplicaOpenPending_.SetValue(false, queue);
        senderNode_ = ReconfigurationAgent::InvalidNode;

        // Local replica is open
        localReplicaOpen_ = true;

        if (updateDeactivationInfo)
        {
            int64 catchupLSN = 0; // New primary so 0 is fine
            if (failoverUnitDesc_.IsDataLossBetweenPCAndCC && transitionErrorCode.IsError(ErrorCodeValue::RAProxyStateChangedOnDataLoss))
            {
                TESTASSERT_IF(replicaDescription.LastAcknowledgedLSN == FABRIC_INVALID_SEQUENCE_NUMBER, "Sequence number must be valid in reply for primary on data loss {0} {1}", replicaDescription, *this);
                catchupLSN = replicaDescription.LastAcknowledgedLSN;
            }

            deactivationInfo_ = ReplicaDeactivationInfo(CurrentConfigurationEpoch, catchupLSN);
        }

        if (setReplicaUpPending)
        {
            fmMessageState_.OnReplicaUp(executionContext.StateUpdateContextObj);
        }

        if (updateReplicaEndpoint)
        {
            UpdateServiceEndpoint(LocalReplica, replicaDescription, executionContext);
        }

        if (updateReplicatorEndpoint)
        {
            UpdateReplicatorEndpoint(LocalReplica, replicaDescription);
        }

        auto action = retryableErrorState_.OnSuccessAndTransitionTo(RetryableErrorStateName::None, config);
        if (action == RetryableErrorAction::ClearHealthReport)
        {
            EnqueueHealthEvent(ReplicaHealthEvent::ClearOpenWarning, queue, make_unique<ClearWarningErrorHealthReportDescriptor>());
        }
    }
}

void FailoverUnit::ProcessReplicaCloseReply(
    TransitionErrorCode const & transitionErrorCode,
    FailoverUnitEntityExecutionContext & executionContext)
{
    if (CloseMode == ReplicaCloseMode::Obliterate)
    {
        // No-Op if the close mode is obliterate
        return;
    }

    StateMachineActionQueue & queue = executionContext.Queue;

    if (transitionErrorCode.IsError(ErrorCodeValue::ApplicationHostCrash))
    {
        // Remove STR and set new state for serviceTypeRegistration_ for next retry
        serviceTypeRegistration_.ReleaseServiceTypeRegistrationAndStart(executionContext.Hosting, queue, LocalReplica.State);
        localReplicaOpen_ = false;
    }    

    if (!transitionErrorCode.IsSuccess())
    {
        auto action = retryableErrorState_.OnFailure(executionContext.Config);

        if (action == RetryableErrorAction::Drop)
        {
            // force drop the replica
            StartCloseLocalReplica(ReplicaCloseMode::ForceAbort, ReconfigurationAgent::InvalidNode, executionContext, ActivityDescription::Empty);
            return;
        }

        if (action == RetryableErrorAction::ReportHealthWarning)
        {
            auto healthDescriptor = make_unique<TransitionHealthReportDescriptor>(transitionErrorCode);
            EnqueueHealthEvent(ReplicaHealthEvent::CloseWarning, queue, move(healthDescriptor));
        }
    }
    else
    {
        TESTASSERT_IF(RetryableErrorStateObj.CurrentState == RetryableErrorStateName::None, "retryableErrorState_ {0} should be ReplicaClose here.", RetryableErrorStateObj.CurrentState);

        auto action = retryableErrorState_.OnSuccessAndTransitionTo(RetryableErrorStateName::None, executionContext.Config);
        if (action == RetryableErrorAction::ClearHealthReport)
        {
            EnqueueHealthEvent(ReplicaHealthEvent::ClearCloseWarning, queue, make_unique<ClearWarningErrorHealthReportDescriptor>());
        }

        FinishCloseLocalReplica(executionContext);
    }
}

bool FailoverUnit::ShouldRetryCloseReplica()
{
    return HasPersistedState && LocalReplicaClosePending.IsSet && !CloseMode.IsAppHostDownTerminalTransition;
}

void FailoverUnit::ProcessHealthOnRoleTransition(
    ProxyErrorCode const & proxyErrorCode,
    FailoverUnitEntityExecutionContext & executionContext)
{
    StateMachineActionQueue & queue = executionContext.Queue;
    auto & config = executionContext.Config;

    if (proxyErrorCode.IsError(ErrorCodeValue::RAProxyDemoteCompleted))
    {
        TESTASSERT_IF(proxyErrorCode.IsUserApiFailure, "proxyErrorCode {0} should not be UserApiFailure {1}", proxyErrorCode, *this);
    }

    if (proxyErrorCode.IsUserApiFailure)
    {
        auto action = retryableErrorState_.OnFailure(config);

        if (action == RetryableErrorAction::Restart)
        {
            // Restart replica
            if (HasPersistedState)
            {
                StartCloseLocalReplica(ReplicaCloseMode::Restart, ReconfigurationAgent::InvalidNode, executionContext, ActivityDescription::Empty);
            }
            else
            {
                StartCloseLocalReplica(ReplicaCloseMode::Drop, ReconfigurationAgent::InvalidNode, executionContext, ActivityDescription::Empty);
            }
        }
        else if (action == RetryableErrorAction::ReportHealthError)
        {
            // Enqueue health error
            auto healthDescriptor = make_unique<TransitionHealthReportDescriptor>(proxyErrorCode);
            EnqueueHealthEvent(ReplicaHealthEvent::Error, queue, move(healthDescriptor));
        }
    }
    else
    {
        auto action = retryableErrorState_.OnSuccessAndTransitionTo(RetryableErrorStateName::None, config);
        if (action == RetryableErrorAction::ClearHealthReport)
        {
            EnqueueHealthEvent(ReplicaHealthEvent::ClearError, queue, make_unique<ClearWarningErrorHealthReportDescriptor>());
        }
    }
}

void FailoverUnit::FinishCloseLocalReplica(FailoverUnitEntityExecutionContext & executionContext)
{
    auto & hosting = executionContext.Hosting;
    auto & queue = executionContext.Queue;

    TESTASSERT_IF(!LocalReplicaClosePending.IsSet, "ClosePending must be set {0}", *this);
    TESTASSERT_IF(!LocalReplicaDeleted && replicaCloseMode_.IsDeleteImplied, "IsDeleted must have been set {0}", *this);

    executionContext.UpdateContextObj.EnableUpdate();

    // Snapshot values that may be reset
    auto mode = replicaCloseMode_;

    // Entity Create health report would have been sent if replica is open and up
    // Delete health report needs to be sent only if created
    bool reportClosedHealthEvent = IsOpen && LocalReplica.IsUp;
    bool isDropImplied = mode.IsDropImplied;

    if (!HasPersistedState)
    {
        UpdateStateOnLocalReplicaDropped(hosting, queue);
    }
    else if (isDropImplied && mode != ReplicaCloseMode::QueuedDelete)
    {
        // For queued deletes transition to down, deleted
        UpdateStateOnLocalReplicaDropped(hosting, queue);
    }
    else
    {
        UpdateStateOnLocalReplicaDown(hosting, queue);
    }

    // Set the appropriate flags depending on the mode
    switch (mode.Name)
    {
    case ReplicaCloseModeName::Close:
    case ReplicaCloseModeName::DeactivateNode:
        fmMessageState_.OnReplicaDown(HasPersistedState, LocalReplicaInstanceId, executionContext.StateUpdateContextObj);
        break;

    case ReplicaCloseModeName::Restart:
        // On a restart don't send a delete health report
        // A new health report with the new instance will be sent when
        // the state is further updated to reflect the new replica instance
        reportClosedHealthEvent = false;
        fmMessageState_.OnReplicaDown(HasPersistedState, LocalReplicaInstanceId, executionContext.StateUpdateContextObj);
        break;

    case ReplicaCloseModeName::AppHostDown:
        fmMessageState_.OnReplicaDown(HasPersistedState, LocalReplicaInstanceId, executionContext.StateUpdateContextObj);
        break;

    case ReplicaCloseModeName::Drop:
    case ReplicaCloseModeName::Abort:
    case ReplicaCloseModeName::ForceAbort:
    case ReplicaCloseModeName::Obliterate:
    case ReplicaCloseModeName::Deactivate:
        fmMessageState_.OnDropped(executionContext.StateUpdateContextObj);
        break;

    case ReplicaCloseModeName::Delete:    
    case ReplicaCloseModeName::ForceDelete:
    case ReplicaCloseModeName::QueuedDelete:
        break;

    default:
        Assert::CodingError("Unknown close mode {0}", static_cast<int>(mode.Name));
    }

    if (reportClosedHealthEvent)
    {
        EnqueueHealthEvent(ReplicaHealthEvent::Close, queue, make_unique<PartitionHealthEventDescriptor>());
    }
}

void FailoverUnit::ProcessReadWriteStatusRevokedNotification(
    Reliability::ReplicaMessageBody const & body,
    FailoverUnitEntityExecutionContext & context)
{
    // Send the reply to RAP always - there is no need for RAP to keep retrying this message
    // The first call to this function will process it
    SendReadWriteStatusRevokedNotificationReply(body, context.Queue);

    ReadWriteStatusRevokedNotification(context);
}

void FailoverUnit::ReadWriteStatusRevokedNotification(
    FailoverUnitEntityExecutionContext & executionContext)
{
    if (!LocalReplicaClosePending.IsSet)
    {
        return;
    }

    // TODO: Move this as a property of the name?
    bool isProcessAllowed = false;
    switch (replicaCloseMode_.Name)
    {
    case ReplicaCloseModeName::Close:
    case ReplicaCloseModeName::Drop:
    case ReplicaCloseModeName::DeactivateNode:
    case ReplicaCloseModeName::Abort:
    case ReplicaCloseModeName::Restart:
    case ReplicaCloseModeName::Deactivate:
        isProcessAllowed = true;
        break;

    default:
        break;
    }

    if (!isProcessAllowed)
    {
        // Some scenarios do not require replica down to be sent to fm
        return;
    }

    fmMessageState_.OnReplicaDown(HasPersistedState, LocalReplicaInstanceId, executionContext.StateUpdateContextObj);
}

void FailoverUnit::UpdateStateOnLocalReplicaDown(Hosting::HostingAdapter & hosting, StateMachineActionQueue & queue)
{
    TESTASSERT_IF(!HasPersistedState, "Cannot invoke this if it has persisted state");

    ResetLocalState(queue);

    serviceTypeRegistration_.OnReplicaDown(hosting, queue);

    LocalReplica.UpdateStateOnLocalReplicaDown(true);

    for (auto & replica : replicaStore_.ConfigurationRemoteReplicas)
    {
        replica.UpdateStateOnLocalReplicaDown(false);
    }

    replicaStore_.IdleReplicas.Clear();
}

void FailoverUnit::UpdateStateOnLocalReplicaDropped(Hosting::HostingAdapter & hosting, StateMachineActionQueue & queue)
{
    ResetLocalState(queue);

    state_ = FailoverUnitStates::Closed;
    replicaStore_.Clear();    
    deactivationInfo_ = ReplicaDeactivationInfo::Dropped;

    if (LocalReplicaDeleted)
    {
        cleanupPendingFlag_.SetValue(true, queue);
    }

    serviceTypeRegistration_.OnReplicaClosed(hosting, queue);
}

void FailoverUnit::ResetLocalState(StateMachineActionQueue & queue)
{
    ResetFlags(queue);
    ResetNonFlagState();
}

void FailoverUnit::ResetFlags(StateMachineActionQueue & queue)
{
    localReplicaClosePending_.SetValue(false, queue);
    localReplicaOpenPending_.SetValue(false, queue);
    localReplicaServiceDescriptionUpdatePendingFlag_.SetValue(false, queue);
    messageRetryActiveFlag_.SetValue(false, queue);
}

void FailoverUnit::ResetNonFlagState()
{
    senderNode_ = ReconfigurationAgent::InvalidNode;

    // Reconfiguration Related State
    dataLossVersionToReport_ = 0;
    reconfigState_.Reset();
    changeConfigReplicaDescList_.clear();

    // Replicator related state
    updateReplicatorConfiguration_ = false;

    lastUpdated_ = DateTime::Now();

    // Replica Lifecycle state
    localReplicaOpen_ = false;
    retryableErrorState_.Reset();
    replicaCloseMode_ = ReplicaCloseMode::None;
}

bool FailoverUnit::CanProcessUpdateServiceDescriptionReply(
    ProxyUpdateServiceDescriptionReplyMessageBody const & body) const
{
    // The following checks apply
    // Replica Id, Instance Id must match
    // Incoming Service Description Update Version must match
    // Result must be success

    if (!localReplicaServiceDescriptionUpdatePendingFlag_.IsSet)
    {
        return false;
    }

    if (!body.ErrorCode.IsSuccess())
    {
        return false;
    }

    if (body.LocalReplicaDescription.ReplicaId != LocalReplicaId ||
        body.LocalReplicaDescription.InstanceId != LocalReplicaInstanceId)
    {
        return false;
    }

    if (body.ServiceDescription.UpdateVersion < serviceDesc_.UpdateVersion)
    {
        return false;
    }

    TESTASSERT_IF(body.ServiceDescription.UpdateVersion > serviceDesc_.UpdateVersion, "RAP cannot have higher version. Incoming {0}\r\n. this = {1}", body, *this);

    return true;
}

void FailoverUnit::ProcessUpdateServiceDescriptionReply(StateMachineActionQueue & queue, ProxyUpdateServiceDescriptionReplyMessageBody const & body)
{
    TESTASSERT_IF(!CanProcessUpdateServiceDescriptionReply(body), "Failed safety check {0} {1}", body, *this);

    localReplicaServiceDescriptionUpdatePendingFlag_.SetValue(false, queue);
}

void FailoverUnit::ProcessReplicaEndpointUpdatedReply(
    ReplicaReplyMessageBody const& msgBody,
    FailoverUnitEntityExecutionContext& executionContext)
{
    if (!msgBody.FailoverUnitDescription.CurrentConfigurationEpoch.IsPrimaryEpochEqual(CurrentConfigurationEpoch) || !msgBody.ErrorCode.IsSuccess())
    {
        return;
    }

    endpointPublishState_.OnFmReply(executionContext.StateUpdateContextObj);    
}

void FailoverUnit::ProcessReplicaDroppedReply(
    Common::ErrorCode const & error,
    Reliability::ReplicaDescription const & incomingDesc,
    FailoverUnitEntityExecutionContext & context)
{
    OnReplicaUploaded(context);

    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FMFailoverUnitNotFound))
    {
        return;
    }


    if (LocalReplicaInstanceId != incomingDesc.InstanceId)
    {
        return;
    }

    fmMessageState_.OnReplicaDroppedReply(context.StateUpdateContextObj);
}

void FailoverUnit::ProcessReplicaUpReply(
    ReplicaDescription const & desc,
    FailoverUnitEntityExecutionContext & context)
{
    OnReplicaUploaded(context);

    if (fmMessageState_.MessageStage == FMMessageStage::ReplicaDropped)
    {
        // Incoming description must be dropped otherwise it is a stale message
        // Consider the case where replica reopened and RA sent replica up SB U 1:1
        // Then FM sent a reply which was delayed
        // Now Replica gets dropped and RA has DD D 1:1 ReplicaDropped pending
        // Delayed FM reply cannot clear
        if (desc.State != Reliability::ReplicaStates::Dropped)
        {
            return; 
        }

        fmMessageState_.OnReplicaDroppedReply(context.StateUpdateContextObj);
    }
    else if (fmMessageState_.MessageStage == FMMessageStage::ReplicaDown)
    {
        if (IsClosed && LocalReplicaInstanceId != desc.InstanceId)
        {
            return;
        }

        // Incoming must be down otherwise it can be the case that
        // Replica opens and SB U i1 ReplicaUp is sent
        // Now ReplicaUpReply is received
        // Now replica goes down
        // The state is SB D i1 -> with replica down pending
        // Now delayed replicaupreply with SB U i1 comes
        // It is stale
        if (desc.IsUp)
        {
            return;
        }

        fmMessageState_.OnReplicaDownReply(desc.InstanceId, context.StateUpdateContextObj);
    }
    else if (fmMessageState_.MessageStage == FMMessageStage::ReplicaUp)
    {
        // FT can be either in the Dropped list or the Up list in the incoming message
        // The ReplicaUpReply means that replica up has been acknowledged
        // So we do not need to send the message again
        // This is only true if the instance id matches
        // In the scenario where we sent the replica as SB D i1 (Reopen Success Wait Interval expired during initial replica upload)
        // The instance on the local replica will be higher (i2) and hence the instance id check is required
        if (desc.InstanceId != LocalReplicaInstanceId)
        {
            return;
        }

        fmMessageState_.OnReplicaUpAcknowledged(context.StateUpdateContextObj);
    }
}

void FailoverUnit::OnReplicaUploaded(
    FailoverUnitEntityExecutionContext & context)
{
    replicaUploadState_.OnUploaded(context.StateUpdateContextObj);
}

void FailoverUnit::OnFMMessageSent(
    Common::StopwatchTime now,
    int64 sequenceNumber,
    FailoverUnitEntityExecutionContext & context) 
{
    fmMessageState_.OnRetry(now, sequenceNumber, context.StateUpdateContextObj);
}

bool FailoverUnit::TryComposeFMMessage(
    NodeInstance const & nodeInstance,
    StopwatchTime now,
    FMMessageData & message) const
{
    message = FMMessageData();

    int64 sequenceNumber = 0;
    if (FMMessageStage == FMMessageStage::None || !fmMessageState_.ShouldRetry(now, sequenceNumber))
    {
        return false;
    }

    switch (FMMessageStage)
    {
    case FMMessageStage::ReplicaUp:
    case FMMessageStage::ReplicaUpload:
    case FMMessageStage::ReplicaDropped:
    case FMMessageStage::ReplicaDown:
        message = ComposeReplicaUpMessageBody(nodeInstance, sequenceNumber);
        return true;

    case FMMessageStage::EndpointAvailable:
    {
        ReplicaMessageBody body(FailoverUnitDescription, LocalReplica.ReplicaDescription, ServiceDescription);
        message = FMMessageData(FMMessageStage, move(body), sequenceNumber);
        return true;
    }

    default:
        Assert::TestAssert("Unknown fm message stage at compose {0}", *this);
        return false;
    }
}

FMMessageData FailoverUnit::ComposeReplicaUpMessageBody(
    NodeInstance const & nodeInstance,
    int64 msgSequenceNumber) const
{
    FailoverUnitInfo ftInfo;
    
    auto rv = TryGetConfiguration(nodeInstance, FMMessageStage == FMMessageStage::ReplicaDown, ftInfo);
    
    TESTASSERT_IF(!rv, "Failed to get configuration when fm message is pending {0}", *this);
    TESTASSERT_IF(FMMessageStage == FMMessageStage::ReplicaDown && ftInfo.LocalReplica.IsUp, "Local replica is up in message when replica down is requested {0}\r\n{1}", ftInfo, *this);
    TESTASSERT_IF(FMMessageStage == FMMessageStage::ReplicaDropped && !ftInfo.LocalReplica.IsDropped, "Local replica is not dropped in message when replica dropped is requested {0}\r\n{1}", ftInfo, *this);
    
    bool isInDroppedList = ftInfo.LocalReplica.IsDropped;
    return FMMessageData(FMMessageStage, move(ftInfo), isInDroppedList, msgSequenceNumber);
}

void FailoverUnit::AssertIfReplicaNotUploaded(wstring const & action) const
{
    TESTASSERT_IF(replicaUploadState_.IsUploadPending, "Replica upload still pending but received {0}. FT: {1}", action, *this);
}

bool FailoverUnit::ValidateUpdateServiceDescription(Reliability::ServiceDescription const & serviceDescription)
{
    // Check that the updated description applies to this FT
    // - The SD name must match
    // - The Instance must match
    // - The FT must have a lesser update version on its sd
    if (ServiceDescription.Name != serviceDescription.Name)
    {
        return false;
    }

    if (ServiceDescription.Instance != serviceDescription.Instance)
    {
        return false;
    }

    if (ServiceDescription.UpdateVersion >= serviceDescription.UpdateVersion)
    {
        return false;
    }

    return true;
}

void FailoverUnit::UpdateServiceDescription(StateMachineActionQueue & queue, Reliability::ServiceDescription const & serviceDescription)
{
    TESTASSERT_IFNOT(ValidateUpdateServiceDescription(serviceDescription), "Validation failure. Incoming {0}\r\nCurrent {1}\r\nFT: {2}", serviceDescription, serviceDesc_, *this);

    InternalSetServiceDescription(serviceDescription);

    UpdateServiceDescriptionUpdatePendingFlag(queue);
}

void FailoverUnit::UpdateServiceDescriptionUpdatePendingFlag(StateMachineActionQueue & queue)
{
    // The following scenarios apply wrt setting the flag for RAP
    // FT is Closed: then the subsequent Open will send the correct information to RAP
    // Replica is down: subsequent open will update the SD
    // Replica is Closing: There is no need to send the updated SD to RAP
    // Replica is Open/Opening/Reopening: RA should send explicit message to RAP at this time
    //  except if there is no service type registration
    if (IsClosed || localReplicaClosePending_.IsSet || !LocalReplica.IsUp || (localReplicaOpenPending_.IsSet && !IsRuntimeActive))
    {
        return;
    }

    localReplicaServiceDescriptionUpdatePendingFlag_.SetValue(true, queue);

    ReconfigurationAgent::AddActionSendMessageToRAProxy(
        queue,
        RAMessage::GetUpdateServiceDescription(),
        ServiceTypeRegistration,
        failoverUnitDesc_,
        LocalReplica.ReplicaDescription,
        serviceDesc_);
}

void FailoverUnit::InternalUpdateLocalReplicaPackageVersionInstance(ServicePackageVersionInstance const & newVersionInstance)
{
    // It is important to keep service description and local replica in sync for rebuild
    LocalReplica.PackageVersionInstance = newVersionInstance;
    serviceDesc_.PackageVersionInstance = newVersionInstance;
}

void FailoverUnit::InternalSetServiceDescription(Reliability::ServiceDescription const & serviceDescription)
{
    serviceDesc_ = serviceDescription;
    UpdateFailoverUnitDescriptionForBackwardCompatiblity();

    // Whenever the SD is set keep the package version instance in sync
    // (UpdateService and CreateReplica both set this)
    if (IsOpen)
    {
        serviceDesc_.PackageVersionInstance = LocalReplica.PackageVersionInstance;
    }
}

void FailoverUnit::InternalSetFailoverUnitDescription(Reliability::FailoverUnitDescription const & ftDesc)
{
    failoverUnitDesc_ = ftDesc;
    UpdateLastStableEpochForBackwardCompatibility();
}

void FailoverUnit::UpdateLastStableEpochForBackwardCompatibility()
{
    /*
        RA stores the deactivation information (epoch, catchup lsn) version 4.0 CU onwards
        Newer versions use this information to identify false progress during reconfiguration

        Prior to this, RA used last stable epoch to identify false progress during reconfiguration
        The semantics of this were that any replica with a last stable epoch lower than the pc would
        be ignored in get lsn considerations

        If fabric is upgraded past 4.0 CU onwards then it must still maintain this field
        because otherwise on a rollback this would be lower than the pc epoch and cause
        undetected data loss
    */
    lastStableEpoch_ = PreviousConfigurationEpoch;
}

void FailoverUnit::UpdateFailoverUnitDescriptionForBackwardCompatiblity()
{
    // Copy over the target and min replica set size from the service description to maintain V1 compatiblity
    if (failoverUnitDesc_.TargetReplicaSetSize != serviceDesc_.TargetReplicaSetSize)
    {
        failoverUnitDesc_.TargetReplicaSetSize = serviceDesc_.TargetReplicaSetSize;
    }

    if (failoverUnitDesc_.MinReplicaSetSize != serviceDesc_.MinReplicaSetSize)
    {
        failoverUnitDesc_.MinReplicaSetSize = serviceDesc_.MinReplicaSetSize;
    }
}

void FailoverUnit::FixupServiceDescriptionOnLfumLoad()
{
    if (IsClosed)
    {
        return;
    }

    /*
        In earlier version the package version instance is maintained at two places
        It is possible that the package version on the service description may be out of sync
        This breaks the invariant that the package version instance on service description and
        local replica be in sync.

        Fixup the service description over here
    */
    if (serviceDesc_.PackageVersionInstance.InstanceId > LocalReplica.PackageVersionInstance.InstanceId)
    {
        LocalReplica.PackageVersionInstance = serviceDesc_.PackageVersionInstance;
    }
    else if (LocalReplica.PackageVersionInstance.InstanceId > serviceDesc_.PackageVersionInstance.InstanceId)
    {
        serviceDesc_.PackageVersionInstance = LocalReplica.PackageVersionInstance;
    }
}

void FailoverUnit::MarkLocalReplicaDeleted(FailoverUnitEntityExecutionContext & executionContext)
{
    if (localReplicaDeleted_)
    {
        return;
    }

    executionContext.UpdateContextObj.EnableUpdate();

    localReplicaDeleted_ = true;

    // Once the replica has been deleted no further message needs to be sent to fm
    // because deleted can only be set if the fm requests it
    // replica should be marked as uploaded as well
    fmMessageState_.Reset(executionContext.StateUpdateContextObj);
    replicaUploadState_.OnUploaded(executionContext.StateUpdateContextObj);

    // It is possible that ft gets closed due to say rf 
    // at that time the updatestateondropped will not mark cleanup pending
    // a subsequent deletereplica will cause this to be set
    if (IsClosed)
    {
        cleanupPendingFlag_.SetValue(true, executionContext.Queue);
    }
}

void FailoverUnit::WriteTo(TextWriter & writer, FormatOptions const &) const
{
    TraceWriter traceWriter(*this);

    writer.WriteLine(traceWriter);

    for (auto const & replica : replicaStore_)
    {
        writer.WriteLine("{0}", replica);
    }
}

wstring FailoverUnit::ToString() const
{
    wstring result;
    StringWriter(result).Write(*this);
    return result;
}

void FailoverUnit::WriteToEtw(uint16 contextSequenceId) const
{
    TraceWriter writer(*this);

    RAEventSource::Events->RAFailoverUnit(
        contextSequenceId,
        writer,
        replicas_);
}

FailoverUnit::TraceWriter::TraceWriter(FailoverUnit const & ft)
: ft_(ft)
{
    Initialize();
}

FailoverUnit::TraceWriter::TraceWriter(FailoverUnit const & ft, TraceWriter const &) : ft_(ft)
{
}

void FailoverUnit::TraceWriter::Initialize()
{
}

string FailoverUnit::TraceWriter::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0} {1} {2} {3}/{4}/{5} {6} {7}:{8} {9} {10} {11} | [{12} {13} {14} {15}] {16} {17} {18} {19}";
    size_t index = 0;

    traceEvent.AddEventField<Guid>(format, name + ".ftId", index);
    traceEvent.AddEventField<FailoverUnitLifeCycleTraceState::Trace>(format, name + ".traceState", index);
    traceEvent.AddEventField<ReconfigurationState>(format, name + ".reconfigState", index);
    traceEvent.AddEventField<Epoch>(format, name + ".pcEpoch", index);
    traceEvent.AddEventField<Epoch>(format, name + ".icEpoch", index);
    traceEvent.AddEventField<Epoch>(format, name + ".ccEpoch", index);
    traceEvent.AddEventField<Reliability::ReplicaDeactivationInfo>(format, name + ".deactivationInfo", index);
    traceEvent.AddEventField<int64>(format, name + ".localReplicaId", index);
    traceEvent.AddEventField<int64>(format, name + ".localReplicaInstanceId", index);
    traceEvent.AddEventField<int>(format, name + ".targetReplicaSetSize", index);
    traceEvent.AddEventField<int>(format, name + ".minReplicaSetSize", index);
    traceEvent.AddEventField<uint64>(format, name + ".updateVersion", index);

    traceEvent.AddEventField<RAReplicaOpenMode::Trace>(format, name + ".rom", index);
    traceEvent.AddEventField<ReplicaCloseMode>(format, name + ".rcm", index);
    traceEvent.AddEventField<FMMessageState>(format, name + ".fmMessageState", index);
    traceEvent.AddEventField<char>(format, name + ".localReplicaDeleted", index);

    traceEvent.AddEventField<char>(format, name + ".updateReplicatorConfiguration", index);
    traceEvent.AddEventField<char>(format, name + ".replicaUploadPending", index);

    traceEvent.AddEventField<ServiceTypeRegistrationWrapper>(format, name + ".str", index);
    traceEvent.AddEventField<DateTime>(format, name + ".lastUpdated", index);

    return format;
}

void FailoverUnit::TraceWriter::FillEventData(Common::TraceEventContext & context) const
{
    auto lifeCycleState = GetLifeCycleTraceState();

    // There is a little bit of trickery involved here
    // DO not use property accessors to write to the event context
    // When write is called the event context stores a pointer to the data being written
    // Ergo, the contract is that the variable needs to be alive
    // If a property accessor is used it may actually create a copy of the object (int64's bools etc)
    // and this will lead to garbage being traced out
    context.Write(ft_.failoverUnitDesc_.FailoverUnitId.Guid);
    context.WriteCopy<uint>(static_cast<uint>(lifeCycleState));
    context.Write(ft_.reconfigState_);
    context.Write(ft_.PreviousConfigurationEpoch);
    context.Write(ft_.IntermediateConfigurationEpoch);
    context.Write(ft_.CurrentConfigurationEpoch);
    context.Write(ft_.deactivationInfo_);
    context.Write(ft_.localReplicaId_);
    context.Write(ft_.localReplicaInstanceId_);
    context.WriteCopy(ft_.TargetReplicaSetSize);
    context.WriteCopy(ft_.MinReplicaSetSize);
    context.WriteCopy(ft_.ServiceDescription.UpdateVersion);

    context.WriteCopy<uint>(static_cast<uint>(ft_.OpenMode));
    context.Write<ReplicaCloseMode>(ft_.replicaCloseMode_);
    context.Write<FMMessageState>(ft_.fmMessageState_);
    context.WriteCopy(static_cast<char>(ft_.LocalReplicaDeleted));

    context.WriteCopy(static_cast<char>(ft_.IsUpdateReplicatorConfiguration));
    context.WriteCopy(static_cast<char>(ft_.replicaUploadState_.IsUploadPending));

    context.Write(ft_.serviceTypeRegistration_);
    context.Write(ft_.lastUpdated_);
}

void FailoverUnit::TraceWriter::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} {2} {3}/{4}/{5} {6}",
        ft_.FailoverUnitId.Guid,
        GetLifeCycleTraceState(),
        ft_.reconfigState_,
        ft_.PreviousConfigurationEpoch,
        ft_.IntermediateConfigurationEpoch,
        ft_.CurrentConfigurationEpoch,
        ft_.deactivationInfo_);

    writer.Write(" {0}:{1} {2} {3} {4} | ",
        ft_.LocalReplicaId,
        ft_.LocalReplicaInstanceId,
        ft_.TargetReplicaSetSize,
        ft_.MinReplicaSetSize,
        ft_.ServiceDescription.UpdateVersion);

    writer.Write("[{0} {1} {2} {3}] ",
        ft_.OpenMode,
        ft_.CloseMode,
        ft_.fmMessageState_,
        ft_.LocalReplicaDeleted ? L"1" : L"0");

    writer.Write("{0} {1} {2} {3}",
        ft_.IsUpdateReplicatorConfiguration ? L"1" : L"0",
        ft_.replicaUploadState_.IsUploadPending ? L"1" : L"0",
        ft_.serviceTypeRegistration_,
        ft_.LastUpdatedTime);
}

FailoverUnitLifeCycleTraceState::Enum FailoverUnit::TraceWriter::GetLifeCycleTraceState() const
{
    if (ft_.IsClosed)
    {
        return FailoverUnitLifeCycleTraceState::Closed;
    }
    else if (!ft_.LocalReplica.IsUp)
    {
        return FailoverUnitLifeCycleTraceState::Down;
    }
    else if (ft_.LocalReplicaClosePending.IsSet)
    {
        return FailoverUnitLifeCycleTraceState::Closing;
    }
    else if (ft_.LocalReplicaOpenPending.IsSet)
    {
        return FailoverUnitLifeCycleTraceState::Opening;
    }
    else if (ft_.LocalReplica.State == ReplicaStates::StandBy && ft_.LocalReplicaOpen)
    {
        return FailoverUnitLifeCycleTraceState::Open;
    }
    else if (ft_.LocalReplicaOpen)
    {
        return FailoverUnitLifeCycleTraceState::Available;
    }
    else
    {
        Assert::TestAssert(
            "Unknown Lifecycle state {0} {1} {2} {3} {4} {5}",
            ft_.State,
            ft_.IsOpen ? ft_.LocalReplica.IsUp : false,
            ft_.LocalReplicaClosePending.IsSet,
            ft_.LocalReplicaOpenPending.IsSet,
            ft_.IsOpen ? ft_.LocalReplica.State : ReplicaStates::Dropped,
            ft_.LocalReplicaOpen);
        return FailoverUnitLifeCycleTraceState::Open;
    }
}
