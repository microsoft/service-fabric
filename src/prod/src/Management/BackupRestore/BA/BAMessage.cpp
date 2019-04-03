// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Transport;
using namespace Management;
using namespace SystemServices;
using namespace BackupRestoreAgentComponent;

GlobalWString BAMessage::IpcFailureAction = make_global<wstring>(L"IpcFailureAction");
GlobalWString BAMessage::RegisterBAPAction = make_global<wstring>(L"RegisterBAPAction");
GlobalWString BAMessage::BackupNowAction = make_global<wstring>(L"BackupNowAction");
GlobalWString BAMessage::BackupCompleteAction = make_global<wstring>(L"BackupCompleteAction");
GlobalWString BAMessage::RestoreCompleteAction = make_global<wstring>(L"RestoreCompleteAction");
GlobalWString BAMessage::GetBackupPolicyAction = make_global<wstring>(L"GetBackupPolicyAction");
GlobalWString BAMessage::GetRestorePointAction = make_global<wstring>(L"GetRestorePointAction");
GlobalWString BAMessage::ForwardToBAPAction = make_global<wstring>(L"ForwardToBAPAction");
GlobalWString BAMessage::ForwardToBRSAction = make_global<wstring>(L"ForwardToBRSAction");
GlobalWString BAMessage::OperationSuccessAction = make_global<wstring>(L"OperationSuccessAction");
GlobalWString BAMessage::UpdatePolicyAction = make_global<wstring>(L"UpdatePolicyAction");
GlobalWString BAMessage::UploadBackupAction = make_global<wstring>(L"UploadBackupAction");
GlobalWString BAMessage::DownloadBackupAction = make_global<wstring>(L"DownloadBackupAction");


void BAMessage::SetBAHeaders(Message & message, Actor::Enum actor, wstring const& action)
{
    message.Headers.Replace(ActorHeader(actor));
    message.Headers.Replace(ActionHeader(action));
    message.Headers.Replace(FabricActivityHeader(Common::Guid::NewGuid()));
}

void BAMessage::SetBAHeaders(Message & message, Actor::Enum actor, wstring const& action, ActivityId const& activityId)
{
    message.Headers.Replace(ActorHeader(actor));
    message.Headers.Replace(ActionHeader(action));
    message.Headers.Replace(FabricActivityHeader(activityId));
}

void BAMessage::WrapForBA(Message & message)
{
    ActorHeader actor;

    if (message.Headers.TryReadFirst(actor))
    {
        if (actor.Actor == Actor::BA)
        {
            return;
        }

        message.Headers.Replace(ForwardMessageHeader(message.Actor, message.Action));
        message.Headers.Replace(ActorHeader(Actor::BA));

        if (actor.Actor == Actor::BRS)
        {
            message.Headers.Replace(ActionHeader(ForwardToBRSAction));
        }
        else if (actor.Actor == Actor::BAP)
        {
            message.Headers.Replace(ActionHeader(ForwardToBAPAction));
        }
    }
}

void BAMessage::UnwrapForBA(Message & message)
{
    ForwardMessageHeader forwardHeader;
    if (message.Headers.TryGetAndRemoveHeader(forwardHeader))
    {
        message.Headers.Replace(ActorHeader(forwardHeader.Actor));
        message.Headers.Replace(ActionHeader(forwardHeader.Action));
    }
}

ErrorCode BAMessage::ValidateIpcReply(__in Message & reply)
{
    if (reply.Action == IpcFailureAction)
    {
        IpcFailureBody body;
        if (reply.GetBody(body))
        {
            return body.TakeError();
        }
        else
        {
            return ErrorCodeValue::OperationFailed;
        }
    }
    else
    {
        return ErrorCodeValue::Success;
    }
}

Transport::MessageUPtr BAMessage::CreateBackupNowMessage(FABRIC_BACKUP_OPERATION_ID operationId, FABRIC_BACKUP_CONFIGURATION const& backupConfiguration, Common::Guid partitionId, Common::NamingUri serviceUri)
{
    BackupPartitionRequestMessageBody messageBody(operationId, backupConfiguration);
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>(messageBody));

    // Update policy message is meant for BAP
    SetBAHeaders(*message, Transport::Actor::BAP, BAMessage::BackupNowAction);
    message->Headers.Replace(Naming::PartitionTargetHeader(Common::Guid(partitionId)));
    message->Headers.Replace(Naming::ServiceTargetHeader(serviceUri));

    return std::move(message);
}

Transport::MessageUPtr BAMessage::CreateUpdatePolicyMessage(BackupPolicy const& backupPolicy, Common::Guid partitionId, Common::NamingUri serviceUri)
{
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>(backupPolicy));

    // Update policy message is meant for BAP
    SetBAHeaders(*message, Transport::Actor::BAP, BAMessage::UpdatePolicyAction);
    message->Headers.Replace(Naming::PartitionTargetHeader(Common::Guid(partitionId)));
    message->Headers.Replace(Naming::ServiceTargetHeader(serviceUri));
    
    return std::move(message);
}

Transport::MessageUPtr BAMessage::CreateUpdatePolicyMessage(Common::Guid partitionId, Common::NamingUri serviceUri)
{
    Transport::MessageUPtr message(Common::make_unique<Transport::Message>());

    // Update policy message is meant for BAP
    SetBAHeaders(*message, Transport::Actor::BAP, BAMessage::UpdatePolicyAction);
    message->Headers.Replace(Naming::PartitionTargetHeader(Common::Guid(partitionId)));
    message->Headers.Replace(Naming::ServiceTargetHeader(serviceUri));

    return std::move(message);
}

Transport::MessageUPtr BAMessage::CreateGetPolicyMessage(wstring serviceName, Common::Guid partitionId)
{
    PartitionInfoMessageBody getPolicyRequestMessageBody(serviceName, partitionId);

    // Get policy message is for BRS.
    MessageUPtr getPolicyMessage = BAMessage::CreateMessageForFabricBRS<PartitionInfoMessageBody>(BAMessage::GetBackupPolicyAction, getPolicyRequestMessageBody);

    return std::move(getPolicyMessage);
}

Transport::MessageUPtr BAMessage::CreateGetRestorePointMessage(wstring serviceName, Common::Guid partitionId)
{
    PartitionInfoMessageBody getRestorePointRequestMessageBody(serviceName, partitionId);

    // Get restore point details message is for BRS.
    MessageUPtr getrestorePointMessage = BAMessage::CreateMessageForFabricBRS<PartitionInfoMessageBody>(BAMessage::GetRestorePointAction, getRestorePointRequestMessageBody);

    return std::move(getrestorePointMessage);
}

Transport::MessageUPtr BAMessage::CreateBackupOperationResultMessage(const FABRIC_BACKUP_OPERATION_RESULT& operationResult)
{
    BackupOperationResultMessageBody backupOperationResultMessageBody(operationResult);

    // Restore operation result message is for BRS.
    MessageUPtr backupOperationResultMessage = BAMessage::CreateMessageForFabricBRS<BackupOperationResultMessageBody>(BAMessage::BackupCompleteAction, backupOperationResultMessageBody);

    return std::move(backupOperationResultMessage);
}

Transport::MessageUPtr BAMessage::CreateRestoreOperationResultMessage(const FABRIC_RESTORE_OPERATION_RESULT& operationResult)
{
    RestoreOperationResultMessageBody restoreOperationResultMessageBody(operationResult);

    // Restore operation result message is for BRS.
    MessageUPtr restoreOperationResultMessage = BAMessage::CreateMessageForFabricBRS<RestoreOperationResultMessageBody>(BAMessage::RestoreCompleteAction, restoreOperationResultMessageBody);

    return std::move(restoreOperationResultMessage);
}

Transport::MessageUPtr BAMessage::CreateUploadBackupMessage(const FABRIC_BACKUP_UPLOAD_INFO& uploadInfo, const FABRIC_BACKUP_STORE_INFORMATION& storeInfo)
{
    UploadBackupMessageBody uploadBackupMessageBody;
    uploadBackupMessageBody.FromPublicApi(uploadInfo, storeInfo);

    // Upload backup message is for BA.
    MessageUPtr uploadBackupMessage = BAMessage::CreateMessage<UploadBackupMessageBody>(BAMessage::UploadBackupAction, uploadBackupMessageBody);

    return std::move(uploadBackupMessage);
}

Transport::MessageUPtr BAMessage::CreateDownloadBackupMessage(const FABRIC_BACKUP_DOWNLOAD_INFO& downloadInfo, const FABRIC_BACKUP_STORE_INFORMATION& storeInfo)
{
    DownloadBackupMessageBody downloadBackupMessageBody;
    downloadBackupMessageBody.FromPublicApi(downloadInfo, storeInfo);

    // Download backup message is for BA.
    MessageUPtr downloadBackupMessage = BAMessage::CreateMessage<DownloadBackupMessageBody>(BAMessage::DownloadBackupAction, downloadBackupMessageBody);

    return std::move(downloadBackupMessage);
}

Transport::MessageUPtr BAMessage::CreateSuccessReply(ActivityId const& activityId)
{
    return CreateMessage(BAMessage::OperationSuccessAction, activityId);
}

Transport::MessageUPtr BAMessage::CreateIpcFailureMessage(SystemServices::IpcFailureBody const & body, Common::ActivityId const & activityId)
{
    return CreateMessage(IpcFailureAction, body, activityId);
}
