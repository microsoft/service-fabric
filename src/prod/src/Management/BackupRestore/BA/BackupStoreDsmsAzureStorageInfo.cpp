// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupStoreDsmsAzureStorageInfo::BackupStoreDsmsAzureStorageInfo() :
    storageCredentialsSourceLocation_(L""),
    containerName_(L""),
    folderPath_(L"")
{
}

BackupStoreDsmsAzureStorageInfo::~BackupStoreDsmsAzureStorageInfo()
{
}

Common::ErrorCode BackupStoreDsmsAzureStorageInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION & backupStoreInfo) const
{
    backupStoreInfo.StorageCredentialsSourceLocation = heap.AddString(storageCredentialsSourceLocation_);
    backupStoreInfo.ContainerName = heap.AddString(containerName_);
    backupStoreInfo.FolderPath = heap.AddString(folderPath_);
    backupStoreInfo.Reserved = NULL;
    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupStoreDsmsAzureStorageInfo::FromPublicApi(
    FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION const * backupStoreInfo)
{
    storageCredentialsSourceLocation_ = backupStoreInfo->StorageCredentialsSourceLocation == NULL ? L"" : backupStoreInfo->StorageCredentialsSourceLocation;
    containerName_ = backupStoreInfo->ContainerName == NULL ? L"" : backupStoreInfo->ContainerName;
    folderPath_ = backupStoreInfo->FolderPath == NULL ? L"" : backupStoreInfo->FolderPath;

    return ErrorCodeValue::Success;
}
