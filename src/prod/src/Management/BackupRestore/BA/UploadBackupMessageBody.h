// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class UploadBackupMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            UploadBackupMessageBody();
            ~UploadBackupMessageBody();

            __declspec(property(get = get_StoreInfo, put = set_StoreInfo)) BackupStoreInfo StoreInfo;
            BackupStoreInfo get_StoreInfo() const { return storeInfo_; }
            void set_StoreInfo(BackupStoreInfo value) { storeInfo_ = value; }

            __declspec(property(get = get_BackupMetadataFilePath, put = set_BackupMetadataFilePath)) wstring BackupMetadataFilePath;
            wstring get_BackupMetadataFilePath() const { return backupMetadataFilePath_; }
            void set_BackupMetadataFilePath(wstring value) { backupMetadataFilePath_ = value; }

            __declspec(property(get = get_LocalBackupPath, put = set_LocalBackupPath)) wstring LocalBackupPath;
            wstring get_LocalBackupPath() const { return localBackupPath_; }
            void set_LocalBackupPath(wstring value) { localBackupPath_ = value; }

            __declspec(property(get = get_DestinationFolderName, put = set_DestinationFolderName)) wstring DestinationFolderName;
            wstring get_DestinationFolderName() const { return destinationFolderName_; }
            void set_DestinationFolderName(wstring value) { destinationFolderName_ = value; }

            Common::ErrorCode FromPublicApi(FABRIC_BACKUP_UPLOAD_INFO const &, FABRIC_BACKUP_STORE_INFORMATION const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_BACKUP_UPLOAD_INFO &, __out FABRIC_BACKUP_STORE_INFORMATION &) const;

            FABRIC_FIELDS_04(
                storeInfo_,
                backupMetadataFilePath_,
                localBackupPath_,
                destinationFolderName_);

        private:
            BackupStoreInfo storeInfo_;
            wstring backupMetadataFilePath_;
            wstring localBackupPath_;
            wstring destinationFolderName_;
        };
    }
}

