//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::ClusterManager;
using namespace ServiceModel;

SingleInstanceApplicationUpgradeDescription::SingleInstanceApplicationUpgradeDescription(
    wstring && deploymentName,
    wstring && applicationName,
    UpgradeType::Enum upgradeType,
    RollingUpgradeMode::Enum upgradeMode,
    bool const forceRestart,
    RollingUpgradeMonitoringPolicy && monitoringPolicy,
    bool const isHealthPolicyValid,
    ApplicationHealthPolicy && applicationHealthPolicy,
    TimeSpan const & replicaSetCheckTimeout,
    ModelV2::ApplicationDescription const & targetApplicationDescription)
    : SingleInstanceDeploymentUpgradeDescription(
        move(deploymentName),
        move(applicationName),
        upgradeType,
        upgradeMode,
        forceRestart,
        move(monitoringPolicy),
        isHealthPolicyValid,
        move(applicationHealthPolicy),
        replicaSetCheckTimeout)
    , TargetApplicationDescription(targetApplicationDescription)
{
}
