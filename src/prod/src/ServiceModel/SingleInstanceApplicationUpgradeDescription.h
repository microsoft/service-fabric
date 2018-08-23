//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class SingleInstanceApplicationUpgradeDescription
        : public SingleInstanceDeploymentUpgradeDescription
    {
        DENY_COPY(SingleInstanceApplicationUpgradeDescription)

    public:
        SingleInstanceApplicationUpgradeDescription() = default;
        SingleInstanceApplicationUpgradeDescription(
            std::wstring && deploymentName,
            std::wstring && applicationName,
            ServiceModel::UpgradeType::Enum upgradeType,
            ServiceModel::RollingUpgradeMode::Enum upgradeMode,
            bool const forceRestart,
            ServiceModel::RollingUpgradeMonitoringPolicy && monitoringPolicy,
            bool const isHealthPolicyValid,
            ServiceModel::ApplicationHealthPolicy && applicationHealthPolicy,
            Common::TimeSpan const & replicaSetCheckTimeout,
            ModelV2::ApplicationDescription const & targetApplicationDescription);

        FABRIC_FIELDS_01(
            TargetApplicationDescription);

    public:
        ModelV2::ApplicationDescription TargetApplicationDescription;

    };
}
