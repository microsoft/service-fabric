// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

DownloadBackupJobQueue::DownloadBackupJobQueue(
    __in ComponentRoot & root)
    : BackupCopierJobQueueBase(wformatString("{0}", TraceComponent), root)
{
    this->InitializeConfigUpdates();
}

ConfigEntryBase & DownloadBackupJobQueue::GetThrottleConfigEntry()
{
    return BackupRestoreService::BackupRestoreServiceConfig::GetConfig().BackupCopierJobQueueThrottleEntry;
}

int DownloadBackupJobQueue::GetThrottleConfigValue()
{
    return BackupRestoreService::BackupRestoreServiceConfig::GetConfig().BackupCopierJobQueueThrottle;
}
