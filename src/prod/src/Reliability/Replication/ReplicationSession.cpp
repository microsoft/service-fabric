// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Transport::MessageUPtr;

using Common::AcquireReadLock;
using Common::AcquireWriteLock;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ComponentRoot;
using Common::ComponentRootSPtr;
using Common::ErrorCode;
using Common::TimeSpan;
using Common::make_com;

using Common::DateTime;
using Common::StringUtility;
using Common::StringWriter;

using std::move;
using std::wstring;

ReplicationSession::ReplicationSession(
    REInternalSettingsSPtr const & config,
    ComProxyStatefulServicePartition const & partition,
    FABRIC_REPLICA_ID replicaId,
    wstring const & replicatorAddress,
    wstring const & transportEndpointId,
    FABRIC_SEQUENCE_NUMBER currentProgress,
    ReplicationEndpointId const & primaryEndpointUniqueId,
    Common::Guid const & partitionId,
    FABRIC_EPOCH const & epoch,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    ReplicationTransportSPtr const & transport)
    : replicationOperationHeadersSPtr_(transport->CreateSharedHeaders(primaryEndpointUniqueId, GetEndpointUniqueId(replicatorAddress), ReplicationTransport::ReplicationOperationAction)),
    copyOperationHeadersSPtr_(transport->CreateSharedHeaders(primaryEndpointUniqueId, GetEndpointUniqueId(replicatorAddress), ReplicationTransport::CopyOperationAction)),
    copyContextAckOperationHeadersSPtr_(transport->CreateSharedHeaders(primaryEndpointUniqueId, GetEndpointUniqueId(replicatorAddress), ReplicationTransport::CopyContextAckAction)),
    startCopyHeadersSPtr_(transport->CreateSharedHeaders(primaryEndpointUniqueId, GetEndpointUniqueId(replicatorAddress), ReplicationTransport::StartCopyAction)),
    requestAckHeadersSPtr_(transport->CreateSharedHeaders(primaryEndpointUniqueId, GetEndpointUniqueId(replicatorAddress), ReplicationTransport::RequestAckAction)),
    induceFaultHeadersSPtr_(transport->CreateSharedHeaders(primaryEndpointUniqueId, GetEndpointUniqueId(replicatorAddress), ReplicationTransport::InduceFaultAction)),
    RemoteSession(
        config,
        partition,
        replicaId,
        replicatorAddress,
        transportEndpointId,
        primaryEndpointUniqueId,
        partitionId,
        epoch,
        apiMonitor,
        transport,
        ReliableOperationSender(
            config,
            config->InitialPrimaryReplicationQueueSize,
            config->MaxPrimaryReplicationQueueSize,
            partitionId,
            Constants::ReplOperationTrace,
            primaryEndpointUniqueId,
            replicaId,
            currentProgress)
        ),
    isPromotedtoActiveSecondary_(false),
    faultLock_(),
    isIdleFaultedDueToSlowProgress_(false),
    isActiveFaultedDueToSlowProgress_(false),
    progressInformationWhenFaulted_(),
    sendInduceFaultMessageTimer_(),
    mustCatchup_(MustCatchup::Enum::Unknown),
    mustCatchupLock_()
{
}

ReplicationSession::~ReplicationSession()
{
}

bool ReplicationSession::get_IsIdleFaultedDueToSlowProgress() const
{
    return isIdleFaultedDueToSlowProgress_.load();
}

bool ReplicationSession::get_IsActiveFaultedDueToSlowProgress() const
{
    return isActiveFaultedDueToSlowProgress_.load();
}

void ReplicationSession::Open()
{
    OnOpen(
        [this](ComOperationCPtr const & op, bool requestAck, FABRIC_SEQUENCE_NUMBER completedSeqNumber)
    {
        if (requestAck)
        {
            ASSERT_IF(op.GetRawPointer() != nullptr, "{0}->{1}: RequestAck: Operation shouldn't be set", primaryEndpointUniqueId_, replicaId_);
            return SendRequestAck();
        }
        else
        {
            return SendReplicateOperation(op, completedSeqNumber);
        }
    });
}

void ReplicationSession::Close()
{
    OnClose();

    {
        AcquireWriteLock grab(faultLock_);
        if (sendInduceFaultMessageTimer_)
        {
            sendInduceFaultMessageTimer_->Cancel();
        }
    }
}

bool ReplicationSession::TryFaultIdleReplicaDueToSlowProgress(
    FABRIC_SEQUENCE_NUMBER firstReplicationLsn,
    FABRIC_SEQUENCE_NUMBER lastReplicationLsn,
    FABRIC_SEQUENCE_NUMBER currentReplicaAtLsn)
{
    wstring content;
    StringWriter writer(content);
    writer.WriteLine("Replica {0} is causing high primary queue usage.", replicaId_);
    writer.WriteLine("First Replication Operation in the Primary Queue = {0}, Last Replication Operation in the Primary Queue = {1}, Current Replica's Progress = {2}", firstReplicationLsn, lastReplicationLsn, currentReplicaAtLsn);
    writer.WriteLine("Detailed Repl Progress: {0}", replicationOperations_);
    writer.WriteLine("Detailed Copy Progress: {0}", copySender_.ToString());

    {
        AcquireWriteLock grab(faultLock_);
        
        // The upper level method in replicamanager that calls this is doing so from a read lock
        // So it is possible for 2 threads to invoke this method one after another.

        // So perform this check here
        if (isIdleFaultedDueToSlowProgress_.load() ||
            IsSessionActive == false)
            return false;

        // Indicate to the copy sender that the replica is slow and we will hence induce a fault.
        // As a result, it should not complete build async operation
        if (copySender_.TryDisableBuildCompletion(content))
        {
            ASSERT_IF(isPromotedtoActiveSecondary_.load() == true, "{0}: Replication Session is already active secondary", ToString());

            // Close the reliable operation sender as the existing com replication operations in them are no longer going to be valid
            // This is because existing REPL operations will potentially get completed
            replicationOperations_.Close();
            isIdleFaultedDueToSlowProgress_.store(true);
            progressInformationWhenFaulted_ = content;

            StartFaultReplicaMessageSendTimerIfNeededCallerHoldsLock();

            return true;
        }
        else
        {
            // We could fail to disable build completion because a racing ACK could result in completing the build.
            // If that is the case, we are not hitting a queue full due to the idle replica anymore
            return false;
        }
    }
}

bool ReplicationSession::TryFaultActiveReplicaDueToSlowProgress(
    StandardDeviation const & avgApplyAckDuration,
    StandardDeviation const & avgReceiveAckDuration,
    FABRIC_SEQUENCE_NUMBER firstReplicationLsn,
    FABRIC_SEQUENCE_NUMBER lastReplicationLsn)
{
    AcquireWriteLock grab(faultLock_);

    if (isActiveFaultedDueToSlowProgress_.load() ||
        IsSessionActive == false)
    {
        return false;
    }

    wstring content;
    StringWriter writer(content);
    writer.WriteLine("Replica {0} is causing high primary queue usage. First Replication Operation = {1}, Last = {2}", replicaId_, firstReplicationLsn, lastReplicationLsn);
    writer.WriteLine("Avg ReceiveAck Duration of quorum of replicas={0}ms (SD={1}ms); Avg ApplyAck Duration of quorum of replicas = {2}ms(SD={3}ms)", avgReceiveAckDuration.Average.TotalMilliseconds(), avgReceiveAckDuration.StdDev.TotalMilliseconds(), avgApplyAckDuration.Average.TotalMilliseconds(), avgApplyAckDuration.StdDev.TotalMilliseconds());

    progressInformationWhenFaulted_ = content;

    TESTASSERT_IF(sendInduceFaultMessageTimer_, "{0}: Did not expect sendInduceFaultMessageTimer_ to be active", primaryEndpointUniqueId_);

    StartFaultReplicaMessageSendTimerCallerHoldsLock();
    
    ReplicatorEventSource::Events->PrimaryFaultedSlowSecondary(
        partitionId_,
        primaryEndpointUniqueId_,
        *this,
        content);

    // Add the content of this replica's progress into the writer so that this information is present when sent to the secondary
    writer.WriteLine("Current replica's progress: {0}", replicationOperations_);
    
    // Close the reliable operation sender as the existing com replication operations in them are no longer going to be valid
    // This is because existing REPL operations will potentially get removed from the queue
    replicationOperations_.Close();
    isActiveFaultedDueToSlowProgress_.store(true);

    return true;
}

void ReplicationSession::StartFaultReplicaMessageSendTimerCallerHoldsLock()
{
    if (sendInduceFaultMessageTimer_ == nullptr)
    {
         auto root = CreateComponentRoot();
        
        sendInduceFaultMessageTimer_ = Common::Timer::Create(
            TimerTagDefault,
            [this, root](Common::TimerSPtr const& timer)
            {
                FaultReplicaMessageSender(timer);
            }, false /* disable parallel callbacks*/);

        // Use a periodic timer
        sendInduceFaultMessageTimer_->Change(config_->RetryInterval, config_->RetryInterval);
    }
}

void ReplicationSession::StartFaultReplicaMessageSendTimerIfNeededCallerHoldsLock()
{
    if (isIdleFaultedDueToSlowProgress_.load() == true &&
        LastCopyQuorumAck == Constants::NonInitializedLSN &&
        LastCopyReceiveAck == Constants::NonInitializedLSN &&
        IsSessionActive == true)
    {
        StartFaultReplicaMessageSendTimerCallerHoldsLock();
    }
}

void ReplicationSession::StartFaultReplicaMessageSendTimerIfNeeded()
{
    AcquireWriteLock grab(faultLock_);
    StartFaultReplicaMessageSendTimerIfNeededCallerHoldsLock();
}

void ReplicationSession::FaultReplicaMessageSender(Common::TimerSPtr const & timer)
{
    if (IsSessionActive == false)
    {
        // We will disable the sending if this session is closed
        timer->Cancel();
        return;
    }

    MessageUPtr message = ReplicationTransport::CreateInduceFaultMessage(
        replicaId_,
        replicaEndpointUniqueId_.IncarnationId,
        progressInformationWhenFaulted_);

    ReplicatorEventSource::Events->PrimarySendInfo(
        partitionId_,
        primaryEndpointUniqueId_,
        replicaId_,
        ReplicationTransport::InduceFaultAction,
        -1,
        L"NA",
        message->MessageId.Guid,
        static_cast<uint32>(message->MessageId.Index));

    SendTransportMessage(induceFaultHeadersSPtr_, move(message), true);
}

void ReplicationSession::OnPromoteToActiveSecondary()
{
    isPromotedtoActiveSecondary_.store(true);
    replicationOperations_.ResetAverageStatistics();
}

FABRIC_SEQUENCE_NUMBER ReplicationSession::get_IdleReplicaProgress() 
{
    // *******************************************OPTIMIZATION:********************************************
    // An idle replica's progress is calculated as follows:
    //      1. If the copy is still in progress(state provider has NOT yet provided last NULL op), the progress of 
    //         the idle replica is equal to its last Received ACK sequence number
    //
    //      2. If the state provider has provided the last NULL op, the progress of the idle replica is equal to
    //         the min(last received ACK sequence number, copyFinishSequenceNumber)
    //
    //         Here, copyFinishSequenceNumber -> the replication LSN at which copy operation is considered to be 
    //                                           finished for this idle replica - See PrimaryReplicator.BuildIdleAsyncOperation.cpp
    // ****************************************************************************************************
    FABRIC_SEQUENCE_NUMBER finishCopy;
    FABRIC_SEQUENCE_NUMBER lastAcked;
    FABRIC_SEQUENCE_NUMBER lastAckedReceived;
    DateTime lastReplicationAckProcessedTime;

    finishCopy = copySender_.FinishCopySequenceNumber;
    replicationOperations_.GetProgress(lastAckedReceived, lastAcked, lastReplicationAckProcessedTime, Constants::InvalidLSN);

    FABRIC_SEQUENCE_NUMBER progress = Constants::InvalidLSN;

    if (finishCopy == Constants::NonInitializedLSN)
    {
        // Copy is still in progress and the primary hasn't yet seen the last NULL operation from the state provider
        progress = lastAckedReceived;
    }
    else 
    {
        // progress = min(lastReceivedLSN, copyFinish)
        progress = lastAckedReceived < finishCopy ? lastAckedReceived : finishCopy;
    }

    // progress = max(progress, lastACKed)
    // If applied ACK is greater than current calculated progress, treat that as the latest progress
    // (Legacy Behavior)
    progress = progress < lastAcked ? lastAcked : progress;
    
    // Convert any negative values to 0
    return (progress < Constants::InvalidLSN) ? Constants::InvalidLSN : progress;
}

void ReplicationSession::StartCopy(
    FABRIC_SEQUENCE_NUMBER replicationStartSeq,
    Transport::SendStatusCallback const & sendStatusCallback)
{
    ASSERT_IF(
        replicationStartSeq <= Constants::InvalidLSN, 
        "{0}: COPY: Replication start sequence number {1} is not valid",
        ToString(),
        replicationStartSeq);
        
    ReplicatorEventSource::Events->PrimaryStartCopy(
        partitionId_,
        primaryEndpointUniqueId_,
        transportTarget_,
        replicationStartSeq);

    FABRIC_OPERATION_METADATA metadata;
    metadata.SequenceNumber = replicationStartSeq;
    metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
    metadata.AtomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
    metadata.Reserved = NULL;
    auto startCopyOp = make_com<ComStartCopyOperation,ComOperation>(metadata);
   
    auto startCopyOpsSenderCopy = std::make_shared<ReliableOperationSender>(
        config_,
        config_->InitialCopyQueueSize,
        config_->MaxCopyQueueSize,
        partitionId_,
        Constants::StartCopyOperationTrace,
        primaryEndpointUniqueId_,
        replicaId_,
        0);

    OnStartCopy(startCopyOp, startCopyOpsSenderCopy, sendStatusCallback);
}

bool ReplicationSession::SendStartCopyMessage(
    FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber,
    Transport::SendStatusCallback const & sendStatusCallback)
{
    MessageUPtr message = ReplicationTransport::CreateStartCopyMessage(
        ReadEpoch(),
        replicaId_,
        replicationStartSequenceNumber);
    message->SetSendStatusCallback(sendStatusCallback);

    ReplicatorEventSource::Events->PrimarySendW(
        partitionId_,
        primaryEndpointUniqueId_,
        replicaId_,
        Constants::StartCopyOperationTrace,
        replicationStartSequenceNumber,
        replicationOperations_,
        message->MessageId.Guid,
        static_cast<uint32>(message->MessageId.Index));

    return SendTransportMessage(startCopyHeadersSPtr_, move(message), false);
}

bool ReplicationSession::SendCopyOperation(
    ComOperationCPtr const & operationPtr,
    bool isLast)
{
    MessageUPtr message = ReplicationTransport::CreateCopyOperationMessage(
        operationPtr,
        replicaId_,
        ReadEpoch(),
        isLast,
        config_->EnableReplicationOperationHeaderInBody);

    if (isLast)
    {
        ReplicatorEventSource::Events->PrimarySendLast(
            partitionId_,
            primaryEndpointUniqueId_,
            replicaId_,
            Constants::CopyOperationTrace,
            operationPtr->SequenceNumber,
            copySender_.ToString(),
            message->MessageId.Guid,
            static_cast<uint32>(message->MessageId.Index));
    }
    else
    {
        ReplicatorEventSource::Events->PrimarySend(
            partitionId_,
            primaryEndpointUniqueId_,
            replicaId_,
            Constants::CopyOperationTrace,
            operationPtr->SequenceNumber,
            copySender_.ToString(),
            message->MessageId.Guid,
            static_cast<uint32>(message->MessageId.Index));
    }

    return SendTransportMessage(copyOperationHeadersSPtr_, move(message), true);
}

bool ReplicationSession::SendReplicateOperation(
    ComOperationCPtr const & operationPtr,
    FABRIC_SEQUENCE_NUMBER completedSeqNumber)
{
    MessageUPtr message = ReplicationTransport::CreateReplicationOperationMessage(
        operationPtr,
        operationPtr->LastOperationInBatch,
        ReadEpoch(),
        config_->EnableReplicationOperationHeaderInBody,
        completedSeqNumber);

    ReplicatorEventSource::Events->PrimarySendW(
        partitionId_,
        primaryEndpointUniqueId_,
        replicaId_,
        Constants::ReplOperationTrace,
        operationPtr->SequenceNumber,
        replicationOperations_,
        message->MessageId.Guid,
        static_cast<uint32>(message->MessageId.Index));

    return SendTransportMessage(replicationOperationHeadersSPtr_, move(message), true);
}

bool ReplicationSession::SendRequestAck()
{
    MessageUPtr message = ReplicationTransport::CreateRequestAckMessage();
    ReplicatorEventSource::Events->PrimarySendW(
        partitionId_,
        primaryEndpointUniqueId_,
        replicaId_,
        Constants::RequestAckTrace,
        -1,
        replicationOperations_,
        message->MessageId.Guid,
        static_cast<uint32>(message->MessageId.Index));

    return SendTransportMessage(requestAckHeadersSPtr_, move(message), false);
}

bool ReplicationSession::SendCopyContextAck(bool shouldTrace)
{
    // Get the completed LSN from the copy context manager's queue
    CopyContextReceiverSPtr cachedCopyContextReceiver = GetCopyContextReceiver();
    if (!cachedCopyContextReceiver)
    {
        // Copy is done, no need to send ACK back
        return false;
    }

    FABRIC_SEQUENCE_NUMBER lastCompletedLSN;
    int errorCodeValue;
    cachedCopyContextReceiver->GetStateAndSetLastAck(lastCompletedLSN, errorCodeValue);

    MessageUPtr message = ReplicationTransport::CreateCopyContextAckMessage(
        lastCompletedLSN,
        errorCodeValue);

    if (shouldTrace)
    {
        ReplicatorEventSource::Events->PrimarySendInfo(
            partitionId_,
            primaryEndpointUniqueId_,
            replicaId_,
            Constants::CopyContextAckOperationTrace,
            lastCompletedLSN,
            errorCodeValue == 0 ? copySender_.ToString() : L"error",
            message->MessageId.Guid,
            static_cast<uint32>(message->MessageId.Index));
    }
    else
    {
        ReplicatorEventSource::Events->PrimarySend(
            partitionId_,
            primaryEndpointUniqueId_,
            replicaId_,
            Constants::CopyContextAckOperationTrace,
            lastCompletedLSN,
            errorCodeValue == 0 ? copySender_.ToString() : L"error",
            message->MessageId.Guid,
            static_cast<uint32>(message->MessageId.Index));
    }

    return SendTransportMessage(copyContextAckOperationHeadersSPtr_, move(message), false);
}

MustCatchup::Enum ReplicationSession::get_MustCatchupEnum() const
{
    AcquireReadLock grab(mustCatchupLock_);
    return mustCatchup_;
}

void ReplicationSession::put_MustCatchupEnum(MustCatchup::Enum value)
{
    Common::AcquireExclusiveLock grab(mustCatchupLock_);

    if (value == MustCatchup::Enum::Yes)
    {
        // Trace only when must catchup is updated to YES to avoid noise
        ReplicatorEventSource::Events->PrimaryMustCatchupUpdated(
            partitionId_,
            primaryEndpointUniqueId_,
            replicaId_,
            mustCatchup_,
            value,
            replicationOperations_.LastAckedSequenceNumber);
    }

    mustCatchup_ = value;
}

wstring ReplicationSession::ToString() const
{
    wstring content;
    StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return content;
}

void ReplicationSession::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "\r\n{0}->{1}: {2}\t{3} acksSkipped={4} IsIdleFaultedDueToSlowProgress={5} IsActiveFaultedDueToSlowProgress={6}\r\n",
        primaryEndpointUniqueId_,
        transportTarget_,
        replicationOperations_,
        copySender_.ToString(),
        0,
        isIdleFaultedDueToSlowProgress_.load(),
        isActiveFaultedDueToSlowProgress_.load());
}

void ReplicationSession::WriteToEtw(uint16 contextSequenceId) const
{
    ReplicatorEventSource::Events->ReplicationSessionTrace(
        contextSequenceId,
        primaryEndpointUniqueId_,
        transportTarget_,
        replicationOperations_,
        copySender_.ToString(),
        0,
        isIdleFaultedDueToSlowProgress_.load(),
        isActiveFaultedDueToSlowProgress_.load());
}

} // end namespace ReplicationComponent
} // end namespace Reliability
