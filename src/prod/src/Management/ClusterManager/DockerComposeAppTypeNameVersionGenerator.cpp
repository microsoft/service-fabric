// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Naming;
using namespace ServiceModel;
using namespace Management::ClusterManager;

ErrorCode DockerComposeAppTypeNameVersionGenerator::GetTypeNameAndVersion(
    Store::StoreTransaction const &storeTx,
    NamingUri const &,
    __out ServiceModelTypeName &typeName,
    __out ServiceModelVersion &typeVersion)
{
    StoreDataSingleInstanceDeploymentCounter deploymentInstanceCounter;

    return deploymentInstanceCounter.GetComposeDeploymentTypeNameAndVersion(storeTx, typeName, typeVersion);
}

ErrorCode DockerComposeAppTypeNameVersionGenerator::GetNextVersion(
    Store::StoreTransaction const &storeTx,
    wstring const & deploymentName,
    ServiceModelVersion const & currentTypeVersion,
    __out ServiceModelVersion &typeVersion)
{
    uint64 currentVersion = 0, previousTargetVersion = 0;

    auto error = StoreDataSingleInstanceDeploymentCounter::GetVersionNumber(currentTypeVersion, &currentVersion);
    if (!error.IsSuccess())
    {
        return error;
    }

    ServiceModelVersion previousVersion;
    error = GetPreviousTargetVersion(storeTx, deploymentName, previousVersion);
    if (error.IsSuccess())
    {
        //
        // If the upgrade failed without rollback, the current version and the target version of that upgrade could
        // be active, so pick a version > both the versions as the next version.
        //
        error = StoreDataSingleInstanceDeploymentCounter::GetVersionNumber(previousVersion, &previousTargetVersion);
        if (!error.IsSuccess())
        {
            error = ErrorCodeValue::Success;
        }
    }

    auto nextVersion = currentVersion > previousTargetVersion ? (currentVersion + 1) : (previousTargetVersion + 1);
    typeVersion = StoreDataSingleInstanceDeploymentCounter::GenerateServiceModelVersion(nextVersion);

    return ErrorCodeValue::Success;
}

ErrorCode DockerComposeAppTypeNameVersionGenerator::GetPreviousTargetVersion(Store::StoreTransaction const &storeTx, wstring const &deploymentName, __out ServiceModelVersion &previousTargetVersion)
{
    unique_ptr<ComposeDeploymentUpgradeContext> composeUpgradeContext = make_unique<ComposeDeploymentUpgradeContext>(deploymentName);

    auto error = storeTx.ReadExact(*composeUpgradeContext);
    if (!error.IsSuccess())
    {
        return error;
    }

    previousTargetVersion = composeUpgradeContext->TargetTypeVersion;

    return ErrorCodeValue::Success;
}
