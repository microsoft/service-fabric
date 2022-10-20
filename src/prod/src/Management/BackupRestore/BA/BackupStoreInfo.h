// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupStoreInfo : public Serialization::FabricSerializable
        {
        public:

            BackupStoreInfo();
            ~BackupStoreInfo();

            __declspec(property(get = get_StoreType, put = set_StoreType)) BackupStoreType::Enum StoreType;
            BackupStoreType::Enum get_StoreType() const { return storeType_; }
            void set_StoreType(BackupStoreType::Enum value) { storeType_ = value; }

            __declspec(property(get = get_fileShareInfo, put = set_fileShareInfo)) BackupStoreFileShareInfo FileShareInfo;
            BackupStoreFileShareInfo get_fileShareInfo() const { return fileShareInfo_; }
            void set_fileShareInfo(BackupStoreFileShareInfo value) { fileShareInfo_ = value; }

            __declspec(property(get = get_AzureStorageInfo, put = set_AzureStorageInfo)) BackupStoreAzureStorageInfo AzureStorageInfo;
            BackupStoreAzureStorageInfo get_AzureStorageInfo() const { return azureStorageInfo_; }
            void set_AzureStorageInfo(BackupStoreAzureStorageInfo value) { azureStorageInfo_ = value; }

            FABRIC_FIELDS_03(
                storeType_,
                fileShareInfo_,
                azureStorageInfo_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_STORE_INFORMATION & backupStoreInfo) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_STORE_INFORMATION const & backupStoreInfo);

        private:
            BackupStoreType::Enum storeType_;
            BackupStoreFileShareInfo fileShareInfo_;
            BackupStoreAzureStorageInfo azureStorageInfo_;
        };
    }
}
