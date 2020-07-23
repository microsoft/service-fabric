// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupCopier
        {
            class DsmsAzureStorageDownloadBackupAsyncOperation : public DsmsAzureStorageBackupCopierAsyncOperationBase
            {
                DENY_COPY(DsmsAzureStorageDownloadBackupAsyncOperation)
            public:
                DsmsAzureStorageDownloadBackupAsyncOperation(
                    __in BackupCopierProxy & owner,
                    Common::ActivityId const & activityId,
                    TimeSpan const timeout,
                    AsyncCallback const & callback,
                    AsyncOperationSPtr const & parent,
                    wstring const & storageCredentialsSourceLocation,
                    wstring const & containerName,
                    wstring const & backupStoreBaseFolderPath,
                    wstring const & sourceFileOrFolderPath,
                    wstring const & targetFolderPath);

                static ErrorCode End(AsyncOperationSPtr const & operation);

                void OnCompleted() override;

                void OnProcessJob(AsyncOperationSPtr const & thisSPtr) override;

            private:

                void DoDownloadBackup(AsyncOperationSPtr const & thisSPtr);

                void OnRunBackupCopierComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            };
        }
    }
}
