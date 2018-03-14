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
            class AzureStorageBackupCopierAsyncOperationBase : public BackupCopierAsyncOperationBase
            {
                DENY_COPY(AzureStorageBackupCopierAsyncOperationBase)
            public:
                AzureStorageBackupCopierAsyncOperationBase(
                    __in BackupCopierProxy & owner,
                    Common::ActivityId const & activityId,
                    TimeSpan const timeout,
                    AsyncCallback const & callback,
                    AsyncOperationSPtr const & parent,
                    wstring const & connectionString,
                    bool isConnectionStringEncrypted,
                    wstring const & containerName,
                    wstring const & targetBaseFolderPath,
                    wstring const & sourceFileOrFolderPath,
                    wstring const & targetFolderPath);

                __declspec(property(get = get_ConnectionString)) wstring const & ConnectionString;
                wstring const & get_ConnectionString() const { return connectionString_; }

                __declspec(property(get = get_ContainerName)) wstring const & ContainerName;
                wstring const & get_ContainerName() const { return containerName_; }

                __declspec(property(get = get_BackupStoreBaseFolderPath)) wstring const & BackupStoreBaseFolderPath;
                wstring const & get_BackupStoreBaseFolderPath() const { return backupStoreBaseFolderPath_; }

                __declspec(property(get = get_SourceFileOrFolderPath)) wstring const & SourceFileOrFolderPath;
                wstring const & get_SourceFileOrFolderPath() const { return sourceFileOrFolderPath_; }

                __declspec(property(get = get_TargetFolderPath)) wstring const & TargetFolderPath;
                wstring const & get_TargetFolderPath() const { return targetFolderPath_; }

                __declspec(property(get = get_IsConnectionStringEncrypted)) bool const & IsConnectionStringEncrypted;
                bool const & get_IsConnectionStringEncrypted() const { return isConnectionStringEncrypted_; }

            protected:
                void DoDataValidation(wstring const & operationName);

                void InitializeAndPopulateCommandLineArguments(wstring &cmdLineArgs, wstring &cmdLineArgsLogString);

            private:
                wstring connectionString_;
                wstring containerName_;
                wstring backupStoreBaseFolderPath_;
                wstring sourceFileOrFolderPath_;
                wstring targetFolderPath_;
                bool isConnectionStringEncrypted_;
            };
        }
    }
}
