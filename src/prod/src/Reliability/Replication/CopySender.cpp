// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireExclusiveLock;
using Common::CompletedAsyncOperation;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ComponentRoot;
using Common::ErrorCode;
using Common::make_com;
using Common::StringWriter;

using std::move;
using std::wstring;

CopySender::CopySender(
    REInternalSettingsSPtr const & config,
    ComProxyStatefulServicePartition const & partition,
    Common::Guid const & partitionId,
    std::wstring const & purpose,
    ReplicationEndpointId const & from,
    FABRIC_REPLICA_ID to,
    bool waitForReplicationAcks,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    Common::ApiMonitoring::ApiName::Enum apiName)
    :   config_(config),
        partition_(partition),
        partitionId_(partitionId),
        purpose_(purpose),
        copyAsyncOperation_(),
        errorCodeValue_(Common::ErrorCodeValue::Success),
        copyState_(waitForReplicationAcks),
        copyLock_(),
        endpointUniqueueId_(from),
        replicaId_(to),
        apiMonitor_(apiMonitor),
        apiName_(apiName),
        disableBuildCompletion_(false)
{    
}

CopySender::~CopySender()
{
}

void CopySender::FinishOperation(ErrorCode const & error)
{
    AsyncOperationSPtr cachedOp;
    {
        AcquireExclusiveLock lock(copyLock_);
        cachedOp = move(copyAsyncOperation_);
        errorCodeValue_ = error.ReadValue();
    }

    if (cachedOp)
    {
        cachedOp->TryComplete(cachedOp, error);
    }
}

void CopySender::OnLastEnumeratorCopyOp(
    FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber,
    FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber)
{
    ReplicatorEventSource::Events->CopySenderOnLastEnumOp(
        partitionId_,
        endpointUniqueueId_,
        replicaId_,
        purpose_,
        lastCopySequenceNumber,
        lastReplicationSequenceNumber);

    AcquireExclusiveLock lock(copyLock_);
    copyState_.SetLSNs(lastCopySequenceNumber, lastReplicationSequenceNumber);
}

bool CopySender::UpdateLastReplicationOperationDuringCopy(FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber)
{
    AcquireExclusiveLock lock(copyLock_);
    if (copyState_.UpdateReplicationLSN(lastReplicationSequenceNumber))
    {
        ReplicatorEventSource::Events->CopySenderUpdateLastReplicationLSN(
            partitionId_,
            endpointUniqueueId_,
            replicaId_,
            purpose_,
            lastReplicationSequenceNumber);

        return true;
    }

    return false;
}

AsyncOperationSPtr CopySender::BeginCopy(
    ComProxyAsyncEnumOperationData && asyncEnumOperationData,
    SendOperationCallback const & copySendCallback,
    EnumeratorLastOpCallback const & enumeratorLastOpCallback,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    ReplicatorEventSource::Events->CopySenderBeginCopy(
        partitionId_,
        endpointUniqueueId_,
        replicaId_,
        purpose_);

    AsyncOperationSPtr copyAsyncOpCopy;
    {
        // Check that no operation is in progress
        // and mark that copy is starting
        AcquireExclusiveLock lock(copyLock_);
        ASSERT_IF(
            copyAsyncOperation_,
            "{0}->{1}:{2}: CopySender: Can't start multiple copy async operations",
            endpointUniqueueId_, replicaId_, purpose_);
        if (copyState_.IsCompleted() || errorCodeValue_ != Common::ErrorCodeValue::Success)
        {
            // It's possible copying context initiated by a stale StartCopy is cancelled before
            // the copy actually happens.  In this case we just simply complete the copy.
            copyAsyncOperation_ = std::make_shared<CompletedAsyncOperation>(
                ErrorCode(static_cast<Common::ErrorCodeValue::Enum>(errorCodeValue_)),
                callback,
                state);
        }
        else
        {
            wstring description;
            StringWriter(description).Write("{0}->{1}:{2}", endpointUniqueueId_, replicaId_, purpose_);
            copyState_.Start();
            copyAsyncOperation_ = std::shared_ptr<CopyAsyncOperation>(new CopyAsyncOperation(
                config_,
                partitionId_,
                purpose_,
                description,
                endpointUniqueueId_,
                replicaId_,
                move(asyncEnumOperationData),
                apiMonitor_,
                apiName_,
                copySendCallback,
                enumeratorLastOpCallback,
                callback,
                state));
#ifdef DBG
            dynamic_cast<CopyAsyncOperation*>(copyAsyncOperation_.get())->OpenReliableOperationSender();
#else
            static_cast<CopyAsyncOperation*>(copyAsyncOperation_.get())->OpenReliableOperationSender();
#endif
        }
        copyAsyncOpCopy = copyAsyncOperation_;
    }

    // Parent(e.g. Build) could have been cancelled, so move the Start out of lock to avoid deadlock.
    copyAsyncOpCopy->Start(copyAsyncOpCopy);

    // Double check error code.
    // For CopyContext copy sender, FailCopy() could be called before the copy starts:
    // Primary notifies secondary of failures by sending CopyContextACK to it, even when no CopyContext is received.
    if (Common::ErrorCodeValue::Success != errorCodeValue_)
    {
        copyAsyncOpCopy->TryComplete(copyAsyncOpCopy, ErrorCode((Common::ErrorCodeValue::Enum)errorCodeValue_));
    }

    return copyAsyncOpCopy;
}

ErrorCode CopySender::EndCopy(
    AsyncOperationSPtr const & asyncOperation, 
    ErrorCode & faultErrorCode,
    wstring & faultDescription)
{
    ErrorCode error;
    auto copyAsyncOp = dynamic_cast<CopyAsyncOperation*>(asyncOperation.get());
    if (copyAsyncOp)
    {
        error = CopyAsyncOperation::End(asyncOperation, faultErrorCode, faultDescription);
    }
    else
    {
        error = CompletedAsyncOperation::End(asyncOperation);
    }

    ReplicatorEventSource::Events->CopySenderEndCopy(
        partitionId_,
        endpointUniqueueId_,
        replicaId_,
        purpose_,
        error.ReadValue());
        
    {
        // Mark that the copy was done
        AcquireExclusiveLock lock(copyLock_);
        copyState_.Finish(error.IsSuccess());
        copyAsyncOperation_.reset();
        
        // If it turns out that we had disabled build completion for this replica due to slow progress, but it 
        // actually completed build successfully, it is probably due to a race condition where we checked for the completion of the build 
        // but it had already started completing.

        // The only option left here is to fail the build and also fault the replicator
        if (error.IsSuccess() &&
            disableBuildCompletion_ == true)
        {
            ReplicatorEventSource::Events->CopySenderBuildSucceededAfterBeingDisabled(
                partitionId_,
                endpointUniqueueId_,
                replicaId_,
                purpose_,
                ToString());
             
            faultErrorCode = Common::ErrorCodeValue::ReplicatorInternalError;
            faultDescription = L"Build for replica completed successfully, in spite of disabling it due to slow progress";
            error = faultErrorCode;
        }
    }

    return error;
}

bool CopySender::TryDisableBuildCompletion(wstring const & progress)
{
    AcquireExclusiveLock grab(copyLock_);

    ASSERT_IF(disableBuildCompletion_, 
        "{0}->{1}:{2}: CopySender: Can't Disable build completion as it is already disabled",
        endpointUniqueueId_, replicaId_, purpose_);
    
    // The copyState_ flag is marked as 'Completed' in the callback of the async op completion.
    // So the async op may already have started 'completing' and hence double checking that here
    if ((copyState_.IsCompleted()) || 
        (copyAsyncOperation_ != nullptr && copyAsyncOperation_->IsCompleted))
    { 
        ReplicatorEventSource::Events->CopySenderDisableBuildCompletionFailed(
            partitionId_,
            endpointUniqueueId_, 
            replicaId_, 
            copyState_.ToString(),
            progress);

        // Copy has already completed. It is too late to disable build completion
        return false;
    }

    disableBuildCompletion_ = true;

    ReplicatorEventSource::Events->CopySenderDisableBuildCompletion(
        partitionId_,
        endpointUniqueueId_, 
        replicaId_,
        copyState_.ToString(),
        progress);

    return true;
}

void CopySender::ProcessReplicationAck(
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN,
    FABRIC_SEQUENCE_NUMBER lastReplicationSequenceNumber)
{
    AsyncOperationSPtr cachedOp;
    bool tryComplete = false;
    {
        AcquireExclusiveLock lock(copyLock_);
        if (!copyAsyncOperation_)
        {
            // Copy is not in progress
            return;
        }

        // Ensure that the ReplicationCompleted is set only after COPY is done.
        // Without that, the first ack from the idle will set the state to ReplCompleted if there are no repl operations on the primary.
        // Now, if the build takes a long time, we will never be able to move up the build complete lsn
        if (copyQuorumLSN == Constants::NonInitializedLSN || 
            copyState_.IsLastCopyLSN(copyQuorumLSN))
        {
            if (!disableBuildCompletion_)
            {
                // we can only complete copy successfully if the replica is not faulted due to slow progress
                // When the replica is indeed faulted, we expect RAP to cancel this build operation

                tryComplete = copyState_.TryCompleteReplication(lastReplicationSequenceNumber);
                cachedOp = copyAsyncOperation_;
            }
        }
    }

    if (tryComplete)
    {   
        auto op = AsyncOperation::Get<CopyAsyncOperation>(cachedOp);
        ReplicatorEventSource::Events->CopySenderReplDone(
            partitionId_,
            endpointUniqueueId_,
            replicaId_,
            purpose_,
            lastReplicationSequenceNumber);

        // Complete if all copy operations done
        op->TryCompleteWithSuccessIfCopyDone(cachedOp);
    }
}

bool CopySender::ProcessCopyAck(
    FABRIC_SEQUENCE_NUMBER copyReceivedLSN,
    FABRIC_SEQUENCE_NUMBER copyQuorumLSN)
{
    bool isLast = false;
    bool shouldCompleteCopy = false;
    FABRIC_SEQUENCE_NUMBER copyUpdateLSN = Constants::InvalidLSN;
    FABRIC_SEQUENCE_NUMBER copyUpdateReceiveLSN = Constants::InvalidLSN;
    AsyncOperationSPtr cachedOp = nullptr;
    
    {
        AcquireExclusiveLock lock(copyLock_);
        if (copyState_.IsInProgress() && copyAsyncOperation_)
        {
            if (copyQuorumLSN == Constants::NonInitializedLSN || 
                copyState_.IsLastCopyLSN(copyQuorumLSN))
            {
                // Either an ACK for a replication operation is received,
                // (which implies that all copy operations have been processed)
                // or an ACK for last copy operation.
                copyUpdateLSN = copyState_.LastCopySequenceNumber;
                isLast = true;
                if (copyState_.IsReplicationCompleted() && 
                    !disableBuildCompletion_)
                {
                    // we can only complete copy successfully if the replica is not faulted due to slow progress
                    // When the replica is indeed faulted, we expect RAP to cancel this build operation
                    shouldCompleteCopy = true;
                    cachedOp = move(copyAsyncOperation_);
                }
                else
                {
                    cachedOp = copyAsyncOperation_;
                }
            }
            else
            {
                // Make a copy of the async operation
                cachedOp = copyAsyncOperation_;
                copyUpdateLSN = copyQuorumLSN;
            }

            if (copyReceivedLSN != Constants::InvalidLSN)
            {
                if (copyReceivedLSN == Constants::NonInitializedLSN)
                {
                    // The last Copy operation was received by the secondary
                    copyUpdateReceiveLSN = copyState_.LastCopySequenceNumber;
                    ASSERT_IF(
                        copyUpdateReceiveLSN == Constants::InvalidLSN,
                        "{0}->{1}:{2}: AckReceive = -1, but last copy LSN not set {3}",
                        endpointUniqueueId_,
                        replicaId_,
                        purpose_,
                        copyUpdateReceiveLSN);
                }
                else
                {
                    copyUpdateReceiveLSN = copyReceivedLSN;
                }
            }
        }
    }

    if (copyUpdateLSN != Constants::InvalidLSN || 
        copyUpdateReceiveLSN != Constants::InvalidLSN)
    {
        ASSERT_IFNOT(cachedOp, "{0}->{1}:{2}: Copy must be in progress", endpointUniqueueId_, replicaId_, purpose_);
        
        // Complete operations in copy queue
        // and, if needed, complete the copy async operation outside the lock
        auto op = AsyncOperation::Get<CopyAsyncOperation>(cachedOp);
        op->UpdateProgress(
            copyUpdateLSN, 
            copyUpdateReceiveLSN,
            cachedOp, 
            isLast);

        if (shouldCompleteCopy)
        {
            ReplicatorEventSource::Events->CopySenderDone(
                partitionId_,
                endpointUniqueueId_,
                replicaId_,
                purpose_);
            // All copy operations and the replication operations
            // have been ACKed
            op->TryCompleteWithSuccessIfCopyDone(cachedOp);
        }
                
        return true;
    }

    return false;
}

bool CopySender::IsOperationLast(ComOperationCPtr const & operation)
{
    AcquireExclusiveLock lock(copyLock_);
    return copyState_.IsLastCopyLSN(operation->SequenceNumber);
}
 
void CopySender::GetCopyProgress(
    __out FABRIC_SEQUENCE_NUMBER & copyReceivedLSN,
    __out FABRIC_SEQUENCE_NUMBER & copyAppliedLSN,
    __out Common::TimeSpan & avgCopyReceiveAckDuration,
    __out Common::TimeSpan & avgCopyApplyAckDuration,
    __out FABRIC_SEQUENCE_NUMBER & notReceivedCopyCount,
    __out FABRIC_SEQUENCE_NUMBER & receivedAndNotAppliedCopyCount,
    __out Common::DateTime & lastCopyAckProcessedTime) const
{
    AsyncOperationSPtr cachedOp;
    {
        AcquireExclusiveLock lock(copyLock_);
        cachedOp = copyAsyncOperation_;
    }

    if (cachedOp)
    {
        auto op = dynamic_cast<CopyAsyncOperation*>(cachedOp.get());
        if (op)
        {
            op->GetCopyProgress(
                copyReceivedLSN, 
                copyAppliedLSN, 
                avgCopyReceiveAckDuration,
                avgCopyApplyAckDuration,
                notReceivedCopyCount,
                receivedAndNotAppliedCopyCount,
                lastCopyAckProcessedTime);
        }
    }
}

wstring CopySender::ToString() const
{
    std::wstring content;
    Common::StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));        
    return content;
}
        
void CopySender::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    AsyncOperationSPtr cachedOp;
    {
        AcquireExclusiveLock lock(copyLock_);
        cachedOp = copyAsyncOperation_;
    }

    if (cachedOp)
    {
        auto op = dynamic_cast<CopyAsyncOperation*>(cachedOp.get());
        if (op)
        {
            w << op->GetOperationSenderProgress();
        }
    }
}

/* *********************************
   * CopyState methods
   *********************************/
CopySender::CopyState::CopyState(bool waitForReplicationAcks) 
    :   lastCopySequenceNumber_(Constants::NonInitializedLSN),
        lastReplicationSequenceNumber_(Constants::NonInitializedLSN),
        state_(NotStarted),
        waitForReplicationAcks_(waitForReplicationAcks)
{
}

bool CopySender::CopyState::IsInProgress() const 
{
    return (state_ != NotStarted && state_ != Completed);
}

bool CopySender::CopyState::IsLastCopyLSN(FABRIC_SEQUENCE_NUMBER copyLSN) const
{
    return (state_ == LSNSet || state_ == ReplCompleted || state_ == Completed) &&
            copyLSN == lastCopySequenceNumber_;
}

void CopySender::CopyState::Start() 
{
    // Primary should prevent Copy being called 2 times for the same replica,
    // so assert that we never get into this state
    ASSERT_IFNOT(
        state_ == NotStarted,
        "Start copy: Invalid state {0}, LSN: COPY {1}, REPL {2}", 
        static_cast<int>(state_),
        lastCopySequenceNumber_,
        lastReplicationSequenceNumber_);
    state_ = Started;
}

void CopySender::CopyState::Finish(bool succeeded) 
{ 
    if (succeeded)
    {
        ASSERT_IF(
            state_ != LSNSet && state_ != ReplCompleted,
            "Finish copy: Invalid state {0}: LSN: COPY {1}, REPL {2}",
            static_cast<int>(state_),
            lastCopySequenceNumber_,
            lastReplicationSequenceNumber_);
    }

    state_ = Completed;
}

void CopySender::CopyState::SetLSNs(
    FABRIC_SEQUENCE_NUMBER copyLSN,
    FABRIC_SEQUENCE_NUMBER replicationLSN)
{
    if (state_ == Completed)
    {
        // Nothing to do, the copy operation completed with errors
        return;
    }

    ASSERT_IF(
        state_ != Started,
        "SetLSNs: Invalid state {0}: Copy LSN {1}, desired {2}, repl LSN {3}, desired {4}", 
        static_cast<int>(state_),
        lastCopySequenceNumber_,
        copyLSN,
        lastReplicationSequenceNumber_,
        replicationLSN);

    lastCopySequenceNumber_ = copyLSN;
    if (waitForReplicationAcks_)
    {
        lastReplicationSequenceNumber_ = replicationLSN;
        state_ = LSNSet;
    }
    else
    {
        // Transition to ReplCompleted state, since no replication ACKs are needed
        state_ = ReplCompleted;
    }
}

bool CopySender::CopyState::UpdateReplicationLSN(FABRIC_SEQUENCE_NUMBER replicationLSN)
{
    ASSERT_IFNOT(waitForReplicationAcks_, "UpdateReplicationLSN: Wait for Replication ACK's should be set");

    if (state_ == Completed ||
        state_ == ReplCompleted ||
        state_ != LSNSet ||
        lastReplicationSequenceNumber_ >= replicationLSN)
    {
        return false;
    }

    lastReplicationSequenceNumber_ = replicationLSN;
    return true;
}

bool CopySender::CopyState::TryCompleteReplication(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    if (state_ == LSNSet && sequenceNumber >= lastReplicationSequenceNumber_)
    {
        state_ = ReplCompleted;
        return true;
    }
            
    return false;
}

wstring CopySender::CopyState::ToString() const
{
    std::wstring content;
    Common::StringWriter writer(content);

    switch (state_)
    {
    case Reliability::ReplicationComponent::CopySender::CopyState::NotStarted:
        writer.Write("NotStarted");
        break;
    case Reliability::ReplicationComponent::CopySender::CopyState::Started:
        writer.Write("Started");
        break;
    case Reliability::ReplicationComponent::CopySender::CopyState::LSNSet:
        writer.Write("LSNSet");
        break;
    case Reliability::ReplicationComponent::CopySender::CopyState::ReplCompleted:
        writer.Write("ReplCompleted");
        break;
    case Reliability::ReplicationComponent::CopySender::CopyState::Completed:
        writer.Write("Completed");
        break;
    default:
        writer.Write("Unknown");
        break;
    } 

    return content;
}

} // end namespace ReplicationComponent
} // end namespace Reliability
