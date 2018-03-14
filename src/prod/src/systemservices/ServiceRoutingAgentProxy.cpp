// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace SystemServices;

// *** Private ServiceRoutingAgentProxy::Impl

StringLiteral const TraceComponent("ServiceRoutingAgentProxy");

ServiceRoutingAgentProxy::~ServiceRoutingAgentProxy()
{
}

class ServiceRoutingAgentProxy::Impl 
    : public RootedObject
    , public NodeTraceComponent<TraceTaskCodes::SystemServices>
{
public:
    Impl(
        Federation::NodeInstance const & nodeInstance,
        IpcClient & ipcClient,
        ComponentRoot const & root)
        : RootedObject(root)
        , NodeTraceComponent(nodeInstance)
        , ipcClient_(ipcClient)
        , ipcMessageHandlers_()
    {
        WriteInfo(TraceComponent, "{0} ctor: client Id = {1}", this->TraceId, ipcClient_.ClientId);
    }

    ~Impl()
    {
        WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
    }


    ErrorCode Open();
    ErrorCode Close();
    void Abort();

    void RegisterMessageHandler(SystemServiceLocation const &, IpcMessageHandler const &);
    void UnregisterMessageHandler(SystemServiceLocation const &);

    AsyncOperationSPtr BeginSendRequest(MessageUPtr &&, TimeSpan const, AsyncCallback const &, AsyncOperationSPtr const &);
    ErrorCode EndSendRequest(AsyncOperationSPtr const &, __out MessageUPtr &);

    void SendIpcReply(MessageUPtr &&, IpcReceiverContext const &);
    void OnIpcFailure(ErrorCode const &, IpcReceiverContext const &, ActivityId const & activityId);    

private:
    void ProcessIpcRequest(MessageUPtr &&, IpcReceiverContextUPtr &&);
    void Cleanup();

    IpcClient & ipcClient_;
    SystemServiceMessageHandlers<IpcMessageHandler> ipcMessageHandlers_;
};  // end class Impl

ErrorCode ServiceRoutingAgentProxy::Impl::Open()
{
    ComponentRootSPtr root = this->Root.CreateComponentRoot();

    ipcClient_.RegisterMessageHandler(
        Actor::ServiceRoutingAgent, 
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        {
            this->ProcessIpcRequest(move(message), move(receiverContext));
        },
        false);

    return ErrorCodeValue::Success;
}

ErrorCode ServiceRoutingAgentProxy::Impl::Close()
{
    this->Cleanup();
    return ErrorCodeValue::Success;
}

void ServiceRoutingAgentProxy::Impl::Abort()
{
    this->Cleanup();
}

void ServiceRoutingAgentProxy::Impl::Cleanup()
{
    ipcClient_.UnregisterMessageHandler(Actor::ServiceRoutingAgent);

    ipcMessageHandlers_.Clear();
}

void ServiceRoutingAgentProxy::Impl::RegisterMessageHandler(
    SystemServiceLocation const & location,
    IpcMessageHandler const & handler)
{
    // Replace existing handler if any
    ipcMessageHandlers_.SetHandler(location, handler);

    WriteInfo(
        TraceComponent, 
        "{0} registered system service location: {1}",
        this->TraceId, 
        location);
}

void ServiceRoutingAgentProxy::Impl::UnregisterMessageHandler(SystemServiceLocation const & location)
{
    ipcMessageHandlers_.RemoveHandler(location);

    WriteInfo(
        TraceComponent, 
        "{0} unregistered system service location: {1}",
        this->TraceId, 
        location);
}

void ServiceRoutingAgentProxy::Impl::ProcessIpcRequest(
    MessageUPtr && message,
    IpcReceiverContextUPtr && receiverContext)
{
    WriteNoise(
        TraceComponent, 
        "[{0}+{1}] processing request {2}", 
        this->TraceId, 
        FabricActivityHeader::FromMessage(*message).ActivityId, 
        message->MessageId);

    auto handler = ipcMessageHandlers_.GetHandler(message);

    if (handler != nullptr)
    {
        ErrorCode error = ServiceRoutingAgentMessage::UnwrapFromIpc(*message);

        if (error.IsSuccess())
        {
            // The service will reply directly
            handler(message, receiverContext);
        }
        else
        {
            this->OnIpcFailure(ErrorCodeValue::InvalidMessage, *receiverContext, FabricActivityHeader::FromMessage(*message).ActivityId);
        }
    }
    else
    {
        this->OnIpcFailure(ErrorCodeValue::MessageHandlerDoesNotExistFault, *receiverContext, FabricActivityHeader::FromMessage(*message).ActivityId);
    }
}

AsyncOperationSPtr ServiceRoutingAgentProxy::Impl::BeginSendRequest(
    MessageUPtr && request, 
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    request->Headers.Replace(TimeoutHeader(timeout));

    ServiceRoutingAgentMessage::WrapForIpc(*request);

    // There used to be an IpcServer/IpcClient bug that miscalculates the message header signature
    // when there are deleted headers. Preserve the Compact() here anyway as good practice.
    //
    request->Headers.Compact();

    return ipcClient_.BeginRequest(move(request), timeout, callback, root);
}

ErrorCode ServiceRoutingAgentProxy::Impl::EndSendRequest(AsyncOperationSPtr const & operation, __out MessageUPtr & result)
{
    MessageUPtr reply;
    ErrorCode error = ipcClient_.EndRequest(operation, reply);
    
    if (error.IsSuccess())
    {
        error = ServiceRoutingAgentMessage::ValidateIpcReply(*reply);

        if (error.IsSuccess())
        {
            result.swap(reply);
        }
    }

    return error;
}

void ServiceRoutingAgentProxy::Impl::SendIpcReply(MessageUPtr && reply, IpcReceiverContext const & receiverContext)
{
    receiverContext.Reply(move(reply));
}

void ServiceRoutingAgentProxy::Impl::OnIpcFailure(ErrorCode const & error, IpcReceiverContext const & receiverContext, ActivityId const & activityId)
{
    auto reply = ServiceRoutingAgentMessage::CreateIpcFailureMessage(IpcFailureBody(error), activityId);
    this->SendIpcReply(move(reply), receiverContext);
}

// *** Public ServiceRoutingAgentProxy

ServiceRoutingAgentProxy::ServiceRoutingAgentProxy(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComponentRoot const & root)
    : implUPtr_(make_unique<Impl>(
        nodeInstance,
        ipcClient,
        root))
{
}

ServiceRoutingAgentProxyUPtr ServiceRoutingAgentProxy::Create(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComponentRoot const & root)
{
    return unique_ptr<ServiceRoutingAgentProxy>(new ServiceRoutingAgentProxy(nodeInstance, ipcClient, root));
}

ServiceRoutingAgentProxySPtr ServiceRoutingAgentProxy::CreateShared(
    Federation::NodeInstance const & nodeInstance,
    __in IpcClient & ipcClient,
    ComponentRoot const & root)
{
    return shared_ptr<ServiceRoutingAgentProxy>(new ServiceRoutingAgentProxy(nodeInstance, ipcClient, root));
}

Federation::NodeInstance const & ServiceRoutingAgentProxy::get_NodeInstance() const
{
    return implUPtr_->NodeInstance;
}

ErrorCode ServiceRoutingAgentProxy::OnOpen()
{
    return implUPtr_->Open();
}

ErrorCode ServiceRoutingAgentProxy::OnClose()
{
    return implUPtr_->Close();
}

void ServiceRoutingAgentProxy::OnAbort()
{
    implUPtr_->Abort();
}

void ServiceRoutingAgentProxy::RegisterMessageHandler(SystemServiceLocation const & location, IpcMessageHandler const & handler)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->RegisterMessageHandler(location, handler);
}

void ServiceRoutingAgentProxy::UnregisterMessageHandler(SystemServiceLocation const & location)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->UnregisterMessageHandler(location);
}

AsyncOperationSPtr ServiceRoutingAgentProxy::BeginSendRequest(
    MessageUPtr && request, 
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    return implUPtr_->BeginSendRequest(move(request), timeout, callback, root);
}

ErrorCode ServiceRoutingAgentProxy::EndSendRequest(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
{
    return implUPtr_->EndSendRequest(operation, reply);
}

void ServiceRoutingAgentProxy::SendIpcReply(MessageUPtr && message, IpcReceiverContext const & receiverContext)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->SendIpcReply(move(message), receiverContext);
}

void ServiceRoutingAgentProxy::OnIpcFailure(ErrorCode const & error, IpcReceiverContext const & receiverContext)
{
    OnIpcFailure(error, receiverContext, ActivityId(Guid::Empty()));
}

void ServiceRoutingAgentProxy::OnIpcFailure(ErrorCode const & error, IpcReceiverContext const & receiverContext, ActivityId const & activityId)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->OnIpcFailure(error, receiverContext, activityId);
}
