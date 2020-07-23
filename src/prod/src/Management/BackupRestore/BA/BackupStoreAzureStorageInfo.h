// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupStoreAzureStorageInfo : public Serialization::FabricSerializable
        {
        public:

            BackupStoreAzureStorageInfo();
            ~BackupStoreAzureStorageInfo();

            __declspec(property(get = get_ConnectionString, put = set_ConnectionString)) wstring ConnectionString;
            wstring get_ConnectionString() const { return connectionString_; }
            void set_ConnectionString(wstring value) { connectionString_ = value; }

            __declspec(property(get = get_ContainerName, put = set_ContainerName)) wstring ContainerName;
            wstring get_ContainerName() const { return containerName_; }
            void set_ContainerName(wstring value) { containerName_ = value; }

            __declspec(property(get = get_FolderPath, put = set_FolderPath)) wstring FolderPath;
            wstring get_FolderPath() const { return folderPath_; }
            void set_FolderPath(wstring value) { folderPath_ = value; }

            __declspec(property(get = get_IsConnectionStringEncrypted, put = set_IsConnectionStringEncrypted)) bool IsConnectionStringEncrypted;
            bool get_IsConnectionStringEncrypted() const { return isConnectionStringEncrypted_; }
            void set_IsConnectionStringEncrypted(bool value) { isConnectionStringEncrypted_ = value; }

            FABRIC_FIELDS_04(
                connectionString_,
                containerName_,
                folderPath_,
                isConnectionStringEncrypted_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION & backupStoreInfo) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION const * backupStoreInfo);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION & backupStoreInfo) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION const * backupStoreInfo);

        private:
            wstring connectionString_;
            wstring containerName_;
            wstring folderPath_;
            bool isConnectionStringEncrypted_;
        };
    }
}
