// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireWriteLock;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ComPointer;
using Common::ErrorCode;
using Common::Event;
using Common::EventArgs;
using Common::EventHandler;
using Common::HHandler;
using Common::StringWriter;
using Common::Threadpool;
using Common::AcquireReadLock;
using std::move;
using std::wstring;

CopyAsyncOperation::CopyAsyncOperation(
    REInternalSettingsSPtr const & config, 
    Common::Guid const & partitionId,
    std::wstring const & purpose,
    std::wstring const & description,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_REPLICA_ID const & replicaId,
    ComProxyAsyncEnumOperationData && asyncEnumOperationData,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    Common::ApiMonitoring::ApiName::Enum apiName, 
    SendOperationCallback const & copySendCallback,
    EnumeratorLastOpCallback const & enumeratorLastOpCallback,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        config_(config),
        partitionId_(partitionId),
        queue_(partitionId, description, config->InitialCopyQueueSize, config->MaxCopyQueueSize, 0, 0, 0, false /*persistedState*/, true /* cleanOnComplete*/, true /*ignoreCommit*/, 1, nullptr),
        lock_(),
        asyncEnumOperationData_(move(asyncEnumOperationData)),
        copySendCallback_(copySendCallback),
        copyOperations_(config, config->InitialCopyQueueSize, config->MaxCopyQueueSize, partitionId, purpose, endpointUniqueId, replicaId, 0 /*lastAcked*/),
        queuePreviouslyFull_(false),
        pendingOperation_(),
        pendingOperationIsLast_(false),
        getNextCallsDone_(false),
        enumeratorLastOpCallback_(enumeratorLastOpCallback),
        readyToComplete_(false),
        faultErrorCode_(),
        faultDescription_(),
        apiMonitor_(apiMonitor),
        apiNameDesc_(Common::ApiMonitoring::ApiNameDescription(Common::ApiMonitoring::InterfaceName::IStateProvider, apiName, HealthReportType::GetHealthPropertyName(apiName, replicaId)))
{
    ASSERT_IFNOT(
        apiName == Common::ApiMonitoring::ApiName::GetNextCopyContext || apiName == Common::ApiMonitoring::ApiName::GetNextCopyState,
        "{0}:{1}->{2} CopyAsync Operation invalid apiName = {3}", partitionId_, endpointUniqueId, replicaId, apiName);
}

CopyAsyncOperation::~CopyAsyncOperation()
{
}

void CopyAsyncOperation::OpenReliableOperationSender()
{
    copyOperations_.Open<AsyncOperationSPtr>(shared_from_this(), copySendCallback_);
}

void CopyAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    // Get copy operations enumerator from the service, 
    // providing the inclusive upto replication sequence number.
    Threadpool::Post(
        [thisSPtr, this](void) -> void
    {
        GetNext(thisSPtr);
    });
}

void CopyAsyncOperation::OnCancel()
{
    bool shouldComplete = false;

    {
        AcquireReadLock grab(lock_);

        // Complete with error if queue was full when cancellation requested
        // This is to ensure no in flight GetNextAsync operation attempts to request data while the log is already closed
        if (queuePreviouslyFull_ || getNextCallsDone_)
        {
            shouldComplete = true;
        }
    }

    if (shouldComplete)
    {
        TryComplete(shared_from_this(), Common::ErrorCodeValue::OperationCanceled);
    }
}

void CopyAsyncOperation::GetNext(AsyncOperationSPtr const & thisSPtr)
{
    for (;;)
    {
        if (this->IsCompleted || this->IsCancelRequested)
        {
            ReplicatorEventSource::Events->CopySenderStopGetNext(
                partitionId_,
                copyOperations_.EndpointUniqueId,
                copyOperations_.ReplicaId,
                copyOperations_.Purpose);

            // cancel requested case
            if (!this->IsCompleted)
            {
                TryComplete(thisSPtr, Common::ErrorCodeValue::OperationCanceled);
            }

            return;
        }

        auto callDesc = ApiMonitoringWrapper::GetApiCallDescriptionFromName(copyOperations_.EndpointUniqueId, apiNameDesc_, config_->SlowApiMonitoringInterval);

        apiMonitor_->StartMonitoring(callDesc);

        auto inner = asyncEnumOperationData_.BeginGetNext(
            [this, callDesc](AsyncOperationSPtr const & asyncOperation) -> void
            {
                this->GetNextCallback(asyncOperation, callDesc);
            }, 
            thisSPtr);

        if (inner->CompletedSynchronously)
        {
            if (!this->FinishGetNext(inner, callDesc))
            {
                // Received last operation from the service,
                // no need to continue
                break;
            }
        }
        else
        {
            break;
        }
    }
}

void CopyAsyncOperation::GetNextCallback(AsyncOperationSPtr const & asyncOperation, Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        if (this->FinishGetNext(asyncOperation, callDesc))
        {
            GetNext(asyncOperation->Parent);
        }
    }
}

bool CopyAsyncOperation::FinishGetNext(AsyncOperationSPtr const & asyncOperation, Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc)
{
    bool isLast = false;
    ComPointer<IFabricOperationData> opPointer;
    ErrorCode error = this->asyncEnumOperationData_.EndGetNext(
        asyncOperation, 
        isLast, 
        opPointer);

    apiMonitor_->StopMonitoring(callDesc, error);

    if (!error.IsSuccess())
    {
        if (!error.IsError(Common::ErrorCodeValue::OperationCanceled))
        {
            faultErrorCode_ = error;
            faultErrorCode_.ReadValue();
            faultDescription_ = L"EndGetNext";
        }
        TryComplete(asyncOperation->Parent, error);
        return false;
    }

    if (this->IsCompleted || this->IsCancelRequested)
    {
        ReplicatorEventSource::Events->CopySenderStopGetNext(
            partitionId_,
            copyOperations_.EndpointUniqueId,
            copyOperations_.ReplicaId,
            copyOperations_.Purpose);
        
        // cancel requested case
        if (!this->IsCompleted)
        {
            TryComplete(asyncOperation->Parent, Common::ErrorCodeValue::OperationCanceled);
        }

        return false;
    }

    if (!this->TryEnqueue(asyncOperation, move(opPointer), isLast))
    {
        return false;
    }

    return !isLast;
}

bool CopyAsyncOperation::TryEnqueue(
    AsyncOperationSPtr const & asyncOperation,
    ComPointer<IFabricOperationData> && opDataPointer, 
    bool isLast)
{
    ErrorCode error;
    ComOperationCPtr opPointer;
    FABRIC_SEQUENCE_NUMBER seq;

    {
        AcquireWriteLock lock(lock_);
        seq = queue_.LastSequenceNumber + 1;
        
        FABRIC_OPERATION_METADATA metadata;
        metadata.Type =  isLast ? FABRIC_OPERATION_TYPE_END_OF_STREAM : FABRIC_OPERATION_TYPE_NORMAL;
        metadata.SequenceNumber = seq;
        metadata.AtomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
        metadata.Reserved = NULL;

        opPointer = Common::make_com<ComUserDataOperation, ComOperation>(
            move(opDataPointer), 
            metadata);

        error = queue_.TryEnqueue(move(opPointer));

        if (error.IsError(Common::ErrorCodeValue::REQueueFull))
        {
            queuePreviouslyFull_ = true;
            // Remember the current operation and isLast value
            // to retry after some operations completed
            pendingOperation_ = move(opPointer);
            pendingOperationIsLast_ = isLast;
        }

        getNextCallsDone_ = isLast;
    }

    if (isLast)
    {
        enumeratorLastOpCallback_(seq);
    }

    if (error.IsSuccess())
    {
        ComOperationRawPtr op = opPointer.GetRawPointer();
        copyOperations_.Add(op, Constants::InvalidLSN);
        return true;
    }
    else if (error.ReadValue() == Common::ErrorCodeValue::REQueueFull)
    {
        return false;
    }
    else
    {
        TryComplete(asyncOperation->Parent, error);
        return false;
    }
}

void CopyAsyncOperation::UpdateProgress(
    FABRIC_SEQUENCE_NUMBER quorumSequenceNumber, 
    FABRIC_SEQUENCE_NUMBER receivedSequenceNumber, 
    AsyncOperationSPtr const & thisSPtr, 
    bool isLast) 
{
    // First, remove the ACKed operations 
    // so they are not sent again if the retry timer fires
    if (receivedSequenceNumber != Constants::InvalidLSN)
    {
        // If Acks are sent on service ACKs,
        // it's possible that the secondary received the operation
        // but and it's still waiting for service ACK.
        // In this case, there's no need to resend it,
        // although we still need to wait for ACK in order to complete copy.
        if (isLast)
        {
            copyOperations_.Close();
        }
        else
        {
            copyOperations_.ProcessOnAck(receivedSequenceNumber, quorumSequenceNumber);
        }
    }

    bool retryEnqueue = false;
    bool pendingOperationIsLastLocal = false;
    ErrorCode error;
    ComOperationRawPtr op = nullptr;

    {
        AcquireWriteLock lock(lock_);
        if (queue_.Complete(quorumSequenceNumber))
        {
            if (queuePreviouslyFull_)
            {
                retryEnqueue = true;
                
                error = queue_.TryEnqueue(pendingOperation_);

                queuePreviouslyFull_ =
                    error.IsError(Common::ErrorCodeValue::REQueueFull) ?
                    true :
                    false;
            }
        }

        pendingOperationIsLastLocal = pendingOperationIsLast_;
    }

    if (retryEnqueue)
    {
        if (error.IsSuccess())
        {
            op = pendingOperation_.GetRawPointer();
            pendingOperation_.Release(); // Releasing this is fine because the operation is being kept alive inside the operation queue

            copyOperations_.Add(op, Constants::InvalidLSN);
            if (!pendingOperationIsLastLocal)
            {
                // Get the next operation from state provider
                // NOTE - It is very important to post this on a threadpool since this is the ack processing thread and blocking it can lead to long durations of no progress,
                // which leads to the primary reducing its SWS as no ack's are seen

                // It affects copy performance significantly
                Threadpool::Post(
                    [thisSPtr, this](void) -> void
                {
                    GetNext(thisSPtr);
                });
            }
        }
        else if (error.ReadValue() == Common::ErrorCodeValue::REQueueFull)
        {
            // We still don't have enough room, so wait for more ack's
        }
        else
        {
            TryComplete(thisSPtr, error);
            return;
        }
    }
    else
    {
        // Ignore error
        error.ReadValue();
    }

    if (isLast)
    {
        readyToComplete_ = true;
    }
}

ErrorCode CopyAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    ErrorCode & faultErrorCode,
    wstring & faultDescription)
{
    auto casted = AsyncOperation::End<CopyAsyncOperation>(asyncOperation);
    // If successful, the copy operations have been closed
    // but close is idempotent, so safe to call anyway
    casted->copyOperations_.Close();
    faultErrorCode = casted->faultErrorCode_;
    faultDescription = casted->faultDescription_;
    return casted->Error;
}

bool CopyAsyncOperation::TryCompleteWithSuccessIfCopyDone(
    AsyncOperationSPtr const & thisSPtr)
{
    if (readyToComplete_)
    {
        return TryComplete(thisSPtr, Common::ErrorCodeValue::Success);
    }

    return false;
}

wstring CopyAsyncOperation::GetOperationSenderProgress() const
{
    wstring progress;
    StringWriter(progress).Write(copyOperations_);
    return progress;
}

void CopyAsyncOperation::GetCopyProgress(
    __out FABRIC_SEQUENCE_NUMBER & copyReceivedLSN,
    __out FABRIC_SEQUENCE_NUMBER & copyAppliedLSN,
    __out Common::TimeSpan & avgCopyReceiveAckDuration,
    __out Common::TimeSpan & avgCopyApplyAckDuration,
    __out FABRIC_SEQUENCE_NUMBER & notReceivedCopyCount,
    __out FABRIC_SEQUENCE_NUMBER & receivedAndNotAppliedCopyCount,
    __out Common::DateTime & lastAckProcessedTime)
{
    copyOperations_.GetProgress(copyReceivedLSN, copyAppliedLSN, lastAckProcessedTime);
    avgCopyReceiveAckDuration = copyOperations_.AverageReceiveAckDuration;
    avgCopyApplyAckDuration = copyOperations_.AverageApplyAckDuration;
    notReceivedCopyCount = copyOperations_.NotReceivedCount;
    receivedAndNotAppliedCopyCount = copyOperations_.ReceivedAndNotAppliedCount;
}
            

} // end namespace ReplicationComponent
} // end namespace Reliability
