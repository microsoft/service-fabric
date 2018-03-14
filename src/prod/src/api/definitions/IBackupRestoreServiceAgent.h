// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IBackupRestoreServiceAgent
    {
    public:
        virtual ~IBackupRestoreServiceAgent() {};

        virtual void Release() = 0;

        virtual void RegisterBackupRestoreService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID,
            IBackupRestoreServicePtr const &,
            __out std::wstring & serviceAddress) = 0;

        virtual void UnregisterBackupRestoreService(
            ::FABRIC_PARTITION_ID,
            ::FABRIC_REPLICA_ID) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateBackupSchedulePolicy(
            ::FABRIC_BACKUP_PARTITION_INFO * info,
            ::FABRIC_BACKUP_POLICY* policy,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndUpdateBackupSchedulePolicy(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginPartitionBackupOperation(
            ::FABRIC_BACKUP_PARTITION_INFO * info,
            ::FABRIC_BACKUP_OPERATION_ID operationId,
            ::FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndPartitionBackupOperation(
            Common::AsyncOperationSPtr const &operation) = 0;
    };
}
