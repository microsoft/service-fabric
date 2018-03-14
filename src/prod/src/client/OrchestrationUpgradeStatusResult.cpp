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

OrchestrationUpgradeStatusResult::OrchestrationUpgradeStatusResult(
    shared_ptr<OrchestrationUpgradeProgress> && progress)
    : progress_(move(progress))
{
}

std::shared_ptr<OrchestrationUpgradeProgress> const & OrchestrationUpgradeStatusResult::GetProgress()
{
    return progress_;
}
