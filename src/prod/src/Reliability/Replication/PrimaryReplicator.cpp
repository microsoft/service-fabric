// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::AutoResetEvent;
using Common::ErrorCode;
using Common::ComPointer;
using Common::Guid;

using Common::AcquireWriteLock;
using Common::AcquireReadLock;
using Common::DateTime;
using Common::make_unique;
using Common::StringWriter;
using Common::TimeSpan;

using Transport::Message;
using Transport::MessageUPtr;
using Transport::ReceiverContextUPtr;
using Transport::ISendTarget;

using std::map;
using std::move;
using std::shared_ptr;
using std::vector;
using std::wstring;

PrimaryReplicatorSPtr PrimaryReplicator::CreatePrimary(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    FABRIC_SEQUENCE_NUMBER initialProgress,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor)
{
    auto primary = std::shared_ptr<PrimaryReplicator>(new PrimaryReplicator(
        config,
        perfCounters,
        replicaId,
        hasPersistedState,
        endpointUniqueId,
        epoch,
        stateProvider,
        partition,
        version,
        transport,
        initialProgress,
        partitionId,
        healthClient,
        apiMonitor));

    return primary;
}

PrimaryReplicatorSPtr PrimaryReplicator::CreatePrimary(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    OperationQueue && secondaryQueue)
{
    auto primary = std::shared_ptr<PrimaryReplicator>(new PrimaryReplicator(
        config,
        perfCounters,
        replicaId,
        hasPersistedState,
        endpointUniqueId, 
        epoch, 
        stateProvider, 
        partition,
        version,
        transport, 
        partitionId, 
        healthClient,
        apiMonitor,
        move(secondaryQueue)));

    return primary;
}

// Create Primary on initial open
PrimaryReplicator::PrimaryReplicator(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    FABRIC_SEQUENCE_NUMBER initialProgress,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor)
    :   config_(config),
        perfCounters_(perfCounters),
        replicaId_(replicaId),
        hasPersistedState_(hasPersistedState),
        endpointUniqueId_(endpointUniqueId),
        stateProvider_(stateProvider),
        partition_(partition),
        version_(version),
        transport_(transport),
        partitionId_(partitionId),
        faultErrorCode_(),
        faultDescription_(),
        faultErrorLock_(),
        catchupAsyncOp_(),
        catchupLock_(),
        healthClient_(healthClient),
        apiMonitor_(apiMonitor)
{
    ReplicatorEventSource::Events->PrimaryCtor(
        partitionId_, 
        endpointUniqueId_, 
        config_->AllowMultipleQuorumSet,
        epoch.DataLossNumber,
        epoch.ConfigurationNumber);

    replicaManagerSPtr_ = std::shared_ptr<ReplicaManager>(new ReplicaManager(
        config_, 
        hasPersistedState_,
        perfCounters_,
        replicaId_, 
        hasPersistedState_ || config->RequireServiceAck, 
        endpointUniqueId_, 
        transport_, 
        initialProgress, 
        epoch, 
        stateProvider_, 
        partition_,
        healthClient_,
        apiMonitor_,
        partitionId_));
}

// Create primary from an old secondary
PrimaryReplicator::PrimaryReplicator(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    OperationQueue && secondaryQueue)
    :   config_(config),
        perfCounters_(perfCounters),
        replicaId_(replicaId),
        hasPersistedState_(hasPersistedState),
        endpointUniqueId_(endpointUniqueId),
        stateProvider_(stateProvider),
        partition_(partition),
        version_(version),
        transport_(transport),
        partitionId_(partitionId),
        faultErrorCode_(),
        faultDescription_(),
        faultErrorLock_(),
        catchupAsyncOp_(),
        catchupLock_(),
        healthClient_(healthClient),
        apiMonitor_(apiMonitor)
{
    ReplicatorEventSource::Events->PrimaryCtor(
        partitionId_, 
        endpointUniqueId_, 
        config_->AllowMultipleQuorumSet,
        epoch.DataLossNumber,
        epoch.ConfigurationNumber);

    replicaManagerSPtr_ = std::shared_ptr<ReplicaManager>(new ReplicaManager(
        config_, 
        hasPersistedState_,
        perfCounters_,
        replicaId_, 
        endpointUniqueId_, 
        transport_, 
        epoch, 
        stateProvider_, 
        partition_,
        partitionId_, 
        healthClient_,
        apiMonitor_,
        move(secondaryQueue)));
}

PrimaryReplicator::~PrimaryReplicator()
{
    ReplicatorEventSource::Events->PrimaryDtor(
        partitionId_,
        endpointUniqueId_);
}

void PrimaryReplicator::CreateSecondary(
    FABRIC_EPOCH const & epoch,
    REPerformanceCountersSPtr const & perfCounters,
    IReplicatorHealthClientSPtr const & healthClient,                
    ApiMonitoringWrapperSPtr const & apiMonitor,
    __out SecondaryReplicatorSPtr & secondary)
{
    secondary = SecondaryReplicator::CreateSecondary(
        config_,
        perfCounters,
        replicaId_,
        hasPersistedState_,
        endpointUniqueId_,
        epoch,
        stateProvider_,
        partition_,
        version_,
        transport_,
        partitionId_,
        healthClient,
        apiMonitor,
        move(replicaManagerSPtr_->Queue));
}

// ******* Open *******
ErrorCode PrimaryReplicator::Open()
{
    return replicaManagerSPtr_->Open();
}

// ******* Close *******
AsyncOperationSPtr PrimaryReplicator::BeginClose(
    bool createSecondary,
    TimeSpan const & waitForQuorumTimeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, createSecondary, waitForQuorumTimeout, callback, state); 
}

ErrorCode PrimaryReplicator::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void PrimaryReplicator::Abort()
{
    ReplicatorEventSource::Events->PrimaryAbort(
        partitionId_,
        endpointUniqueId_);

    // Cancel all pending replicate async operations and
    // Close replicas, 
    // which will cancel any pending BuildIdle async operations
    replicaManagerSPtr_->Close(false);

    // Cancel the catchup async operation (if in progress)
    AsyncOperationSPtr copy;
    {
        AcquireWriteLock lock(catchupLock_);
        copy = std::move(catchupAsyncOp_);
    }

    if (copy != nullptr)
    {
        copy->Cancel();
    }

    // We will wait for all replicate operations to complete
    AutoResetEvent waitForReplicateOperationsToCompleteEvent(false);

    AsyncOperationSPtr asyncOp = replicaManagerSPtr_->BeginWaitForReplicateOperationsCallbacksToComplete(
        [&](AsyncOperationSPtr const & asyncOp)
        {
            UNREFERENCED_PARAMETER(asyncOp);
            waitForReplicateOperationsToCompleteEvent.Set();
        },
        nullptr);

    waitForReplicateOperationsToCompleteEvent.WaitOne();
    ErrorCode error = replicaManagerSPtr_->EndWaitForReplicateOperationsCallbacksToComplete(asyncOp);

    ReplicatorEventSource::Events->PrimaryAbortDone(
        partitionId_,
        endpointUniqueId_,
        error.ReadValue());
}

// ******* GetStatus *******
ErrorCode PrimaryReplicator::GetCurrentProgress(
    __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber) const
{
    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }
    return replicaManagerSPtr_->GetCurrentProgress(lastSequenceNumber);
}

ErrorCode PrimaryReplicator::GetCatchUpCapability(
    __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber) const
{
    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }
    return replicaManagerSPtr_->GetCatchUpCapability(firstSequenceNumber);
}

// ******* UpdateCatchUpConfiguration *******
ErrorCode PrimaryReplicator::UpdateCatchUpReplicaSetConfiguration(
    ReplicaInformationVector const & previousActiveSecondaryReplicas, 
    ULONG previousWriteQuorum,
    ReplicaInformationVector const & currentActiveSecondaryReplicas, 
    ULONG currentWriteQuorum)
{
    ReplicatorEventSource::Events->PrimaryUpdateCatchUpConfig(
        partitionId_,
        endpointUniqueId_, 
        static_cast<uint64>(currentActiveSecondaryReplicas.size()),
        static_cast<uint64>(currentWriteQuorum),
        static_cast<uint64>(previousActiveSecondaryReplicas.size()),
        static_cast<uint64>(previousWriteQuorum));
    ASSERT_IF(currentWriteQuorum == 1 && currentActiveSecondaryReplicas.size() > 0, "1 write quorum with non-empty current replica set");

    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }

    // The vector of replicas contains all replicas
    // except the current one. 
    // Check if there are any changes and if so,
    // update the queues.
    auto error = replicaManagerSPtr_->UpdateConfiguration(
        previousActiveSecondaryReplicas,
        previousWriteQuorum,
        currentActiveSecondaryReplicas, 
        currentWriteQuorum,
        true,
        false);

    if (error.IsSuccess())
    {
        // Get the replication queue status
        // and update the catchup operation
        UpdateCatchupOperation();
    }

    return error;
}

// ******* CatchupReplicaSet *******
AsyncOperationSPtr PrimaryReplicator::BeginWaitForCatchUpQuorum(
    FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    // Create the catchup operation inside the lock and start it outside it
    AsyncOperationSPtr localOp;
    {
        AcquireWriteLock lock(catchupLock_);
        ASSERT_IF(
            catchupAsyncOp_, 
            "{0}: Can't start multiple catchup operations in parallel", 
            endpointUniqueId_);
        catchupAsyncOp_ = AsyncOperation::CreateAndStart<CatchupAsyncOperation>(
            *this, catchUpMode, callback, state); 
        localOp = catchupAsyncOp_;
    }

    if (!CheckReportedFault())
    {
        // Check progress to see if the operation can complete immediately
        UpdateCatchupOperation();
    }
    return localOp;
}

ErrorCode PrimaryReplicator::EndWaitForCatchUpQuorum(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = CatchupAsyncOperation::End(asyncOperation);
    
    {
        AcquireWriteLock lock(catchupLock_);
        catchupAsyncOp_.reset();
    }

    if (!error.IsSuccess())
    {
        ReplicatorEventSource::Events->PrimaryCatchUpError(
            partitionId_,
            endpointUniqueId_,
            error.ReadValue());
    }
    return error;
}

// ******* UpdateCurrentReplicaSetConfiguration *******
ErrorCode PrimaryReplicator::UpdateCurrentReplicaSetConfiguration(
    ReplicaInformationVector const & currentActiveSecondaryReplicas, 
    ULONG currentWriteQuorum)
{
    ReplicatorEventSource::Events->PrimaryUpdateCurrentConfig(
        partitionId_,
        endpointUniqueId_, 
        static_cast<uint64>(currentActiveSecondaryReplicas.size()),
        static_cast<uint64>(currentWriteQuorum));
    ASSERT_IF(currentWriteQuorum == 1 && currentActiveSecondaryReplicas.size() > 0, "1 write quorum with non-empty replica set");

    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }

    // The vector of replicas contains all replicas
    // except the current one. 
    // Check if there are any changes and if so,
    // update the queues.
    ReplicaInformationVector previousActiveSecondaryReplicas;
    auto error = replicaManagerSPtr_->UpdateConfiguration(
        previousActiveSecondaryReplicas,
        0ul,
        currentActiveSecondaryReplicas, 
        currentWriteQuorum,
        false,
        false);

    if (error.IsSuccess())
    {
        // Get the replication queue status
        // and update the catchup operation
        UpdateCatchupOperation();
    }

    return error;
}

// ******* BuildIdleReplica *******
AsyncOperationSPtr PrimaryReplicator::BeginBuildReplica(
    ReplicaInformation const & toReplica, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<BuildIdleAsyncOperation>(
        *this, toReplica, callback, state); 
}

ErrorCode PrimaryReplicator::EndBuildReplica(AsyncOperationSPtr const & asyncOperation)
{
    return BuildIdleAsyncOperation::End(asyncOperation);
}

// ******* RemoveReplica *******
ErrorCode PrimaryReplicator::RemoveReplica(FABRIC_REPLICA_ID replicaId)
{
    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }

    if (!replicaManagerSPtr_->RemoveReplica(replicaId))
    {
        // The replica doesn't exist
        return ErrorCode(Common::ErrorCodeValue::REReplicaDoesNotExist);
    }

    UpdateCatchupOperation();
    return ErrorCode(Common::ErrorCodeValue::Success);
}

// ******* ResetReplicationQueue *******
ErrorCode PrimaryReplicator::ResetReplicationQueue(FABRIC_SEQUENCE_NUMBER newProgress)
{
    return replicaManagerSPtr_->ResetReplicationQueue(newProgress);
}

// ******* Replicate *******
AsyncOperationSPtr PrimaryReplicator::BeginReplicate(
     ComPointer<IFabricOperationData> && comOperationPointer,
     __out FABRIC_SEQUENCE_NUMBER & sequenceNumber,
     AsyncCallback const & callback, 
     AsyncOperationSPtr const & state)
{
    auto inner = std::make_shared<ReplicateAsyncOperation>(
        *this, 
        move(comOperationPointer), 
        callback, 
        state); 

    inner->Start(inner);
    sequenceNumber = inner->SequenceNumber;
    return inner;
}

ErrorCode PrimaryReplicator::EndReplicate(
    AsyncOperationSPtr const & asyncOperation)
{
    return ReplicateAsyncOperation::End(asyncOperation);
}

// ******* UpdateEpoch *******
AsyncOperationSPtr PrimaryReplicator::BeginUpdateEpoch(
     __in FABRIC_EPOCH epoch,
     AsyncCallback const & callback, 
     AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
        *this, 
        epoch,
        callback, 
        state); 
}

ErrorCode PrimaryReplicator::EndUpdateEpoch(
    AsyncOperationSPtr const & asyncOperation)
{
    return UpdateEpochAsyncOperation::End(asyncOperation);
}

// ******* Message handler *******
void PrimaryReplicator::CopyContextMessageHandler(
    __in Message & message, 
    ReplicationFromHeader const & fromHeader)
{
    wstring const & toAddress = fromHeader.Address;
    ReplicationEndpointId const & toActor = fromHeader.DemuxerActor;

    if (!hasPersistedState_)
    {
        // Drop the message since there is no processing needed
        // for non-persisted replicas
        ReplicatorEventSource::Events->PrimaryReceiveCCNotPersisted(
            partitionId_,
            endpointUniqueId_);
        return;
    }
   
    ComOperationCPtr op;
    Guid incarnationId = toActor.IncarnationId;
    bool isLast;
    if (!ReplicationTransport::GetCopyContextOperationFromMessage(message, op, isLast))
    {
        // Drop the invalid message
        ReplicatorEventSource::Events->ReplicatorInvalidMessage(
            partitionId_, 
            endpointUniqueId_,
            Constants::CopyContextOperationTrace,
            fromHeader.Address, 
            fromHeader.DemuxerActor);

        return;
    }

    ASSERT_IFNOT(op, "CopyContextMessageHandler: operation shouldn't be null");

    FABRIC_SEQUENCE_NUMBER lsn = op->SequenceNumber;
    FABRIC_REPLICA_ROLE role;

    ReplicationSessionSPtr session = replicaManagerSPtr_->GetReplica(toAddress, toActor, role);
    if (session && role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
    {
        if (session->IncarnationId != incarnationId)
        {
            ReplicatorEventSource::Events->PrimaryDropMsgDueToIncarnationId(
                partitionId_, 
                endpointUniqueId_,
                Constants::CopyContextOperationTrace,
                session->IncarnationId,
                incarnationId);
        }
        else
        {
            session->CompleteEstablishCopyAsyncOperation(
                incarnationId,
                ErrorCode(Common::ErrorCodeValue::Success));

            ReplicatorEventSource::Events->PrimaryReceiveCC(
                partitionId_, 
                endpointUniqueId_,
                lsn,
                fromHeader.Address, 
                fromHeader.DemuxerActor,
                L"ok");
    
            session->ProcessCopyContext(move(op), isLast);
        }
    }
    else
    {
        // Drop message because the replica was not found, not up or not building anymore
        ReplicatorEventSource::Events->PrimaryReceiveCC(
            partitionId_,
            endpointUniqueId_,
            lsn,
            fromHeader.Address,
            fromHeader.DemuxerActor,
            L"Invalid replica state");
    }
}

void PrimaryReplicator::ReplicationAckMessageHandler(
    __in Message & message, 
    ReplicationFromHeader const & fromHeader,
    PrimaryReplicatorWPtr primaryReplicatorWPtr)
{
    Guid incarnationId = fromHeader.DemuxerActor.IncarnationId;
    FABRIC_SEQUENCE_NUMBER replicationReceivedLSN; 
    FABRIC_SEQUENCE_NUMBER replicationQuorumLSN;
    FABRIC_SEQUENCE_NUMBER copyReceivedLSN;
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN;
    int copyErrorCodeValue;
    ReplicationTransport::GetAckFromMessage(
        message, 
        replicationReceivedLSN, 
        replicationQuorumLSN, 
        copyReceivedLSN, 
        copyQuorumLSN,
        copyErrorCodeValue);

    wstring const & toAddress = fromHeader.Address;
    ReplicationEndpointId const & toActor = fromHeader.DemuxerActor;
   
    FABRIC_REPLICA_ROLE role;
    ReplicationSessionSPtr session = replicaManagerSPtr_->GetReplica(toAddress, toActor, role);
    if (session != nullptr)
    {
        ASSERT_IF(this->partitionId_ != toActor.PartitionId, "PartitionId not equal, out trace workaround is invalid: {0} != {1}", this->partitionId_, toActor.PartitionId);
        
        wstring messageId;
        Common::StringWriter writer(messageId);
        writer.Write("{0}:{1}", message.MessageId.Guid.ToString(), message.MessageId.Index);

        if (session->IncarnationId != incarnationId)
        {
            ReplicatorEventSource::Events->PrimaryDropMsgDueToIncarnationId(
                partitionId_, 
                endpointUniqueId_,
                L"ReplicationAck",
                session->IncarnationId,
                incarnationId);
        }
        else
        {
            if (!hasPersistedState_ || 
                copyErrorCodeValue != 0)
            {
                // CopyContext errors are communicated via the ACK message from the secondary
                // So we complete the copy establishment operation if there was an error 
                // OR
                // If this replica has no persisted state, we don't expect any CopyCC messages and hence complete 
                session->CompleteEstablishCopyAsyncOperation(
                    incarnationId,
                    ErrorCode(static_cast<Common::ErrorCodeValue::Enum>(copyErrorCodeValue)));
            }
            
            // This invokes an async ack processing on the session and if there was any progress made, 
            // the callback is invoked
            if (role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
            { 
                if (copyErrorCodeValue != 0)
                {
                    session->OnCopyFailure(copyErrorCodeValue);
                }

                session->UpdateAckProgress(
                    replicationReceivedLSN,
                    replicationQuorumLSN,
                    copyReceivedLSN,
                    copyQuorumLSN,
                    messageId,
                    [primaryReplicatorWPtr]()
                {
                    UpdateProgressAndCatchupOperation(primaryReplicatorWPtr);
                });
            }
            else
            {
                TESTASSERT_IF(session->IsIdleFaultedDueToSlowProgress, "Session {0} cannot be faulted if already promoted", session->ReplicaId);

                session->UpdateAckProgress(
                    replicationReceivedLSN,
                    replicationQuorumLSN,
                    messageId,
                    [primaryReplicatorWPtr]()
                {
                    UpdateProgressAndCatchupOperation(primaryReplicatorWPtr);
                });
            }
        }
    }
    else
    {
        ASSERT_IF(this->partitionId_ != toActor.PartitionId, "PartitionId not equal, out trace workaround is invalid: {0} != {1}", this->partitionId_, toActor.PartitionId);

        ReplicatorEventSource::Events->PrimaryAckReceive(
            partitionId_,
            endpointUniqueId_, 
            replicationReceivedLSN, 
            replicationQuorumLSN,
            copyReceivedLSN, 
            copyQuorumLSN,
            toAddress,
            //toActor.PartitionId,
            toActor.ReplicaId,
            L"Replica not found");
    }
}

void PrimaryReplicator::UpdateProgressAndCatchupOperation(PrimaryReplicatorWPtr primaryWPtr)
{
    auto primarySPtr = primaryWPtr.lock();
    if (primarySPtr)
    {
        // Update replication queue 
        // then update the catchup operation
        if (primarySPtr->replicaManagerSPtr_->UpdateProgress())
        {
            primarySPtr->UpdateCatchupOperation();
        }
    }
}

ErrorCode PrimaryReplicator::GetLastSequenceNumber(
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER & lastSequenceNumber)
{
    return replicaManagerSPtr_->UpdateEpochAndGetLastSequenceNumber(epoch, lastSequenceNumber);
}

// ******* Helper methods *******
void PrimaryReplicator::UpdateCatchupOperation()
{
    bool hasEnoughReplicasForQuorum;
    FABRIC_SEQUENCE_NUMBER quorumAckLSN;
    FABRIC_SEQUENCE_NUMBER allAckLSN;
    FABRIC_SEQUENCE_NUMBER latestLSN;
    FABRIC_SEQUENCE_NUMBER previousConfigCatchupLsn;
    FABRIC_SEQUENCE_NUMBER lowestLSNAmongstMustCatchupReplicas;
    AsyncOperationSPtr copy;
    bool getQueueProgressSucceeded = false;

    {
        AcquireWriteLock lock(catchupLock_);
        if (!catchupAsyncOp_)
        {
            // Nothing to catchup, return immediately
            return;
        }

        getQueueProgressSucceeded = replicaManagerSPtr_->GetQueueProgressForCatchup(
            hasEnoughReplicasForQuorum,
            quorumAckLSN,
            allAckLSN,
            latestLSN,
            previousConfigCatchupLsn,
            lowestLSNAmongstMustCatchupReplicas);

        if (getQueueProgressSucceeded && !hasEnoughReplicasForQuorum)
        {
            // Since there are not enough replicas, no need to check quorum
            return;
        }
        
        if (!getQueueProgressSucceeded)
        {
            copy = move(catchupAsyncOp_);
        }
        else
        {
            auto op = AsyncOperation::Get<CatchupAsyncOperation>(catchupAsyncOp_);
            bool catchupCompleted = false;
            
            switch (op->CatchupMode)
            {
                case FABRIC_REPLICA_SET_QUORUM_ALL:
                    catchupCompleted = op->IsAllProgressAchieved(allAckLSN, latestLSN);
                    break;
                case FABRIC_REPLICA_SET_WRITE_QUORUM:
                    catchupCompleted = op->IsQuorumProgressAchieved(quorumAckLSN, previousConfigCatchupLsn, lowestLSNAmongstMustCatchupReplicas);
                    break;
                default:
                    Assert::CodingError("Unknown catchup mode {0} in UpdateCatchupOperation", op->CatchupMode);
            }

            if (catchupCompleted)
            {
                copy = move(catchupAsyncOp_);
            }
        }
    }

    if (copy != nullptr)
    {
        if (getQueueProgressSucceeded)
        {
            copy->TryComplete(copy, ErrorCode(Common::ErrorCodeValue::Success));
        }
        else
        {
            copy->TryComplete(copy, ErrorCode(Common::ErrorCodeValue::ObjectClosed));
        }
    }
}

ErrorCode PrimaryReplicator::GetReplicationQueueCounters(
    __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::ObjectClosed;
    }

    return replicaManagerSPtr_->GetReplicationQueueCounters(counters);
}

ErrorCode PrimaryReplicator::GetReplicatorStatus(
    __out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    return replicaManagerSPtr_->GetReplicatorStatus(result);
}

void PrimaryReplicator::SetReportedFault(
    ErrorCode const & error,
    wstring const & faultDescription)
{
    AcquireWriteLock grab(faultErrorLock_);
    if (faultErrorCode_.IsSuccess())
    {
        faultErrorCode_ = error;
        faultDescription_ = faultDescription;

        Replicator::ReportFault(
            partition_,
            partitionId_,
            endpointUniqueId_,
            faultDescription_,
            faultErrorCode_);
    }
}

bool PrimaryReplicator::CheckReportedFault(bool callReportFault) const
{
    AcquireReadLock grab(faultErrorLock_);
    if (!faultErrorCode_.IsSuccess())
    {
        if (callReportFault)
        {
            Replicator::ReportFault(
                partition_,
                partitionId_,
                endpointUniqueId_,
                L"Primary CheckReportedFault",
                faultErrorCode_);
        }
        return true;
    }
    return false;
}

ServiceModel::ReplicatorStatusQueryResultSPtr PrimaryReplicator::GetReplicatorStatusQueryResult(
    std::vector<ReplicationSessionSPtr> const & idleReplicas,
    std::vector<ReplicationSessionSPtr> const & readyReplicas,
    OperationQueue const & queue)
{
    std::vector<ServiceModel::RemoteReplicatorStatus> details;

    for (auto it = idleReplicas.begin(); it != idleReplicas.end(); it++)
    {
        details.push_back((*it)->GetDetailsForQuery(true /*isIdle*/));
    }

    for (auto it = readyReplicas.begin(); it != readyReplicas.end(); it++)
    {
        details.push_back((*it)->GetDetailsForQuery(false /*isIdle*/));
    }
    
    return ServiceModel::PrimaryReplicatorStatusQueryResult::Create(
        std::make_shared<ServiceModel::ReplicatorQueueStatus>(move(Replicator::GetReplicatorQueueStatusForQuery(queue))),
        move(details));
}

} // end namespace ReplicationComponent
} // end namespace Reliability
