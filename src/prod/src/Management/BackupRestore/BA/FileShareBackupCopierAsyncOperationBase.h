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
            class FileShareBackupCopierAsyncOperationBase : public BackupCopierAsyncOperationBase
            {
                DENY_COPY(FileShareBackupCopierAsyncOperationBase)
            public:
                FileShareBackupCopierAsyncOperationBase(
                    __in BackupCopierProxy & owner,
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

                __declspec(property(get = get_AccessType)) BackupStoreFileShareAccessType::Enum const & AccessType;
                BackupStoreFileShareAccessType::Enum const & get_AccessType() const { return accessType_; }

                __declspec(property(get = get_FileSharePath)) wstring const & FileSharePath;
                wstring const & get_FileSharePath() const { return fileSharePath_; }

                __declspec(property(get = get_PrimaryUserName)) wstring const & PrimaryUserName;
                wstring const & get_PrimaryUserName() const { return primaryUserName_; }

                __declspec(property(get = get_PrimaryPassword)) wstring const & PrimaryPassword;
                wstring const & get_PrimaryPassword() const { return primaryPassword_; }

                __declspec(property(get = get_SecondaryUserName)) wstring const & SecondaryUserName;
                wstring const & get_SecondaryUserName() const { return secondaryUserName_; }

                __declspec(property(get = get_SecondaryPassword)) wstring const & SecondaryPassword;
                wstring const & get_SecondaryPassword() const { return secondaryPassword_; }

                __declspec(property(get = get_SourceFileOrFolderPath)) wstring const & SourceFileOrFolderPath;
                wstring const & get_SourceFileOrFolderPath() const { return sourceFileOrFolderPath_; }

                __declspec(property(get = get_TargetFolderPath)) wstring const & TargetFolderPath;
                wstring const & get_TargetFolderPath() const { return targetFolderPath_; }

                __declspec(property(get = get_IsPasswordEncrypted)) bool const & IsPasswordEncrypted;
                bool const & get_IsPasswordEncrypted() const { return isPasswordEncrypted_; }

            protected:
                void DoDataValidation(wstring const & operationName);

                void InitializeAndPopulateCommandLineArguments(wstring &cmdLineArgs, wstring &cmdLineArgsLogString);

            private:
                BackupStoreFileShareAccessType::Enum accessType_;
                bool isPasswordEncrypted_;
                wstring fileSharePath_;
                wstring primaryUserName_;
                wstring primaryPassword_;
                wstring secondaryUserName_;
                wstring secondaryPassword_;
                wstring sourceFileOrFolderPath_;
                wstring targetFolderPath_;
            };
        }
    }
}
