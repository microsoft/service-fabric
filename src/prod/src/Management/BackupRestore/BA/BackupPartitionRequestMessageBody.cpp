// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;

BackupPartitionRequestMessageBody::BackupPartitionRequestMessageBody()
{
}

BackupPartitionRequestMessageBody::BackupPartitionRequestMessageBody(GUID operationId, const FABRIC_BACKUP_CONFIGURATION& configuration) :
    operationId_(operationId)
    , backupConfiguration_()
{
    backupConfiguration_.FromPublicApi(configuration);
}

BackupPartitionRequestMessageBody::~BackupPartitionRequestMessageBody()
{
}
