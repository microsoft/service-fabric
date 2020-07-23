// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Management;
using namespace ServiceModel;
using namespace BackupRestoreAgentComponent;

RestorePointDetails::RestorePointDetails() :
    userInitiatedOperation_(false),
    backupStoreInfo_()
{
}

RestorePointDetails::~RestorePointDetails()
{
}

Common::ErrorCode RestorePointDetails::FromPublicApi(FABRIC_RESTORE_POINT_DETAILS const &restorePointDetails)
{
    ErrorCode error(ErrorCodeValue::Success);

    userInitiatedOperation_ = restorePointDetails.UserInitiatedOperation != 0;
    operationId_ = Common::Guid(restorePointDetails.OperationId);
    error = backupStoreInfo_.FromPublicApi(*restorePointDetails.StoreInformation);
    if (!error.IsSuccess()) { return error; }
    error = StringList::FromPublicApi(*restorePointDetails.BackupLocations, backupLocationList_);
    return error;
}

Common::ErrorCode RestorePointDetails::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_RESTORE_POINT_DETAILS &restorePointDetails) const
{
    ErrorCode error(ErrorCodeValue::Success);

    restorePointDetails.OperationId = operationId_.AsGUID();
    restorePointDetails.UserInitiatedOperation = userInitiatedOperation_;

    auto storeInfo = heap.AddItem<FABRIC_BACKUP_STORE_INFORMATION>();
    error = backupStoreInfo_.ToPublicApi(heap, *storeInfo);
    if (!error.IsSuccess()) { return error; }

    restorePointDetails.StoreInformation = storeInfo.GetRawPointer();

    auto bkpLocationList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, backupLocationList_, bkpLocationList);

    restorePointDetails.BackupLocations = bkpLocationList.GetRawPointer();
    restorePointDetails.Reserved = NULL;

    return error;
}
