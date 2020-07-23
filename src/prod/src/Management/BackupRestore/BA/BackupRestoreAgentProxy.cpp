// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace SystemServices;
using namespace ServiceModel;
using namespace Naming;

IBackupRestoreAgentProxyUPtr Management::BackupRestoreAgentComponent::BackupRestoreAgentProxyFactory(BackupRestoreAgentProxyConstructorParameters & parameters) 
{
    return make_unique<BackupRestoreAgentProxy>(parameters);
}

BackupRestoreAgentProxy::BackupRestoreAgentProxy(BackupRestoreAgentProxyConstructorParameters & parameters)
    : applicationHost_(*parameters.ApplicationHost),
      ipcClient_(*parameters.IpcClient),
      Common::RootedObject(*parameters.Root),
      isCloseAlreadyCalled_(false),
      lock_()
{
    BAPEventSource::Events->Ctor();
}

BackupRestoreAgentProxy::~BackupRestoreAgentProxy()
{
    BAPEventSource::Events->Dtor(id_);
}

// Open the BA-proxy
AsyncOperationSPtr BackupRestoreAgentProxy::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);

    wstring nodeIdString, hostIdString, nodeName;
    uint64 nodeInstanceId = 0;

    auto error = applicationHost_.GetHostInformation(nodeIdString, nodeInstanceId, hostIdString, nodeName);
    if(!error.IsSuccess())
    {
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
    }

    Federation::NodeId nodeId;
    if (!Federation::NodeId::TryParse(nodeIdString, nodeId))
    {
        Assert::CodingError("Node Id {0} must be parseable", nodeIdString);
    }

    Guid hostId(hostIdString);
    id_ = BackupRestoreAgentProxyId(nodeId, hostId);

    // Register message handler so it can receive messages from the BA
    auto root = Root.CreateComponentRoot();
    ipcClient_.RegisterMessageHandler( 
        Actor::BAP,
        [this, root] (MessageUPtr & message, IpcReceiverContextUPtr & request) 
        {
            this->ProcessIpcMessage(move(message), move(request));
        },
        false /*dispatchOnTransportThread*/);

    BAPEventSource::Events->Open(id_);

    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(callback, parent);
}

AsyncOperationSPtr BackupRestoreAgentProxy::BeginSendRequest(
    MessageUPtr && request,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(request)).ActivityId, request->Actor, request->Action);

    request->Headers.Replace(TimeoutHeader(timeout));
    request->Headers.Compact();
    return ipcClient_.BeginRequest(
        std::move(request),
        timeout,
        callback,
        root);
}

ErrorCode BackupRestoreAgentProxy::EndSendRequest(AsyncOperationSPtr const & operation, __out MessageUPtr & result)
{
    MessageUPtr reply;
    ErrorCode error = ipcClient_.EndRequest(operation, reply);
    ActivityId activityId = ActivityId();

    if (error.IsSuccess())
    {
        activityId = FabricActivityHeader::FromMessage(*reply).ActivityId;
        error = BAMessage::ValidateIpcReply(*reply);

        if (error.IsSuccess())
        {
            result.swap(reply);
        }
    }

    if (error.IsSuccess())
    {
        BAPEventSource::Events->EndSendSuccess(id_, result->Actor, result->Action, activityId, result->MessageId);
    }
    else
    {
        BAPEventSource::Events->EndSendFailure(id_, activityId, error);
    }

    return error;
}

Common::ErrorCode BackupRestoreAgentProxy::RegisterBackupRestoreReplica(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    Api::IBackupRestoreHandlerPtr const & backupRestoreHandler)
{
    auto partitionGuid = Common::Guid(partitionId);
    BAPEventSource::Events->RegisterPartition(id_, partitionGuid);

    ReplicaRegistrationInfoSPtr regInfo = std::make_shared<ReplicaRegistrationInfo>(replicaId, backupRestoreHandler);

    {
        AcquireWriteLock lock(lock_);
        replicaMap_.insert_or_assign(partitionGuid, regInfo);
    }

    return ErrorCode::Success();
}

Common::ErrorCode BackupRestoreAgentProxy::UnregisterBackupRestoreReplica(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId)
{
    UNREFERENCED_PARAMETER(replicaId);
    auto partitionGuid = Common::Guid(partitionId);

    BAPEventSource::Events->UnregisterPartition(id_, partitionGuid);

    {
        AcquireWriteLock lock(lock_);
        replicaMap_.erase(partitionGuid);
    }

    return ErrorCode::Success();
}

Common::AsyncOperationSPtr BackupRestoreAgentProxy::BeginGetPolicyRequest(
    wstring serviceName,
    Common::Guid partitionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    MessageUPtr getPolicyMessage = BAMessage::CreateGetPolicyMessage(serviceName, partitionId);
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(getPolicyMessage)).ActivityId, getPolicyMessage->Actor, getPolicyMessage->Action);

    // Wrap the message for BA
    BAMessage::WrapForBA(*getPolicyMessage);

    auto operation = this->BeginSendRequest(
        move(getPolicyMessage), 
        timeout, 
        callback,
        parent);

    return operation;
}

Common::ErrorCode BackupRestoreAgentProxy::EndGetPolicyRequest(
    Common::AsyncOperationSPtr const & operation,
    __out BackupPolicy & result)
{
    MessageUPtr reply;
    ErrorCode error = this->EndSendRequest(operation, reply);

    if (error.IsSuccess())
    {
        GetPolicyReplyMessageBody replyBody;
        bool res = reply->GetBody<GetPolicyReplyMessageBody>(replyBody);
        if (res)
        {
            result = replyBody.Policy;
        }
        else
        {
            // TODO: Handle failure.
        }
    }

    return error;
}

Common::AsyncOperationSPtr BackupRestoreAgentProxy::BeginGetRestorePointDetailsRequest(
    wstring serviceName,
    Common::Guid partitionId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    MessageUPtr getRestorePointRequestMessage = BAMessage::CreateGetRestorePointMessage(serviceName, partitionId);
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(getRestorePointRequestMessage)).ActivityId, getRestorePointRequestMessage->Actor, getRestorePointRequestMessage->Action);

    // Wrap the message for BA
    BAMessage::WrapForBA(*getRestorePointRequestMessage);

    auto operation = this->BeginSendRequest(
        move(getRestorePointRequestMessage),
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode BackupRestoreAgentProxy::EndGetRestorePointDetailsRequest(
    Common::AsyncOperationSPtr const & operation,
    __out RestorePointDetails & result)
{
    MessageUPtr reply;
    ErrorCode error = this->EndSendRequest(operation, reply);

    if (error.IsSuccess())
    {
        GetRestorePointReplyMessageBody replyBody;
        bool res = reply->GetBody<GetRestorePointReplyMessageBody>(replyBody);
        if (res)
        {
            result = replyBody.RestorePoint;
        }
        else
        {
            // TODO: Handle failure.
        }
    }

    return error;
}

Common::AsyncOperationSPtr BackupRestoreAgentProxy::BeginReportBackupOperationResult(
    const FABRIC_BACKUP_OPERATION_RESULT& operationResult,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    MessageUPtr getBackupOperationResultMessage = BAMessage::CreateBackupOperationResultMessage(operationResult);
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(getBackupOperationResultMessage)).ActivityId, getBackupOperationResultMessage->Actor, getBackupOperationResultMessage->Action);

    // Wrap the message for BA
    BAMessage::WrapForBA(*getBackupOperationResultMessage);

    auto operation = this->BeginSendRequest(
        move(getBackupOperationResultMessage),
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode BackupRestoreAgentProxy::EndReportBackupOperationResult(
    Common::AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    ErrorCode error = this->EndSendRequest(operation, reply);
    return error;
}

Common::AsyncOperationSPtr BackupRestoreAgentProxy::BeginReportRestoreOperationResult(
    const FABRIC_RESTORE_OPERATION_RESULT& operationResult,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    MessageUPtr getRestoreOperationResultMessage = BAMessage::CreateRestoreOperationResultMessage(operationResult);
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(getRestoreOperationResultMessage)).ActivityId, getRestoreOperationResultMessage->Actor, getRestoreOperationResultMessage->Action);
    
    // Wrap the message for BA
    BAMessage::WrapForBA(*getRestoreOperationResultMessage);

    auto operation = this->BeginSendRequest(
        move(getRestoreOperationResultMessage),
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode BackupRestoreAgentProxy::EndReportRestoreOperationResult(
    Common::AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    ErrorCode error = this->EndSendRequest(operation, reply);
    return error;
}

Common::AsyncOperationSPtr BackupRestoreAgentProxy::BeginUploadBackup(
    const FABRIC_BACKUP_UPLOAD_INFO& uploadInfo,
    const FABRIC_BACKUP_STORE_INFORMATION& storeInfo,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    MessageUPtr uploadBackupMessage = BAMessage::CreateUploadBackupMessage(uploadInfo, storeInfo);
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(uploadBackupMessage)).ActivityId, uploadBackupMessage->Actor, uploadBackupMessage->Action);
    
    auto operation = this->BeginSendRequest(
        move(uploadBackupMessage),
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode BackupRestoreAgentProxy::EndUploadBackup(
    Common::AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    ErrorCode error = this->EndSendRequest(operation, reply);
    return error;

}

Common::AsyncOperationSPtr BackupRestoreAgentProxy::BeginDownloadBackup(
    const FABRIC_BACKUP_DOWNLOAD_INFO& downloadInfo,
    const FABRIC_BACKUP_STORE_INFORMATION& storeInfo,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    MessageUPtr downloadBackupMessage = BAMessage::CreateDownloadBackupMessage(downloadInfo, storeInfo);
    BAPEventSource::Events->BeginSend(id_, FabricActivityHeader::FromMessage(*std::move(downloadBackupMessage)).ActivityId, downloadBackupMessage->Actor, downloadBackupMessage->Action);
    
    auto operation = this->BeginSendRequest(
        move(downloadBackupMessage),
        timeout,
        callback,
        parent);

    return operation;
}

Common::ErrorCode BackupRestoreAgentProxy::EndDownloadBackup(
    Common::AsyncOperationSPtr const & operation)
{
    MessageUPtr reply;
    ErrorCode error = this->EndSendRequest(operation, reply);
    return error;
}

ErrorCode BackupRestoreAgentProxy::OnEndOpen(AsyncOperationSPtr const & openAsyncOperation)
{
    return CompletedAsyncOperation::End(openAsyncOperation);
}

AsyncOperationSPtr BackupRestoreAgentProxy::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    UNREFERENCED_PARAMETER(timeout);
    BAPEventSource::Events->Close(id_);
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, parent);
}

ErrorCode BackupRestoreAgentProxy::OnEndClose(AsyncOperationSPtr const & closeAsyncOperation)
{
    return CloseAsyncOperation::End(closeAsyncOperation);
}

void BackupRestoreAgentProxy::OnAbort()
{
    BAPEventSource::Events->Abort(id_);
}

void BackupRestoreAgentProxy::ProcessIpcMessage(Transport::MessageUPtr && message, Transport::IpcReceiverContextUPtr && receiverContext)
{  
    ActivityId activityId = FabricActivityHeader::FromMessage(*message).ActivityId;
    
    BAPEventSource::Events->ProcessIpcMessage(id_, message->Actor, message->Action, activityId, message->MessageId);

    if (State.Value != FabricComponentState::Opened)
    {
        BAPEventSource::Events->NotOpen(id_, activityId);
        this->SendOperationFailedIpcReply(ErrorCodeValue::InvalidState, *receiverContext, activityId);
        return;
    }

    // Extract target partition from header.
    PartitionTargetHeader partitionHeader;
    if (message->Headers.TryReadFirst(partitionHeader))
    {
        ReplicaRegistrationInfoSPtr regInfo = NULL;

        {
            AcquireReadLock lock(lock_);
            auto replicaMapIter = replicaMap_.find(partitionHeader.PartitionId);
            if (replicaMapIter != replicaMap_.end())
            {
                regInfo = replicaMapIter->second;
            }
        }
        
        if (NULL != regInfo)
        {
            // TODO: Create an async operation for incoming requests
            if (message->Action == BAMessage::UpdatePolicyAction)
            {
                ScopedHeap heap;
                FABRIC_BACKUP_POLICY policy = {};
                BackupPolicySPtr backupPolicySPtr;
                auto nullPolicy = message->SerializedBodySize() == 0;

                if (!nullPolicy)
                {
                    backupPolicySPtr = std::make_shared<BackupPolicy>();
                    message->GetBody<BackupPolicy>(*backupPolicySPtr);
                    backupPolicySPtr->ToPublicApi(heap, policy);
                }

                shared_ptr<IpcReceiverContext> receiverContextSPtr(receiverContext.release());
                auto timeoutHeader = TimeoutHeader::FromMessage(*message);

                auto beginUpdateBackupPolicyOperation = regInfo->BackupRestoreHandler->BeginUpdateBackupSchedulingPolicy(
                    nullPolicy ? NULL : &policy,
                    timeoutHeader.Timeout,
                    [this, regInfo, activityId, receiverContextSPtr](AsyncOperationSPtr const & operation) mutable
                    {
                        OnUpdateBackupSchedulingPolicyCompleted(operation, regInfo, activityId, *receiverContextSPtr, false);
                    },
                    this->get_Root().CreateAsyncOperationRoot());

                OnUpdateBackupSchedulingPolicyCompleted(beginUpdateBackupPolicyOperation, regInfo, activityId, *receiverContextSPtr, true);
            }
            else if (message->Action == BAMessage::BackupNowAction)
            {
                ScopedHeap heap;
                FABRIC_BACKUP_CONFIGURATION config = {};
                BackupPartitionRequestMessageBodySPtr backupConfigSPtr = std::make_shared<BackupPartitionRequestMessageBody>();
                message->GetBody<BackupPartitionRequestMessageBody>(*backupConfigSPtr);

                shared_ptr<IpcReceiverContext> receiverContextSPtr(receiverContext.release());
                auto timeoutHeader = TimeoutHeader::FromMessage(*message);

                backupConfigSPtr->BackupConfiguration.ToPublicApi(heap, config);

                auto beginBackupPartitionOperation = regInfo->BackupRestoreHandler->BeginPartitionBackupOperation(
                    backupConfigSPtr->OperationId,
                    &config,
                    timeoutHeader.Timeout,
                    [this, regInfo, activityId, receiverContextSPtr](AsyncOperationSPtr const & operation) mutable
                    {
                        OnRequestBackupPartitionCompleted(operation, regInfo, activityId, *receiverContextSPtr, false);
                    },
                    this->get_Root().CreateAsyncOperationRoot());

                OnRequestBackupPartitionCompleted(beginBackupPartitionOperation, regInfo, activityId, *receiverContextSPtr, true);
            }
            else
            {
                BAPEventSource::Events->InvalidAction(id_, activityId, message->Action);
                this->SendOperationFailedIpcReply(ErrorCodeValue::InvalidMessage, *receiverContext, activityId);
            }
        }
        else
        {
            BAPEventSource::Events->PartitionNotRegistered(id_, activityId, partitionHeader.PartitionId);
            this->SendOperationFailedIpcReply(ErrorCodeValue::PartitionNotFound, *receiverContext, activityId);
            return;
        }
    }
    else
    {
        BAPEventSource::Events->PartitionHeaderMissing(id_, activityId);
        this->SendOperationFailedIpcReply(ErrorCodeValue::InvalidMessage, *receiverContext, activityId);
        return;
    }
}

void BackupRestoreAgentProxy::OnUpdateBackupSchedulingPolicyCompleted(Common::AsyncOperationSPtr const & operation, ReplicaRegistrationInfoSPtr regInfo, Common::ActivityId const& activityId, Transport::IpcReceiverContext const & receiverContext, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    Common::ErrorCode error = regInfo->BackupRestoreHandler->EndUpdateBackupSchedulingPolicy(operation);
    
    if (error.IsSuccess())
    {
        this->SendIpcReply(BAMessage::CreateSuccessReply(activityId), receiverContext);
    }
    else
    {
        BAPEventSource::Events->UpdatePolicyFailure(id_, activityId, error);
        this->SendOperationFailedIpcReply(error, receiverContext, activityId);
    }
}

void BackupRestoreAgentProxy::OnRequestBackupPartitionCompleted(Common::AsyncOperationSPtr const & operation, ReplicaRegistrationInfoSPtr regInfo, Common::ActivityId const& activityId, Transport::IpcReceiverContext const & receiverContext, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    Common::ErrorCode error = regInfo->BackupRestoreHandler->EndPartitionBackupOperation(operation);

    if (error.IsSuccess())
    {
        this->SendIpcReply(BAMessage::CreateSuccessReply(activityId), receiverContext);
    }
    else
    {
        BAPEventSource::Events->BackupNowRequestFailure(id_, activityId, error);
        this->SendOperationFailedIpcReply(error, receiverContext, activityId);
    }
}


void BackupRestoreAgentProxy::SendIpcReply(MessageUPtr&& reply, IpcReceiverContext const & receiverContext)
{
    receiverContext.Reply(move(reply));
}

void BackupRestoreAgentProxy::SendOperationFailedIpcReply(Common::ErrorCode const& error, Transport::IpcReceiverContext const & receiverContext, Common::ActivityId const& activityId)
{
    auto reply = BAMessage::CreateIpcFailureMessage(IpcFailureBody(error), activityId);
    this->SendIpcReply(move(reply), receiverContext);
}
