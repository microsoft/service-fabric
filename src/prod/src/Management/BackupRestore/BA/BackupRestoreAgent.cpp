// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Management;
using namespace Transport;
using namespace Hosting2;
using namespace Api;
using namespace BackupRestoreAgentComponent;

BackupRestoreAgent::BackupRestoreAgent(
    Common::ComponentRoot const & root,
    IHostingSubsystemSPtr hosting,
    FederationWrapperUPtr && federationWrapper,
    ServiceAdminClient & serviceAdminClient,
    IQueryClientPtr queryClientPtr,
    IpcServer & ipcServer,
    IBaToBapTransportUPtr && bapTransport,
    wstring const & workingDirectory) :
    Common::RootedObject(root),
    hosting_(hosting),
    federationWrapper_(move(federationWrapper)),
    serviceAdminClient_(serviceAdminClient),
    queryClientPtr_(queryClientPtr),
    ipcServer_(ipcServer),
    bapTransport_(move(bapTransport)),
    nodeInstanceIdStr_(federationWrapper_->InstanceIdStr),
    workingDir_(workingDirectory)
{
    BAEventSource::Events->Ctor(NodeInstanceIdStr);
    
    // Create BackupCopierProxy.
    backupCopierProxy_ = BackupCopierProxy::Create(
        Environment::GetExecutablePath(),
        workingDir_,
        federationWrapper_->Instance
    );
}

BackupRestoreAgent::~BackupRestoreAgent()
{
    BAEventSource::Events->Dtor(NodeInstanceIdStr);
}

BackupRestoreAgentUPtr BackupRestoreAgent::Create(
    Common::ComponentRoot const & root, 
    Hosting2::IHostingSubsystemSPtr hosting,
    Federation::FederationSubsystem& federationSubsystem,
    Reliability::ServiceResolver & serviceResolver,
    Reliability::ServiceAdminClient & adminclient,
    Api::IQueryClientPtr queryClientPtr,
    Transport::IpcServer & ipcServer,
    std::wstring const & workingDirectory)
{
    IBaToBapTransportUPtr bapTransport = make_unique<BaToBapTransport>(ipcServer);
    FederationWrapperUPtr federationWrapper = make_unique<BAFederationWrapper>(federationSubsystem, serviceResolver, queryClientPtr);
    return make_unique<BackupRestoreAgent>(root, hosting, move(federationWrapper), adminclient, queryClientPtr, ipcServer, std::move(bapTransport), workingDirectory);
}

#pragma region LifeCycle

ErrorCode BackupRestoreAgent::Open()
{
    BAEventSource::Events->Open(NodeInstanceIdStr);

    RegisterIpcMessageHandler();

    RegisterFederationSubsystemEvents();

    ErrorCode result = ErrorCode::Success();
    
    return result;
}

void BackupRestoreAgent::Abort()
{
    BAEventSource::Events->Abort(NodeInstanceIdStr);
}

void BackupRestoreAgent::Close()
{
    BAEventSource::Events->Close(NodeInstanceIdStr);
}

#pragma endregion

void BackupRestoreAgent::RegisterFederationSubsystemEvents()
{
    auto root = Root.CreateComponentRoot();
    federationWrapper_->RegisterFederationSubsystemEvents(
        [this, root](MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
        {
            this->ProcessTransportRequest(message, move(oneWayReceiverContext));
        },
            [this, root](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        {
            this->ProcessTransportRequestResponseRequest(message, move(requestReceiverContext));
        });

    BAEventSource::Events->RegisterWithFederation(NodeInstanceIdStr);
}

void BackupRestoreAgent::RegisterIpcMessageHandler()
{
    auto root = Root.CreateComponentRoot();
    BAPTransport.RegisterMessageHandler(
        Actor::BA, 
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & receiverContext)
        { 
            this->ProcessTransportIpcRequest(message, move(receiverContext));
        },
        false /*dispatchOnTransportThread*/);

    BAEventSource::Events->RegisterIpcMessageHandler(NodeInstanceIdStr);
}

void BackupRestoreAgent::ProcessTransportRequest(MessageUPtr & /*message*/, OneWayReceiverContextUPtr && /*oneWayReceiverContext*/)
{
    // No action
}

void BackupRestoreAgent::ProcessTransportRequestResponseRequest(MessageUPtr & message, RequestReceiverContextUPtr && requestReceiverContext)
{
    // Incoming message over federation
    auto activityId = FabricActivityHeader::FromMessage(*message).ActivityId;
    BAEventSource::Events->ProcessTransportRequestResponseRequest(nodeInstanceIdStr_, activityId, message->MessageId);

    auto timeoutHeader = TimeoutHeader::FromMessage(*message);

    auto operation = AsyncOperation::CreateAndStart<NodeToServiceAsyncOperation>(
        move(message),
        move(requestReceiverContext),
        ipcServer_,
        serviceAdminClient_,
        hosting_,
        timeoutHeader.Timeout,
        [this, activityId](AsyncOperationSPtr const & operation)
        {
            this->OnProcessTransportRequestResponseRequestCompleted(operation, activityId, false);
        },
        Root.CreateAsyncOperationRoot()
        );

    this->OnProcessTransportRequestResponseRequestCompleted(operation, activityId, true);
}

void BackupRestoreAgent::OnProcessTransportRequestResponseRequestCompleted(AsyncOperationSPtr const & operation, ActivityId const& activityId, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    RequestReceiverContextUPtr receiverContext;
    auto errorCode = NodeToServiceAsyncOperation::End(operation, reply, receiverContext);

    if (errorCode.IsSuccess())
    {
        BAEventSource::Events->FederationSendReply(nodeInstanceIdStr_, activityId);
        receiverContext->Reply(move(reply));
    }
    else
    {
        BAEventSource::Events->FederationReject(nodeInstanceIdStr_, activityId, errorCode);
        receiverContext->Reject(errorCode);
    }
}

void BackupRestoreAgent::ProcessTransportIpcRequest(MessageUPtr & message, IpcReceiverContextUPtr && receiverContext)
{
    // Incoming message over IPC. 
    ActivityId activityId = FabricActivityHeader::FromMessage(*message).ActivityId;
    BAEventSource::Events->ProcessTransportIpcRequest(nodeInstanceIdStr_, activityId, message->MessageId);

    auto timeoutHeader = TimeoutHeader::FromMessage(*message);

    Common::AsyncOperationSPtr operation;

    // Check if the message is for upload or download backup
    if (message->Action == BAMessage::UploadBackupAction
        || message->Action == BAMessage::DownloadBackupAction)
    {
        bool backupUpload = message->Action == BAMessage::UploadBackupAction;

        operation = AsyncOperation::CreateAndStart<BackupCopyAsyncOperation>(
            move(message),
            move(receiverContext),
            backupCopierProxy_,
            backupUpload,
            timeoutHeader.Timeout,
            [this, activityId](AsyncOperationSPtr const & operation)
            {
                this->OnProcessTransportIpcRequestCompleted(operation, true, activityId, false);
            },
            Root.CreateAsyncOperationRoot()
            );

        this->OnProcessTransportIpcRequestCompleted(operation, true, activityId, true);
    }
    else
    {
        operation = federationWrapper_->BeginSendToServiceNode(
            move(message),
            move(receiverContext),
            timeoutHeader.Timeout,
            [this, activityId](AsyncOperationSPtr const & operation)
            {
                this->OnProcessTransportIpcRequestCompleted(operation, false, activityId, false);
            },
            Root.CreateAsyncOperationRoot()
            );

        this->OnProcessTransportIpcRequestCompleted(operation, false, activityId, true);
    }
}

void BackupRestoreAgent::OnProcessTransportIpcRequestCompleted(AsyncOperationSPtr const & operation, bool completedLocally, ActivityId const & activityId, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    MessageUPtr reply;
    IpcReceiverContextUPtr receiverContext;
    ErrorCode errorCode(ErrorCodeValue::Success);

    if (completedLocally)
    {
        errorCode = BackupCopyAsyncOperation::End(operation, reply, receiverContext);
    }
    else
    {
        errorCode = federationWrapper_->EndSendToServiceNode(operation, reply, receiverContext);
    }

    if (errorCode.IsSuccess())
    {
        BAEventSource::Events->IpcSendReply(nodeInstanceIdStr_, activityId, reply->MessageId);
        receiverContext->Reply(move(reply));
    }
    else
    {
        BAEventSource::Events->IpcSendFailure(nodeInstanceIdStr_, activityId, errorCode);

        MessageUPtr failureMessage = BAMessage::CreateIpcFailureMessage(SystemServices::IpcFailureBody(errorCode.ReadValue()), activityId);
        receiverContext->Reply(move(failureMessage));
    }
}
