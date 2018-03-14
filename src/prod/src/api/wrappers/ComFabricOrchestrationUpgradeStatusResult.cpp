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

ComFabricOrchestrationUpgradeStatusResult::ComFabricOrchestrationUpgradeStatusResult(
    IFabricOrchestrationUpgradeStatusResultPtr const& impl)
    : impl_(impl)
    , heap_()
{
}

FABRIC_ORCHESTRATION_UPGRADE_PROGRESS * STDMETHODCALLTYPE ComFabricOrchestrationUpgradeStatusResult::get_Progress()
{
    auto result = impl_->GetProgress();
    auto progressPtr = heap_.AddItem<FABRIC_ORCHESTRATION_UPGRADE_PROGRESS>();
	result->ToPublicApi(heap_, *progressPtr);

    return progressPtr.GetRawPointer();
}
