// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using Common::ComUtility;

using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace std;

using namespace Management::UpgradeOrchestrationService;

ComFabricUpgradeOrchestrationServiceStateResult::ComFabricUpgradeOrchestrationServiceStateResult(
    IFabricUpgradeOrchestrationServiceStateResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE * STDMETHODCALLTYPE ComFabricUpgradeOrchestrationServiceStateResult::get_State()
{
    auto result = impl_->GetState();
    auto statePtr = heap_.AddItem<FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE>();
	result->ToPublicApi(heap_, *statePtr);

    return statePtr.GetRawPointer();
}
