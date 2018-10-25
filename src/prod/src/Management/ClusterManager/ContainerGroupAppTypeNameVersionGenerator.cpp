//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;
using namespace Naming;
using namespace ServiceModel;

ErrorCode ContainerGroupAppTypeNameVersionGenerator::GetTypeNameAndVersion(
    Store::StoreTransaction const & storeTx,
    NamingUri const &,
    __out ServiceModelTypeName & typeName,
    __out ServiceModelVersion & typeVersion)
{
    StoreDataSingleInstanceDeploymentCounter deploymentInstanceCounter;

    return deploymentInstanceCounter.GetSingleInstanceDeploymentTypeNameAndVersion(storeTx, typeName, typeVersion);
}

ErrorCode ContainerGroupAppTypeNameVersionGenerator::GetNextVersion(
    Store::StoreTransaction const &,
    wstring const &,
    ServiceModelVersion const & currentTypeVersion,
    __out ServiceModelVersion & typeVersion)
{
    uint64 currentVersion = 0;
    ErrorCode error = StoreDataSingleInstanceDeploymentCounter::GetVersionNumber(currentTypeVersion, &currentVersion);
    if (!error.IsSuccess())
    {
        return error;
    }

    typeVersion = StoreDataSingleInstanceDeploymentCounter::GenerateServiceModelVersion(currentVersion + 1);

    return ErrorCodeValue::Success;
}
