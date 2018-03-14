// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace std;
using namespace Common;
using namespace Transport;

StringLiteral const TraceSource = "ClientTestSession";

int const ClientTestSession::ConnectTimeoutInSeconds = 120;

ClientTestSession::ClientTestSession(
    wstring const& serverAddress, 
    wstring const& clientId,
    wstring const& label, 
    bool autoMode, 
    TestDispatcher& dispatcher, 
    std::wstring const& dispatcherHelpFileName)
    : TestSession(label, autoMode, dispatcher, dispatcherHelpFileName),
    ipcClient_(),
    clientId_(clientId),
    commands_(ReaderQueue<wstring>::Create()),
    isClosed_(true)
{
    ipcClient_ = make_unique<IpcClient>(
        *this,
        clientId,
        serverAddress,
        false /* disallow use of unreliable transport */,
        L"ClientTestSession");
}

wstring ClientTestSession::GetInput()
{
    auto waiter = make_shared<AsyncOperationWaiter>();
    auto operation = commands_->BeginDequeue(
        TimeSpan::MaxValue, 
        [this, waiter] (AsyncOperationSPtr const &) 
    { 
        waiter->Set();
    },
        AsyncOperationSPtr());

    waiter->WaitOne();
    unique_ptr<wstring> commandUPtr;
    ErrorCode error = commands_->EndDequeue(operation, commandUPtr);
    TestSession::FailTestIfNot(error.IsSuccess(), "commands_->EndDequeue failed with an error {0}", error);

    if(!commandUPtr)
    {
        TestSession::FailTestIfNot(isClosed_.load(), "Client Session is not closed but received null from ReaderQueue");
        TestSession::WriteInfo(TraceSource, clientId_, "Received null command. Session ending");
        return wstring();
    }

    return *commandUPtr;
}

void ClientTestSession::ReportClientData(wstring const& dataType, vector<byte> && data)
{
    MessageUPtr message = DistributedSessionMessage::GetReportClientDataMessage(dataType, move(data));
    TestSession::WriteNoise(TraceSource, clientId_, "Sending ReportClientData message with type {0}", dataType);
    ipcClient_->SendOneWay(move(message));
}

bool ClientTestSession::OnOpenSession()
{
    TestSession::WriteInfo(TraceSource, clientId_, "Open Client Session");
    isClosed_.store(false);
    auto errorCode = ipcClient_->Open();
    if(errorCode.IsSuccess())
    {
        auto root = this->CreateComponentRoot();
        ipcClient_->RegisterMessageHandler(
            DistributedSessionMessage::Actor,
            [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & context)
        {
            this->ProcessServerMessage(message, context);
        },
            true);
    }

    // Send connect message and wait for reply.
    MessageUPtr message = DistributedSessionMessage::GetConnectMessage();
    ManualResetEvent resetEvent(false);
    TimeSpan connectTimeout(TimeSpan::FromSeconds(ClientTestSession::ConnectTimeoutInSeconds));
    auto operation = ipcClient_->BeginRequest(
        move(message),
        connectTimeout,
        [&resetEvent] (AsyncOperationSPtr const&) -> void 
    { 
        resetEvent.Set();
    });

    resetEvent.WaitOne();

    MessageUPtr reply;
    errorCode = ipcClient_->EndRequest(operation, reply);

    if(errorCode.IsSuccess())
    {
        ASSERT_IFNOT(
            reply->Action == DistributedSessionMessage::ConnectReplyAction,
            "Incorrect action {0} received on connect reply",
            reply->Action);
    }

    return errorCode.IsSuccess();
}

void ClientTestSession::ProcessServerMessage(Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr &)
{
    Actor::Enum actor = message->Actor;
    TestSession::FailTestIfNot(actor == DistributedSessionMessage::Actor, "Incorrect dispatch by IPC. Actual actor = {0}", actor);
    wstring const& action = message->Action;
    if (action == DistributedSessionMessage::ExecuteCommandAction)
    {
        StringBody body;
        message->GetBody(body);
        bool success = commands_->EnqueueAndDispatch(make_unique<wstring>(body.String));

        if(success)
        {
            TestSession::WriteNoise(TraceSource, clientId_, "Enqueued command {0}", body.String);
        }
        else
        {
            TestSession::WriteInfo(TraceSource, clientId_, "Dropping command {0} since session is closed", body.String);
        }
    }
    else
    {
        TestSession::FailTest("Invalid action '{0}' at ClientSession", action);
    }
}

void ClientTestSession::OnCloseSession()
{
    isClosed_.store(true);

    TestSession::WriteInfo(TraceSource, clientId_, "ClientTestSession::OnCloseSession invoked");

    ipcClient_->UnregisterMessageHandler(DistributedSessionMessage::Actor);
    auto errorCode = ipcClient_->Close();
    commands_->Close();
    if(!errorCode.IsSuccess())
    {
        ipcClient_->Abort();
    }
}
