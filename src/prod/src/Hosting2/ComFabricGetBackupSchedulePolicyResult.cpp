// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// ComFabricGetBackupSchedulePolicyResult::ComFabricGetBackupSchedulePolicyResult Implementation
//

ComFabricGetBackupSchedulePolicyResult::ComFabricGetBackupSchedulePolicyResult(
    FabricGetBackupSchedulePolicyResultImplSPtr impl)
    : IFabricGetBackupSchedulePolicyResult(),
    ComUnknownBase(),
    policyResultImplSPtr_(impl)
{
}

ComFabricGetBackupSchedulePolicyResult::~ComFabricGetBackupSchedulePolicyResult()
{
}

const FABRIC_BACKUP_POLICY * ComFabricGetBackupSchedulePolicyResult::get_BackupSchedulePolicy(void)
{
    return policyResultImplSPtr_->get_BackupSchedulePolicy();
}
