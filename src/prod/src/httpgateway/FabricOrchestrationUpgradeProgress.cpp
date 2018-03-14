// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Management::UpgradeOrchestrationService;
using namespace ServiceModel;
using namespace Naming;
using namespace HttpGateway;

FabricOrchestrationUpgradeProgress::FabricOrchestrationUpgradeProgress()
    : upgradeState_(FABRIC_UPGRADE_STATE_INVALID)
{
}

ErrorCode FabricOrchestrationUpgradeProgress::FromInternalInterface(IFabricOrchestrationUpgradeStatusResultPtr &resultPtr)
{
    auto progress = resultPtr->GetProgress();
    upgradeState_ = OrchestrationUpgradeState::ToPublicApi(progress->State);
    progressStatus_ = progress->ProgressStatus;
    configVersion_ = progress->ConfigVersion;
    details_ = progress->Details;
    return ErrorCode::Success();
}
