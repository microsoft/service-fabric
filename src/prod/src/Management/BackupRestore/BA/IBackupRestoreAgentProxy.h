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
        class IBackupRestoreAgentProxy : public Common::AsyncFabricComponent
        {
            DENY_COPY(IBackupRestoreAgentProxy);

        public:
            IBackupRestoreAgentProxy() {}
            virtual ~IBackupRestoreAgentProxy() {}

            virtual Common::ErrorCode RegisterBackupRestoreReplica(
                FABRIC_PARTITION_ID partitionId,
                FABRIC_REPLICA_ID replicaId,
                Api::IBackupRestoreHandlerPtr const & backupRestoreHandler)=0;

            virtual Common::ErrorCode UnregisterBackupRestoreReplica(
                FABRIC_PARTITION_ID partitionId,
                FABRIC_REPLICA_ID replicaId)=0;

            virtual Common::AsyncOperationSPtr BeginGetPolicyRequest(
                wstring serviceName,
                Common::Guid partitionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)=0;

            virtual Common::ErrorCode EndGetPolicyRequest(
                Common::AsyncOperationSPtr const & operation,
                __out BackupPolicy & result)=0;

            virtual Common::AsyncOperationSPtr BeginGetRestorePointDetailsRequest(
                wstring serviceName,
                Common::Guid partitionId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)=0;

            virtual Common::ErrorCode EndGetRestorePointDetailsRequest(
                Common::AsyncOperationSPtr const & operation,
                __out RestorePointDetails & result)=0;

            virtual Common::AsyncOperationSPtr BeginReportBackupOperationResult(
                const FABRIC_BACKUP_OPERATION_RESULT& operationResult,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndReportBackupOperationResult(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginReportRestoreOperationResult(
                const FABRIC_RESTORE_OPERATION_RESULT& operationResult,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndReportRestoreOperationResult(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginUploadBackup(
                const FABRIC_BACKUP_UPLOAD_INFO& uploadInfo,
                const FABRIC_BACKUP_STORE_INFORMATION& storeInfo,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndUploadBackup(
                Common::AsyncOperationSPtr const & operation) = 0;

            virtual Common::AsyncOperationSPtr BeginDownloadBackup(
                const FABRIC_BACKUP_DOWNLOAD_INFO& downloadInfo,
                const FABRIC_BACKUP_STORE_INFORMATION& storeInfo,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent) = 0;

            virtual Common::ErrorCode EndDownloadBackup(
                Common::AsyncOperationSPtr const & operation) = 0;
        };

        struct BackupRestoreAgentProxyConstructorParameters
        {
            Hosting2::IApplicationHost* ApplicationHost;
            Transport::IpcClient * IpcClient;
            Common::ComponentRoot const * Root;
        };

        typedef IBackupRestoreAgentProxyUPtr BackupRestoreAgentProxyFactoryFunctionPtr(BackupRestoreAgentProxyConstructorParameters &);

        BackupRestoreAgentProxyFactoryFunctionPtr BackupRestoreAgentProxyFactory;
    }
}
