// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupStoreFileShareInfo::BackupStoreFileShareInfo() :
    accessType_(BackupStoreFileShareAccessType::Enum::None),
    fileSharePath_(L""),
    primaryUserName_(L""),
    primaryPassword_(L""),
    secondaryUserName_(L""),
    secondaryPassword_(L""),
    isPasswordEncrypted_(false)
{
}

BackupStoreFileShareInfo::~BackupStoreFileShareInfo()
{
}

Common::ErrorCode BackupStoreFileShareInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION & backupStoreInfo) const
{
    backupStoreInfo.AccessType = BackupStoreFileShareAccessType::ToPublic(accessType_);
    backupStoreInfo.FileSharePath = heap.AddString(fileSharePath_);
    backupStoreInfo.IsPasswordEncrypted = isPasswordEncrypted_ ? TRUE : FALSE;
    backupStoreInfo.PrimaryPassword = heap.AddString(primaryPassword_);
    backupStoreInfo.PrimaryUserName = heap.AddString(primaryUserName_);
    backupStoreInfo.SecondaryPassword = heap.AddString(secondaryPassword_);
    backupStoreInfo.SecondaryUserName = heap.AddString(secondaryUserName_);
    
    backupStoreInfo.Reserved = NULL;
    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupStoreFileShareInfo::FromPublicApi(
    FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION const * backupStoreInfo)
{
    ErrorCode error(ErrorCodeValue::Success);
    error = BackupStoreFileShareAccessType::FromPublic(backupStoreInfo->AccessType, accessType_);
    if (!error.IsSuccess()) { return error; }

    fileSharePath_ = backupStoreInfo->FileSharePath == NULL ? L"" : backupStoreInfo->FileSharePath;
    primaryUserName_ = backupStoreInfo->PrimaryUserName == NULL ? L"" : backupStoreInfo->PrimaryUserName;
    primaryPassword_ = backupStoreInfo->PrimaryPassword == NULL ? L"" : backupStoreInfo->PrimaryPassword;
    secondaryUserName_ = backupStoreInfo->SecondaryUserName == NULL ? L"" : backupStoreInfo->SecondaryUserName;
    secondaryPassword_ = backupStoreInfo->SecondaryPassword == NULL ? L"" : backupStoreInfo->SecondaryPassword;
    isPasswordEncrypted_ = backupStoreInfo->IsPasswordEncrypted ? true : false;
    return ErrorCodeValue::Success;
}
