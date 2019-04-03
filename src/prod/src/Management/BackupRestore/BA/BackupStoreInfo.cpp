// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupStoreInfo::BackupStoreInfo() :
    storeType_(BackupStoreType::Enum::Invalid)
    , fileShareInfo_()
    , azureStorageInfo_()
{
}

BackupStoreInfo::~BackupStoreInfo()
{
}

Common::ErrorCode BackupStoreInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_STORE_INFORMATION & backupStoreInfo) const
{
    ErrorCode error(ErrorCodeValue::Success);

    backupStoreInfo.Reserved = NULL;
    backupStoreInfo.StoreType = BackupStoreType::ToPublic(storeType_);
    
    switch (storeType_)
    {
        case BackupStoreType::Enum::AzureStore:
        {
            auto azureStorage = heap.AddItem<FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION>();
            error = azureStorageInfo_.ToPublicApi(heap, *azureStorage);
            if (!error.IsSuccess()) { return error; }
            backupStoreInfo.StoreAccessInformation = azureStorage.GetRawPointer();
        }
        break;

        case BackupStoreType::Enum::FileShare:
        {
            auto fileStorage = heap.AddItem<FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION>();
            error = fileShareInfo_.ToPublicApi(heap, *fileStorage);
            if (!error.IsSuccess()) { return error; }
            backupStoreInfo.StoreAccessInformation = fileStorage.GetRawPointer();
        }
        break;

        case BackupStoreType::Enum::DsmsAzureStore:
        {
            auto dsmsAzureStorage = heap.AddItem<FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION>();
            error = azureStorageInfo_.ToPublicApi(heap, *dsmsAzureStorage);
            if (!error.IsSuccess()) { return error; }
            backupStoreInfo.StoreAccessInformation = dsmsAzureStorage.GetRawPointer();
        }
        break;
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupStoreInfo::FromPublicApi(
    FABRIC_BACKUP_STORE_INFORMATION const & backupStoreInfo)
{
    ErrorCode error(ErrorCodeValue::Success);

    error = BackupStoreType::FromPublic(backupStoreInfo.StoreType, storeType_);
    if (!error.IsSuccess()) { return error; }

    switch (storeType_)
    {
        case BackupStoreType::Enum::AzureStore:
        {
            error = azureStorageInfo_.FromPublicApi((FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION*)backupStoreInfo.StoreAccessInformation);
            if (!error.IsSuccess()) { return error; }
        }
        break;

        case BackupStoreType::Enum::FileShare:
        {
            error = fileShareInfo_.FromPublicApi((FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION*)backupStoreInfo.StoreAccessInformation);
            if (!error.IsSuccess()) { return error; }
        }
        break;

        case BackupStoreType::Enum::DsmsAzureStore:
        {
            error = azureStorageInfo_.FromPublicApi((FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION*)backupStoreInfo.StoreAccessInformation);
            if (!error.IsSuccess()) { return error; }
        }
        break;
    }

    return error;
}
