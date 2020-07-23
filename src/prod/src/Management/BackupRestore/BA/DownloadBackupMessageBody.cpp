// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace Management;
using namespace BackupRestoreAgentComponent;

DownloadBackupMessageBody::DownloadBackupMessageBody() :
    storeInfo_()
{
}

DownloadBackupMessageBody::~DownloadBackupMessageBody()
{
}

Common::ErrorCode DownloadBackupMessageBody::FromPublicApi(FABRIC_BACKUP_DOWNLOAD_INFO const & downloadInfo, FABRIC_BACKUP_STORE_INFORMATION const & storeInfo)
{
    ErrorCode error(ErrorCodeValue::Success);

    error = storeInfo_.FromPublicApi(storeInfo);
    if (!error.IsSuccess()) { return error; }

    destinationRootPath_ = downloadInfo.DestinationRootPath;
    error = StringList::FromPublicApi(*downloadInfo.BackupLocations, backupLocationList_);
    return error;
}

Common::ErrorCode DownloadBackupMessageBody::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_BACKUP_DOWNLOAD_INFO & downloadInfo, __out FABRIC_BACKUP_STORE_INFORMATION & storeInfo) const
{
    ErrorCode error(ErrorCodeValue::Success);

    auto bkpLocationList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, backupLocationList_, bkpLocationList);

    downloadInfo.DestinationRootPath = heap.AddString(destinationRootPath_);
    downloadInfo.BackupLocations = bkpLocationList.GetRawPointer();

    error = storeInfo_.ToPublicApi(heap, storeInfo);
    return error;
}
