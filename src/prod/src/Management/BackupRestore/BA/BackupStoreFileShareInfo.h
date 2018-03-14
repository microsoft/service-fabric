// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupStoreFileShareInfo : public Serialization::FabricSerializable
        {
        public:

            BackupStoreFileShareInfo();
            ~BackupStoreFileShareInfo();

            __declspec(property(get = get_AccessType, put = set_AccessType)) BackupStoreFileShareAccessType::Enum AccessType;
            BackupStoreFileShareAccessType::Enum get_AccessType() const { return accessType_; }
            void set_AccessType(BackupStoreFileShareAccessType::Enum value) { accessType_ = value; }

            __declspec(property(get = get_FileSharePath, put = set_FileSharePath)) wstring FileSharePath;
            wstring get_FileSharePath() const { return fileSharePath_; }
            void set_FileSharePath(wstring value) { fileSharePath_ = value; }

            __declspec(property(get = get_PrimaryUserName, put = set_PrimaryUserName)) wstring PrimaryUserName;
            wstring get_PrimaryUserName() const { return primaryUserName_; }
            void set_PrimaryUserName(wstring value) { primaryUserName_ = value; }

            __declspec(property(get = get_PrimaryPassword, put = set_PrimaryPassword)) wstring PrimaryPassword;
            wstring get_PrimaryPassword() const { return primaryPassword_; }
            void set_PrimaryPassword(wstring value) { primaryPassword_ = value; }

            __declspec(property(get = get_SecondaryUserName, put = set_SecondaryUserName)) wstring SecondaryUserName;
            wstring get_SecondaryUserName() const { return secondaryUserName_; }
            void set_SecondaryUserName(wstring value) { secondaryUserName_ = value; }

            __declspec(property(get = get_SecondaryPassword, put = set_SecondaryPassword)) wstring SecondaryPassword;
            wstring get_SecondaryPassword() const { return secondaryPassword_; }
            void set_SecondaryPassword(wstring value) { secondaryPassword_ = value; }

            __declspec(property(get = get_IsPasswordEncrypted, put = set_IsPasswordEncrypted)) bool IsPasswordEncrypted;
            bool get_IsPasswordEncrypted() const { return isPasswordEncrypted_; }
            void set_IsPasswordEncrypted(bool value) { isPasswordEncrypted_ = value; }

            FABRIC_FIELDS_07(
                accessType_,
                fileSharePath_,
                primaryUserName_,
                primaryPassword_,
                secondaryUserName_,
                secondaryPassword_,
                isPasswordEncrypted_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION & backupStoreInfo) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION const * backupStoreInfo);

        private:
            BackupStoreFileShareAccessType::Enum accessType_;
            wstring fileSharePath_;
            wstring primaryUserName_;
            wstring primaryPassword_;
            wstring secondaryUserName_;
            wstring secondaryPassword_;
            bool isPasswordEncrypted_;
        };
    }
}
