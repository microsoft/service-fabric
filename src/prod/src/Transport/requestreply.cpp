// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Transport;

static const StringLiteral TraceType("RequestReply");

class RequestReply::RequestReplyAsyncOperation : public AsyncOperation
{
    DENY_COPY(RequestReplyAsyncOperation);

public:
    RequestReplyAsyncOperation(
        RequestReply & requestReply,
        MessageUPtr && request,
        ISendTarget::SPtr const & to,
        TransportPriority::Enum priority,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        requestReply_(requestReply),
        request_(move(request)),
        target_(move(to)),
        priority_(priority),
        timeout_(timeout),
        requestSendCompleted_(false)
    {
    }

    static ErrorCode End(Common::AsyncOperationSPtr const & operationSPtr, Transport::MessageUPtr & reply)
    {
        RequestReplyAsyncOperation* thisPtr = AsyncOperation::End<RequestReplyAsyncOperation>(operationSPtr);
        if (thisPtr->requestReply_.enableDifferentTimeoutError_)
        {
            if (thisPtr->Error.IsError(ErrorCodeValue::Timeout) && (!thisPtr->requestSendCompleted_))
            {
                return ErrorCodeValue::SendFailed;
            }
        }
        reply = std::move(thisPtr->reply_);
        return thisPtr->Error;
    }

    ISendTarget const* SendTarget() const
    {
        return target_.get();
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!target_)
        {
            TryComplete(thisSPtr, ErrorCodeValue::NotFound);
            return;
        }

        request_->Headers.Add(ExpectsReplyHeader(true));
        if (request_->MessageId.IsEmpty())
        {
            request_->Headers.Add(MessageIdHeader(MessageId()));
        }

        MessageId const & id = request_->MessageId;
        RequestAsyncOperationSPtr operation = std::make_shared<RequestAsyncOperation>(
            this->requestReply_.requestTable_,
            id,
            timeout_,
            [this](AsyncOperationSPtr const & asyncOperation) { OnRequestCompleted(asyncOperation); },
            thisSPtr);
        operation->Start(operation);

        auto error = this->requestReply_.requestTable_.TryInsertEntry(id, operation);
        if (error.IsSuccess())
        {
            if (operation->IsCompleted)
            {
                // If there was a racing with RequestAsyncOperation completion and we added RequestAsyncOperation after its
                // completion logic ran (timing out /table closing), we must remove RequestAsyncOperation here since its
                // completion logic was supposed to remove RequestAsyncOperation from the request table.
                this->requestReply_.requestTable_.TryRemoveEntry(id, operation);
            }
            else
            {
                request_->SetSendStatusCallback([thisSPtr, operation](ErrorCode const & sendFailure, MessageUPtr &&)
                {
                    RequestAsyncOperationSPtr removed;
                    RequestReplyAsyncOperation * thisPtr = (RequestReplyAsyncOperation*)(thisSPtr.get());
                    if (!sendFailure.IsSuccess())
                    {
                        // Need to remove explicitly instead of relying on RequestAsyncOperationSPtr::OnCancel,
                        // because OnRequestFailure completes RequestReplyAsyncOperation before RequestAsyncOperationSPtr
                        thisPtr->requestReply_.requestTable_.TryRemoveEntry(operation->MessageId, removed);
                        thisPtr->OnRequestFailure(thisSPtr, sendFailure, operation);
                    }
                    else
                    {
                        thisPtr->OnRequestSendStatusCompleted(operation);
                    }

                });

                requestReply_.datagramTransport_->SendOneWay(target_, move(request_), timeout_, priority_);
            }

            if (operation->CompletedSynchronously)
            {
                // no need to check dispatchOnTransportThread_ here as this cannot be the reply dispatching thread
                FinishRequest(operation);
            }
        }
        else
        {
            OnRequestFailure(thisSPtr, error, operation);
        }
    }

private:

    void OnRequestSendStatusCompleted(RequestAsyncOperationSPtr requestOp)
    {
        this->requestSendCompleted_ = true;
    }

    void OnRequestFailure(AsyncOperationSPtr const & thisSPtr, ErrorCode error, RequestAsyncOperationSPtr requestOp)
    {
        TryComplete(thisSPtr, error);

        // Cancel after TryComplete, otherwise we will complete with Canceled error code.
        requestOp->Cancel();
    }

    void OnRequestCompleted(AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            if (this->requestReply_.dispatchOnTransportThread_)
            {
                this->FinishRequest(operation);
            }
            else
            {
                Threadpool::Post([this, operation] { this->FinishRequest(operation); });
            }
        }
    }

    void FinishRequest(AsyncOperationSPtr const & operation)
    {
        TryComplete(operation->Parent, RequestAsyncOperation::End(operation, reply_));
    }

  //Caller of this Async Operation needs to make sure requestReply  is alive till this  AsyncOperation is destructed.
    RequestReply & requestReply_;
    MessageUPtr request_;
    ISendTarget::SPtr target_;
    TransportPriority::Enum priority_;
    TimeSpan const timeout_;
    MessageUPtr reply_;
    bool requestSendCompleted_;
};

RequestReply::RequestReply(
    ComponentRoot const & root,
    IDatagramTransportSPtr const & datagramTransport,
    bool dispatchOnTransportThread)
    : RootedObject(root)
    , datagramTransport_(datagramTransport)
    , dispatchOnTransportThread_(dispatchOnTransportThread)
    , enableDifferentTimeoutError_(false)
{
    WriteInfo(TraceType, "{0}: created with transport {1}", TextTraceThis, TextTracePtr(datagramTransport_.get()));
}

void RequestReply::Open()
{
    disconnectHHandler_ = datagramTransport_->RegisterDisconnectEvent(
        [this, root = Root.CreateComponentRoot()](IDatagramTransport::DisconnectEventArgs const & eventArgs) { OnDisconnected(eventArgs); });
}
void RequestReply::Close()
{
    datagramTransport_->UnregisterDisconnectEvent(disconnectHHandler_);
    this->requestTable_.Close();
}

void RequestReply::OnDisconnected(IDatagramTransport::DisconnectEventArgs const & eventArgs)
{
    if (eventArgs.Fault.IsError(SESSION_EXPIRATION_FAULT))
    {
        WriteInfo(
            TraceType,
            "{0}: OnDisconnected: targetTraceId={1}, targetAddress='{2}', ignore fault {3} as connection will be kept alive for pending reply",
            TextTraceThis,
            eventArgs.Target->TraceId(),
            eventArgs.Target->Address(),
            eventArgs.Fault);

        return;
    }
    
    WriteInfo(
        TraceType,
        "{0}: OnDisconnected: targetTraceId={1}, targetAddress='{2}', fault={3}",
        TextTraceThis,
        eventArgs.Target->TraceId(),
        eventArgs.Target->Address(),
        eventArgs.Fault);

    auto matchTargetValue = eventArgs.Target;
    auto removed = requestTable_.RemoveIf([matchTargetValue](pair<MessageId, RequestAsyncOperationSPtr> const & entry)
    {
        auto tableEntryValue = ((RequestReplyAsyncOperation*)entry.second->Parent.get())->SendTarget();
        return tableEntryValue == matchTargetValue;
    });

    for (auto & entry : removed)
    {
        entry.second->TryComplete(entry.second, eventArgs.Fault);
    }
}

Common::AsyncOperationSPtr RequestReply::BeginRequest(
    MessageUPtr && request,
    ISendTarget::SPtr const & to,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return this->BeginRequest(
        move(request),
        to,
        TransportPriority::Normal,
        timeout,
        callback,
        parent);
}

Common::AsyncOperationSPtr RequestReply::BeginRequest(
    MessageUPtr && request,
    ISendTarget::SPtr const & to,
    TransportPriority::Enum priority,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(*this, move(request), to, priority, timeout, callback, parent);
}

Common::ErrorCode RequestReply::EndRequest(
    Common::AsyncOperationSPtr const & operation,
    MessageUPtr & reply)
{
    return RequestReplyAsyncOperation::End(operation, reply);
}

Common::AsyncOperationSPtr RequestReply::BeginNotification(
    MessageUPtr && request,
    ISendTarget::SPtr const & to,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    request->Headers.Add(UncorrelatedReplyHeader());
    return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(*this, move(request), to, TransportPriority::Normal, timeout, callback, parent);
}

Common::ErrorCode RequestReply::EndNotification(
    Common::AsyncOperationSPtr const & operation,
    MessageUPtr & reply)
{
    return RequestReplyAsyncOperation::End(operation, reply);
}

bool RequestReply::OnReplyMessage(Transport::Message & reply, ISendTarget::SPtr const & sendTarget)
{
    UNREFERENCED_PARAMETER(sendTarget);

    return this->requestTable_.OnReplyMessage(reply);
}

void RequestReply::EnableDifferentTimeoutError()
{
    this->enableDifferentTimeoutError_ = true;
}

AsyncOperationSPtr RequestReply::CreateRequestAsyncOperation(
    MessageUPtr && request,
    ISendTarget::SPtr const & to,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return make_shared<RequestReplyAsyncOperation>(
        *this, std::move(request), to, TransportPriority::Normal, timeout, callback, parent);
}
