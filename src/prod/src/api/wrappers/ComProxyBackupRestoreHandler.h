// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyBackupRestoreHandler :
        public Common::ComponentRoot,
        public IBackupRestoreHandler
    {
        DENY_COPY(ComProxyBackupRestoreHandler);

    public:
        ComProxyBackupRestoreHandler(Common::ComPointer<IFabricBackupRestoreHandler> const & comImpl);
        virtual ~ComProxyBackupRestoreHandler();

        virtual Common::AsyncOperationSPtr BeginUpdateBackupSchedulingPolicy(
            FABRIC_BACKUP_POLICY * policy,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        virtual Common::ErrorCode EndUpdateBackupSchedulingPolicy(
            Common::AsyncOperationSPtr const &operation);

        virtual Common::AsyncOperationSPtr BeginPartitionBackupOperation(
            FABRIC_BACKUP_OPERATION_ID operationId,
            FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        virtual Common::ErrorCode EndPartitionBackupOperation(
            Common::AsyncOperationSPtr const &operation);

    private:
        class UpdateBackupSchedulingPolicyAsyncOperation;
        class BackupPartitionAsyncOperation;

        Common::ComPointer<IFabricBackupRestoreHandler> const comImpl_;
    };
}
