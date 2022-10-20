// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupConfiguration::BackupConfiguration() :
    operationTimeoutMilliseconds_(0),
    backupStoreInfo_()
{
}

BackupConfiguration::~BackupConfiguration()
{
}

Common::ErrorCode BackupConfiguration::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_BACKUP_CONFIGURATION & fabricBackupConfiguration) const
{
    ErrorCode error(ErrorCodeValue::Success);
    auto storeInfo = heap.AddItem<FABRIC_BACKUP_STORE_INFORMATION>();

    fabricBackupConfiguration.OperationTimeoutMilliseconds = operationTimeoutMilliseconds_;
    error = backupStoreInfo_.ToPublicApi(heap, *storeInfo);

    fabricBackupConfiguration.StoreInformation = storeInfo.GetRawPointer();
    fabricBackupConfiguration.Reserved = NULL;

    return ErrorCodeValue::Success;
}

Common::ErrorCode BackupConfiguration::FromPublicApi(
    FABRIC_BACKUP_CONFIGURATION const & fabricBackupConfiguration)
{
    ErrorCode error(ErrorCodeValue::Success);

    operationTimeoutMilliseconds_ = fabricBackupConfiguration.OperationTimeoutMilliseconds;
    if (fabricBackupConfiguration.StoreInformation != NULL)
    {
        error = backupStoreInfo_.FromPublicApi(*fabricBackupConfiguration.StoreInformation);
    }
    
    return error;
}
