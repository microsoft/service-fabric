// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyBackupRestoreService :
        public Common::ComponentRoot,
        public IBackupRestoreService
    {
        DENY_COPY(ComProxyBackupRestoreService);

    public:
        ComProxyBackupRestoreService(Common::ComPointer<IFabricBackupRestoreService> const & comImpl);
        virtual ~ComProxyBackupRestoreService();

        virtual Common::AsyncOperationSPtr BeginGetBackupSchedulingPolicy(
            FABRIC_BACKUP_PARTITION_INFO * partitionInfo,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        virtual Common::ErrorCode EndGetBackupSchedulingPolicy(
            Common::AsyncOperationSPtr const &operation,
            IFabricGetBackupSchedulePolicyResult ** policyResult);

        virtual Common::AsyncOperationSPtr BeginGetRestorePointDetails(
            FABRIC_BACKUP_PARTITION_INFO * partitionInfo,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        virtual Common::ErrorCode EndGetGetRestorePointDetails(
            Common::AsyncOperationSPtr const &operation,
            __out IFabricGetRestorePointDetailsResult ** restorePointResult);

        virtual Common::AsyncOperationSPtr BeginReportBackupOperationResult(
            FABRIC_BACKUP_OPERATION_RESULT * operationResult,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        virtual Common::ErrorCode EndReportBackupOperationResult(
            Common::AsyncOperationSPtr const &operation);

        virtual Common::AsyncOperationSPtr BeginReportRestoreOperationResult(
            FABRIC_RESTORE_OPERATION_RESULT * operationResult,
            Common::TimeSpan const timeoutMilliseconds,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        virtual Common::ErrorCode EndReportRestoreOperationResult(
            Common::AsyncOperationSPtr const &operation);

    private:
        class GetBackupSchedulingPolicyAsyncOperation;
        class GetRestorePointDetailsAsyncOperation;
        class ReportBackupOperationResultAsyncOperation;
        class ReportRestoreOperationResultAsyncOperation;
        
        Common::ComPointer<IFabricBackupRestoreService> const comImpl_;
    };
}
