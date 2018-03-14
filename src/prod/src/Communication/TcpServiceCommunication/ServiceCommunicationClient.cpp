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

static const StringLiteral TraceType("ServiceCommunicationClient");

class ServiceCommunicationClient::ProcessRequestAsyncOperation
    : public TimedAsyncOperation
{
    DENY_COPY(ProcessRequestAsyncOperation);

public:
    ProcessRequestAsyncOperation(
        __in IServiceCommunicationMessageHandlerPtr const & service,
        wstring const & clientId,
        MessageUPtr && message,
        ReceiverContextUPtr && receiverContext,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , messageHandler_(service)
        , request_(move(message))
        , receiverContext_(move(receiverContext))
        , clientId_(clientId)
    {
    }

    virtual ~ProcessRequestAsyncOperation() {}

    __declspec(property(get = get_MessageHandler)) IServiceCommunicationMessageHandlerPtr & MessageHandler;
    IServiceCommunicationMessageHandlerPtr & get_MessageHandler() { return messageHandler_; }

    __declspec(property(get = get_Request)) Message & Request;
    Message & get_Request() { return *request_; }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out MessageUPtr & reply, __out ReceiverContextUPtr & receiverContext)
    {
        auto casted = AsyncOperation::End<ProcessRequestAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }
        receiverContext = move(casted->receiverContext_);

        return casted->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        IsAsyncOperationHeader isAsyncHeader;
        bool asyncOperation;
        if (request_->Headers.TryReadFirst(isAsyncHeader))
        {
            asyncOperation = isAsyncHeader.IsAsyncOperation;
        }
        else
        {
            asyncOperation = false;
        }

        if (asyncOperation)
        {
            auto operation = this->MessageHandler->BeginProcessRequest(
                clientId_,
                move(request_),
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnProcessRequestComplete(operation, false); },
                thisSPtr);

            this->OnProcessRequestComplete(operation, true);
        }
        else
        {
            auto error = this->MessageHandler->HandleOneWay(
                clientId_,
                move(request_));

            this->TryComplete(thisSPtr, error);
        }
    }

private:

    void OnProcessRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = this->MessageHandler->EndProcessRequest(operation, reply_);
        this->TryComplete(operation->Parent, error);
    }

    IServiceCommunicationMessageHandlerPtr  messageHandler_;
    MessageUPtr request_;
    wstring clientId_;
    ReceiverContextUPtr receiverContext_;
    MessageUPtr reply_;
};

ServiceCommunicationClient::ServiceCommunicationClient(ServiceCommunicationTransportSettingsUPtr  const & settings,
                                                       wstring const & serverAddress,
                                                       IServiceCommunicationMessageHandlerPtr const &  messageHandler,
                                                       IServiceConnectionEventHandlerPtr const & eventHandler,
                                                        bool explicitConnect)
                                                       : StateMachine(Created)
                                                       , connectError_()
                                                       ,clientId_(Guid::NewGuid().ToString())
                                                       ,transport_(DatagramTransportFactory::CreateTcpClient(clientId_, L"ServiceCommunicationClient"))
                                                       , demuxer_(make_unique<Demuxer>(*this, transport_))
                                                       , requestReply_(*this, transport_, false /* dispatchOnTransportThread */)
                                                       , serverAddress_(serverAddress)
                                                       , eventHandler_(eventHandler)
                                                       , messageHandler_(messageHandler)
                                                       , securitySettings_(settings->get_SecuritySetting())
                                                       , explicitConnect_(explicitConnect)
{
    transport_->SetMaxOutgoingFrameSize(settings->MaxMessageSize);
    transport_->EnableInboundActivityTracing();
    transport_->SetKeepAliveTimeout(settings->KeepAliveTimeout);
    transport_->SetConnectionIdleTimeout(TimeSpan::Zero);
    demuxer_->SetReplyHandler(this->requestReply_);
    requestReply_.EnableDifferentTimeoutError();
    this->connectTimeout_ = settings->ConnectTimeout;
}

ErrorCode ServiceCommunicationClient::Release()
{
    transport_->RemoveConnectionFaultHandler();
    this->transport_->Stop();
    this->requestReply_.Close();
    return this->demuxer_->Close();
}

ErrorCode ServiceCommunicationClient::OnClose()
{
    return Release();;
}

void ServiceCommunicationClient::OnAbort()
{
    Release();
}

void ServiceCommunicationClient::AbortClient()
{
    this->Abort();
}

void ServiceCommunicationClient::CloseClient()
{
    auto error = this->TransitionToClosed();
    if (error.IsSuccess())
    {
        OnClose();
    }
}

ServiceCommunicationClient::~ServiceCommunicationClient()
{
    OnClose();
}

ErrorCode ServiceCommunicationClient::Open()
{
    auto error = this->TransitionToOpening();
    if(error.IsSuccess())
    {
        error =  OnOpen();
       if(error.IsSuccess())
       {
           error = this->TransitionToOpened();
       }
    }
    return error;
}

ErrorCode ServiceCommunicationClient::OnOpen()
{
    requestReply_.Open();

    auto errorCode = this->SetServiceLocation();
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    errorCode = transport_->SetSecurity(securitySettings_);

    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    this->demuxer_->RegisterMessageHandler(
        Actor::ServiceCommunicationActor,
        [this](MessageUPtr & message, ReceiverContextUPtr & context)
    {
        this->ProcessRequest(this->messageHandler_, message, context);

    }, true);

    errorCode = this->demuxer_->Open();
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    auto root = this->CreateComponentRoot();

    transport_->SetConnectionFaultHandler([this, root](ISendTarget const & target, ErrorCode const & error)
    {
        WriteNoise(TraceType,this->clientId_, "Disconnecting connection for the SendTarget : {0} with the ErroCode : {1}", target.TraceId(), error);
        this->OnDisconnect(error);
    });

    errorCode = this->transport_->Start();

    if (errorCode.IsSuccess())
    {
        serverSendTarget_ = transport_->ResolveTarget(serverAddress_);
    }

    if (explicitConnect_)
    {
        //It needs to be called after we set set service location
        this->commonMessageHeadersSptr_ = this->CreateSharedHeaders();
    }
    
    return errorCode;
}

AsyncOperationSPtr ServiceCommunicationClient::BeginOpen(
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) 
{
    return AsyncOperation::CreateAndStart<TryConnectAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceCommunicationClient::EndOpen(Common::AsyncOperationSPtr const &  operation)
{
    return TryConnectAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCommunicationClient::BeginClose(
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const &  callback,
    Common::AsyncOperationSPtr const & parent) 
{
    return AsyncOperation::CreateAndStart<DisconnectAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceCommunicationClient::EndClose(
    Common::AsyncOperationSPtr const & operation
)
{
    return DisconnectAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceCommunicationClient::BeginRequest(
    MessageUPtr  && message,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    // During disconnect , we abort the client which internally close the transport.
    // Request Reply will then return ObjectClosed Error for all request came after client disconnected.
    this->OnSending(message, timeout);
    return AsyncOperation::CreateAndStart<SendRequestAsyncOperation>(
        *this,
       move(message),
        timeout,
        callback,
        parent);
  }

ErrorCode ServiceCommunicationClient::EndRequest(
    AsyncOperationSPtr const &  operation,
    MessageUPtr & reply)
{
    return SendRequestAsyncOperation::End(operation, reply);
}

ErrorCode ServiceCommunicationClient::SendOneWay(
    Transport::MessageUPtr && message)
{
    OnSending(message, TimeSpan::MaxValue);
    auto errorcode = transport_->SendOneWay(serverSendTarget_, move(message));

    if (!errorcode.IsSuccess() && (ServiceCommunicationHelper::IsCannotConnectErrorDuringUserRequest(errorcode)))
    {
        WriteWarning(TraceType,clientId_, "Error While Sending Message : {0}", errorcode);
        return ErrorCodeValue::ServiceCommunicationCannotConnect;
    }

    return errorcode;
}

void ServiceCommunicationClient::ProcessRequest(
    IServiceCommunicationMessageHandlerPtr const & handler,
    MessageUPtr & message,
    ReceiverContextUPtr & context)
{
    TimeoutHeader timeoutHeader;
    TimeSpan timeout;
    if (message->Headers.TryReadFirst(timeoutHeader))
    {
        timeout = timeoutHeader.Timeout;
    }

    TcpClientIdHeader clientIdHeader;
    wstring clientId;
    if (message->Headers.TryReadFirst(clientIdHeader))
    {
        clientId = clientIdHeader.ClientId;
    }
    auto operation = AsyncOperation::CreateAndStart<ProcessRequestAsyncOperation>(
        handler,
        clientId,
        move(message),
        move(context),
        timeout,
        [this](AsyncOperationSPtr const & operation) { return this->OnProcessRequestComplete(operation, false); },
        handler.get_Root()->CreateAsyncOperationRoot());

    this->OnProcessRequestComplete(operation, true);
}

void ServiceCommunicationClient::OnProcessRequestComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    ReceiverContextUPtr receiverContext;
    auto error = ProcessRequestAsyncOperation::End(operation, reply, receiverContext);

    if (error.IsSuccess())
    {
        if (!reply)
        {
            reply = make_unique<Transport::Message>();
        }

        reply->Headers.Add(ServiceCommunicationErrorHeader(ErrorCodeValue::Success));
    }
    else
    {
        if (!reply)
        {
            reply = make_unique<Transport::Message>();
        }

        reply->Headers.Add(ServiceCommunicationErrorHeader(error.ReadValue()));
    }
    receiverContext->Reply(move(reply));
}


void  ServiceCommunicationClient::OnDisconnect(ErrorCode const & error)
{
    if (this->TransitionToDisconnected().IsSuccess())
    {
        if (this->eventHandler_.get() != nullptr)
        {
            this->eventHandler_->OnDisconnected(this->serverAddress_, error);
        }
    }

       
}

KBuffer::SPtr ServiceCommunicationClient::CreateSharedHeaders()
{
    auto sharedStream = FabricSerializer::CreateSerializableStream();
    auto status = MessageHeaders::Serialize(*sharedStream, (ActorHeader(Actor::ServiceCommunicationActor)));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize ServiceCommunicationActor");
    status = MessageHeaders::Serialize(*sharedStream, ActionHeader(Constants::FabricServiceCommunicationAction));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize FabricServiceCommunicationAction");
    status = MessageHeaders::Serialize(*sharedStream, ServiceLocationActorHeader(serviceLocation_));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize ServiceLocationActorHeader");

    status = MessageHeaders::Serialize(*sharedStream, TcpClientIdHeader(this->clientId_));
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to serialize TcpClientIdHeader");

    KBuffer::SPtr buffer;
    status = FabricSerializer::CreateKBufferFromStream(*sharedStream, buffer);
    ASSERT_IFNOT(NT_SUCCESS(status), "Failed to create kbuffer from stream");

    return buffer;
}

void ServiceCommunicationClient::OnSending(MessageUPtr & message, TimeSpan const& timeout)
{
    message->Headers.Replace(TimeoutHeader(timeout));
    message->Headers.Add(FabricActivityHeader(ActivityId()));
    if (this->explicitConnect_) 
    {
        message->Headers.Add(*this->commonMessageHeadersSptr_,false);
    }
    else
    {
        message->Headers.Add(ActorHeader(Actor::ServiceCommunicationActor));
        message->Headers.Add(ServiceLocationActorHeader(serviceLocation_));
        message->Headers.Add(ActionHeader(Constants::FabricServiceCommunicationAction));
        TcpClientIdHeader header{ this->clientId_ };
        message->Headers.Add(header);
    }
}

ErrorCode  ServiceCommunicationClient::SetServiceLocation()
{
    auto index = serverAddress_.find(*Constants::ListenAddressPathDelimiter);

    if (index != wstring::npos)
    {
        this->serviceLocation_ = serverAddress_.substr(index + 1);
        return ErrorCodeValue::Success;
    }

    WriteWarning(TraceType,clientId_, "Can't parse endpoint, it is in an incorrect format: {0}", this->serverAddress_);
    return ErrorCodeValue::InvalidAddress;
}
