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
            class DsmsAzureStorageBackupCopierAsyncOperationBase : public BackupCopierAsyncOperationBase
            {
                DENY_COPY(DsmsAzureStorageBackupCopierAsyncOperationBase)
            public:
                DsmsAzureStorageBackupCopierAsyncOperationBase(
                    __in BackupCopierProxy & owner,
                    Common::ActivityId const & activityId,
                    TimeSpan const timeout,
                    AsyncCallback const & callback,
                    AsyncOperationSPtr const & parent,
                    wstring const & storageCredentialsSourceLocation,
                    wstring const & containerName,
                    wstring const & targetBaseFolderPath,
                    wstring const & sourceFileOrFolderPath,
                    wstring const & targetFolderPath);

                __declspec(property(get = get_StorageCredentialsSourceLocation)) wstring const & StorageCredentialsSourceLocation;
                wstring const & get_StorageCredentialsSourceLocation() const { return storageCredentialsSourceLocation_; }

                __declspec(property(get = get_ContainerName)) wstring const & ContainerName;
                wstring const & get_ContainerName() const { return containerName_; }

                __declspec(property(get = get_BackupStoreBaseFolderPath)) wstring const & BackupStoreBaseFolderPath;
                wstring const & get_BackupStoreBaseFolderPath() const { return backupStoreBaseFolderPath_; }

                __declspec(property(get = get_SourceFileOrFolderPath)) wstring const & SourceFileOrFolderPath;
                wstring const & get_SourceFileOrFolderPath() const { return sourceFileOrFolderPath_; }

                __declspec(property(get = get_TargetFolderPath)) wstring const & TargetFolderPath;
                wstring const & get_TargetFolderPath() const { return targetFolderPath_; }

            protected:
                void DoDataValidation(wstring const & operationName);

                void InitializeAndPopulateCommandLineArguments(wstring &cmdLineArgs, wstring &cmdLineArgsLogString);

            private:
                wstring storageCredentialsSourceLocation_;
                wstring containerName_;
                wstring backupStoreBaseFolderPath_;
                wstring sourceFileOrFolderPath_;
                wstring targetFolderPath_;
            };
        }
    }
}
