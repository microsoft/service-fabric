// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Client;
using namespace ClientServerTransport;
using namespace ServiceModel;
using namespace Naming;

static const Common::StringLiteral TraceType("ClientServerPassThroughTransport");
const GlobalWString ClientServerPassThroughTransport::FriendlyName = make_global<wstring>(L"PassThroughFabricClient");

class ClientServerPassThroughTransport::RequestReplyAsyncOperation
    : public TimedAsyncOperation
{
public:
    RequestReplyAsyncOperation(
        ClientServerPassThroughTransport &owner,
        ClientServerRequestMessageUPtr &&message,
        TimeSpan const &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , message_(move(message))
    {
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        auto op = owner_.namingMessageProcessorSPtr_->BeginProcessRequest(
            move(message_->GetTcpMessage()),
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestReplyComplete(operation, false);
            },
            thisSPtr);

        OnRequestReplyComplete(op, true);
    }

    void OnRequestReplyComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = owner_.namingMessageProcessorSPtr_->EndProcessRequest(operation, reply);
        if (!error.IsSuccess())
        {
            reply = move(NamingTcpMessage::GetClientOperationFailureReply(move(error))->GetTcpMessage());
        }

        reply_ = move(make_unique<ClientServerReplyMessage>(move(reply)));

        //
        // Response to request is currently dispatched by entreeservice in the same transport thread, so if this thread blocks,
        // it would prevent further responses from the transport queue from being dispatched. So this completion
        // is posted to threadpool.
        //
        auto thisSPtr = operation->Parent;
        Threadpool::Post([thisSPtr]()
        {
            //
            // Send is always successful for passthrough transport
            //
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
        });
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out ClientServerReplyMessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<RequestReplyAsyncOperation>(operation);
        reply = move(casted->reply_);

        return casted->Error;
    }

private:
    ClientServerPassThroughTransport &owner_;
    ClientServerRequestMessageUPtr message_;
    ClientServerReplyMessageUPtr reply_;
};

class ClientServerPassThroughTransport::ProcessNotificationAsyncOperation
    : public TimedAsyncOperation
{
public:
    ProcessNotificationAsyncOperation(
        ClientServerPassThroughTransport &owner,
        Transport::MessageUPtr &&message,
        TimeSpan const &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , message_(move(message))
    {
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        //
        // If there is no handler registered, the operation times out.
        //
        owner_.InvokeHandler(thisSPtr, move(message_));
    }

    static ErrorCode End(AsyncOperationSPtr const &operation, __out ClientServerReplyMessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<ProcessNotificationAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

    void SetReply(MessageUPtr &&reply) { reply_ = move(make_unique<ClientServerReplyMessage>(move(reply))); }

private:
    ClientServerPassThroughTransport &owner_;
    Transport::MessageUPtr message_;
    ClientServerReplyMessageUPtr reply_;
};


ClientServerPassThroughTransport::ClientServerPassThroughTransport(
    Common::ComponentRoot const &root,
    std::wstring const &traceContext,
    std::wstring const &owner,
    Naming::INamingMessageProcessorSPtr const &namingMessageProcessorSPtr)
    : ClientServerTransport(root)
    , traceContext_(traceContext)
    , notificationHandlerLock_()
    , notificationHandler_(nullptr)
    , namingMessageProcessorSPtr_(namingMessageProcessorSPtr)
{
    //
    // Passthrough send target doesnt have a target address.
    //
    sendTarget_ = make_shared<PassThroughSendTarget>(L"", owner, *this);
    
    WriteInfo(
        TraceType,
        traceContext_,
        "Created for Owner:{0} on Node:{1}",
        owner,
        namingMessageProcessorSPtr->Id);
}

ErrorCode ClientServerPassThroughTransport::OnOpen()
{
    return ErrorCodeValue::Success;
}

ErrorCode ClientServerPassThroughTransport::OnClose()
{
    return ErrorCodeValue::Success;
}

void ClientServerPassThroughTransport::OnAbort()
{
}

void ClientServerPassThroughTransport::SetConnectionFaultHandler(IDatagramTransport::ConnectionFaultHandler const &)
{
    //
    // There is no connection fault in this transport. Since the notification handling mechanism in the 
    // gateway currently relies on a connection fault to cleanup the registered notifications, notifications 
    // registered via the passthrough clients would remain, incase the passthrough client goes away. It is
    // ok currently, since the passthrough client's lifetime is tied to the fabricnode.
    //
    return;
}

void ClientServerPassThroughTransport::RemoveConnectionFaultHandler()
{
    return;
}

ErrorCode ClientServerPassThroughTransport::SetSecurity(SecuritySettings const &)
{
    //
    // Since send and receive are just direct function calls, no need for security.
    //
    return ErrorCodeValue::Success;
}

ErrorCode ClientServerPassThroughTransport::SetKeepAliveTimeout(TimeSpan const &)
{
    return ErrorCodeValue::Success;
}

ISendTarget::SPtr ClientServerPassThroughTransport::ResolveTarget(
    wstring const &,
    wstring const &,
    uint64 )
{
    return sendTarget_;
}

ISendTarget::SPtr ClientServerPassThroughTransport::ResolveTarget(
    wstring const &,
    wstring const &,
    wstring const &)
{
    return sendTarget_;
}

void ClientServerPassThroughTransport::RegisterMessageHandler(
    Actor::Enum ,
    Demuxer::MessageHandler const &,
    bool)
{
    //
    // This transport only supports the actor NamingGateway. The only other actor we register from
    // fabricclient is for the FileTransferGateway. That functionality isnt needed for fabricclients
    // running within fabric.exe. So this is a no-op.
    //
    return;
}

void ClientServerPassThroughTransport::UnregisterMessageHandler(Actor::Enum)
{
    return;
}

void ClientServerPassThroughTransport::SetNotificationHandler(DuplexRequestReply::NotificationHandler const & handler, bool)
{
    AcquireWriteLock writeLock(notificationHandlerLock_);
    notificationHandler_ = handler;
}

void ClientServerPassThroughTransport::RemoveNotificationHandler()
{
    AcquireWriteLock writeLock(notificationHandlerLock_);
    notificationHandler_ = nullptr;
}

AsyncOperationSPtr ClientServerPassThroughTransport::BeginRequestReply(
    ClientServerRequestMessageUPtr && request,
    ISendTarget::SPtr const &to,
    TransportPriority::Enum priority,
    TimeSpan timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    UNREFERENCED_PARAMETER(to);
    UNREFERENCED_PARAMETER(priority);

    request->Headers.Replace(ClientIdentityHeader(L"", *FriendlyName));
    return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
        *this,
        move(request),
        timeout,
        callback,
        parent);
}

ErrorCode ClientServerPassThroughTransport::EndRequestReply(
    AsyncOperationSPtr const & operation,
    ClientServerReplyMessageUPtr & reply)
{
    return RequestReplyAsyncOperation::End(operation, reply);
}

ErrorCode ClientServerPassThroughTransport::SendOneWay(
    ISendTarget::SPtr const & target,
    ClientServerRequestMessageUPtr && message,
    Common::TimeSpan expiration,
    Transport::TransportPriority::Enum priority)
{
    UNREFERENCED_PARAMETER(target);
    UNREFERENCED_PARAMETER(priority);

    message->Headers.Replace(ClientIdentityHeader(L"", *FriendlyName));
    // TODO: Expiration could be maxvalue, sanitize it.
    namingMessageProcessorSPtr_->BeginProcessRequest(
        move(message->GetTcpMessage()),
        expiration,
        [this](AsyncOperationSPtr const &op)
        {
            MessageUPtr message;
            this->namingMessageProcessorSPtr_->EndProcessRequest(op, message);
        },
        Root.CreateAsyncOperationRoot());
    
    //
    // Send is always successful for passthrough transport
    //
    return ErrorCodeValue::Success;
}

AsyncOperationSPtr ClientServerPassThroughTransport::BeginReceiveNotification(
    MessageUPtr && message,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ProcessNotificationAsyncOperation>(
        *this,
        move(message),
        timeout,
        callback,
        parent);
}
    
ErrorCode ClientServerPassThroughTransport::EndReceiveNotification(
    AsyncOperationSPtr const & operation,
    ClientServerReplyMessageUPtr & reply)
{
    return ProcessNotificationAsyncOperation::End(operation, reply);
}

void ClientServerPassThroughTransport::InvokeHandler(
    AsyncOperationSPtr const & operation,
    MessageUPtr && message)
{
    ReceiverContextUPtr receiverContext = make_unique<PassThroughReceiverContext>(
        operation,
        message->MessageId,
        sendTarget_,
        *this);

    DuplexRequestReply::NotificationHandler handler = nullptr;
    {
        AcquireReadLock readLock(notificationHandlerLock_);
        handler = notificationHandler_;
    }

    if (handler)
    {
        handler(*message, receiverContext);
    }
    else
    {
        WriteWarning(
            TraceType,
            traceContext_,
            "Notification message Action:{0} being sent when no handler's are registered on the client",
            message->Action);
        
        Assert::TestAssert(
            "Notification message Action:{0} being sent when no handler's are registered on the client",
            message->Action);
    }
}

void ClientServerPassThroughTransport::FinishNotification(
    AsyncOperationSPtr const & operation,
    MessageUPtr & message)
{
    auto notificationOperation = AsyncOperation::Get<ProcessNotificationAsyncOperation>(operation);
    notificationOperation->SetReply(move(message));
    operation->TryComplete(operation, ErrorCodeValue::Success);
}
