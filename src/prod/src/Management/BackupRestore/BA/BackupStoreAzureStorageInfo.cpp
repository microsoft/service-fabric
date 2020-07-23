// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupStoreAzureStorageInfo::BackupStoreAzureStorageInfo() :
    connectionString_(L""),
    containerName_(L""),
    folderPath_(L""),
    isConnectionStringEncrypted_(false)
{
}

BackupStoreAzureStorageInfo::~BackupStoreAzureStorageInfo()
{
}

Common::ErrorCode BackupStoreAzureStorageInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION & backupStoreInfo) const
{
    backupStoreInfo.ConnectionString = heap.AddString(connectionString_);
    backupStoreInfo.ContainerName = heap.AddString(containerName_);
    backupStoreInfo.FolderPath = heap.AddString(folderPath_);
    backupStoreInfo.IsConnectionStringEncrypted = isConnectionStringEncrypted_ ? TRUE : FALSE;
    backupStoreInfo.Reserved = NULL;
    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupStoreAzureStorageInfo::FromPublicApi(
    FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION const * backupStoreInfo)
{
    connectionString_ = backupStoreInfo->ConnectionString == NULL ? L"" : backupStoreInfo->ConnectionString;
    containerName_ = backupStoreInfo->ContainerName == NULL ? L"" : backupStoreInfo->ContainerName;
    folderPath_ = backupStoreInfo->FolderPath == NULL ? L"" : backupStoreInfo->FolderPath;
    isConnectionStringEncrypted_ = backupStoreInfo->IsConnectionStringEncrypted == TRUE ? true : false;

    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupStoreAzureStorageInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION & backupStoreInfo) const
{
    backupStoreInfo.StorageCredentialsSourceLocation = heap.AddString(connectionString_);
    backupStoreInfo.ContainerName = heap.AddString(containerName_);
    backupStoreInfo.FolderPath = heap.AddString(folderPath_);
    backupStoreInfo.Reserved = NULL;
    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupStoreAzureStorageInfo::FromPublicApi(
    FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION const * backupStoreInfo)
{
    connectionString_ = backupStoreInfo->StorageCredentialsSourceLocation == NULL ? L"" : backupStoreInfo->StorageCredentialsSourceLocation;
    containerName_ = backupStoreInfo->ContainerName == NULL ? L"" : backupStoreInfo->ContainerName;
    folderPath_ = backupStoreInfo->FolderPath == NULL ? L"" : backupStoreInfo->FolderPath;

    return ErrorCodeValue::Success;
}
