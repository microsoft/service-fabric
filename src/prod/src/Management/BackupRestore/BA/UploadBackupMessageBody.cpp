// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

UploadBackupMessageBody::UploadBackupMessageBody() :
    storeInfo_()
{
}



UploadBackupMessageBody::~UploadBackupMessageBody()
{
}


Common::ErrorCode UploadBackupMessageBody::FromPublicApi(FABRIC_BACKUP_UPLOAD_INFO const &uploadInfo, FABRIC_BACKUP_STORE_INFORMATION const & storeInfo)
{
    ErrorCode error(ErrorCodeValue::Success);

    error = storeInfo_.FromPublicApi(storeInfo);
    if (!error.IsSuccess()) { return error; }

    backupMetadataFilePath_ = uploadInfo.BackupMetadataFilePath;
    localBackupPath_ = uploadInfo.LocalBackupPath;
    if (uploadInfo.DestinationFolderName != nullptr)
    {
        destinationFolderName_ = uploadInfo.DestinationFolderName;
    }

    return error;
}

Common::ErrorCode UploadBackupMessageBody::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_BACKUP_UPLOAD_INFO &uploadInfo, __out FABRIC_BACKUP_STORE_INFORMATION & storeInfo) const
{
    ErrorCode error(ErrorCodeValue::Success);
    uploadInfo.BackupMetadataFilePath = heap.AddString(backupMetadataFilePath_);
    uploadInfo.DestinationFolderName = heap.AddString(destinationFolderName_);
    uploadInfo.LocalBackupPath = heap.AddString(localBackupPath_);
    uploadInfo.Reserved = NULL;

    error = storeInfo_.ToPublicApi(heap, storeInfo);
    return error;
}
