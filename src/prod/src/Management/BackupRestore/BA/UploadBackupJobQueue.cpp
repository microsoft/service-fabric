// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

UploadBackupJobQueue::UploadBackupJobQueue(
    __in ComponentRoot & root)
    : BackupCopierJobQueueBase(wformatString("{0}", TraceComponent), root)
{
    this->InitializeConfigUpdates();
}

ConfigEntryBase & UploadBackupJobQueue::GetThrottleConfigEntry()
{
    return BackupRestoreService::BackupRestoreServiceConfig::GetConfig().BackupCopierJobQueueThrottleEntry;
}

int UploadBackupJobQueue::GetThrottleConfigValue()
{
    return BackupRestoreService::BackupRestoreServiceConfig::GetConfig().BackupCopierJobQueueThrottle;
}

