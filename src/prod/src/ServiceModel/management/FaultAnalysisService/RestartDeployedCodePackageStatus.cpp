// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("RestartDeployedCodePackageStatus");

RestartDeployedCodePackageStatus::RestartDeployedCodePackageStatus()
: deployedCodePackageResultSPtr_()
{
}

RestartDeployedCodePackageStatus::RestartDeployedCodePackageStatus(
    shared_ptr<Management::FaultAnalysisService::DeployedCodePackageResult> && deployedCodePackageResultSPtr)
    : deployedCodePackageResultSPtr_(std::move(deployedCodePackageResultSPtr))
{
}

RestartDeployedCodePackageStatus::RestartDeployedCodePackageStatus(RestartDeployedCodePackageStatus && other)
: deployedCodePackageResultSPtr_(move(other.deployedCodePackageResultSPtr_))
{
}

RestartDeployedCodePackageStatus & RestartDeployedCodePackageStatus::operator=(RestartDeployedCodePackageStatus && other)
{
    if (this != &other)
    {
        deployedCodePackageResultSPtr_ = move(other.deployedCodePackageResultSPtr_);
    }

    return *this;
}

shared_ptr<DeployedCodePackageResult> const & RestartDeployedCodePackageStatus::GetRestartDeployedCodePackageResult()
{
    return deployedCodePackageResultSPtr_;
}
