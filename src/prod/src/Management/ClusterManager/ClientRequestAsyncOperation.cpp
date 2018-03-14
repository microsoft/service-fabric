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

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("ClientRequestAsyncOperation");

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
    __in ClusterManagerReplica & owner,
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : TimedAsyncOperation(timeout, callback, root)
    , ReplicaActivityTraceComponent(owner.PartitionedReplicaId, FabricActivityHeader::FromMessage(*request).ActivityId)
    , owner_(owner)
    , error_(ErrorCodeValue::Success)
    , request_(move(request))
    , requestContext_(move(requestContext))
    , requestInstance_(0)
    , retryTimer_()
    , timerLock_()
    , isLocalRetry_(false)
{
}

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
    __in ClusterManagerReplica & owner,
    ErrorCode const & error,
    MessageUPtr && request,
    RequestReceiverContextUPtr && requestContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
    : TimedAsyncOperation(timeout, callback, root)
    , ReplicaActivityTraceComponent(owner.PartitionedReplicaId, FabricActivityHeader::FromMessage(*request).ActivityId)
    , owner_(owner)
    , error_(error.ReadValue())
    , request_(move(request))
    , requestContext_(move(requestContext))
    , requestInstance_(0)
    , retryTimer_()
    , timerLock_()
    , isLocalRetry_(false)
{
}

ClientRequestAsyncOperation::~ClientRequestAsyncOperation()
{
}

void ClientRequestAsyncOperation::SetReply(Transport::MessageUPtr && reply)
{
    reply_ = std::move(reply);
}

void ClientRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);

    RequestInstanceHeader requestInstanceHeader;
    if (request_->Headers.TryReadFirst(requestInstanceHeader))
    {
        requestInstance_ = requestInstanceHeader.Instance;

        if (requestInstance_ < 0)
        {
            wstring rejectReason;
            StringWriter(rejectReason).Write("Negative Request Instance: {0}", requestInstance_);

            CMEvents::Trace->RequestValidationFailed2(
                this->ReplicaActivityId,
                this->Request.Action,
                this->Request.MessageId,
                rejectReason);

            Assert::TestAssert();

            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);

            return;
        }
    }

    this->HandleRequest(thisSPtr);
}

void ClientRequestAsyncOperation::HandleRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
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

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = this->EndAcceptRequest(operation);

    this->FinishRequest(thisSPtr, error);
}

void ClientRequestAsyncOperation::FinishRequest(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    if (IsRetryable(error))
    {
        TimeSpan retryDelay = owner_.GetRandomizedOperationRetryDelay(error);

        {
            AcquireExclusiveLock lock(timerLock_);

            if (!thisSPtr->IsCompleted)
            {
                CMEvents::Trace->RequestRetry(
                    this->ReplicaActivityId,
                    this->Request.MessageId,
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
    else if (!error.IsSuccess())
    {
        this->TryComplete(thisSPtr, error);
    }

    // on success, this replica has accepted the operation for processing
    // and will take care of completing the client request
}

ErrorCode ClientRequestAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & reply,
    __out RequestReceiverContextUPtr & requestContext)
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
            reply = ClusterManagerMessage::GetClientOperationSuccess();
        }
    }

    // request context is used to reply with errors to client, so always return it
    requestContext = move(casted->requestContext_);

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
    ClientRequestSPtr &&,
    TimeSpan const, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error_, callback, root);
}

ErrorCode ClientRequestAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return CompletedAsyncOperation::End(operation);
}

void ClientRequestAsyncOperation::CompleteRequest(Common::ErrorCode const & error)
{
    auto thisSPtr = this->shared_from_this();

    this->TryComplete(thisSPtr, ToGatewayError(error));
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
        case ErrorCodeValue::CMRequestAlreadyProcessing:

        //case ErrorCodeValue::StoreRecordAlreadyExists: // same as StoreWriteConflict
        case ErrorCodeValue::StoreOperationCanceled:
        case ErrorCodeValue::StoreWriteConflict:
        case ErrorCodeValue::StoreSequenceNumberCheckFailed:

        case ErrorCodeValue::CMImageBuilderRetryableError:

        // CM is overloaded
        case ErrorCodeValue::JobQueueFull:
        // Naming is overloaded, its job queue is full etc
        case ErrorCodeValue::NamingServiceTooBusy:
            return true;

        default:
            return false;
    }
}

ErrorCode ClientRequestAsyncOperation::ToGatewayError(Common::ErrorCode const & error)
{
    switch (error.ReadValue())
    {
        // The client must retry in order to pick up the
        // new package version and go through validations
        // again (e.g. a new upgrade may have started,
        // the service type may be removed, etc.)
        //
        case ErrorCodeValue::StaleServicePackageInstance:
            return ErrorCodeValue::OperationCanceled;

        default:
            return error;
    }
}
