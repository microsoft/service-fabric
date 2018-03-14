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

RemoteSession::RemoteSession(
    REInternalSettingsSPtr const & config,
    ComProxyStatefulServicePartition const & partition,
    FABRIC_REPLICA_ID replicaId,
    wstring const & replicatorAddress,
    wstring const & transportEndpointId,
    ReplicationEndpointId const & primaryEndpointUniqueId,
    Common::Guid const & partitionId,
    FABRIC_EPOCH const & epoch,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    ReplicationTransportSPtr const & transport,
    ReliableOperationSender && replicationOperationSender)
    : ComponentRoot(),
    config_(config),
    replicaId_(replicaId),
    endpoint_(replicatorAddress),
    replicaEndpointUniqueId_(GetEndpointUniqueId(replicatorAddress)),
    primaryEndpointUniqueId_(primaryEndpointUniqueId),
    partitionId_(partitionId),
    partition_(partition),
    transport_(transport),
    sendTarget_(transport_->ResolveTarget(endpoint_, transportEndpointId)),
    transportTarget_(Common::wformatString("{0}:{1}", replicatorAddress, transportEndpointId)),
    copySender_(config, partition, partitionId_, Constants::CopyOperationTrace, primaryEndpointUniqueId_, replicaId_, true, apiMonitor, Common::ApiMonitoring::ApiName::GetNextCopyState),
    replicationOperations_(move(replicationOperationSender)),
    isSessionActive_(false),
    lock_(),
    copyContextReceiver_(),
    startCopyOpsSender_(),
    establishCopyLock_(),
    establishCopyAsyncOperation_(),
    replicationAckProcessingLock_(),
    replicationAckProcessingInProgress_(false),
    latestReplicationQuorumAck_(0),
    latestReplicationReceiveAck_(0),
    latestCopyQuorumAck_(0),
    latestCopyReceiveAck_(0),
    processCopyAcks_(true),
    latestTraceMessageId_(),
    lastAckTraceWatch_(),
    epochLock_(),
    epoch_(epoch)
{
    lastAckTraceWatch_.Start();
    ASSERT_IF(endpoint_.empty(), "{0}: Endpoint shouldn't be empty", replicaId_);
}

RemoteSession::~RemoteSession()
{
}

ReplicationEndpointId RemoteSession::GetEndpointUniqueId(std::wstring const & replicatorAddress)
{
    Common::StringCollection tokens;
    StringUtility::Split<wstring>(replicatorAddress, tokens, L"/");
    size_t size = tokens.size();

    if (size > 0)
    {
        return ReplicationEndpointId::FromString(tokens[size - 1]);
    }

    Common::Assert::CodingError("Can't parse endpoint, it is in an incorrect format: {0}", replicatorAddress);
}

void RemoteSession::OnOpen(__in SendOperationCallback const & replicationOperationSendCallback)
{
    replicationOperations_.Open<ComponentRootSPtr>(
        CreateComponentRoot(),
        replicationOperationSendCallback);

    isSessionActive_.store(true);
}

void RemoteSession::OnClose()
{
    isSessionActive_.store(false);
    replicationOperations_.Close();
    CloseStartCopyReliableOperationSender();
}

FABRIC_EPOCH RemoteSession::ReadEpoch() const
{
    AcquireReadLock grab(epochLock_);
    return epoch_;
}

void RemoteSession::SetEpoch(FABRIC_EPOCH const & epoch)
{
    AcquireWriteLock grab(epochLock_);
    ASSERT_IF(
        epoch < epoch_,
        "{0}: Replication Session OnUpdateEpoch({1}) not be older than current epoch {2}",
        primaryEndpointUniqueId_,
        epoch,
        epoch_);

    epoch_ = epoch;
}

void RemoteSession::OnUpdateEpoch(FABRIC_EPOCH const & epoch)
{
    SetEpoch(epoch);
}

CopyContextReceiverSPtr RemoteSession::GetCopyContextReceiver()
{
    CopyContextReceiverSPtr cachedCopyContextReceiver;
    {
        AcquireReadLock lock(lock_);
        cachedCopyContextReceiver = copyContextReceiver_;
    }

    return cachedCopyContextReceiver;
}

FABRIC_SEQUENCE_NUMBER RemoteSession::GetLastReceiveLsn(FABRIC_SEQUENCE_NUMBER completedLsn)
{
    FABRIC_SEQUENCE_NUMBER lastQuorumAck;
    FABRIC_SEQUENCE_NUMBER lastReceiveAck;
    DateTime lastAckProcessedTime;
    replicationOperations_.GetProgress(lastReceiveAck, lastQuorumAck, lastAckProcessedTime, completedLsn);
    return (lastReceiveAck < Constants::InvalidLSN) ? Constants::InvalidLSN : lastReceiveAck;
}

FABRIC_SEQUENCE_NUMBER RemoteSession::GetLastApplyLsn(FABRIC_SEQUENCE_NUMBER completedLsn)
{
    // This number is used by replica manager to figure out the progress of the non-idle replicas.
    // In that context, -1 means no progress, so
    // transform any -1 to 0 to make computations easier.
    FABRIC_SEQUENCE_NUMBER lastQuorumAck;
    FABRIC_SEQUENCE_NUMBER lastReceiveAck;
    DateTime lastAckProcessedTime;
    replicationOperations_.GetProgress(lastReceiveAck, lastQuorumAck, lastAckProcessedTime, completedLsn);
    return (lastQuorumAck < Constants::InvalidLSN) ? Constants::InvalidLSN : lastQuorumAck;
}

ServiceModel::RemoteReplicatorStatus RemoteSession::GetDetailsForQuery(bool isIdle)
{
    FABRIC_SEQUENCE_NUMBER lastReplicationReceivedLSN;
    FABRIC_SEQUENCE_NUMBER lastReplicationAppliedLSN;

    Common::TimeSpan avgReplicationReceiveAckDuration;
    Common::TimeSpan avgReplicationApplyAckDuration;

    FABRIC_SEQUENCE_NUMBER notReceivedReplicationCount;
    FABRIC_SEQUENCE_NUMBER receivedAndNotAppliedReplicationCount;

    DateTime lastReplicationAckProcessedTime;

    replicationOperations_.GetProgress(lastReplicationReceivedLSN, lastReplicationAppliedLSN, lastReplicationAckProcessedTime);

    notReceivedReplicationCount = replicationOperations_.NotReceivedCount;
    receivedAndNotAppliedReplicationCount = replicationOperations_.ReceivedAndNotAppliedCount;
    avgReplicationReceiveAckDuration = replicationOperations_.AverageReceiveAckDuration;
    avgReplicationApplyAckDuration = replicationOperations_.AverageApplyAckDuration;

    FABRIC_SEQUENCE_NUMBER lastCopyReceivedLSN = 0;
    FABRIC_SEQUENCE_NUMBER lastCopyAppliedLSN = 0;

    Common::TimeSpan avgCopyReceiveAckDuration = TimeSpan::Zero;
    Common::TimeSpan avgCopyApplyAckDuration = TimeSpan::Zero;

    FABRIC_SEQUENCE_NUMBER notReceivedCopyCount = 0;
    FABRIC_SEQUENCE_NUMBER receivedAndNotAppliedCopyCount = 0;

    DateTime lastCopyAckProcessedTime = DateTime::Zero;

    if (isIdle)
    {
        copySender_.GetCopyProgress(
            lastCopyReceivedLSN,
            lastCopyAppliedLSN,
            avgCopyReceiveAckDuration,
            avgCopyApplyAckDuration,
            notReceivedCopyCount,
            receivedAndNotAppliedCopyCount,
            lastCopyAckProcessedTime);
    }

    DateTime lastAckProcessedTime = lastCopyAckProcessedTime > lastReplicationAckProcessedTime ?
        lastCopyAckProcessedTime : lastReplicationAckProcessedTime;

    return ServiceModel::RemoteReplicatorStatus(
        ReplicaId,
        move(lastAckProcessedTime),
        lastReplicationReceivedLSN,
        lastReplicationAppliedLSN,
        isIdle,
        lastCopyReceivedLSN,
        lastCopyAppliedLSN,
        move(ServiceModel::RemoteReplicatorAcknowledgementStatus(
            move(ServiceModel::RemoteReplicatorAcknowledgementDetail(
                avgReplicationReceiveAckDuration.TotalMilliseconds(),
                avgReplicationApplyAckDuration.TotalMilliseconds(),
                notReceivedReplicationCount,
                receivedAndNotAppliedReplicationCount)),
            move(ServiceModel::RemoteReplicatorAcknowledgementDetail(
                avgCopyReceiveAckDuration.TotalMilliseconds(),
                avgCopyApplyAckDuration.TotalMilliseconds(),
                notReceivedCopyCount,
                receivedAndNotAppliedCopyCount))
        ))
    );
}

bool RemoteSession::IsSameReplica(
    FABRIC_REPLICA_ID replica) const
{
    return replica == replicaId_;
}

bool RemoteSession::HasEndpoint(std::wstring const & address, ReplicationEndpointId const & endpointUniqueId) const
{
    return
        replicaEndpointUniqueId_ == endpointUniqueId &&
        StringUtility::StartsWithCaseInsensitive(endpoint_, address);
}

void RemoteSession::OnStartCopy(
    __in ComOperationCPtr const & startCopyOp,
    __in ReliableOperationSenderSPtr & startCopyOpsSender,
    __in Transport::SendStatusCallback const & sendStatusCallback)
{
    // To keep session alive when session is removed after timer fires and before the message is sent out
    auto sessionRoot = CreateComponentRoot();
    // The timer inside ReliableOperationSender must keep the sender alive
    startCopyOpsSender->Open<ComponentRootSPtr>(
        startCopyOpsSender->CreateComponentRoot(),
        [this, sessionRoot, sendStatusCallback](ComOperationCPtr const & op, bool requestAck, FABRIC_SEQUENCE_NUMBER completedSeqNumber)
        {
            UNREFERENCED_PARAMETER(completedSeqNumber);
            ASSERT_IF(requestAck, "{0}->{1}: StartCopy shouldn't ask for Ack", primaryEndpointUniqueId_, replicaId_);
            return SendStartCopyMessage(op->SequenceNumber, sendStatusCallback);
        });

    {
        AcquireWriteLock lock(lock_);
        ASSERT_IF(
            startCopyOpsSender_,
            "{0}->{1}: Start copy sender already exists",
            primaryEndpointUniqueId_,
            replicaId_);

        startCopyOpsSender_ = startCopyOpsSender;
    }

    startCopyOp_ = startCopyOp;

    // Send the start copy message;
    // this must be done after the copy sender is set,
    // otherwise it's possible for an ACK to come and be ignored
    startCopyOpsSender->Add(startCopyOp_.GetRawPointer(), Constants::InvalidLSN);
}

AsyncOperationSPtr RemoteSession::BeginEstablishCopy(
    FABRIC_SEQUENCE_NUMBER replicationStartSeq,
    bool hasPersistedState,
    Transport::SendStatusCallback const & sendStatusCallback,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    AsyncOperationSPtr asyncOperation;
    {
        AcquireWriteLock grab(establishCopyLock_);
        asyncOperation = AsyncOperation::CreateAndStart<EstablishCopyAsyncOperation>(callback, state);
        establishCopyAsyncOperation_ = asyncOperation;
    }
    AsyncOperation::Get<EstablishCopyAsyncOperation>(asyncOperation)->ResumeOutsideLock(asyncOperation, *this, replicationStartSeq, hasPersistedState, sendStatusCallback);
    return asyncOperation;
}

ErrorCode RemoteSession::EndEstablishCopy(
    AsyncOperationSPtr const & asyncOperation,
    ComPointer<IFabricOperationDataStream> & context)
{
    return EstablishCopyAsyncOperation::End(asyncOperation, context);
}

void RemoteSession::CancelCopy()
{
    AsyncOperationSPtr asyncOperation;
    {
        AcquireWriteLock grab(establishCopyLock_);
        asyncOperation = std::move(establishCopyAsyncOperation_);
    }
    if (asyncOperation)
    {
        asyncOperation->Cancel();
    }
    copySender_.FinishOperation(ErrorCode(Common::ErrorCodeValue::OperationCanceled));
}

void RemoteSession::CompleteEstablishCopyAsyncOperation(
    Common::Guid const & incarnationId,
    Common::ErrorCode const & errorCode)
{
    AsyncOperationSPtr asyncOperation;
    {
        AcquireWriteLock grab(establishCopyLock_);
        asyncOperation = std::move(establishCopyAsyncOperation_);
    }
    if (asyncOperation)
    {
        ReplicatorEventSource::Events->PrimaryCopySessionEstablished(
            partitionId_,
            primaryEndpointUniqueId_,
            replicaId_,
            incarnationId,
            errorCode.ReadValue());
        // Will call user service's GetCopyState, so do it on a separate thread.
        Common::Threadpool::Post([asyncOperation]()
        {
            asyncOperation->TryComplete(asyncOperation);
        });
    }
}

void RemoteSession::CloseStartCopyReliableOperationSender()
{
    ReliableOperationSenderSPtr startCopyOpsSenderCopy;

    {
        AcquireWriteLock lock(lock_);
        startCopyOpsSenderCopy = move(startCopyOpsSender_);
    }

    if (startCopyOpsSenderCopy)
    {
        startCopyOpsSenderCopy->Close();
        startCopyOpsSenderCopy.reset();
    }
}

void RemoteSession::OnLastEnumeratorCopyOp(
    FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber,
    FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber,
    ErrorCode error)
{
    if (error.IsSuccess())
    {
        copySender_.OnLastEnumeratorCopyOp(
            lastCopySequenceNumber,
            lastReplicationSequenceNumber);
        // Check whether all replication operations are ACKed
        copySender_.ProcessReplicationAck(
            Constants::InvalidLSN,
            replicationOperations_.LastAckedSequenceNumber);
    }
    else
    {
        copySender_.FinishOperation(error);
    }
}

AsyncOperationSPtr RemoteSession::BeginCopy(
    ComProxyAsyncEnumOperationData && asyncEnumOperationData,
    EnumeratorLastOpCallback const & enumeratorLastOpCallback,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    SendOperationCallback copySendCallback = 
        [this](ComOperationCPtr const & op, bool requestAck, FABRIC_SEQUENCE_NUMBER completedSeqNumber)
        {
            UNREFERENCED_PARAMETER(completedSeqNumber);

            if (requestAck)
            {
                ASSERT_IF(op.GetRawPointer() != nullptr, "{0}->{1}: RequestAck: Operation shouldn't be set", primaryEndpointUniqueId_, replicaId_);
                return SendRequestAck();
            }
            else
            {
                return SendCopyOperation(op, copySender_.IsOperationLast(op));
            }
        };

    return copySender_.BeginCopy(
        move(asyncEnumOperationData),
        copySendCallback,
        enumeratorLastOpCallback,
        callback,
        state);
}

ErrorCode RemoteSession::EndCopy(
    AsyncOperationSPtr const & asyncOperation,
    ErrorCode & faultErrorCode,
    wstring & faultDescription)
{
    auto error = copySender_.EndCopy(asyncOperation, faultErrorCode, faultDescription);
    CleanupCopyContextReceiver(error);

    return error;
}

bool RemoteSession::UpdateLastReplicationSequenceNumberForIdle(FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber)
{
    return copySender_.UpdateLastReplicationOperationDuringCopy(lastReplicationSequenceNumber);
}

void RemoteSession::AddReplicateOperation(
    ComOperationRawPtr const operationPtr,
    FABRIC_SEQUENCE_NUMBER completedSeqNumber)
{
    replicationOperations_.Add(operationPtr, completedSeqNumber);
}

void RemoteSession::AddReplicateOperations(
    ComOperationRawPtrVector const & operations,
    FABRIC_SEQUENCE_NUMBER completedSeqNumber)
{
    replicationOperations_.Add(operations, completedSeqNumber);
}

void RemoteSession::UpdateAckProgress(
    FABRIC_SEQUENCE_NUMBER replicationReceivedLSN,
    FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
    FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
    wstring messageId,
    ReplicationAckProgressCallback callback)
{
    bool progress = UpdateAckProgress(
        replicationReceivedLSN,
        replicationQuorumLSN,
        copyReceivedLSN,
        copyQuorumLSN,
        true,
        messageId,
        callback);

    if (progress)
    {
        StartFaultReplicaMessageSendTimerIfNeeded();
    }
}

void RemoteSession::UpdateAckProgress(
    FABRIC_SEQUENCE_NUMBER replicationReceivedLSN,
    FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
    wstring messageId,
    ReplicationAckProgressCallback callback)
{
    bool progress = UpdateAckProgress(
        replicationReceivedLSN,
        replicationQuorumLSN,
        Constants::NonInitializedLSN,
        Constants::NonInitializedLSN,
        false,
        messageId,
        callback);

    if (progress)
    {
        StartFaultReplicaMessageSendTimerIfNeeded();
    }
}

bool RemoteSession::UpdateAckProgress(
    FABRIC_SEQUENCE_NUMBER replicationReceivedLSN,
    FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
    FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
    bool processCopyAck,
    wstring messageId,
    ReplicationAckProgressCallback callback)
{
    bool duplicateAck = true;

    {
        AcquireWriteLock progressLock(replicationAckProcessingLock_);
        if (replicationReceivedLSN > latestReplicationReceiveAck_ ||
            replicationQuorumLSN > latestReplicationQuorumAck_)
        {
            latestReplicationReceiveAck_ = replicationReceivedLSN;
            latestReplicationQuorumAck_ = replicationQuorumLSN;
            processCopyAcks_ = processCopyAck;
            duplicateAck = false;
        }

        if (processCopyAck)
        {
            if ((latestCopyReceiveAck_ != Constants::NonInitializedLSN) &&
                (copyReceivedLSN > latestCopyReceiveAck_ || copyReceivedLSN == Constants::NonInitializedLSN))
            {
                // new copy receive ack was seen
                processCopyAcks_ = true;
                latestCopyReceiveAck_ = copyReceivedLSN;
                duplicateAck = false;
            }
            if ((latestCopyQuorumAck_ != Constants::NonInitializedLSN) &&
                (copyQuorumLSN > latestCopyQuorumAck_ || copyQuorumLSN == Constants::NonInitializedLSN))
            {
                // new copy quorum ack was seen
                processCopyAcks_ = true;
                latestCopyQuorumAck_ = copyQuorumLSN;
                duplicateAck = false;
            }
        }
        else
        {
            TESTASSERT_IF(copyReceivedLSN != Constants::NonInitializedLSN && copyQuorumLSN != Constants::NonInitializedLSN, "Copy Receive and Apply LSN must be -1");
            processCopyAcks_ = false;
        }

        if (duplicateAck)
        {
            return false;
        }

        latestTraceMessageId_ = messageId;

        if (replicationAckProcessingInProgress_)
        {
            return true;
        }

        // Before starting the processing, use the latest values of all ACK's
        replicationAckProcessingInProgress_ = true;
        replicationReceivedLSN = latestReplicationReceiveAck_;
        replicationQuorumLSN = latestReplicationQuorumAck_;
        copyReceivedLSN = latestCopyReceiveAck_;
        copyQuorumLSN = latestCopyQuorumAck_;
    }

    // Initiate processing of ack if :
    // 1. currently it is not running AND
    // 2. The ACK progress was greater than what was already known 

    // The second condition is to ensure the sending window size does not increase due to duplicate ACK's
    auto root = CreateComponentRoot();
    Common::Threadpool::Post([this, root, replicationQuorumLSN, replicationReceivedLSN, processCopyAck, copyReceivedLSN, copyQuorumLSN, callback]
    {
        AckProcessor(replicationReceivedLSN, replicationQuorumLSN, processCopyAck, copyReceivedLSN, copyQuorumLSN, callback);
    });

    return true;
}

void RemoteSession::AckProcessor(
    FABRIC_SEQUENCE_NUMBER replicationReceivedLSN,
    FABRIC_SEQUENCE_NUMBER replicationQuorumLSN,
    bool processCopyAck,
    FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
    ReplicationAckProgressCallback progressCallback)
{
    wstring cacheLatestTraceMessageId;
    bool shouldTrace = false;
    bool continueProcessing = true;

    // Continue to process ACK's in the loop as long as there are new ACK's being received
    while (continueProcessing)
    {
        bool progressDone = false;
        FABRIC_SEQUENCE_NUMBER replLastAckedLSN = Constants::NonInitializedLSN;

        // Look for start copy ops sender only if we are still IB
        if (processCopyAck && replicationReceivedLSN >= Constants::InvalidLSN)
        {
            CloseStartCopyReliableOperationSender();
        }

        progressDone = replicationOperations_.ProcessOnAck(replicationReceivedLSN, replicationQuorumLSN);
        replLastAckedLSN = replicationOperations_.LastAckedSequenceNumber;

        // Step 3- Update Copy Sender's progress
        // NOTE - Always call ProcessReplicationAck before ProcessCopyAck
        copySender_.ProcessReplicationAck(copyQuorumLSN, replLastAckedLSN);
        if (progressDone &&
            progressCallback != nullptr)
        {
            progressCallback();
        }

        if (processCopyAck)
        {
            copySender_.ProcessCopyAck(copyReceivedLSN, copyQuorumLSN);
        }

        // Step 4- Check if any new ack's were received.
        // Do this inside a write lock as we may have to update the boolean flag if ack's were not received
        {
            AcquireWriteLock grab(replicationAckProcessingLock_);

            if (latestReplicationReceiveAck_ > replicationReceivedLSN ||
                latestReplicationQuorumAck_ > replicationQuorumLSN)
            {
                // new replication ACK's were seen, continue processing with the new numbers.
            }
            else if ((copyReceivedLSN != Constants::NonInitializedLSN) &&
                (latestCopyReceiveAck_ > copyReceivedLSN || latestCopyReceiveAck_ == Constants::NonInitializedLSN))
            {
                // new copy receive ACK's were seen. continue processing
            }
            else if ((copyQuorumLSN != Constants::NonInitializedLSN) &&
                (latestCopyQuorumAck_ > copyQuorumLSN || latestCopyQuorumAck_ == Constants::NonInitializedLSN))
            {
                // new copy quorum ACK's were seen. continue processing
            }
            else
            {
                replicationAckProcessingInProgress_ = false;

                if (lastAckTraceWatch_.Elapsed >= config_->QueueFullTraceInterval)
                {
                    lastAckTraceWatch_.Restart();
                    shouldTrace = true;
                }
                cacheLatestTraceMessageId = latestTraceMessageId_;

                // disable processing as there was no new progress
                continueProcessing = false;
            }

            // Update the local variables with the latest values
            copyReceivedLSN = latestCopyReceiveAck_;
            copyQuorumLSN = latestCopyQuorumAck_;
            replicationReceivedLSN = latestReplicationReceiveAck_;
            replicationQuorumLSN = latestReplicationQuorumAck_;
            processCopyAck = processCopyAcks_;
        }
    }

    ReplicatorEventSource::Events->PrimaryAckProcess(
        partitionId_,
        primaryEndpointUniqueId_,
        replicaId_,
        replicationReceivedLSN,
        replicationQuorumLSN,
        processCopyAck,
        copyReceivedLSN,
        copyQuorumLSN,
        0,
        0,
        cacheLatestTraceMessageId);
}

bool RemoteSession::Test_IsReplicationAckProcessingInProgress() const
{
    AcquireReadLock lock(replicationAckProcessingLock_);
    return replicationAckProcessingInProgress_;
}

void RemoteSession::OnCopyFailure(int errorCodeValue)
{
    copySender_.FinishOperation(ErrorCode(static_cast<Common::ErrorCodeValue::Enum>(errorCodeValue)));
}

bool RemoteSession::ProcessCopyContext(
    ComOperationCPtr && op,
    bool isLast)
{
    ASSERT_IFNOT(op, "op should be not null");
    CopyContextReceiverSPtr cachedCopyContextReceiver;
    {
        AcquireReadLock lock(lock_);
        cachedCopyContextReceiver = copyContextReceiver_;
    }

    if (cachedCopyContextReceiver)
    {
        return cachedCopyContextReceiver->ProcessCopyContext(move(op), isLast);
    }
    else
    {
        return false;
    }
}

void RemoteSession::CreateCopyContext(
    __out Common::ComPointer<IFabricOperationDataStream> & context)
{
    // Create copy context manager to buffer
    // the copy context messages from the replica.
    // The context returned is on top of the queue of these messages
    auto ccManager = std::make_shared<CopyContextReceiver>(
        config_,
        partitionId_,
        Constants::CopyContextOperationTrace,
        primaryEndpointUniqueId_,
        replicaId_);

    auto root = CreateComponentRoot();

    ccManager->Open(
        [this, root](bool shouldTrace)
    {
        return SendCopyContextAck(shouldTrace);
    });

    ccManager->CreateEnumerator(context);

    {
        AcquireWriteLock lock(lock_);
        copyContextReceiver_ = move(ccManager);
    }
}

void RemoteSession::CleanupCopyContextReceiver(ErrorCode error)
{
    CopyContextReceiverSPtr cachedCopyContextReceiver;
    {
        AcquireWriteLock lock(lock_);
        cachedCopyContextReceiver = move(copyContextReceiver_);
    }

    if (cachedCopyContextReceiver)
    {
        // Close the copy context manager,
        // since copy is finishing
        if (error.IsSuccess() && !cachedCopyContextReceiver->AllOperationsReceived)
        {
            ReplicatorEventSource::Events->PrimaryCCMissing(
                partitionId_,
                primaryEndpointUniqueId_,
                replicaId_);
        }

        cachedCopyContextReceiver->Close(error);
    }
}

bool RemoteSession::SendTransportMessage(
    KBuffer::SPtr const & sharedHeaders,
    MessageUPtr && message,
    bool setExpiration)
{
    auto expiration = setExpiration ?
        config_->RetryInterval :
        TimeSpan::MaxValue;

    auto errorCode = transport_->SendMessage(
        *sharedHeaders, primaryEndpointUniqueId_.PartitionId, sendTarget_, move(message), expiration);

    bool ret = true;

    switch (errorCode.ReadValue())
    {
    case Common::ErrorCodeValue::MessageTooLarge:
        Replicator::ReportFault(
            partition_,
            partitionId_,
            primaryEndpointUniqueId_,
            L"SendTransportMessage failed",
            errorCode);
        break;
    case Common::ErrorCodeValue::TransportSendQueueFull:
        // We only return failure if we hit send queue full error so that 
        // the sender stops retrying
        //
        // Other error codes are ignored by upper layer
        ret = false;
        break;
    }

    return ret;
}

} // end namespace ReplicationComponent
} // end namespace Reliability
