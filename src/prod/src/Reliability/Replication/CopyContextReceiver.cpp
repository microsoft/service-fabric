// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireExclusiveLock;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ComponentRoot;
using Common::ErrorCode;
using Common::make_com;
using Common::make_unique;
using Common::ReaderQueue;
using Common::Threadpool;

using std::move;
using std::wstring;

CopyContextReceiver::CopyContextReceiver(
    REInternalSettingsSPtr const & config,
    Common::Guid const & partitionId,
    std::wstring const & purpose,
    ReplicationEndpointId const & from, 
    FABRIC_REPLICA_ID to)
    :   Common::ComponentRoot(),
        config_(config),
        partitionId_(partitionId),
        endpointUniqueId_(from),
        replicaId_(to),
        purpose_(purpose),
        queue_(
            partitionId_, 
            GetDescription(purpose, from, to),
            config->InitialCopyQueueSize, 
            config->MaxCopyQueueSize, 
            0, // no max memory size for copy context queue
            0,
            0,
            /*hasPersistedState*/ false, 
            /*cleanOnComplete*/ true, 
            /*ignoreCommit*/ true, 
            1,
            nullptr),
        lastCopyContextSequenceNumber_(Constants::NonInitializedLSN),
        allOperationsReceived_(false),
        dispatchQueue_(ReaderQueue<ComOperationCPtr>::Create()),
        ackSender_(config_, partitionId_, endpointUniqueId_),
        lastAckSent_(Constants::InvalidLSN),
        copyContextEnumerator_(),
        errorCodeValue_(0),
        isActive_(true),
        lock_()
{
    queue_.SetCommitCallback(
        [this] (ComOperationCPtr const & operation) { this->DispatchOperation(operation); });
}

CopyContextReceiver::~CopyContextReceiver()
{
}

wstring CopyContextReceiver::GetDescription(
    std::wstring const & purpose,
    ReplicationEndpointId const & from,
    FABRIC_REPLICA_ID to)
{
    wstring desc;
    Common::StringWriter(desc).Write("{0}->{1}:{2}", from, to, purpose);
    return desc;
}

void CopyContextReceiver::GetStateAndSetLastAck(
    __out FABRIC_SEQUENCE_NUMBER & lastCompletedSequenceNumber,
    __out int & errorCodeValue)
{
    AcquireExclusiveLock lock(lock_);
    errorCodeValue = errorCodeValue_;
    if (lastCopyContextSequenceNumber_ != Constants::NonInitializedLSN && 
        (lastCopyContextSequenceNumber_ - 1) == queue_.LastCompletedSequenceNumber)
    {
        // last operation (empty) has been seen and queue completed to lastSeenLSN-1
        lastCompletedSequenceNumber = lastCopyContextSequenceNumber_;
    }
    else
    {
        lastCompletedSequenceNumber = queue_.LastCompletedSequenceNumber;
    }
    lastAckSent_ = lastCompletedSequenceNumber;
}

void CopyContextReceiver::Open(
    SendCallback const & copySendCallback)
{
    ackSender_.Open(*this, copySendCallback);
}

void CopyContextReceiver::Close(ErrorCode const error)
{
    bool errorsEncountered = !error.IsSuccess();
    // First, set up the state and close the copy context enumerator;
    // then close the dispatch queue (which will finish any pending GetNext 
    // on the enumerator) and, if needed, schedule one last ACK
    // to let the other part know that we completed with error.
    {
        AcquireExclusiveLock lock(lock_);
        isActive_ = false;
        errorCodeValue_ = error.ReadValue();
        // Close the enumerator, which will break the pointer cycle
        copyContextEnumerator_->Close(errorsEncountered);
    }

    dispatchQueue_->Close();
    if (errorsEncountered)
    {
        // Send one last message to let the other part know that an error occurred
        ackSender_.ScheduleOrSendAck(true);
    }

    ackSender_.Close();
}

void CopyContextReceiver::CreateEnumerator(
    __out ComPointer<IFabricOperationDataStream> & context)
{
    ComPointer<ComOperationDataAsyncEnumerator> localEnum = 
        make_com<ComOperationDataAsyncEnumerator>(
            *this,
            dispatchQueue_);
    context = ComPointer<IFabricOperationDataStream>(localEnum, IID_IFabricOperationDataStream);

    {
        // Keep a copy of the enumerator, 
        // to be able to report any outside errors
        AcquireExclusiveLock lock(lock_);
        copyContextEnumerator_ = move(localEnum);
    }
}

bool CopyContextReceiver::ProcessCopyContext(
    ComOperationCPtr && operation,
    bool isLast)
{
    bool shouldDispatch = false;
    bool forceSend = false;
    bool shouldCloseDispatchQueue = false;

    {
        AcquireExclusiveLock lock(lock_);
        // Process the copy context operation, unless
        // All operations have already been received
        // or the copy context receiver already encountered an error.
        // Otherwise, no processing is needed, but the ACK must be sent
        if (isActive_ && !allOperationsReceived_ && errorCodeValue_ == 0)
        {
            if (isLast)
            {
                lastCopyContextSequenceNumber_ = operation->SequenceNumber;
            }
            else
            {
                auto localOperation = move(operation);
                if (queue_.TryEnqueue(localOperation).IsSuccess())
                {
                    if (!queue_.Complete())
                    {
                        // There are no in-order items, there is no need to send ACK.
                        return false;
                    }
                }
                shouldDispatch = true;
            }
          
            if (queue_.LastCompletedSequenceNumber == (lastCopyContextSequenceNumber_ - 1))
            {
                // Since the last completed sequence number is the last copy sequenceNumber,
                // copy context is done.
                ReplicatorEventSource::Events->PrimaryAllCCReceived(
                    partitionId_, 
                    endpointUniqueId_,
                    replicaId_,
                    purpose_,
                    lastCopyContextSequenceNumber_);
            
                allOperationsReceived_ = true;
                shouldCloseDispatchQueue = true;
                forceSend = true;
            }
            else if (queue_.LastCompletedSequenceNumber > lastAckSent_ + config_->MaxPendingAcknowledgements)
            {
                forceSend = true;
            }
        }
    }

    if (shouldCloseDispatchQueue)
    {
        dispatchQueue_->Close();
    }

    if (shouldDispatch)
    {
        // Dispatch all items in the dispatch queue
        // outside the lock and on a different thread.
        auto root = this->CreateComponentRoot();
        Threadpool::Post([this, root]() 
            { 
                this->dispatchQueue_->Dispatch();
            });
    }

    // If we got to this point, we need to send ACK back to the secondary
    ackSender_.ScheduleOrSendAck(forceSend);
    return true;
}

void CopyContextReceiver::DispatchOperation(ComOperationCPtr const & operation)
{
    bool success = dispatchQueue_->EnqueueWithoutDispatch(make_unique<ComOperationCPtr>(operation));
    ASSERT_IFNOT(success, "{0}->{1}:{2}: EnqueueWithoutDispatch CopyCONTEXT should succeed", endpointUniqueId_, replicaId_, purpose_);
    ReplicatorEventSource::Events->PrimaryCCEnqueueWithoutDispatch(
        partitionId_, 
        endpointUniqueId_,
        replicaId_,
        purpose_,
        operation->SequenceNumber);
}

} // end namespace ReplicationComponent
} // end namespace Reliability
