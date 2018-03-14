// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IBackupRestoreService
    {
    public:
        virtual ~IBackupRestoreService() {};

        virtual Common::AsyncOperationSPtr BeginGetBackupSchedulingPolicy(
            FABRIC_BACKUP_PARTITION_INFO * partitionInfo,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndGetBackupSchedulingPolicy(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricGetBackupSchedulePolicyResult ** policyResult) = 0;

        virtual Common::AsyncOperationSPtr BeginGetRestorePointDetails(
            FABRIC_BACKUP_PARTITION_INFO * partitionInfo,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndGetGetRestorePointDetails(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricGetRestorePointDetailsResult ** restorePointResult) = 0;

        virtual Common::AsyncOperationSPtr BeginReportBackupOperationResult(
            FABRIC_BACKUP_OPERATION_RESULT * operationResult,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndReportBackupOperationResult(
            Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginReportRestoreOperationResult(
            FABRIC_RESTORE_OPERATION_RESULT * operationResult,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndReportRestoreOperationResult(
            Common::AsyncOperationSPtr const &operation) = 0;
    };
}
