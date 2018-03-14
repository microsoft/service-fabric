// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class DownloadBackupMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            DownloadBackupMessageBody();
            ~DownloadBackupMessageBody();

            __declspec(property(get = get_StoreInfo, put = set_StoreInfo)) BackupStoreInfo StoreInfo;
            BackupStoreInfo get_StoreInfo() const { return storeInfo_; }
            void set_StoreInfo(BackupStoreInfo value) { storeInfo_ = value; }

            __declspec(property(get = get_DestinationRootPath, put = set_DestinationRootPath)) wstring DestinationRootPath;
            wstring get_DestinationRootPath() const { return destinationRootPath_; }
            void set_DestinationRootPath(wstring value) { destinationRootPath_ = value; }

            __declspec(property(get = get_BackupLocationList)) std::vector<std::wstring> & BackupLocationList;
            std::vector<std::wstring> get_BackupLocationList() const { return backupLocationList_; }

            Common::ErrorCode FromPublicApi(FABRIC_BACKUP_DOWNLOAD_INFO const &, FABRIC_BACKUP_STORE_INFORMATION const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_BACKUP_DOWNLOAD_INFO &, __out FABRIC_BACKUP_STORE_INFORMATION &) const;

            FABRIC_FIELDS_03(
                storeInfo_,
                backupLocationList_,
                destinationRootPath_);
                
        private:
            BackupStoreInfo storeInfo_;
            wstring destinationRootPath_;
            std::vector<std::wstring> backupLocationList_;
        };
    }
}

