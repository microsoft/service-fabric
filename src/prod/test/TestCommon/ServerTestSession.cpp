// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TestCommon;
using namespace std;
using namespace Common;
using namespace Transport;

StringLiteral const TraceSource = "ServerTestSession";

ServerTestSession::ServerTestSession(
    wstring const& listenAddress, 
    wstring const& label, 
    Common::TimeSpan maxIdleTime,
    bool autoMode, 
    TestDispatcher& dispatcher, 
    std::wstring const& dispatcherHelpFileName)
    : TestSession(label, autoMode, dispatcher, dispatcherHelpFileName),
    lock_(),
    ipcServer_(),
    clientMap_(),
    isClosed_(true),
    maxIdleTime_(maxIdleTime)
{
    ipcServer_ = make_unique<IpcServer>(
        *this,
        listenAddress,
        L"ServerTestSession",
        false /* disallow use of unreliable transport */,
        L"ServerTestSession");
}

void ServerTestSession::ExecuteCommandAtClient(wstring const& command, wstring const& clientId)
{
    AcquireExclusiveLock grab(lock_);
    auto iterator = clientMap_.find(clientId);
    if(iterator == clientMap_.end()) 
    {
        //Possible if client disconnected just before calling method
        TestSession::WriteNoise(TraceSource, "Command {0} to client {1} dropped", command, clientId);
        return;
    }

    MessageUPtr message = DistributedSessionMessage::GetExecuteCommandMessage(command);
    ipcServer_->SendOneWay(iterator->first, move(message));
}

void ServerTestSession::ExecuteCommandAtAllClients(wstring const& command)
{
    AcquireExclusiveLock grab(lock_);
    for (auto iterator = clientMap_.begin(); iterator != clientMap_.end(); iterator++)
    {
        MessageUPtr message = DistributedSessionMessage::GetExecuteCommandMessage(command);
        ipcServer_->SendOneWay(iterator->first, move(message));
    }
}

bool ServerTestSession::OnOpenSession()
{
    TestSession::WriteInfo(TraceSource, "Open Session at {0}", ipcServer_->TransportListenAddress);
    AcquireExclusiveLock grab(lock_);
    isClosed_ = false;
    auto errorCode = ipcServer_->Open();
    if(errorCode.IsSuccess())
    {
        auto root = this->CreateComponentRoot();
        ipcServer_->RegisterMessageHandler(
            DistributedSessionMessage::Actor,
            [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & context)
        {
            this->ProcessClientMessage(message, context);
        },
            true);
    }

    return errorCode.IsSuccess();
}

void ServerTestSession::ProcessClientMessage(Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr & context)
{
    Actor::Enum actor = message->Actor;
    TestSession::FailTestIfNot(actor == DistributedSessionMessage::Actor, "Incorrect dispatch by IPC. Actual actor = {0}", actor);
    wstring const& action = message->Action;
    if(action == DistributedSessionMessage::ConnectAction)
    {
        {
            AcquireExclusiveLock grab(lock_);
            ConnectMessageBody connectMessageBody;
            message->GetBody(connectMessageBody);
            TestSession::WriteNoise(TraceSource, "New Connection message from Client {0} with PID {1}", context->From, connectMessageBody.ProcessId);

            bool processExitedBeforeOpen = false;
            bool success = StartMonitoringClient(connectMessageBody.ProcessId, context->From, processExitedBeforeOpen);

            // Fix RDBug 11139724.
            // If process already exited when OpenProcess is called in StartMonitoringClient, then OpenProcess failure is expected so we will not fail the test. 
            TestSession::FailTestIf(!success && !isClosed_ && !processExitedBeforeOpen, "Client {0} with PID {1} failed to start monitoring", context->From, connectMessageBody.ProcessId);
        }

        OnClientConnection(context->From);

        MessageUPtr reply = DistributedSessionMessage::GetConnectReplyMessage();
        context->Reply(move(reply));
        TestSession::WriteNoise(TraceSource, "Sent reply to Client {0}", context->From);
    }
    else
    {
        {
            AcquireExclusiveLock grab(lock_);
            if(clientMap_.find(context->From) == clientMap_.end())
            {
                // Client must have already failed so ignore message
                TestSession::WriteInfo(TraceSource, "{0} message for non existent connection {1}", action, context->From);
                return;
            }
        }

        ClientDataBody body;
        message->GetBody(body);
        TestSession::WriteNoise(TraceSource, "Dispatching client data message with Action {0} from {1}", action, context->From);
        OnClientDataReceived(action, body.ClientData, context->From);
    }
}

bool ServerTestSession::StartMonitoringClient(DWORD processId, wstring const& clientId, __out bool & processExitedBeforeOpen)
{
    HandleUPtr processHandle;
    DWORD osErrorCode = 0;
    auto errorCode = ProcessUtility::OpenProcess(
        SYNCHRONIZE,
        FALSE,
        processId,
        processHandle,
        osErrorCode);

    if (!errorCode.IsSuccess())
    {
        // If osErrorCode is ERROR_INVALID_PARAMETER after OpenProcess call, it means processId is invalid because the other 2 args are always valid. 
        // In this case, the process already exited before OpenProcess call and we set processExitedBeforeOpen to true. 
        if (osErrorCode == ERROR_INVALID_PARAMETER)
        {
            processExitedBeforeOpen = true;
        }
        
        TestSession::WriteError(TraceSource, "OpenProcess for monitoring failed with error {0}", errorCode);
        return false;
    }

    auto root = this->CreateComponentRoot();
    auto localClientId = clientId;
    ProcessWaitSPtr proccessMonitor = ProcessWait::CreateAndStart(
        move(*processHandle),
        processId,
        [this, root, localClientId](pid_t processId, ErrorCode const &, DWORD)
    {
        OnClientProcessTerminate(processId, localClientId);
    });

    TestSession::FailTestIf(clientId.empty(), "Client id is empty");

    TestSession::FailTestIfNot(clientMap_.find(clientId) == clientMap_.end(), "Process {0} already being monitored", processId);
    clientMap_.insert(make_pair(clientId, move(proccessMonitor)));

    return true;
}

void ServerTestSession::OnClientProcessTerminate(DWORD processId, wstring const& clientId)
{
    TestSession::WriteNoise(TraceSource, "OnClientProcessTerminate invoked for id {0}:{1}", clientId, processId);
    if(!isClosed_)
    {
        {
            AcquireExclusiveLock grab(lock_);
            TestSession::FailTestIf(clientMap_.find(clientId) == clientMap_.end(), "Client {0} not found in OnClientProcessTerminate", clientId);
            clientMap_.erase(clientId);
        }

        OnClientFailure(clientId);
    }
}

void ServerTestSession::OnCloseSession()
{
    AcquireExclusiveLock grab(lock_);
    isClosed_ = false;
    ipcServer_->UnregisterMessageHandler(DistributedSessionMessage::Actor);
    auto errorCode = ipcServer_->Close();
    if(!errorCode.IsSuccess())
    {
        ipcServer_->Abort();
    }
}
