// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;
using namespace ServiceModel;
using namespace Naming;
using namespace SystemServices;

static const Common::StringLiteral TraceType("ProxyToEntreeServiceIpcChannel");

#define WriteWarning TextTraceComponent<TraceTaskCodes::EntreeServiceProxy>::WriteWarning
#define WriteInfo TextTraceComponent<TraceTaskCodes::EntreeServiceProxy>::WriteInfo
#define WriteNoise TextTraceComponent<TraceTaskCodes::EntreeServiceProxy>::WriteNoise

class ProxyToEntreeServiceIpcChannel::OpenAsyncOperation : public AsyncOperation
{
public:
    OpenAsyncOperation(ProxyToEntreeServiceIpcChannel & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.Root.TraceId,
            "Opening timeout : {0}",
            timeoutHelper_.GetRemainingTime());

        OpenIPC(thisSPtr);
    }

private:
    void OpenIPC(AsyncOperationSPtr const & thisSPtr)
    {
        Transport::SecuritySettings ipcClientSecuritySettings;
        auto error = SecuritySettings::CreateNegotiateClient(SecurityConfig::GetConfig().ClusterSpn, ipcClientSecuritySettings);
        owner_.SecuritySettings = ipcClientSecuritySettings;

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "SecuritySettings.CreateNegotiate failed with error: {0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        error = owner_.Open();
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "IPC Client Open failed with error: {0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        GetInitializationData(thisSPtr);
    }

    void GetInitializationData(AsyncOperationSPtr thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Getting initialization data");

        //
        // Create a channel(Priority::High) for server->client communication,
        //
        auto request = owner_.CreateInitializationRequest(timeoutHelper_.GetRemainingTime());
        auto operation = owner_.BeginRequest(
            move(request),
            TransportPriority::High,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation){ OnGetInitializationDataComplete(operation, false); },
            thisSPtr);
        OnGetInitializationDataComplete(operation, true);
    }

    void OnGetInitializationDataComplete(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.EndRequest(operation, reply);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "OnGetInitializationDataComplete: ErrorCode={0}", 
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        EntreeServiceIpcInitResponseMessageBody response;
        if (!reply->GetBody(response))
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "OnGetInitializationDataComplete, get response failed");
            TryComplete(operation->Parent, ErrorCodeValue::InvalidMessage);
            return;
        }

        owner_.instance_ = response.NodeInstance;
        owner_.nodeName_ = response.NodeName;

        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Established IPC communication with local fabric node name: {0}, instance : {1}",
            owner_.nodeName_,
            owner_.instance_);

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }

private:
    TimeoutHelper timeoutHelper_;
    ProxyToEntreeServiceIpcChannel & owner_;
};

class ProxyToEntreeServiceIpcChannel::CloseAsyncOperation : public AsyncOperation
{
public:
    CloseAsyncOperation(
        ProxyToEntreeServiceIpcChannel &owner,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    static ErrorCode CloseAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.TraceId,
            "Closing..");

        auto error = owner_.Close();
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                owner_.TraceId,
                "Failed to close ipcclient. Error {0}",
                error);
        }
        TryComplete(thisSPtr, error);
    }

private:
    TimeoutHelper timeoutHelper_;
    ProxyToEntreeServiceIpcChannel & owner_;
};

ProxyToEntreeServiceIpcChannel::ProxyToEntreeServiceIpcChannel(
    ComponentRoot const &root,
    wstring const &traceId,
    wstring const &ipcServerListenAddress,
    bool useUnreliable)
    : EntreeServiceIpcChannelBase(root)
    , IpcClient(
        root, 
        Guid::NewGuid().ToString(), 
        ipcServerListenAddress, 
        useUnreliable, 
        L"ProxyToEntreeServiceIpcChannel")
    , traceId_(traceId)
{
    WriteNoise(
        TraceType,
        traceId,
        "Ctor : ListenAddress {0}",
        ipcServerListenAddress);
}

ProxyToEntreeServiceIpcChannel::~ProxyToEntreeServiceIpcChannel()
{
    WriteNoise(
        TraceType,
        "dtor");
}

AsyncOperationSPtr ProxyToEntreeServiceIpcChannel::BeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);  
}

ErrorCode ProxyToEntreeServiceIpcChannel::EndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr ProxyToEntreeServiceIpcChannel::BeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);  
}

ErrorCode ProxyToEntreeServiceIpcChannel::EndClose(AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

AsyncOperationSPtr ProxyToEntreeServiceIpcChannel::BeginSendToEntreeService(
    MessageUPtr &&message,
    TimeSpan const &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return BeginRequest(
        move(message),
        TransportPriority::Normal,
        timeout,
        callback,
        parent);
}

ErrorCode ProxyToEntreeServiceIpcChannel::EndSendToEntreeService(
    AsyncOperationSPtr const &operation,
    __out MessageUPtr &reply)
{
    return EndRequest(
        operation,
        reply);
}

bool ProxyToEntreeServiceIpcChannel::RegisterNotificationsHandler(
    Actor::Enum actor,
    BeginHandleNotification const &beginFunction,
    EndHandleNotification const &endFunction)
{
    auto ret = RegisterHandler(actor, beginFunction, endFunction);
    if (!ret) { return ret; }

    ComponentRootSPtr root = Root.CreateComponentRoot();
    IpcClient::RegisterMessageHandler(
        actor,
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        {
            this->ProcessIpcRequest(move(message), move(receiverContext));
        },
        false);

    return true;
}

bool ProxyToEntreeServiceIpcChannel::UnregisterNotificationsHandler(Actor::Enum actor)
{
    auto ret = UnregisterHandler(actor);
    IpcClient::UnregisterMessageHandler(actor);
    return ret;
}

void ProxyToEntreeServiceIpcChannel::OnIpcFailure(ErrorCode const &error, ReceiverContext const &receiverContext)
{
    auto reply = NamingMessage::GetIpcOperationFailure(error);
    this->SendIpcReply(move(reply), receiverContext);
}

bool ProxyToEntreeServiceIpcChannel::IsValidIpcRequest(MessageUPtr &message)
{
    if (!message)
    {
        WriteInfo(TraceType, TraceId, "null request");
        return false;
    }

    TimeoutHeader timeoutHeader;
    if (!message->Headers.TryReadFirst(timeoutHeader))
    {
        WriteInfo(TraceType, TraceId, "Action {0} missing timeout header", message->Action);
        return false;
    }

    return true;
}

void ProxyToEntreeServiceIpcChannel::ProcessIpcRequest(MessageUPtr &&message, IpcReceiverContextUPtr &&context)
{
    if (!IsValidIpcRequest(message))
    {
        OnIpcFailure(ErrorCodeValue::InvalidMessage, *context);
        return;
    }

    auto timeoutHeader = TimeoutHeader::FromMessage(*message);
    auto inner = AsyncOperation::CreateAndStart<ProcessIpcRequestAsyncOperation<IpcReceiverContext>>(
        *this,
        move(message),
        move(context),
        timeoutHeader.Timeout,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnProcessIpcRequestComplete<IpcReceiverContext>(operation, false);
        },
        Root.CreateAsyncOperationRoot());

    OnProcessIpcRequestComplete<IpcReceiverContext>(inner, true);
}

AsyncOperationSPtr ProxyToEntreeServiceIpcChannel::BeginRequest(
    MessageUPtr && request,
    Transport::TransportPriority::Enum priority,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return IpcClient::BeginRequest(
        move(request),
        priority,
        timeout,
        callback,
        parent);
}

ErrorCode ProxyToEntreeServiceIpcChannel::EndRequest(
    AsyncOperationSPtr const & operation, 
    MessageUPtr & reply)
{
    auto error = IpcClient::EndRequest(operation, reply);
    if (!error.IsSuccess())
    {
        //
        // Socket send failures for the IPC channel should be communicated to
        // the client as a retriable error.
        //
        if (!error.IsError(ErrorCodeValue::Timeout))
        {
            return ErrorCodeValue::GatewayUnreachable;
        }

        return error;
    }

    if (reply->Action == NamingMessage::IpcFailureReplyAction)
    {
        IpcFailureBody body;
        if (!reply->GetBody(body))
        {
            WriteWarning(
                TraceType,
                TraceId,
                "Unable to get ClientOperationFailureMessageBody from message");
            return ErrorCodeValue::InvalidMessage;
        }

        error = body.TakeError();
    }

    return error;
}

void ProxyToEntreeServiceIpcChannel::UpdateTraceId(wstring const &newId)
{
    WriteInfo(
        TraceType,
        TraceId,
        "Updating Trace id, old : {0} new : {1}",
        traceId_,
        newId);

    traceId_ = newId;
}

MessageUPtr ProxyToEntreeServiceIpcChannel::CreateInitializationRequest(TimeSpan const &timeout)
{
    auto request = NamingMessage::GetInitialHandshake();
    request->Headers.Replace(TimeoutHeader(timeout));
    request->Headers.Replace(FabricActivityHeader(Guid::NewGuid()));
    request->Headers.Replace(IpcHeader(ClientId, ProcessId));

    return move(request);
}

void ProxyToEntreeServiceIpcChannel::Reconnect()
{
    WriteInfo(
        TraceType,
        TraceId,
        "OnDisconect, Reconnecting to node {0}",
        NodeName);

    auto op = IpcClient::SendOneWay(move(CreateInitializationRequest(TimeSpan::MaxValue)), TimeSpan::MaxValue, TransportPriority::High);
}
