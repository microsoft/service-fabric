// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

// ********************************************************************************************************************
// ComFabricGetRestorePointDetailsResult::ComFabricGetRestorePointDetailsResult Implementation
//

ComFabricGetRestorePointDetailsResult::ComFabricGetRestorePointDetailsResult(
    FabricGetRestorePointDetailsResultImplSPtr impl)
    : IFabricGetRestorePointDetailsResult(),
    ComUnknownBase(),
    restorePointDetailsResultImplSPtr_(impl)
{
}

ComFabricGetRestorePointDetailsResult::~ComFabricGetRestorePointDetailsResult()
{
}

const FABRIC_RESTORE_POINT_DETAILS * ComFabricGetRestorePointDetailsResult::get_RestorePointDetails(void)
{
    return restorePointDetailsResultImplSPtr_->get_RestorePointDetails();
}
