// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Naming;
using namespace Common;
using namespace Transport;
using namespace std;
using namespace ClientServerTransport;

StringLiteral const TraceType("ProcessRequest");

#define IS_NOTIFICATION_ACTION(Action)                                              \
    ((Action == NamingTcpMessage::NotificationClientConnectionRequestAction) ||     \
    (Action == NamingTcpMessage::NotificationClientSynchronizationRequestAction) || \
    (Action == NamingTcpMessage::RegisterServiceNotificationFilterRequestAction) || \
    (Action == NamingTcpMessage::UnregisterServiceNotificationFilterRequestAction))

EntreeServiceProxy::RequestAsyncOperationBase::RequestAsyncOperationBase(
    __in EntreeServiceProxy &owner,
    MessageUPtr && receivedMessage,
    ISendTarget::SPtr const &to,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : ActivityTracedComponent(owner.InstanceString, FabricActivityHeader::FromMessage(*receivedMessage))
    , TimedAsyncOperation(timeout, callback, parent)
    , owner_(owner)
    , receivedMessage_(move(receivedMessage))
    , to_(to)
{
}

void EntreeServiceProxy::RequestAsyncOperationBase::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    if (receivedMessage_->Action == NamingTcpMessage::PingRequestAction)
    {
        GatewayDescription gateway(
            Owner.ReceivingChannel->ListenAddress(),
            Owner.Instance,
            Owner.NodeName);

        MessageUPtr reply = NamingTcpMessage::GetGatewayPingReply(Common::make_unique<PingReplyMessageBody>(move(gateway)))->GetTcpMessage();
        reply->Headers.Add(*ClientProtocolVersionHeader::CurrentVersionHeader);

        SetReplyAndComplete(
            thisSPtr,
            move(reply),
            ErrorCodeValue::Success);

    }
    else if (IS_NOTIFICATION_ACTION(receivedMessage_->Action))
    {
        auto op = owner_.notificationManagerProxyUPtr_->BeginProcessRequest(
            move(receivedMessage_),
            to_,
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                OnNotificationComplete(operation, false);
            },
            thisSPtr);

        OnNotificationComplete(op, true);
    }
    else
    {
        receivedMessage_->Headers.Replace(TimeoutHeader(RemainingTime));
        auto operation = owner_.BeginProcessRequest(
            move(receivedMessage_),
            to_,
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestComplete(operation, false);
            },
            thisSPtr);

        OnRequestComplete(operation, true);
    }
}

void EntreeServiceProxy::RequestAsyncOperationBase::OnRequestComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    auto error = owner_.EndProcessRequest(operation, reply);
    SetReplyAndComplete(operation->Parent, move(reply), error);
}

void EntreeServiceProxy::RequestAsyncOperationBase::OnNotificationComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    auto error = owner_.notificationManagerProxyUPtr_->EndProcessRequest(operation, reply);
    SetReplyAndComplete(operation->Parent, move(reply), error);
}

void EntreeServiceProxy::RequestAsyncOperationBase::SetReplyAndComplete(
    AsyncOperationSPtr const &thisSPtr,
    MessageUPtr &&reply,
    ErrorCode const &error)
{
    reply_ = move(reply);
    TryComplete(thisSPtr, error);
}

ErrorCode EntreeServiceProxy::RequestAsyncOperationBase::End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
{
    auto casted = AsyncOperation::End<RequestAsyncOperationBase>(operation);
    if (casted->Error.IsSuccess())
    {
        reply = move(casted->reply_);
    }

    return casted->Error;
}
