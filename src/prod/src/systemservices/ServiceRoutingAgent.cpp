// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Hosting2;
using namespace Naming;
using namespace Transport;
using namespace ServiceModel;
using namespace SystemServices;

// *** Private ServiceRoutingAgent::Impl

StringLiteral const TraceComponent("ServiceRoutingAgent");

class ServiceRoutingAgent::Impl 
    : public RootedObject
    , private NodeTraceComponent<Common::TraceTaskCodes::SystemServices>
{
public:
    Impl(
        __in IpcServer & ipcServer,
        __in FederationSubsystem & federation, 
        __in IHostingSubsystem & hosting, 
        __in IGateway & naming, 
        ComponentRoot const & root)
        : RootedObject(root)
        , NodeTraceComponent(federation.Instance)
        , ipcServer_(ipcServer)
        , federation_(federation) 
        , hosting_(hosting)
        , naming_(naming)
    { 
        REGISTER_MESSAGE_HEADER(SystemServiceFilterHeader);

        WriteInfo(TraceComponent, "{0} ctor", this->TraceId);
    }

    virtual ~Impl() 
    { 
        WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
    }

    ErrorCode Open();
    ErrorCode Close();
    void Abort();

private:
    class RequestAsyncOperationBase 
        : public TimedAsyncOperation
        , public NodeActivityTraceComponent<TraceTaskCodes::SystemServices>
    {
    public:
        RequestAsyncOperationBase(
            Impl & owner,
            MessageUPtr && message, 
            ErrorCodeValue::Enum error,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : TimedAsyncOperation(timeout, callback, root)
            , NodeActivityTraceComponent(owner.NodeInstance, FabricActivityHeader::FromMessage(*message).ActivityId)
            , owner_(owner)
            , request_(move(message))
            , error_(error)
            , reply_()
        {
        }

        virtual ~RequestAsyncOperationBase() { }

        MessageUPtr && TakeReply() { return move(reply_); }

    protected:
        virtual void OnStart(AsyncOperationSPtr const & thisSPtr) { this->TryComplete(thisSPtr, error_); }

        void SetReply(MessageUPtr && reply) { reply_.swap(reply); }

        Impl & owner_;
        MessageUPtr request_;
        ErrorCodeValue::Enum error_;

    private:
        MessageUPtr reply_;
    };

    class ServiceToNodeAsyncOperation : public RequestAsyncOperationBase
    {
    public:
        ServiceToNodeAsyncOperation(
            Impl & owner,
            MessageUPtr && message, 
            IpcReceiverContextUPtr && receiverContext,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : RequestAsyncOperationBase(
                owner,
                move(message),
                ErrorCodeValue::Success,
                timeout, 
                callback, 
                root)
            , receiverContext_(move(receiverContext))
        {
        }

        ServiceToNodeAsyncOperation(
            Impl & owner,
            ErrorCodeValue::Enum error,
            IpcReceiverContextUPtr && receiverContext,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : RequestAsyncOperationBase(
                owner,
                MessageUPtr(),
                error,
                timeout, 
                callback, 
                root)
            , receiverContext_(move(receiverContext))
        {
        }

        virtual ~ServiceToNodeAsyncOperation() { }

        __declspec (property(get=get_IpcReceiverContext)) IpcReceiverContext const & ReceiverContext;
        IpcReceiverContext const & get_IpcReceiverContext() const { return *receiverContext_; }

    protected:
        virtual void OnStart(AsyncOperationSPtr const &);

    private:
        void OnNodeProcessingComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        IpcReceiverContextUPtr receiverContext_;
    };

    class NodeToServiceAsyncOperation : public RequestAsyncOperationBase
    {
    public:
        NodeToServiceAsyncOperation(
            Impl & owner,
            MessageUPtr && message, 
            RequestReceiverContextUPtr && receiverContext,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : RequestAsyncOperationBase(
                owner,
                move(message),
                ErrorCodeValue::Success,
                timeout, 
                callback, 
                root)
            , receiverContext_(move(receiverContext))
        {
        }

        NodeToServiceAsyncOperation(
            Impl & owner,
            ErrorCodeValue::Enum error,
            RequestReceiverContextUPtr && receiverContext,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root)
            : RequestAsyncOperationBase(
                owner,
                MessageUPtr(),
                error,
                timeout, 
                callback, 
                root)
            , receiverContext_(move(receiverContext))
        {
        }

        virtual ~NodeToServiceAsyncOperation() { }

        __declspec (property(get=get_RequestReceiverContext)) RequestReceiverContext & ReceiverContext;
        RequestReceiverContext & get_RequestReceiverContext() { return *receiverContext_; }

    protected:
        virtual void OnStart(AsyncOperationSPtr const &);

    private:
        ErrorCode GetHostId(ServiceTypeIdentifier const &, __out wstring & clientId);
        void OnForwardToServiceComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        RequestReceiverContextUPtr receiverContext_;
    };

    void Cleanup();

    bool IsValidRequest(MessageUPtr const &);

    void ProcessIpcRequest(MessageUPtr &&, IpcReceiverContextUPtr &&);
    void OnIpcRequestComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
    void SendIpcReply(MessageUPtr &&, IpcReceiverContext const &);
    void OnIpcFailure(ErrorCode const &, IpcReceiverContext const &, ActivityId const & activityId);

    void ProcessFederationRequest(MessageUPtr &&, RequestReceiverContextUPtr &&);
    void OnFederationRequestComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
    void SendFederationReply(MessageUPtr &&, RequestReceiverContext &);
    void OnFederationFailure(ErrorCode const &, RequestReceiverContext &);

    AsyncOperationSPtr BeginRouteGatewayMessage(
        MessageUPtr & message,
        TimeSpan,
        AsyncCallback const &, 
        AsyncOperationSPtr const & );
    
    ErrorCode EndRouteGatewayMessage(
        AsyncOperationSPtr const &, 
        _Out_ Transport::MessageUPtr & );

    IpcServer & ipcServer_;
    FederationSubsystem & federation_;
    IHostingSubsystem & hosting_;
    IGateway & naming_;
};  // end class Impl

ServiceRoutingAgent::~ServiceRoutingAgent()
{
}

ErrorCode ServiceRoutingAgent::Impl::Open()
{
    WriteInfo(
        TraceComponent, 
        "{0} registering federation",
        this->TraceId); 
    
    ComponentRootSPtr root = this->Root.CreateComponentRoot();

    ipcServer_.RegisterMessageHandler(
        Actor::ServiceRoutingAgent,
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        {
            this->ProcessIpcRequest(move(message), move(receiverContext));
        },
        false);

    federation_.RegisterMessageHandler(
        Actor::ServiceRoutingAgent,
        [] (MessageUPtr &, OneWayReceiverContextUPtr &) 
        { 
            Assert::CodingError("ServiceRoutingAgent does not support oneway messages");
        },
        [this, root] (MessageUPtr & message, RequestReceiverContextUPtr & receiverContext) 
        { 
            this->ProcessFederationRequest(move(message), move(receiverContext));
        },
        false);

    naming_.RegisterGatewayMessageHandler(
        Actor::ServiceRoutingAgent,
        [this, root] (MessageUPtr &message, TimeSpan timeout, AsyncCallback const& callback, AsyncOperationSPtr const& parent)
        {
            return BeginRouteGatewayMessage(message, timeout, callback, parent);
        },
        [this, root](AsyncOperationSPtr const& operation, _Out_ MessageUPtr & reply)
        {
            return EndRouteGatewayMessage(operation, reply);
        });

    return ErrorCodeValue::Success;
}

ErrorCode ServiceRoutingAgent::Impl::Close()
{
    this->Cleanup();

    return ErrorCodeValue::Success;
}

void ServiceRoutingAgent::Impl::Abort()
{
    this->Cleanup();
}

void ServiceRoutingAgent::Impl::Cleanup()
{
    federation_.UnRegisterMessageHandler(Actor::ServiceRoutingAgent);
    ipcServer_.UnregisterMessageHandler(Actor::ServiceRoutingAgent);
    naming_.UnRegisterGatewayMessageHandler(Actor::ServiceRoutingAgent);
}

bool ServiceRoutingAgent::Impl::IsValidRequest(MessageUPtr const & message)
{
    if (!message)
    {
        WriteInfo(TraceComponent, "{0} null request", this->TraceId);
        return false;
    }

    if (message->Actor != Actor::ServiceRoutingAgent)
    {
        WriteInfo(TraceComponent, "{0} invalid actor: {1}", this->TraceId, message->Actor);
        return false;
    }

    TimeoutHeader timeoutHeader;
    if (!message->Headers.TryReadFirst(timeoutHeader))
    {
        WriteInfo(TraceComponent, "{0} missing timeout header: {1}", this->TraceId, message->Action);
        return false;
    }
    
    return true;
}

//
// *** IPC
//

void ServiceRoutingAgent::Impl::ProcessIpcRequest(MessageUPtr && message, IpcReceiverContextUPtr && receiverContext)
{
    if (!IsValidRequest(message))
    {
        this->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext, ActivityId(Guid::Empty()));
        return;
    }

    auto timeoutHeader = TimeoutHeader::FromMessage(*message);

    if (message->Action == ServiceRoutingAgentMessage::ServiceRouteRequestAction)
    {
        auto operation = AsyncOperation::CreateAndStart<ServiceToNodeAsyncOperation>(
            *this,
            move(message),
            move(receiverContext),
            timeoutHeader.Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnIpcRequestComplete(operation, false); },
            this->Root.CreateAsyncOperationRoot());
        this->OnIpcRequestComplete(operation, true);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} invalid action: {1}", this->TraceId, message->Action);

        auto operation = AsyncOperation::CreateAndStart<ServiceToNodeAsyncOperation>(
            *this,
            ErrorCodeValue::InvalidMessage,
            move(receiverContext),
            timeoutHeader.Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnIpcRequestComplete(operation, false); },
            this->Root.CreateAsyncOperationRoot());
        this->OnIpcRequestComplete(operation, true);
    }
}

void ServiceRoutingAgent::Impl::OnIpcRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto casted = AsyncOperation::End<ServiceToNodeAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        MessageUPtr reply = casted->TakeReply();

        // There used to be an IpcServer/IpcClient bug that miscalculates the message header signature
        // when there are deleted headers. Preserve the Compact() here anyway as good practice.
        //
        reply->Headers.Compact();

        this->SendIpcReply(move(reply), casted->ReceiverContext);
    }
    else
    {
        this->OnIpcFailure(casted->Error, casted->ReceiverContext, casted->ActivityId);
    }
}

void ServiceRoutingAgent::Impl::SendIpcReply(MessageUPtr && reply, IpcReceiverContext const & receiverContext)
{
    receiverContext.Reply(move(reply));
}

void ServiceRoutingAgent::Impl::OnIpcFailure(ErrorCode const & error, IpcReceiverContext const & receiverContext, ActivityId const & activityId)
{
    auto reply = ServiceRoutingAgentMessage::CreateIpcFailureMessage(IpcFailureBody(error.ReadValue()), activityId);
    this->SendIpcReply(move(reply), receiverContext);
}

//
// *** Federation
//

void ServiceRoutingAgent::Impl::ProcessFederationRequest(MessageUPtr && message, RequestReceiverContextUPtr && receiverContext)
{
    if (!IsValidRequest(message))
    {
        this->OnFederationFailure(ErrorCodeValue::InvalidMessage, *receiverContext);
        return;
    }

    auto timeoutHeader = TimeoutHeader::FromMessage(*message);

    if (message->Action == ServiceRoutingAgentMessage::ServiceRouteRequestAction)
    {
        auto operation = AsyncOperation::CreateAndStart<NodeToServiceAsyncOperation>(
            *this,
            move(message),
            move(receiverContext),
            timeoutHeader.Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnFederationRequestComplete(operation, false); },
            this->Root.CreateAsyncOperationRoot());
        this->OnFederationRequestComplete(operation, true);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} invalid action: {1}", this->TraceId, message->Action);

        auto operation = AsyncOperation::CreateAndStart<NodeToServiceAsyncOperation>(
            *this,
            ErrorCodeValue::InvalidMessage,
            move(receiverContext),
            timeoutHeader.Timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnFederationRequestComplete(operation, false); },
            this->Root.CreateAsyncOperationRoot());
        this->OnFederationRequestComplete(operation, true);
    }
}

void ServiceRoutingAgent::Impl::OnFederationRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto casted = AsyncOperation::End<NodeToServiceAsyncOperation>(operation);

    if (casted->Error.IsSuccess())
    {
        this->SendFederationReply(casted->TakeReply(), casted->ReceiverContext);
    }
    else
    {
        this->OnFederationFailure(casted->Error, casted->ReceiverContext);
    }
}

void ServiceRoutingAgent::Impl::SendFederationReply(MessageUPtr && reply, RequestReceiverContext & receiverContext)
{
    receiverContext.Reply(move(reply));
}

void ServiceRoutingAgent::Impl::OnFederationFailure(ErrorCode const & error, RequestReceiverContext & receiverContext)
{
    receiverContext.Reject(error);
}

// ** Gateway

AsyncOperationSPtr ServiceRoutingAgent::Impl::BeginRouteGatewayMessage(
    MessageUPtr & message,
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    AsyncOperationSPtr operation;

    if (message->Action == ServiceRoutingAgentMessage::ServiceRouteRequestAction)
    {
       operation = AsyncOperation::CreateAndStart<NodeToServiceAsyncOperation>(
            *this,
            move(message),
            nullptr, // RequestReceiverContextUPtr
            timeout,
            callback,
            parent);
    }
    else
    {
        WriteInfo(TraceComponent, "{0} invalid action in Gateway routing message: {1}", this->TraceId, message->Action);

        operation = AsyncOperation::CreateAndStart<NodeToServiceAsyncOperation>(
            *this,
            ErrorCodeValue::InvalidMessage,
            nullptr, // RequestReceiverContextUPtr
            timeout,
            callback,
            parent);
    }

    return operation;
}

ErrorCode ServiceRoutingAgent::Impl::EndRouteGatewayMessage(
    AsyncOperationSPtr const& operation,
    MessageUPtr &reply)
{
    auto casted = AsyncOperation::End<NodeToServiceAsyncOperation>(operation);
    if (casted->Error.IsSuccess())
    {
        reply = casted->TakeReply();
    }

    return casted->Error;
}

// *** ServiceToNodeAsyncOperation

void ServiceRoutingAgent::Impl::ServiceToNodeAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (error_ != ErrorCodeValue::Success)
    {
        RequestAsyncOperationBase::OnStart(thisSPtr);
        return;
    }

    ErrorCode error = ServiceRoutingAgentMessage::UnwrapFromIpc(*request_);

    if (error.IsSuccess())
    {
        WriteNoise(
            TraceComponent, 
            "{0} forwarding request {1}:{2} to Naming",
            this->TraceId,
            request_->Actor,
            request_->Action);

        auto operation = owner_.naming_.BeginProcessRequest(
            request_->Clone(),
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnNodeProcessingComplete(operation, false); },
            thisSPtr);
        this->OnNodeProcessingComplete(operation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

void ServiceRoutingAgent::Impl::ServiceToNodeAsyncOperation::OnNodeProcessingComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    ErrorCode error = owner_.naming_.EndProcessRequest(operation, reply);

    if (error.IsSuccess())
    {
        this->SetReply(move(reply));
    }

    this->TryComplete(operation->Parent, error);
}

// *** NodeToServiceAsyncOperation

void ServiceRoutingAgent::Impl::NodeToServiceAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (error_ != ErrorCodeValue::Success)
    {
        RequestAsyncOperationBase::OnStart(thisSPtr);
        return;
    }

    ServiceRoutingAgentHeader routingAgentHeader;
    if (!request_->Headers.TryReadFirst(routingAgentHeader))
    {
        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "ServiceRoutingAgentHeader missing: {0}", request_->MessageId);

        this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
        return;
    }

    wstring hostId;
    ErrorCode error = this->GetHostId(routingAgentHeader.TypeId, hostId);

    if (error.IsSuccess())
    {
        error = ServiceRoutingAgentMessage::RewrapForRoutingAgentProxy(*request_, routingAgentHeader);
    }

    if (error.IsSuccess())
    {
        WriteNoise(
            TraceComponent, 
            "{0} forwarding request {1} to host {2}", 
            this->TraceId,
            routingAgentHeader, 
            hostId);

        // There used to be an IpcServer/IpcClient bug that miscalculates the message header signature
        // when there are deleted headers. Preserve the Compact() here anyway as good practice.
        //
        request_->Headers.Compact();

        auto operation = owner_.ipcServer_.BeginRequest(
            request_->Clone(),
            hostId,
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnForwardToServiceComplete(operation, false); },
            thisSPtr);
        this->OnForwardToServiceComplete(operation, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}

ErrorCode ServiceRoutingAgent::Impl::NodeToServiceAsyncOperation::GetHostId(
    ServiceTypeIdentifier const & serviceTypeId,
    __out wstring & hostId)
{
    auto error = owner_.hosting_.GetHostId(
        VersionedServiceTypeIdentifier(serviceTypeId, ServicePackageVersionInstance()),
        SystemServiceApplicationNameHelper::SystemServiceApplicationName,
        hostId);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent, 
            "{0} host Id for service type {1} not found: {2}",
            this->TraceId,
            serviceTypeId,
            error); 
    }

    return error;
}

void ServiceRoutingAgent::Impl::NodeToServiceAsyncOperation::OnForwardToServiceComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    ErrorCode error = owner_.ipcServer_.EndRequest(operation, reply);

    if (error.IsError(ErrorCodeValue::CannotConnectToAnonymousTarget))
    {
        // Translate to an error that is retryable at the gateway.
        // It will re-resolve and resend the request.
        //
        error = ErrorCodeValue::MessageHandlerDoesNotExistFault;
    }

    if (error.IsSuccess())
    {
        error = ServiceRoutingAgentMessage::ValidateIpcReply(*reply);

        if (error.IsSuccess())
        {
            this->SetReply(move(reply));
        }
    }

    this->TryComplete(operation->Parent, error);
}

// *** Public ServiceRoutingAgent

ServiceRoutingAgent::ServiceRoutingAgent(
    __in IpcServer & ipcServer,
    __in FederationSubsystem & federation,
    __in IHostingSubsystem & hosting,
    __in IGateway & naming,
    ComponentRoot const & root)
    : implUPtr_(make_unique<Impl>(
        ipcServer,
        federation,
        hosting,
        naming,
        root))
{
}

ServiceRoutingAgentUPtr ServiceRoutingAgent::Create(
    __in IpcServer & ipcServer,
    __in FederationSubsystem & federation,
    __in IHostingSubsystem & hosting,
    __in IGateway & naming,
    ComponentRoot const & root)
{
    return unique_ptr<ServiceRoutingAgent>(new ServiceRoutingAgent(ipcServer, federation, hosting, naming, root));
}

ErrorCode ServiceRoutingAgent::OnOpen()
{
    return implUPtr_->Open();
}

ErrorCode ServiceRoutingAgent::OnClose()
{
    return implUPtr_->Close();
}

void ServiceRoutingAgent::OnAbort()
{
    implUPtr_->Abort();
}
