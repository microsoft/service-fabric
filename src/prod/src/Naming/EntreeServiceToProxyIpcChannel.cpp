// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Naming;
using namespace Common;
using namespace Transport;
using namespace std;
using namespace Reliability;
using namespace Query;
using namespace Federation;
using namespace SystemServices;
using namespace ServiceModel;

StringLiteral const TraceType("EntreeServiceToProxyIpcChannel");
GlobalWString const ComponentName = make_global<wstring>(L"EntreeService");

class EntreeServiceToProxyIpcChannel::ClientRequestJobItem : public CommonTimedJobItem<EntreeServiceToProxyIpcChannel>
{
public:
    ClientRequestJobItem(
        Transport::MessageUPtr && message,
        ReceiverContextUPtr && context,
        TimeSpan &&timeout)
        : CommonTimedJobItem(timeout)
        , message_(move(message))
        , context_(move(context))
    {}

    virtual void Process(EntreeServiceToProxyIpcChannel &owner)
    {
        FabricActivityHeader activityHeader;
        if (!message_->Headers.TryReadFirst(activityHeader))
        {
            TRACE_AND_TESTASSERT(WriteWarning, TraceType, owner.TraceId, "Action {0} missing activity header", message_->Action);
        }

        owner.EventSource.EntreeServiceIpcChannelRequestProcessing(
            activityHeader.ActivityId,
            message_->Action,
            RemainingTime);

        owner.ProcessQueuedIpcRequest(move(message_), move(context_), RemainingTime);
    }

    virtual void OnTimeout(EntreeServiceToProxyIpcChannel &owner) override
    {
        FabricActivityHeader activityHeader;
        if (!message_->Headers.TryReadFirst(activityHeader))
        {
            TRACE_AND_TESTASSERT(WriteWarning, TraceType, owner.TraceId, "Action {0} missing activity header", message_->Action);
        }

        WriteInfo(
            TraceType,
            owner.TraceId,
            "{0} Request timed out, Action: {1}",
            activityHeader.ActivityId,
            message_->Action);
    }

private:
    Transport::MessageUPtr message_;
    ReceiverContextUPtr context_;
};

EntreeServiceToProxyIpcChannel::EntreeServiceToProxyIpcChannel(
    ComponentRoot const & root,
    wstring const &listenAddress,
    Federation::NodeInstance const & instance,
    wstring const &nodeName,
    GatewayEventSource const &eventSource,
    bool useUnreliable)
    : EntreeServiceIpcChannelBase(root)
    , transportListenAddress_(listenAddress)
    , nodeInstance_(instance)
    , nodeName_(nodeName)
    , entreeServiceProxyProcessId_(0)
    , eventSource_(eventSource)
{
    Common::StringWriter(traceId_).Write("[{0}]", nodeInstance_);
    WriteInfo(TraceType, TraceId, "ctor");

    transport_ = DatagramTransportFactory::CreateTcp(
        transportListenAddress_, 
        wformatString("EntreeServiceToProxyIpcChannel#{0}", nodeInstance_.ToString()),
        L"EntreeServiceToProxyIpcChannel");

    if (useUnreliable)
    {
        transport_ = DatagramTransportFactory::CreateUnreliable(root, transport_);
    }

    demuxer_ = make_unique<Demuxer>(Root, transport_);
    requestReply_ = make_unique<RequestReply>(root, transport_, false /* dispatchOnTransportThread */);
}

EntreeServiceToProxyIpcChannel::~EntreeServiceToProxyIpcChannel()
{
    WriteInfo(TraceType, TraceId, "dtor");
}

ErrorCode EntreeServiceToProxyIpcChannel::OnOpen()
{
    WriteNoise(
        TraceType,
        TraceId,
        "Open");

    requestReply_->Open();

    // Disable connection idle timeout and session expiration, since IpcServer cannot connect back to IpcClient
    transport_->SetConnectionIdleTimeout(TimeSpan::Zero);
    transport_->DisableSecureSessionExpiration();

    transport_->DisableThrottle();

    // No need to set max incoming message size on transport_, as gateway alreadys does that

    demuxer_->SetReplyHandler(*requestReply_);

    auto root = Root.CreateComponentRoot();

    demuxer_->RegisterMessageHandler(
        Actor::EntreeServiceTransport,
        [this, root](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
           this->ProcessOrQueueIpcRequest(move(message), move(receiverContext));
        },
        true);

    auto error = demuxer_->Open();
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            TraceId,
            "Demuxer open failed : {0}",
            error);
        return error;
    }

    //
    // Currently FabricHost adds the user under which FabricGateway is activated to the WinfabAdminGroup.
    // The IPC server settings below needs to be updated if that is changed in future.
    //
    SecuritySettings ipcServerSecuritySettings;
    error = SecuritySettings::CreateNegotiateServer(
        FabricConstants::WindowsFabricAdministratorsGroupName,
        ipcServerSecuritySettings);
    
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            TraceId,
            "SecuritySettings.CreateNegotiateServer error={0}",
            error);

        return error;
    }

    error = transport_->SetSecurity(ipcServerSecuritySettings);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, 
            TraceId,
            "SetSecurity failed error={0}",
            error);

        return error;
    }

    auto perfCounter = JobQueuePerfCounters::CreateInstance(
        *ComponentName, 
        wformatString("{0}:{1}", nodeName_, nodeInstance_));

    requestJobQueue_ = make_unique<CommonTimedJobQueue<EntreeServiceToProxyIpcChannel>>(
        *ComponentName + nodeName_,
        *this,
        false,
        0,
        perfCounter,
        ServiceModelConfig::GetConfig().RequestQueueSize);

    error = transport_->Start();
    if (error.IsSuccess())
    {
        transportListenAddress_ = transport_->ListenAddress();
    }

    return error;
}

ErrorCode EntreeServiceToProxyIpcChannel::OnClose()
{
    ErrorCode error;
    Cleanup();
    if (transport_)
    {
        transport_->Stop();
    }

    if (demuxer_)
    {
        error = demuxer_->Close();
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    if (requestReply_)
    {
        requestReply_->Close();
    }

    return error;
}

void EntreeServiceToProxyIpcChannel::OnAbort()
{
    Cleanup();
    if (transport_)
    {
        transport_->Stop();
    }

    if (demuxer_)
    {
        demuxer_->Abort();
    }

    if (requestReply_)
    {
        requestReply_->Close();
    }
}

void EntreeServiceToProxyIpcChannel::Cleanup()
{
    if (demuxer_)
    {
        demuxer_->UnregisterMessageHandler(Actor::EntreeServiceTransport);
    }

    if (requestJobQueue_)
    {
        requestJobQueue_->Close();
    }
}

bool EntreeServiceToProxyIpcChannel::RegisterMessageHandler(
    Actor::Enum actor,
    BeginHandleMessageFromProxy const& beginFunction,
    EndHandleMessageFromProxy const& endFunction)
{
    bool ret = RegisterHandler(actor, beginFunction, endFunction);
    if (!ret) { return ret; }

    ComponentRootSPtr root = Root.CreateComponentRoot();
    demuxer_->RegisterMessageHandler(
        actor,
        [this, root](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
    {
        this->ProcessOrQueueIpcRequest(move(message), move(receiverContext));
    },
        true);

    return true;
}

bool EntreeServiceToProxyIpcChannel::UnregisterMessageHandler(
    Actor::Enum actor)
{
    bool ret = UnregisterHandler(actor);
    if (!ret) { return ret; }

    demuxer_->UnregisterMessageHandler(actor);
    return true;
}

//
// This method is called on the transport message dispatch thread. This method should do minimal work to return
// back to transport quickly.
//
void EntreeServiceToProxyIpcChannel::ProcessOrQueueIpcRequest(MessageUPtr &&message, ReceiverContextUPtr &&context)
{
    if (message->Actor == Actor::EntreeServiceTransport)
    {
        ProcessEntreeServiceTransportMessage(move(message), context);
    }
    else
    {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteWarning(TraceType, TraceId, "Action {0} missing timeout header", message->Action);
            OnIpcFailure(ErrorCodeValue::InvalidMessage, *context);
            return;
        }

        if (!requestJobQueue_->Enqueue(make_unique<ClientRequestJobItem>(move(message->Clone()), move(context), move(timeoutHeader.Timeout))))
        {
            WriteInfo(
                TraceType,
                TraceId,
                "Dropping message because JobQueue is full, QueueSize : {0}", requestJobQueue_->GetQueueLength());
        }
    }
}

void EntreeServiceToProxyIpcChannel::ProcessEntreeServiceTransportMessage(MessageUPtr &&message, Transport::ReceiverContextUPtr const& context)
{
    TimeSpan messageTimeout;
    ActivityId messageActivityId;

    TimeoutHeader timeoutHeader;
    if (!message->Headers.TryReadFirst(timeoutHeader))
    {
        WriteWarning(TraceType, TraceId, "Action {0} missing timeout header", message->Action);
        OnIpcFailure(ErrorCodeValue::InvalidMessage, *context);
        return;
    }
    messageTimeout = timeoutHeader.Timeout;

    FabricActivityHeader activityHeader;
    if (!message->Headers.TryReadFirst(activityHeader))
    {
        WriteWarning(TraceType, TraceId, "Action {0} missing activity header", message->Action);
        OnIpcFailure(ErrorCodeValue::InvalidMessage, *context);
        return;
    }
    messageActivityId = activityHeader.ActivityId;

    if (message->Action == NamingMessage::InitialHandshakeAction)
    {
        IpcHeader ipcHeader;
        if(!message->Headers.TryReadFirst(ipcHeader))
        {
            WriteWarning(
                TraceType,
                TraceId,
                "IpcHeader missing - messageId {0}, activityId {1}",
                message->TraceId(),
                messageActivityId);

            OnIpcFailure(ErrorCodeValue::InvalidMessage, *context);
            return;
        }

        //
        // We have seen cases where the tcp connection gets closed(not sure why) for some reason without the client
        // crashing or explicitly closing the connection. To handle such cases we proxy retries on connection disconnects
        // so we raise invoke the disconnection handler only when the connection happens with a different processid.
        //
        RaiseConnectionFaultHandlerIfNeeded(ipcHeader);

        {
            AcquireWriteLock writeLock(lock_);
            entreeServiceProxySendTarget_ = context->ReplyTarget;
            entreeServiceProxyClientId_ = ipcHeader.From;
            entreeServiceProxyProcessId_ = ipcHeader.FromProcessId;

            if (!entreeServiceProxySendTarget_->TryEnableDuplicateDetection())
            {
                WriteWarning(
                    TraceType,
                    TraceId,
                    "Duplicate detection for IPC channel failed");
            }

            WriteInfo(
                TraceType,
                TraceId,
                "Connected to entreeservice proxy - process id: {0}, Ipc client id: {1}",
                entreeServiceProxyProcessId_,
                entreeServiceProxyClientId_);
        }

        EntreeServiceIpcInitResponseMessageBody body(nodeInstance_, nodeName_);
        auto reply = NamingMessage::GetInitialHandshakeReply(body);
        SendIpcReply(move(reply), *context);
    }
    else
    {
        WriteWarning(
            TraceType,
            TraceId,
            "{0} Unknown action :{1}",
            messageActivityId,
            message->Action);
        OnIpcFailure(ErrorCodeValue::InvalidMessage, *context);
    }
}

void EntreeServiceToProxyIpcChannel::ProcessQueuedIpcRequest(MessageUPtr &&message, Transport::ReceiverContextUPtr && context, TimeSpan const &remainingTime)
{
    auto inner = AsyncOperation::CreateAndStart<ProcessIpcRequestAsyncOperation<Transport::ReceiverContext>>(
        *this,
        move(message),
        move(context),
        remainingTime,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnProcessIpcRequestComplete<Transport::ReceiverContext>(operation, false);
        },
        Root.CreateAsyncOperationRoot());

    OnProcessIpcRequestComplete<Transport::ReceiverContext>(inner, true);
}

void EntreeServiceToProxyIpcChannel::OnIpcFailure(ErrorCode const &error, Transport::ReceiverContext const &receiverContext)
{
    auto reply = NamingMessage::GetIpcOperationFailure(error, Actor::Enum::EntreeServiceProxy);
    SendIpcReply(move(reply), receiverContext);
}

void EntreeServiceToProxyIpcChannel::SetConnectionFaultHandler(ConnectionFaultHandler const & handler)
{
    AcquireWriteLock writeLock(lock_);
    ASSERT_IF(faultHandler_, "Handler already set");
    faultHandler_ = handler;
}

void EntreeServiceToProxyIpcChannel::RemoveConnectionFaultHandler()
{
    AcquireWriteLock writeLock(lock_);
    faultHandler_ = nullptr;
}

void EntreeServiceToProxyIpcChannel::RaiseConnectionFaultHandlerIfNeeded(IpcHeader &header)
{
    ConnectionFaultHandler handler = nullptr;
    {
        AcquireReadLock readLock(lock_);
        if (entreeServiceProxyProcessId_ != 0 && entreeServiceProxyProcessId_ != header.FromProcessId)
        {
            handler = faultHandler_;

            WriteInfo(
                TraceType,
                TraceId,
                "Disconnected from entreeservice proxy - process id: {0}, Ipc client id: {1}",
                entreeServiceProxyProcessId_,
                entreeServiceProxyClientId_);
        }

    }

    if (handler) { handler(ErrorCodeValue::CannotConnect); }
}

AsyncOperationSPtr EntreeServiceToProxyIpcChannel::BeginSendToProxy(
    MessageUPtr &&message,
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return requestReply_->BeginRequest(
        move(message),
        GetProxySendTarget(),
        timeout,
        callback,
        parent);
}

ErrorCode EntreeServiceToProxyIpcChannel::EndSendToProxy(
    AsyncOperationSPtr const &operation,
    MessageUPtr &reply)
{
    auto error = requestReply_->EndRequest(operation, reply);
    if (!error.IsSuccess())
    {
        return error; // when would this happen
    }

    if (reply->Action == NamingMessage::IpcFailureReplyAction)
    {
        // This could be an error in the client->proxy transport or some error given by client
        IpcFailureBody body;
        if (!reply->GetBody(body))
        {
            WriteWarning(
                TraceType,
                TraceId,
                "Unable to get IpcFailureMessageBody from message");
            return ErrorCodeValue::InvalidMessage;
        }

        error = body.TakeError();
    }

    return error;
}

ISendTarget::SPtr EntreeServiceToProxyIpcChannel::GetProxySendTarget()
{
    AcquireReadLock readLock(lock_);
    return entreeServiceProxySendTarget_;
}
