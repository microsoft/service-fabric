// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::FaultAnalysisService;
using namespace Client;

RestartDeployedCodePackageResult::RestartDeployedCodePackageResult(
    RestartDeployedCodePackageStatus && status)
    : status_(move(status))
{
}

shared_ptr<DeployedCodePackageResult> const & RestartDeployedCodePackageResult::GetRestartDeployedCodePackageResult()
{
    return status_.GetRestartDeployedCodePackageResult();
}
