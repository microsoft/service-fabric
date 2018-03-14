// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace SystemServices;

// *** Private ServiceDirectMessagingAgent::Impl

StringLiteral const TraceComponent("ServiceDirectMessagingAgent");

class ServiceDirectMessagingAgent::Impl 
    : public RootedObject
    , public Federation::NodeTraceComponent<TraceTaskCodes::SystemServices>
{
public:
    Impl(
        Federation::NodeInstance const & nodeInstance,
        wstring const & hostAddress,
        ComponentRoot const & root)
        : RootedObject(root)
        , Federation::NodeTraceComponent<TraceTaskCodes::SystemServices>(nodeInstance)
        , hostAddress_(hostAddress)
        , directMessageHandlers_()
    {
        WriteInfo(TraceComponent, "{0} ctor: host address = {1}", this->TraceId, hostAddress);
    }

    ~Impl()
    {
        WriteInfo(TraceComponent, "{0} ~dtor", this->TraceId);
    }

    __declspec(property(get=get_HostAddress)) std::wstring const & HostAddress;
    std::wstring const & get_HostAddress() const { return hostAddress_; }

    ErrorCode Open();
    ErrorCode Close();
    void Abort();

    void RegisterMessageHandler(SystemServiceLocation const &, DemuxerMessageHandler const &);
    void UnregisterMessageHandler(SystemServiceLocation const &);

    void SendDirectReply(MessageUPtr &&, ReceiverContext const &);
    void OnDirectFailure(ErrorCode const &, ReceiverContext const &, ActivityId const & activityId);    

private:
    void ProcessDirectRequest(MessageUPtr &&, ReceiverContextUPtr &&);
    void Cleanup();

    wstring hostAddress_;
    IDatagramTransportSPtr transport_;
    DemuxerUPtr demuxer_;

    SystemServiceMessageHandlers<DemuxerMessageHandler> directMessageHandlers_;
};

ErrorCode ServiceDirectMessagingAgent::Impl::Open()
{
    auto root = this->Root.CreateComponentRoot();

    transport_ = DatagramTransportFactory::CreateTcp(
        hostAddress_,
        this->TraceId,
        L"ServiceDirectMessagingAgent");
    demuxer_ = make_unique<Transport::Demuxer>(*root, transport_);

    auto error = demuxer_->Open();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "Demuxer::Open failed : {0}",
            error);
        return error;
    }

    error = transport_->Start();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "Transport::Start failed : {0}",
            error);
        return error;
    }

    demuxer_->RegisterMessageHandler(
        Actor::DirectMessagingAgent,
        [this, root](MessageUPtr & m, ReceiverContextUPtr & r)
        {
            this->ProcessDirectRequest(move(m), move(r));
        },
        false);

    return ErrorCodeValue::Success;
}

ErrorCode ServiceDirectMessagingAgent::Impl::Close()
{
    this->Cleanup();
    return ErrorCodeValue::Success;
}

void ServiceDirectMessagingAgent::Impl::Abort()
{
    this->Cleanup();
}

void ServiceDirectMessagingAgent::Impl::Cleanup()
{
    demuxer_->UnregisterMessageHandler(Actor::DirectMessagingAgent);

    demuxer_->Close();

    transport_->Stop();

    directMessageHandlers_.Clear();
}

void ServiceDirectMessagingAgent::Impl::RegisterMessageHandler(
    SystemServiceLocation const & location,
    DemuxerMessageHandler const & handler)
{
    directMessageHandlers_.SetHandler(location, handler);

    WriteInfo(
        TraceComponent, 
        "{0} registered system service location: {1}",
        this->TraceId, 
        location);
}

void ServiceDirectMessagingAgent::Impl::UnregisterMessageHandler(SystemServiceLocation const & location)
{
    directMessageHandlers_.RemoveHandler(location);

    WriteInfo(
        TraceComponent, 
        "{0} unregistered system service location: {1}",
        this->TraceId, 
        location);
}

void ServiceDirectMessagingAgent::Impl::ProcessDirectRequest(
    MessageUPtr && message,
    ReceiverContextUPtr && receiverContext)
{
    WriteNoise(
        TraceComponent, 
        "[{0}+{1}] processing request {2}", 
        this->TraceId, 
        FabricActivityHeader::FromMessage(*message).ActivityId, 
        message->MessageId);

    auto handler = directMessageHandlers_.GetHandler(message);

    if (handler != nullptr)
    {
        auto error = ServiceDirectMessagingAgentMessage::UnwrapFromTransport(*message);

        if (error.IsSuccess())
        {
            // The service will reply directly
            handler(message, receiverContext);
        }
        else
        {
            this->OnDirectFailure(ErrorCodeValue::InvalidMessage, *receiverContext, FabricActivityHeader::FromMessage(*message).ActivityId);
        }
    }
    else
    {
        this->OnDirectFailure(ErrorCodeValue::MessageHandlerDoesNotExistFault, *receiverContext, FabricActivityHeader::FromMessage(*message).ActivityId);
    }
}

void ServiceDirectMessagingAgent::Impl::SendDirectReply(MessageUPtr && reply, ReceiverContext const & receiverContext)
{
    receiverContext.Reply(move(reply));
}

void ServiceDirectMessagingAgent::Impl::OnDirectFailure(ErrorCode const & error, ReceiverContext const & receiverContext, ActivityId const & activityId)
{
    auto reply = ServiceDirectMessagingAgentMessage::CreateFailureMessage(error, activityId);

    this->SendDirectReply(move(reply), receiverContext);
}

// *** Public ServiceDirectMessagingAgent

ServiceDirectMessagingAgent::ServiceDirectMessagingAgent(
    Federation::NodeInstance const & nodeInstance,
    wstring const & hostAddress,
    ComponentRoot const & root)
    : implUPtr_(make_unique<Impl>(
        nodeInstance,
        hostAddress,
        root))
{
}

ServiceDirectMessagingAgentSPtr ServiceDirectMessagingAgent::CreateShared(
    Federation::NodeInstance const & nodeInstance,
    wstring const & hostAddress,
    ComponentRoot const & root)
{
    return shared_ptr<ServiceDirectMessagingAgent>(new ServiceDirectMessagingAgent(nodeInstance, hostAddress, root));
}

Federation::NodeInstance const & ServiceDirectMessagingAgent::get_NodeInstance() const
{
    return implUPtr_->NodeInstance;
}

std::wstring const & ServiceDirectMessagingAgent::get_HostAddress() const
{
    return implUPtr_->HostAddress;
}

ErrorCode ServiceDirectMessagingAgent::OnOpen()
{
    return implUPtr_->Open();
}

ErrorCode ServiceDirectMessagingAgent::OnClose()
{
    return implUPtr_->Close();
}

void ServiceDirectMessagingAgent::OnAbort()
{
    implUPtr_->Abort();
}

void ServiceDirectMessagingAgent::RegisterMessageHandler(SystemServiceLocation const & location, DemuxerMessageHandler const & handler)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->RegisterMessageHandler(location, handler);
}

void ServiceDirectMessagingAgent::UnregisterMessageHandler(SystemServiceLocation const & location)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->UnregisterMessageHandler(location);
}

void ServiceDirectMessagingAgent::SendDirectReply(MessageUPtr && message, ReceiverContext const & receiverContext)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->SendDirectReply(move(message), receiverContext);
}

void ServiceDirectMessagingAgent::OnDirectFailure(ErrorCode const & error, ReceiverContext const & receiverContext)
{
    OnDirectFailure(error, receiverContext, ActivityId(Guid::Empty()));
}

void ServiceDirectMessagingAgent::OnDirectFailure(ErrorCode const & error, ReceiverContext const & receiverContext, ActivityId const & activityId)
{
    ThrowIfCreatedOrOpening();

    implUPtr_->OnDirectFailure(error, receiverContext, activityId);
}
