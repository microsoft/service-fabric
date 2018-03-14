// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::AsyncCallback;
using Common::ComPointer;
using Common::ErrorCode;
using Common::make_com;
using Common::make_unique;
using Common::ReaderQueue;
using Common::StringWriter;

using std::move;
using std::shared_ptr;
using std::wstring;

SecondaryCopyReceiver::SecondaryCopyReceiver(
    REInternalSettingsSPtr const & config,
    bool requireServiceAck,
    ReplicationEndpointId const & endpointUniqueId,
    Common::Guid const & partitionId,
    bool enableEOSOpAndStreamFaults,
    bool copyDone)
    :   config_(config),
        requireServiceAck_(requireServiceAck),
        endpointUniqueId_(endpointUniqueId),
        partitionId_(partitionId),
        enableEOSOpAndStreamFaults_(enableEOSOpAndStreamFaults),
        dispatchQueue_(ReaderQueue<ComOperationCPtr>::Create()),
        stream_(),
        streamCreated_(false),
        copyQueue_(nullptr),
        lastCopySequenceNumber_(Constants::NonInitializedLSN),
        allCopyOperationsReceived_(copyDone),
        copyQueueClosed_(copyDone),
        lastCopyCompletedAck_(Constants::InvalidLSN),
        enqueuedEndOfStreamOperation_(false),
        ackedEndOfStreamOperation_(!requireServiceAck),
        replicationOrCopyStreamFaulted_(false)
{
    if (!copyDone)
    {
        CreateCopyQueue();
    }
    else
    {
        dispatchQueue_->Close();
    }
}
    
SecondaryCopyReceiver::~SecondaryCopyReceiver()
{
}

OperationStreamSPtr SecondaryCopyReceiver::GetStream(
    SecondaryReplicatorSPtr const & secondary)
{
    auto streamSPtr = stream_.lock();
    if (!streamSPtr)
    {
        streamSPtr = std::make_shared<OperationStream>(
            false,     // this is not a replication stream
            secondary,
            dispatchQueue_,
            partitionId_,
            endpointUniqueId_, 
            enableEOSOpAndStreamFaults_);
        stream_ = streamSPtr;
        streamCreated_.store(true);
    }
    
    return streamSPtr;
}

void SecondaryCopyReceiver::CleanQueue(
    Common::ComponentRoot const & root,
    bool ignoreEOSOperationAck)
{
    DiscardOutOfOrderOperations();

    if (copyQueue_)
    {
        if (requireServiceAck_)
        {
            // Use the operation queue to mark 
            // all committed operations to ignore Acknowledge callback
            ComOperationRawPtrVector committedOperations;
            copyQueue_->GetOperations(
                copyQueue_->LastCompletedSequenceNumber + 1,
                committedOperations);

            for (auto const & operation : committedOperations)
            {
                operation->SetIgnoreAckAndKeepParentAlive(root);
            }

            copyQueue_->Reset(copyQueue_->NextToBeCompletedSequenceNumber);
        }

        copyQueueClosed_ = true;
    }
    
    if (ignoreEOSOperationAck)
    {
        MarkEOSOperationACK(true /* ignoreAck */);
    }
}

void SecondaryCopyReceiver::CleanQueueOnClose(
    Common::ComponentRoot const & root,
    bool ignoreEOSOperationAck)
{
    CleanQueue(root, ignoreEOSOperationAck);
}

void SecondaryCopyReceiver::CleanQueueOnUpdateEpochFailure(
    Common::ComponentRoot const & root)
{
    CleanQueue(root, true);
}

void SecondaryCopyReceiver::EnqueueEndOfStreamOperationAndKeepRootAlive(OperationAckCallback const & callback, Common::ComponentRoot const & root)
{
    ASSERT_IFNOT(enableEOSOpAndStreamFaults_, "{0}: Cannot enqueue end of stream operation if config is not enabled", endpointUniqueId_);
    if (enqueuedEndOfStreamOperation_)
    {
        return;
    }

    enqueuedEndOfStreamOperation_ = true;
    dispatchQueue_->EnqueueWithoutDispatch(
        make_unique<ComOperationCPtr>(ComFromBytesOperation::CreateEndOfStreamOperationAndKeepRootAlive(callback, root)));

    ReplicatorEventSource::Events->SecondaryEOSOperation(partitionId_, endpointUniqueId_, Constants::CopyOperationTrace, Constants::EnqueueOperationTrace);
}

void SecondaryCopyReceiver::DiscardOutOfOrderOperations()
{
    if (copyQueue_)
    {
        if (requireServiceAck_)
        {
            // Discard any out of order items, which have not been given to the service yet
            copyQueue_->DiscardNonCommittedOperations();
        }
        else
        {
            copyQueue_->DiscardNonCompletedOperations();
        }
    }
}

void SecondaryCopyReceiver::GetAck(
    __out FABRIC_SEQUENCE_NUMBER & copyCommittedLSN,
    __out FABRIC_SEQUENCE_NUMBER & copyCompletedLSN)
{
    if (copyQueue_)
    {
        if (allCopyOperationsReceived_)
        {
            copyCommittedLSN = Constants::NonInitializedLSN;
        }
        else
        {
            copyCommittedLSN = copyQueue_->LastCommittedSequenceNumber;
        }

        copyCompletedLSN = copyQueue_->LastCompletedSequenceNumber;
    }
    else
    {
        copyCommittedLSN = Constants::NonInitializedLSN;
        copyCompletedLSN = Constants::NonInitializedLSN;
    }

    lastCopyCompletedAck_ = copyCompletedLSN;
}

bool SecondaryCopyReceiver::ProcessCopyOperation(
    ComOperationCPtr && operation,
    bool isLast,
    __out bool & progressDone,
    __out bool & shouldCloseDispatchQueue,
    __out bool & checkForceSendAck,
    __out bool & checkCopySender)
{
    progressDone = false;
    shouldCloseDispatchQueue = false;
    checkForceSendAck = false;
    checkCopySender = false;

    if (isLast)
    {
        lastCopySequenceNumber_ = operation->SequenceNumber;
        // There are cases in unit test CIT's where the last operation is not empty. Hence use TESTASSERT as taef unit tests dont enable test flag 
        TESTASSERT_IFNOT(operation->IsEmpty(), "{0}: Last Operation is not empty", endpointUniqueId_);
    }
    
    // If all copy operations are received, send ACK to let primary know.
    if (allCopyOperationsReceived_ || copyQueueClosed_)
    {
        return true;
    }

    // Otherwise, process the operation
    if (!isLast || /* This is just a general operation*/
        !operation->IsEmpty() || /* This is for test support where last operation is non empty*/
        (isLast && enableEOSOpAndStreamFaults_) /* This is when last operation must be enqueued and acked due to null operation ack config enabled*/)
    {
        if (isLast && enableEOSOpAndStreamFaults_)
        {
            // A last copy operation received over the network should ALWAYS be of type end of stream ( Even if the message comes from a V1 primary)
            // This is because the copy operation header does the right conversion at the receiving end
            ASSERT_IFNOT(operation->IsEndOfStreamOperation, "{0}: Last Copy operation should be of type end of stream", endpointUniqueId_);
            ReplicatorEventSource::Events->SecondaryEOSOperation(partitionId_, endpointUniqueId_, Constants::CopyOperationTrace, Constants::EnqueueOperationTrace);
        }

        if (copyQueue_->TryEnqueue(move(operation)).IsSuccess())
        {
            if (!TryDispatchCopy(checkForceSendAck))
            {
                // There are no in-order items, there is no need to send ACK.
                return false;
            }

            progressDone = true;
            allCopyOperationsReceived_ = (copyQueue_->LastCommittedSequenceNumber == lastCopySequenceNumber_);
        }
    }
    else
    {
        // An empty operation is used to mark the end of copy.
        // The operation is not processed, but an ACK is sent back.
        ASSERT_IF(
            lastCopySequenceNumber_ < 1, 
            "{0}: Empty copy operation received, but last copy sequence number is not correctly set ({0})",
            endpointUniqueId_, 
            lastCopySequenceNumber_);
        // The last enqueued operation is the non-null one,
        // so modify last copy sequence number to look for that one
        lastCopySequenceNumber_ -= 1;
        allCopyOperationsReceived_ = (copyQueue_->LastCommittedSequenceNumber == lastCopySequenceNumber_);
    }

    if (allCopyOperationsReceived_)
    {
        // Since the last completed sequence number is the last copy sequenceNumber,
        // copy is done.
        ReplicatorEventSource::Events->SecondaryAllCopyReceived(
            partitionId_, 
            endpointUniqueId_, 
            lastCopySequenceNumber_);

        shouldCloseDispatchQueue = true;

        // Set progress done, so the caller can schedule Dispatch on a different thread
        progressDone = true;

        enqueuedEndOfStreamOperation_ = true;

        // When optimistic acks are sent, the copy queue can be immediately reset;
        // when ACKs are sent after service ACKed, we need to wait until all 
        // operations are ACKed. If the last operation was NULL,
        // this may be already accomplished
        ASSERT_IF(
            !requireServiceAck_ && copyQueue_->HasPendingOperations(), 
            "{0}: Can't have pending operations when all copy received", 
            endpointUniqueId_);
        if (!copyQueue_->HasPendingOperations())
        {
            copyQueue_.reset();
            checkCopySender = true;
            checkForceSendAck = true;
        }
    }

    return true;
}

void SecondaryCopyReceiver::DispatchCopyOperation(ComOperationCPtr const & operation)
{
    // Increase the ref count to keep the operation alive
    // until the user is done with it.
    // User must release the operation so it can be
    // destroyed when its ref count reaches 0.
    bool success = dispatchQueue_->EnqueueWithoutDispatch(make_unique<ComOperationCPtr>(operation));
    ASSERT_IFNOT(success, "{0}: EnqueueWithoutDispatch Copy should succeed", endpointUniqueId_);
    ReplicatorEventSource::Events->SecondaryEnqueueWithoutDispatch(
        partitionId_, 
        endpointUniqueId_, 
        Constants::CopyOperationTrace,
        operation->SequenceNumber);
}

bool SecondaryCopyReceiver::OnAckCopyOperation(
    ComOperation const & operation,
    __out bool & checkCopySender)
{
    checkCopySender = false;
    
    if (operation.IsEndOfStreamOperation)
    {
        OnAckEndOfStreamOperation(operation);
    }

    // Caller holds lock
    if (!copyQueue_)
    {
        ReplicatorEventSource::Events->SecondaryCopyOperationCompletedInOtherThread(
            partitionId_, 
            endpointUniqueId_, 
            operation.SequenceNumber);
        return false;
    }

    if (copyQueue_->Complete())
    {
        if ((allCopyOperationsReceived_ || copyQueueClosed_) && !copyQueue_->HasPendingOperations())
        {
            // The last copy operation was ACKed, so copy is done
            ReplicatorEventSource::Events->SecondaryAllCopyAcked(
                partitionId_,
                endpointUniqueId_,
                allCopyOperationsReceived_,
                copyQueueClosed_);
            
            if (allCopyOperationsReceived_)
            {
                copyQueue_.reset();
            }
            checkCopySender = true;            
        }

        return true;
    }
    
    return false;
}

void SecondaryCopyReceiver::MarkEOSOperationACK(bool ackIgnored)
{
    if (enqueuedEndOfStreamOperation_)
    {
        // If end of stream was enqueued already, we have ignored its ACK above. If we dont mark this, we will be stuck waiting for the ACK forever
        if (!ackedEndOfStreamOperation_)
        {
            if (ackIgnored)
            {
                ReplicatorEventSource::Events->SecondaryEOSOperation(
                    partitionId_, endpointUniqueId_, Constants::CopyOperationTrace, L"Ignore ACK");
            }
            else
            {
                ReplicatorEventSource::Events->SecondaryEOSOperation(
                    partitionId_, endpointUniqueId_, Constants::CopyOperationTrace, Constants::AckOperationTrace);
            }
            ackedEndOfStreamOperation_ = true;
        }
    }
}

void SecondaryCopyReceiver::OnAckEndOfStreamOperation(ComOperation const & operation)
{
    ASSERT_IFNOT(operation.IsEndOfStreamOperation, "{0}: Unknown end of stream Operation Type {1}", endpointUniqueId_, operation.Metadata.Type);
    ASSERT_IFNOT(enqueuedEndOfStreamOperation_, "{0}: Received end of stream operation ACK without enqueing it", endpointUniqueId_);
    
    MarkEOSOperationACK();
}

void SecondaryCopyReceiver::CreateCopyQueue()
{
    wstring queueLog;
    Common::StringWriter writer(queueLog);
    writer << L"Secondary-COPY-" << endpointUniqueId_;
    // For persisted services, operations are committed when 
    // are ready to be dispatched to the service,
    // and completed when service ACKs them.
    // For non-persisted services, commit and complete
    // happen when the operations are enqueued in the dispatch queue.
    bool ignoreCommit = !requireServiceAck_; 
    OperationQueueUPtr queue = make_unique<OperationQueue>(
        partitionId_, 
        queueLog, 
        config_->InitialCopyQueueSize, 
        config_->MaxCopyQueueSize, 
        0, // no max memory size for copy queue
        0,
        0,
        requireServiceAck_,
        true /*cleanOnComplete*/, 
        ignoreCommit, 
        1,
        nullptr);

    queue->SetCommitCallback(
        [this](ComOperationCPtr const & operation)->void 
        { 
            this->DispatchCopyOperation(operation); 
        });
    
    copyQueue_ = move(queue);
}

bool SecondaryCopyReceiver::TryDispatchCopy(__out bool & checkForceSendAck)
{
    checkForceSendAck = false;
    if (requireServiceAck_)
    {
        return copyQueue_->Commit();
    }
    else
    {
        if (copyQueue_->Complete())
        {
            checkForceSendAck = true;
            return true;
        }
     
        return false;
    }
}

FABRIC_SEQUENCE_NUMBER SecondaryCopyReceiver::GetAckProgress()
{
    FABRIC_SEQUENCE_NUMBER progress = 0;
    if (lastCopyCompletedAck_ != Constants::NonInitializedLSN)
    {
        if (!copyQueue_)
        {
            progress = lastCopySequenceNumber_ - lastCopyCompletedAck_ + 1;
        }
        else
        {
            progress = copyQueue_->LastCompletedSequenceNumber - lastCopyCompletedAck_;
        }
    }

    return progress;
}

void SecondaryCopyReceiver::OnStreamReportFault(Common::ComponentRoot const & root)
{
    replicationOrCopyStreamFaulted_ = true; 
    CleanQueueOnClose(root, false/*ignoreEOSOperationAck*/);
    CloseCopyOperationQueue();
}

ServiceModel::ReplicatorQueueStatusSPtr SecondaryCopyReceiver::GetQueueStatusForQuery() const
{
    if (copyQueue_)
    {
        return move(std::make_shared<ServiceModel::ReplicatorQueueStatus>(Replicator::GetReplicatorQueueStatusForQuery(*copyQueue_)));
    }
    else
    {
        return move(std::make_shared<ServiceModel::ReplicatorQueueStatus>());
    }
}

} // end namespace ReplicationComponent
} // end namespace Reliability
