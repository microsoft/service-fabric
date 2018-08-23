//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("SingleInstanceDeploymentUpgradeDescription");

SingleInstanceDeploymentUpgradeDescription::SingleInstanceDeploymentUpgradeDescription(
    std::wstring &&deploymentName,
    std::wstring &&applicationName,
    ServiceModel::UpgradeType::Enum upgradeType,
    ServiceModel::RollingUpgradeMode::Enum upgradeMode,
    bool forceRestart,
    ServiceModel::RollingUpgradeMonitoringPolicy &&monitoringPolicy,
    bool isHealthPolicyValid,
    ServiceModel::ApplicationHealthPolicy &&healthPolicy,
    Common::TimeSpan const &replicaSetCheckTimeout)
    : deploymentName_(move(deploymentName))
    , applicationName_(move(applicationName))
    , upgradeType_(upgradeType)
    , rollingUpgradeMode_(upgradeMode)
    , forceRestart_(forceRestart)
    , monitoringPolicy_(move(monitoringPolicy))
    , healthPolicy_(move(healthPolicy))
    , isHealthPolicyValid_(isHealthPolicyValid)
    , replicaSetCheckTimeout_(replicaSetCheckTimeout)
{
}

ErrorCode SingleInstanceDeploymentUpgradeDescription::Validate()
{
    if ((rollingUpgradeMode_ != RollingUpgradeMode::Monitored) &&
        (rollingUpgradeMode_ != RollingUpgradeMode::UnmonitoredAuto) &&
        (rollingUpgradeMode_ != RollingUpgradeMode::UnmonitoredManual))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}
