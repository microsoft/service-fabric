// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    { 
        // Portion of backup restore agent that resides in the service host
        class BackupRestoreAgentProxy :
            public Common::RootedObject, 
            public Common::TextTraceComponent<Common::TraceTaskCodes::BAP>, 
            public IBackupRestoreAgentProxy,
            public IMessageHandler
        {
            DENY_COPY(BackupRestoreAgentProxy);

        public:
            BackupRestoreAgentProxy(BackupRestoreAgentProxyConstructorParameters & parameters);

            virtual ~BackupRestoreAgentProxy();

            Common::AsyncOperationSPtr OnBeginOpen(
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & openAsyncOperation);

            Common::AsyncOperationSPtr OnBeginClose(
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & closeAsyncOperation);

            Common::AsyncOperationSPtr BeginSendRequest(
                Transport::MessageUPtr && request,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            
            Common::ErrorCode EndSendRequest(
                Common::AsyncOperationSPtr const & operation,
                __out Transport::MessageUPtr & result);

            Common::ErrorCode RegisterBackupRestoreReplica(
                FABRIC_PARTITION_ID partitionId,
                FABRIC_REPLICA_ID replicaId,
                Api::IBackupRestoreHandlerPtr const & backupRestoreHandler);

            Common::ErrorCode UnregisterBackupRestoreReplica(
                FABRIC_PARTITION_ID partitionId,
                FABRIC_REPLICA_ID replicaId);

            Common::AsyncOperationSPtr BeginGetPolicyRequest(
                wstring serviceName,
                Common::Guid partitionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndGetPolicyRequest(
                Common::AsyncOperationSPtr const & operation,
                __out BackupPolicy & result);

            Common::AsyncOperationSPtr BeginGetRestorePointDetailsRequest(
                wstring serviceName,
                Common::Guid partitionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndGetRestorePointDetailsRequest(
                Common::AsyncOperationSPtr const & operation,
                __out RestorePointDetails & result);

            Common::AsyncOperationSPtr BeginReportBackupOperationResult(
                const FABRIC_BACKUP_OPERATION_RESULT& operationResult,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndReportBackupOperationResult(
                Common::AsyncOperationSPtr const & operation);

            Common::AsyncOperationSPtr BeginReportRestoreOperationResult(
                const FABRIC_RESTORE_OPERATION_RESULT& operationResult,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndReportRestoreOperationResult(
                Common::AsyncOperationSPtr const & operation);

            Common::AsyncOperationSPtr BeginUploadBackup(
                const FABRIC_BACKUP_UPLOAD_INFO& uploadInfo,
                const FABRIC_BACKUP_STORE_INFORMATION& storeInfo,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndUploadBackup(
                Common::AsyncOperationSPtr const & operation);

            Common::AsyncOperationSPtr BeginDownloadBackup(
                const FABRIC_BACKUP_DOWNLOAD_INFO& downloadInfo,
                const FABRIC_BACKUP_STORE_INFORMATION& storeInfo,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndDownloadBackup(
                Common::AsyncOperationSPtr const & operation);
            
            void OnAbort();

            void ProcessIpcMessage(Transport::MessageUPtr && message, Transport::IpcReceiverContextUPtr && receiverContext);

        private:
            class ReplicaRegistrationInfo
            {
            public:
                ReplicaRegistrationInfo(
                    FABRIC_REPLICA_ID replicaId,
                    Api::IBackupRestoreHandlerPtr const & backupRestoreHandler):
                    replicaId_(replicaId),
                    backupRestoreHandler_(backupRestoreHandler)
                {}

                __declspec(property(get = get_BackupRestoreHandler)) Api::IBackupRestoreHandlerPtr const & BackupRestoreHandler;
                Api::IBackupRestoreHandlerPtr const & get_BackupRestoreHandler() const { return backupRestoreHandler_; }

            private:
                FABRIC_REPLICA_ID replicaId_;
                Api::IBackupRestoreHandlerPtr const backupRestoreHandler_;
            };
            typedef std::shared_ptr<ReplicaRegistrationInfo> ReplicaRegistrationInfoSPtr;
            typedef std::map<Common::Guid, ReplicaRegistrationInfoSPtr> ReplicaRegistrationMap;

        private:
            void OnUpdateBackupSchedulingPolicyCompleted(Common::AsyncOperationSPtr const & operation, ReplicaRegistrationInfoSPtr regInfo, Common::ActivityId const& activityId, Transport::IpcReceiverContext const & receiverContext, bool expectedCompletedSynchronously);
            void OnRequestBackupPartitionCompleted(Common::AsyncOperationSPtr const & operation, ReplicaRegistrationInfoSPtr regInfo, Common::ActivityId const& activityId, Transport::IpcReceiverContext const & receiverContext, bool expectedCompletedSynchronously);
            void SendIpcReply(Transport::MessageUPtr&& reply, Transport::IpcReceiverContext const & receiverContext);
            void SendOperationFailedIpcReply(Common::ErrorCode const& error, Transport::IpcReceiverContext const & receiverContext, Common::ActivityId const& activityId);

            class CloseAsyncOperation;
            Hosting2::IApplicationHost & applicationHost_;
            Transport::IpcClient & ipcClient_;
            BackupRestoreAgentProxyId id_;
            Common::atomic_bool isCloseAlreadyCalled_;
            ReplicaRegistrationMap replicaMap_;
            Common::RwLock lock_;
        };
    }
}
