// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupOperationResultMessageBody::BackupOperationResultMessageBody()
{
}

BackupOperationResultMessageBody::BackupOperationResultMessageBody(const FABRIC_BACKUP_OPERATION_RESULT& operationResult) :
    serviceName_(operationResult.ServiceName),
    partitionId_(operationResult.PartitionId),
    operationTimestampUtc_(operationResult.TimeStampUtc),
    operationId_(operationResult.OperationId),
    errorCode_(operationResult.ErrorCode)
{
    if (operationResult.Message != NULL)
    {
        Common::StringUtility::LpcwstrToTruncatedWstring(operationResult.Message, false, 0, 4096, message_);
    }

    if (errorCode_ == ERROR_SUCCESS)
    {
        backupId_ = Guid(operationResult.BackupId);
        backupLocation_ = operationResult.BackupLocation;
        epoch_ = operationResult.EpochOfLastBackupRecord;
        lsn_ = operationResult.LsnOfLastBackupRecord;
    }
}

BackupOperationResultMessageBody::~BackupOperationResultMessageBody()
{
}
