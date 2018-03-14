// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Reliability;
using namespace Store;
using namespace std;
using namespace ClientServerTransport;

using namespace Management::RepairManager;

StringLiteral const TraceComponent("ClientRequestAsyncOperation");

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    Common::ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : TimedAsyncOperation(timeout, callback, root)
    , ReplicaActivityTraceComponent(owner.PartitionedReplicaId, activityId)
    , owner_(owner)
    , error_(ErrorCodeValue::Success)
    , request_()
    , receiverContext_()
    , retryTimer_()
    , timerLock_()
    , isLocalRetry_(false)
{
}

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : TimedAsyncOperation(timeout, callback, root)
    , ReplicaActivityTraceComponent(owner.PartitionedReplicaId, FabricActivityHeader::FromMessage(*request).ActivityId)
    , owner_(owner)
    , error_(ErrorCodeValue::Success)
    , request_(move(request))
    , receiverContext_(move(receiverContext))
    , retryTimer_()
    , timerLock_()
    , isLocalRetry_(false)
{
}

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
    __in RepairManagerServiceReplica & owner,
    ErrorCode const & error,
    MessageUPtr && request,
    IpcReceiverContextUPtr && receiverContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : TimedAsyncOperation(timeout, callback, root)
    , ReplicaActivityTraceComponent(owner.PartitionedReplicaId, FabricActivityHeader::FromMessage(*request).ActivityId)
    , owner_(owner)
    , error_(error.ReadValue())
    , request_(move(request))
    , receiverContext_(move(receiverContext))
    , retryTimer_()
    , timerLock_()
    , isLocalRetry_(false)
{
}

ClientRequestAsyncOperation::~ClientRequestAsyncOperation()
{
}

MessageId ClientRequestAsyncOperation::get_RequestMessageId() const
{
    if (this->HasRequest)
    {
        return request_->MessageId;
    }

    return MessageId();
}

void ClientRequestAsyncOperation::SetReply(Transport::MessageUPtr && reply)
{
    reply_ = std::move(reply);
}

void ClientRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);

    this->HandleRequest(thisSPtr);
}

void ClientRequestAsyncOperation::HandleRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto error = owner_.BeginOperation(this->ReplicaActivityId);
    if (!error.IsSuccess())
    {
        // TODO IsRetryable?
        this->TryComplete(thisSPtr, error);
        return;
    }

    auto operation = this->BeginAcceptRequest(
        dynamic_pointer_cast<ClientRequestAsyncOperation>(shared_from_this()),
        this->RemainingTime,
        [this](AsyncOperationSPtr const & operation) { this->OnAcceptRequestComplete(operation, false); },
        thisSPtr);
    this->OnAcceptRequestComplete(operation, true);
}

void ClientRequestAsyncOperation::OnAcceptRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    owner_.EndOperation(this->ReplicaActivityId);

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = this->EndAcceptRequest(operation);

    if (IsRetryable(error))
    {
        TimeSpan retryDelay = owner_.GetRandomizedOperationRetryDelay(error);

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!thisSPtr->IsCompleted)
            {
                RMEvents::Trace->RequestRetry(
                    this->ReplicaActivityId,
                    this->RequestMessageId,
                    retryDelay,
                    error);

                retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
                { 
                    timer->Cancel();
                    isLocalRetry_ = true;
                    this->HandleRequest(thisSPtr);
                },
                true); // allow concurrency
                retryTimer_->Change(retryDelay);
            }
        }
    }
    else
    {
        // Distinguish non-retryable sequence number check failures (caused by the expected version
        // passed in by the client) from retryable ones (returned by the replicated store).
        // StaleRequest is used to indicate the non-retryable type, but this should get remapped to
        // FABRIC_E_SEQUENCE_NUMBER_CHECK_FAILED (StoreSequenceNumberCheckFailed) just before replying
        // to the client.
        if (error.IsError(ErrorCodeValue::StaleRequest))
        {
            error.Overwrite(ErrorCodeValue::StoreSequenceNumberCheckFailed);
        }

        this->TryComplete(thisSPtr, error);
    }
}

ErrorCode ClientRequestAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & reply,
    __out IpcReceiverContextUPtr & receiverContext)
{
    auto casted = AsyncOperation::End<ClientRequestAsyncOperation>(asyncOperation);

    if (casted->Error.IsSuccess())
    {
        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }
        else
        {
            // No reply was set by the derived class, so create a default reply message.
            reply = RepairManagerTcpMessage::GetClientOperationSuccess()->GetTcpMessage();
        }
    }

    // receiver context is used to reply with errors to client, so always return it
    receiverContext = move(casted->receiverContext_);

    return casted->Error;
}

void ClientRequestAsyncOperation::OnTimeout(AsyncOperationSPtr const & thisSPtr)
{
    UNREFERENCED_PARAMETER(thisSPtr);

    this->CancelRetryTimer();
}

void ClientRequestAsyncOperation::OnCancel()
{
    this->CancelRetryTimer();
}

void ClientRequestAsyncOperation::OnCompleted()
{
    TimedAsyncOperation::OnCompleted();

    this->CancelRetryTimer();
}

void ClientRequestAsyncOperation::CancelRetryTimer()
{
    TimerSPtr snap;
    {
        AcquireExclusiveLock lock(timerLock_); 
        snap = retryTimer_;
    }

    if (snap)
    {
        snap->Cancel();
    }
}

AsyncOperationSPtr ClientRequestAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    UNREFERENCED_PARAMETER(clientRequest);
    UNREFERENCED_PARAMETER(timeout);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error_, callback, root);
}

ErrorCode ClientRequestAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

bool ClientRequestAsyncOperation::IsLocalRetry()
{
    return isLocalRetry_;
}

bool ClientRequestAsyncOperation::IsRetryable(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        case ErrorCodeValue::NoWriteQuorum:
        case ErrorCodeValue::ReconfigurationPending:
        //case ErrorCodeValue::StoreRecordAlreadyExists: // same as StoreWriteConflict
        case ErrorCodeValue::StoreOperationCanceled:
        case ErrorCodeValue::StoreWriteConflict:
        case ErrorCodeValue::StoreSequenceNumberCheckFailed:
            return true;

        default:
            return false;
    }
}
