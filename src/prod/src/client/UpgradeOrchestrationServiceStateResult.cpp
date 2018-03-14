// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::UpgradeOrchestrationService;
using namespace Client;

UpgradeOrchestrationServiceStateResult::UpgradeOrchestrationServiceStateResult(
    shared_ptr<UpgradeOrchestrationServiceState> && state)
    : state_(move(state))
{
}

std::shared_ptr<UpgradeOrchestrationServiceState> const & UpgradeOrchestrationServiceStateResult::GetState()
{
    return state_;
}
