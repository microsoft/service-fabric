// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IBackupRestoreHandler
    {
    public:
        virtual ~IBackupRestoreHandler() {};

        virtual Common::AsyncOperationSPtr BeginUpdateBackupSchedulingPolicy(
            FABRIC_BACKUP_POLICY * policy,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndUpdateBackupSchedulingPolicy(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginPartitionBackupOperation(
            FABRIC_BACKUP_OPERATION_ID operationId,
            FABRIC_BACKUP_CONFIGURATION* backupConfiguration,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndPartitionBackupOperation(
            Common::AsyncOperationSPtr const &operation) = 0;
    };
}
