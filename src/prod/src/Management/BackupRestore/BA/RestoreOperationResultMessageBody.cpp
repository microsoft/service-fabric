// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

RestoreOperationResultMessageBody::RestoreOperationResultMessageBody()
{
}

RestoreOperationResultMessageBody::RestoreOperationResultMessageBody(wstring serviceName, Common::Guid partitionId, Common::Guid operationId, HRESULT errorCode, Common::DateTime operationTimestampUtc, wstring message) :
    serviceName_(serviceName),
    partitionId_(partitionId),
    operationId_(operationId),
    errorCode_(errorCode),
    operationTimestampUtc_(operationTimestampUtc),
    message_(message)
{
}

RestoreOperationResultMessageBody::RestoreOperationResultMessageBody(const FABRIC_RESTORE_OPERATION_RESULT& operationResult) :
    serviceName_(operationResult.ServiceName),
    partitionId_(operationResult.PartitionId),
    operationId_(operationResult.OperationId),
    operationTimestampUtc_(operationResult.TimeStampUtc)
{
    errorCode_ = operationResult.ErrorCode;
    if (operationResult.Message != NULL)
    {
        Common::StringUtility::LpcwstrToTruncatedWstring(operationResult.Message, false, 0, 4096, message_);
    }
}

RestoreOperationResultMessageBody::~RestoreOperationResultMessageBody()
{
}
