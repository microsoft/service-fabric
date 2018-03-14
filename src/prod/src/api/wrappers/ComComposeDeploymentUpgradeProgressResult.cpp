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

ComComposeDeploymentUpgradeProgressResult::ComComposeDeploymentUpgradeProgressResult(
    ComposeDeploymentUpgradeProgress const & internalProgress)
{
    upgradeProgress_ = heap_.AddItem<FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS>();
    internalProgress.ToPublicApi(heap_, *upgradeProgress_);
}

const FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS * STDMETHODCALLTYPE ComComposeDeploymentUpgradeProgressResult::get_UpgradeProgress()
{
    return upgradeProgress_.GetRawPointer();
}
