// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Communication::TcpServiceCommunication;
using namespace std;
using namespace Transport;
using namespace Api;

static const StringLiteral TraceType("ServiceMethodCallDispatcher");

class ServiceMethodCallDispatcher::DispatchMessageAsyncOperation
    : public TimedAsyncOperation
{
    DENY_COPY(DispatchMessageAsyncOperation);

public:
    DispatchMessageAsyncOperation(
        __in IServiceCommunicationMessageHandler & service,
        MessageUPtr && message,
        ReceiverContextUPtr && receiverContext,
        TimeSpan const & timeout,
        wstring traceId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , service_(service)
        , request_(move(message))
        , receiverContext_(move(receiverContext))
        , traceId_(traceId)
    {
    }

    ~DispatchMessageAsyncOperation() {}

    __declspec(property(get = get_Service)) IServiceCommunicationMessageHandler & Service;
    IServiceCommunicationMessageHandler & get_Service() { return service_; }

    __declspec(property(get = get_Request)) Message & Request;
    Message & get_Request() { return *request_; }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<DispatchMessageAsyncOperation>(operation);

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
       
        TcpClientIdHeader clientIdHeader;
        wstring clientId = receiverContext_->ReplyTarget->Id();
        if (request_->Headers.TryReadFirst(clientIdHeader))
        {
            clientId = clientIdHeader.ClientId;
        }

        auto operation = this->Service.BeginProcessRequest(
            clientId,
            move(request_),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnSendMessageRequestComplete(operation, false);
        },
            thisSPtr);

        this->OnSendMessageRequestComplete(operation, true);

    }

private:

    void OnSendMessageRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = this->Service.EndProcessRequest(operation, reply_);

        if (error.IsSuccess())
        {
            if (!reply_)
            {
                reply_ = ServiceCommunicationHelper::CreateReplyMessageWithError(error);
            }
            else
            {
                reply_->Headers.Add(ServiceCommunicationErrorHeader(error.ReadValue()));
            }
        }
        else
        {
            WriteNoise(TraceType, this->traceId_, "Dispatching Completed with ErrorCode:{0}", error);
            reply_ = ServiceCommunicationHelper::CreateReplyMessageWithError(error);
        }

        error = receiverContext_->Reply(move(reply_));

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, this->traceId_, "Sending Reply with ErrorCode: {0}", error);
            reply_ = ServiceCommunicationHelper::CreateReplyMessageWithError(error);
            receiverContext_->Reply(move(reply_));
        }
        this->TryComplete(operation->Parent, error);
    }

    IServiceCommunicationMessageHandler & service_;
    MessageUPtr request_;
    wstring traceId_;
    ReceiverContextUPtr receiverContext_;
    MessageUPtr reply_;

};

class ServiceMethodCallDispatcher::ServiceMethodCallWorkItem
    : public CommonTimedJobItem<ServiceCommunicationListener>
{
    DENY_COPY(ServiceMethodCallWorkItem);

public:
    ServiceMethodCallWorkItem(
        MessageUPtr && message,
        ReceiverContextUPtr && context,
        TimeSpan const &  timeout,
        wstring const & traceId)
        :CommonTimedJobItem(timeout)
        , message_(move(message))
        , context_(move(context))
        ,traceId_(traceId)
    {
    }

    virtual void Process(ServiceCommunicationListener & owner)
    {
        this->Dispatch(owner.GetDispatcher()->rootedServicePtr_, this->message_, this->context_, this->get_RemainingTime(),this->traceId_,owner.GetDispatcher()->get_ServiceInfo());
    }

    virtual void OnTimeout(ServiceCommunicationListener & owner) override
    {
        Transport::FabricActivityHeader activityHeader;
        if (message_->Headers.TryReadFirst(activityHeader))
        {
            WriteWarning(TraceType,this->traceId_, "Message:{0} Timed out for Service: {1} ", activityHeader.ActivityId, owner.GetDispatcher()->get_ServiceInfo());
        }
    }

    void ServiceMethodCallWorkItem::Dispatch(
        IServiceCommunicationMessageHandlerPtr const & servicePtr,
        MessageUPtr & message,
        ReceiverContextUPtr & context,
        TimeSpan const & timeout,
        wstring const & traceId,
        wstring const & serviceInfo)
    {
        Transport::FabricActivityHeader activityHeader;

        if (message->Headers.TryReadFirst(activityHeader))
        {
            WriteNoise(TraceType, traceId, "Dispatching Message {0} to Service: {1}", activityHeader.ActivityId, serviceInfo);
        }


        auto operation = AsyncOperation::CreateAndStart<DispatchMessageAsyncOperation>(
            *servicePtr.get(),
            move(message),
            move(context),
            timeout,
            traceId,
            [this](AsyncOperationSPtr const & operation) { this->OnDispatchMessageComplete(operation, false); },
            servicePtr.get_Root()->CreateAsyncOperationRoot());

        this->OnDispatchMessageComplete(operation, true);
    }

    void ServiceMethodCallWorkItem::OnDispatchMessageComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {

        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = DispatchMessageAsyncOperation::End(operation);


    }

private:
    MessageUPtr message_;
    ReceiverContextUPtr context_;
    wstring const & traceId_;
};

void ServiceMethodCallDispatcher::AddToQueue(MessageUPtr & message, ReceiverContextUPtr & context)
{
    TimeoutHeader timeoutHeader;
    TimeSpan timeout;
    if (message->Headers.TryReadFirst(timeoutHeader))
    {
        timeout = timeoutHeader.get_Timeout();
    }
    else
    {
        timeout = this->timeout_;
    }

    //Creating copy as it will be moved when its Enqueue and we needed this in case Enqueue fails
    auto recieverContext =  *context;

    auto jobItem = make_unique<ServiceMethodCallWorkItem>(
        message->Clone(),
        move(context),
        timeout,
        serviceInfo_);

    if (!requestQueue_->Enqueue(move(jobItem)))
    {
        Transport::FabricActivityHeader activityHeader;
        wstring activityId = L"";
        if (message->Headers.TryReadFirst(activityHeader))
        {
            activityId = activityHeader.ActivityId.ToString();
        }

        WriteWarning(
            "ServiceMethodCallDispatcher",
             serviceInfo_,
            "Dropping message {0} because queue is full, QueueSize : {1}",
            activityId,
            requestQueue_->GetQueueLength());
     
         MessageUPtr reply = make_unique<Message>();
         reply->Headers.Add(ServiceCommunicationErrorHeader(ErrorCodeValue::ServiceTooBusy));;
         recieverContext.Reply(move(reply));
     }
 }

void ServiceMethodCallDispatcher::Close()
{
    requestQueue_->Close();
}


ServiceMethodCallDispatcher::~ServiceMethodCallDispatcher()
{
    WriteInfo("ServiceMethodCallDispatcher","Destructing ServiceMethodCallDispatcher for Service {0}",this->ServiceInfo);
}
