// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupCopierProxy : 
            public Federation::NodeTraceComponent<Common::TraceTaskCodes::BA>
            , public Common::ComponentRoot
        {
            friend BackupCopier::RunBackupCopierExeAsyncOperation;
            friend BackupCopier::AzureStorageBackupCopierAsyncOperationBase;
            friend BackupCopier::AzureStorageUploadBackupAsyncOperation;
            friend BackupCopier::AzureStorageDownloadBackupAsyncOperation;
            friend BackupCopier::DsmsAzureStorageBackupCopierAsyncOperationBase;
            friend BackupCopier::DsmsAzureStorageUploadBackupAsyncOperation;
            friend BackupCopier::DsmsAzureStorageDownloadBackupAsyncOperation;
            friend BackupCopier::FileShareBackupCopierAsyncOperationBase;
            friend BackupCopier::FileShareUploadBackupAsyncOperation;
            friend BackupCopier::FileShareDownloadBackupAsyncOperation;

            DENY_COPY(BackupCopierProxy);

        protected:
            BackupCopierProxy(wstring const & backupCopierExeDirectory, wstring const & workingDirectory, Federation::NodeInstance const &);

            void Initialize();

        public:
            static std::shared_ptr<BackupCopierProxy> Create(
                wstring const & backupCopierExeDirectory,
                wstring const & workingDirectory,
                Federation::NodeInstance const &);

            ~BackupCopierProxy();

            virtual void Abort(HANDLE hProcess);

        public:
            virtual Common::AsyncOperationSPtr BeginUploadBackupToAzureStorage(
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                wstring const & connectionString,
                bool isConnectionStringEncrypted,
                wstring const & containerName,
                wstring const & backupStoreBaseFolderPath,
                wstring const & sourceFileOrFolderPath,
                wstring const & targetFolderPath,
                wstring const & backupMetadataFileName);

            virtual Common::ErrorCode EndUploadBackupToAzureStorage(
                Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginDownloadBackupFromAzureStorage(
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                wstring const & connectionString,
                bool isConnectionStringEncrypted,
                wstring const & containerName,
                wstring const & backupStoreBaseFolderPath,
                wstring const & sourceFileOrFolderPath,
                wstring const & targetFolderPath);

            virtual Common::ErrorCode EndDownloadBackupFromAzureStorage(
                Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginUploadBackupToFileShare(
                Common::ActivityId const & activityId,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent,
                BackupStoreFileShareAccessType::Enum const & accessType,
                bool isPasswordEncrypted,
                wstring const & fileSharePath,
                wstring const & primaryUserName,
                wstring const & primaryPassword,
                wstring const & secondaryUserName,
                wstring const & secondaryPassword,
                wstring const & sourceFileOrFolderPath,
                wstring const & targetFolderPath,
                wstring const & recoveryMetadataFileName);

            virtual Common::ErrorCode EndUploadBackupToFileShare(
                Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginDownloadBackupFromFileShare(
                Common::ActivityId const & activityId,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent,
                BackupStoreFileShareAccessType::Enum const & accessType,
                bool isPasswordEncrypted,
                wstring const & fileSharePath,
                wstring const & primaryUserName,
                wstring const & primaryPassword,
                wstring const & secondaryUserName,
                wstring const & secondaryPassword,
                wstring const & sourceFileOrFolderPath,
                wstring const & targetFolderPath);

            virtual Common::ErrorCode EndDownloadBackupFromFileShare(
                Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginUploadBackupToDsmsAzureStorage(
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                wstring const & storageCredentialsSourceLocation,
                wstring const & containerName,
                wstring const & backupStoreBaseFolderPath,
                wstring const & sourceFileOrFolderPath,
                wstring const & targetFolderPath,
                wstring const & backupMetadataFileName);

            virtual Common::ErrorCode EndUploadBackupToDsmsAzureStorage(
                Common::AsyncOperationSPtr const &);

            virtual Common::AsyncOperationSPtr BeginDownloadBackupFromDsmsAzureStorage(
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                wstring const & storageCredentialsSourceLocation,
                wstring const & containerName,
                wstring const & backupStoreBaseFolderPath,
                wstring const & sourceFileOrFolderPath,
                wstring const & targetFolderPath);

            virtual Common::ErrorCode EndDownloadBackupFromDsmsAzureStorage(
                Common::AsyncOperationSPtr const &);
        public:
            static Common::GlobalWString BackupCopierExeName;
            static Common::GlobalWString TempWorkingDirectoryPrefix;
            static Common::GlobalWString ErrorDetailsFilename;
            static Common::GlobalWString ProgressDetailsFilename;

            // Command Line Arguments Key
            static Common::GlobalWString OperationKeyName;

            static Common::GlobalWString StorageCredentialsSourceLocationKeyName;

            static Common::GlobalWString ConnectionStringKeyName;
            static Common::GlobalWString IsConnectionStringEncryptedKeyName;
            static Common::GlobalWString ContainerNameKeyName;
            static Common::GlobalWString BackupStoreBaseFolderPathKeyName;

            static Common::GlobalWString SourceFileOrFolderPathKeyName;
            static Common::GlobalWString TargetFolderPathKeyName;
            static Common::GlobalWString BackupMetadataFilePathKeyName;
            
            static Common::GlobalWString FileSharePathKeyName;
            static Common::GlobalWString FileShareAccessTypeKeyName;
            static Common::GlobalWString IsPasswordEncryptedKeyName;
            static Common::GlobalWString PrimaryUserNameKeyName;
            static Common::GlobalWString PrimaryPasswordKeyName;
            static Common::GlobalWString SecondaryUserNameKeyName;
            static Common::GlobalWString SecondaryPasswordKeyName;

            static Common::GlobalWString FileShareAccessTypeValue_None;
            static Common::GlobalWString FileShareAccessTypeValue_DomainUser;

            static Common::GlobalWString BooleanStringValue_True;
            static Common::GlobalWString BooleanStringValue_False;

            static Common::GlobalWString TimeoutKeyName;
            static Common::GlobalWString WorkingDirKeyName;
            static Common::GlobalWString ProgressFileKeyName;
            static Common::GlobalWString ErrorDetailsFileKeyName;

            // Command Line Arguments Values for operation command
            static Common::GlobalWString AzureBlobStoreUpload;
            static Common::GlobalWString AzureBlobStoreDownload;
            static Common::GlobalWString DsmsAzureBlobStoreUpload;
            static Common::GlobalWString DsmsAzureBlobStoreDownload;
            static Common::GlobalWString FileShareUpload;
            static Common::GlobalWString FileShareDownload;

        private:
            void InitializeCommandLineArguments(
                __inout std::wstring & args,
                __inout wstring & argsLogString);

            void AddCommandLineArgument(
                __inout std::wstring & args,
                __inout wstring & argsLogString,
                std::wstring const & argSwitch,
                std::wstring const & argValue);

            Common::AsyncOperationSPtr BeginRunBackupCopierExe(
                Common::ActivityId const &,
                std::wstring const & operation,
                __in std::wstring & args,
                __in std::wstring & argsLogString,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode EndRunBackupCopierExe(
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode TryStartBackupCopierProcess(
                std::wstring const & operation,
                __in std::wstring & args,
                __in std::wstring & argsLogString,
                __inout Common::TimeSpan & timeout,
                __out wstring & tempWorkingDirectory,
                __out wstring & errorDetailsFile,
                __out HANDLE & hProcess,
                __out HANDLE & hThread,
                __out pid_t & pid);

            Common::ErrorCode FinishBackupCopierProcess(
                ErrorCode const & waitForProcessError,
                DWORD exitCode,
                std::wstring const & tempWorkingDirectory,
                std::wstring const & errorDetailsFile,
                HANDLE hProcess,
                HANDLE hThread);

            bool TryRemoveProcessHandle(HANDLE);

            static int64 HandleToInt(HANDLE);

            std::wstring GetBackupCopierTempWorkingDirectory();

            Common::ErrorCode GetBackupCopierError(
                DWORD exitCode,
                std::wstring const & errorDetailsFile);

            Common::ErrorCode ReadBackupCopierOutput(
                std::wstring const & inputFile,
                __out std::wstring & result,
                __in bool BOMPresent = true);

            Common::ErrorCode ReadFromFile(
                __in Common::File & file,
                __out std::wstring & result,
                __in bool BOMPresent = true);

            Common::ErrorCode CreateOutputDirectory(std::wstring const &);

            void DeleteDirectory(std::wstring const &);

        private:
            std::wstring workingDir_;
            std::wstring backupCopierExePath_;

            bool isEnabled_;
            std::vector<HANDLE> processHandles_;
            Common::ExclusiveLock processHandlesLock_;

            std::unique_ptr<BackupCopier::UploadBackupJobQueue> uploadBackupJobQueue_;
            std::unique_ptr<BackupCopier::DownloadBackupJobQueue> downloadBackupJobQueue_;
        };
    }
}

