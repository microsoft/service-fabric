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

static const Common::StringLiteral TraceType("EntreeServiceTransport");

class EntreeServiceTransport::NotificationRequestAsyncOperation
    : public TimedAsyncOperation
{
public:
    NotificationRequestAsyncOperation(
        EntreeServiceTransport &owner,
        MessageUPtr &&request,
        TimeSpan timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , request_(move(request))
        , passThroughTarget_(false)
    {
        //
        // Passthrough send target needs to be special cased.
        // It is identified by an empty address in the ClientIdentityHeader
        //
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        AsyncOperationSPtr operation;
        if (passThroughTarget_)
        {
            //TODO
            Assert::TestAssert("Push notification for passthrough fabricclient not supported");
        }
        else
        {
            request_->Headers.Replace(TimeoutHeader(RemainingTime));
            operation = owner_.ipcToProxy_.BeginSendToProxy(
                move(request_),
                RemainingTime,
                [this](AsyncOperationSPtr const &operation)
                {
                    this->OnComplete(operation, false);
                },
                thisSPtr);
        }
        OnComplete(operation, true);
    }

    void OnComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error;
        if (passThroughTarget_)
        {
            // TODO
            Assert::TestAssert("Push notification for passthrough fabricclient not supported");
        }
        else
        {
            // Get reply from proxy
            error = owner_.ipcToProxy_.EndSendToProxy(operation, reply_);
            if (!error.IsSuccess() && error.IsError(ErrorCodeValue::NotFound))
            {
                // TODO: this case happens when the proxy is not known or has just been disconnected, 
                // can this be handled better
                error = ErrorCodeValue::CannotConnectToAnonymousTarget;
            }
        }

        TryComplete(operation->Parent, error);
    }

    static ErrorCode End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<NotificationRequestAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }

        return casted->Error;
    }

private:

    bool passThroughTarget_;
    EntreeServiceTransport &owner_;
    MessageUPtr request_;
    ISendTarget::SPtr target_;
    MessageUPtr reply_;
};

EntreeServiceTransport::EntreeServiceTransport(
    ComponentRoot const &root,
    wstring const &listenAddress,
    Federation::NodeInstance const &instance,
    wstring const &nodeName,
    GatewayEventSource const &eventSource,
    bool useUnreliable)
    : NodeTraceComponent(instance)
    , ipcToProxy_(
        root,
        listenAddress,
        instance,
        nodeName,
        eventSource,
        useUnreliable)
{
}

ErrorCode EntreeServiceTransport::OnOpen()
{
    auto  error = ipcToProxy_.Open();
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceType,
            "Open failed : {0}",
            error);
    }

    return error;
}

ErrorCode EntreeServiceTransport::OnClose()
{
    ipcToProxy_.Close();

    return ErrorCodeValue::Success;
}

void EntreeServiceTransport::OnAbort()
{
    ipcToProxy_.Abort();
}

void EntreeServiceTransport::RegisterMessageHandler(
    Actor::Enum actor,
    EntreeServiceToProxyIpcChannel::BeginHandleMessageFromProxy const& beginFunction,
    EntreeServiceToProxyIpcChannel::EndHandleMessageFromProxy const& endFunction)
{
    ipcToProxy_.RegisterMessageHandler(actor, beginFunction, endFunction);
}

void EntreeServiceTransport::UnregisterMessageHandler(Actor::Enum actor)
{
    ipcToProxy_.UnregisterMessageHandler(actor);
}

AsyncOperationSPtr EntreeServiceTransport::Test_BeginRequestReply(
    MessageUPtr &&request,
    ISendTarget::SPtr const &target,
    TimeSpan timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return ipcToProxy_.GetInnerRequestReply()->BeginRequest(
        move(request),
        target,
        timeout,
        callback,
        parent);
}

ErrorCode EntreeServiceTransport::Test_EndRequestReply(
    AsyncOperationSPtr const &operation,
    MessageUPtr &reply)
{
    return ipcToProxy_.GetInnerRequestReply()->EndRequest(operation, reply);
}

AsyncOperationSPtr EntreeServiceTransport::BeginNotification(
    MessageUPtr &&request,
    TimeSpan timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    //
    // This asyncoperation does the special casing for passthrough transport.
    //
    return AsyncOperation::CreateAndStart<NotificationRequestAsyncOperation>(
        *this,
        move(request),
        timeout,
        callback,
        parent);
}

ErrorCode EntreeServiceTransport::EndNotification(
    AsyncOperationSPtr const &operation,
    MessageUPtr &reply)
{
    return NotificationRequestAsyncOperation::End(operation, reply);
}

void EntreeServiceTransport::SetConnectionFaultHandler(EntreeServiceToProxyIpcChannel::ConnectionFaultHandler const & handler)
{
    ipcToProxy_.SetConnectionFaultHandler(handler);
}

void EntreeServiceTransport::RemoveConnectionFaultHandler()
{
    ipcToProxy_.RemoveConnectionFaultHandler();
}
