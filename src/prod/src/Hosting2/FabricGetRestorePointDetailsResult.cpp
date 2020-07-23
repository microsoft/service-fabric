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
// FabricGetRestorePointDetailsResult::FabricGetRestorePointDetailsResult Implementation
//

FabricGetRestorePointDetailsResult::FabricGetRestorePointDetailsResult()
    : ComponentRoot()
{
}

FabricGetRestorePointDetailsResult::FabricGetRestorePointDetailsResult(
    __in Management::BackupRestoreAgentComponent::RestorePointDetails &restorePointDetails)
    : ComponentRoot(),
    restorePointDetails_(restorePointDetails)
{
}

FabricGetRestorePointDetailsResult::~FabricGetRestorePointDetailsResult()
{
}

const FABRIC_RESTORE_POINT_DETAILS * FabricGetRestorePointDetailsResult::get_RestorePointDetails(void)
{
    auto rpDetails = heap_.AddItem<FABRIC_RESTORE_POINT_DETAILS>();
    restorePointDetails_.ToPublicApi(heap_, *rpDetails);
    return rpDetails.GetRawPointer();
}
