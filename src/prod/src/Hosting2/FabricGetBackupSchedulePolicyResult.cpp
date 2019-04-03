// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management::BackupRestoreAgentComponent;

// ********************************************************************************************************************
// FabricGetBackupSchedulePolicyResult::FabricGetBackupSchedulePolicyResult Implementation
//

FabricGetBackupSchedulePolicyResult::FabricGetBackupSchedulePolicyResult()
    : ComponentRoot()
{
}

FabricGetBackupSchedulePolicyResult::FabricGetBackupSchedulePolicyResult(__in Management::BackupRestoreAgentComponent::BackupPolicy &policy)
    : ComponentRoot(),
    backupPolicy_(policy)
{
}

FabricGetBackupSchedulePolicyResult::~FabricGetBackupSchedulePolicyResult()
{
}

const FABRIC_BACKUP_POLICY * FabricGetBackupSchedulePolicyResult::get_BackupSchedulePolicy(void)
{
    auto policy = heap_.AddItem<FABRIC_BACKUP_POLICY>();
    backupPolicy_.ToPublicApi(heap_, *policy);
    return policy.GetRawPointer();
}
