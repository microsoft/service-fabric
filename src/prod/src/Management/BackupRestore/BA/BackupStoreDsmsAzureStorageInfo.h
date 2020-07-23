// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupStoreDsmsAzureStorageInfo : public Serialization::FabricSerializable
        {
        public:

            BackupStoreDsmsAzureStorageInfo();
            ~BackupStoreDsmsAzureStorageInfo();

            __declspec(property(get = get_StorageCredentialsSourceLocation, put = set_StorageCredentialsSourceLocation)) wstring StorageCredentialsSourceLocation;
            wstring get_StorageCredentialsSourceLocation() const { return storageCredentialsSourceLocation_; }
            void set_StorageCredentialsSourceLocation(wstring value) { storageCredentialsSourceLocation_ = value; }

            __declspec(property(get = get_ContainerName, put = set_ContainerName)) wstring ContainerName;
            wstring get_ContainerName() const { return containerName_; }
            void set_ContainerName(wstring value) { containerName_ = value; }

            __declspec(property(get = get_FolderPath, put = set_FolderPath)) wstring FolderPath;
            wstring get_FolderPath() const { return folderPath_; }
            void set_FolderPath(wstring value) { folderPath_ = value; }

            FABRIC_FIELDS_03(
                storageCredentialsSourceLocation_,
                containerName_,
                folderPath_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION & backupStoreInfo) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION const * backupStoreInfo);

        private:
            wstring storageCredentialsSourceLocation_;
            wstring containerName_;
            wstring folderPath_;
        };
    }
}
