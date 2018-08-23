//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class SingleInstanceDeploymentUpgradeDescription : public ServiceModel::ClientServerMessageBody
    {
        DENY_COPY(SingleInstanceDeploymentUpgradeDescription)

    public:
        SingleInstanceDeploymentUpgradeDescription()
            : deploymentName_()
            , applicationName_()
            , isHealthPolicyValid_(false)
            , forceRestart_(false)
            , upgradeType_(ServiceModel::UpgradeType::Enum::Invalid)
            , rollingUpgradeMode_(ServiceModel::RollingUpgradeMode::Enum::Invalid)
            , monitoringPolicy_()
        {
        }

        SingleInstanceDeploymentUpgradeDescription(
            std::wstring &&deploymentName,
            std::wstring &&applicationName,
            ServiceModel::UpgradeType::Enum,
            ServiceModel::RollingUpgradeMode::Enum,
            bool forceRestart,
            ServiceModel::RollingUpgradeMonitoringPolicy &&,
            bool isHealthPolicyValid,
            ServiceModel::ApplicationHealthPolicy &&,
            Common::TimeSpan const &replicaSetCheckTimeout);

        Common::ErrorCode Validate();

        __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum UpgradeType;
        __declspec(property(get=get_UpgradeMode)) ServiceModel::RollingUpgradeMode::Enum UpgradeMode;
        __declspec(property(get=get_ForceRestart)) bool ForceRestart;
        __declspec(property(get=get_HealthPolicyValid)) bool HealthPolicyValid;
        __declspec(property(get=get_ReplicaSetCheckTimeout)) Common::TimeSpan const & ReplicaSetCheckTimeout;
        __declspec(property(get=get_DeploymentName)) std::wstring const &DeploymentName;
        __declspec(property(get=get_MonitoringPolicy)) ServiceModel::RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
        __declspec(property(get=get_HealthPolicy)) ServiceModel::ApplicationHealthPolicy const & HealthPolicy;

        ServiceModel::UpgradeType::Enum get_UpgradeType() const { return upgradeType_; }
        ServiceModel::RollingUpgradeMode::Enum get_UpgradeMode() const { return rollingUpgradeMode_; }
        bool get_ForceRestart() const { return forceRestart_; }
        bool get_HealthPolicyValid() const { return isHealthPolicyValid_; }
        Common::TimeSpan const & get_ReplicaSetCheckTimeout() const { return replicaSetCheckTimeout_; }
        std::wstring const & get_DeploymentName() const { return deploymentName_; }
        ServiceModel::RollingUpgradeMonitoringPolicy const & get_MonitoringPolicy() const { return monitoringPolicy_; }
        ServiceModel::ApplicationHealthPolicy const & get_HealthPolicy() const { return healthPolicy_; }

        std::wstring && TakeDeploymentName() { return std::move(deploymentName_); }
        std::wstring && TakeApplicationName() { return std::move(applicationName_); }
        ServiceModel::RollingUpgradeMonitoringPolicy && TakeUpgradeMonitoringPolicy() { return std::move(monitoringPolicy_); }
        ServiceModel::ApplicationHealthPolicy && TakeApplicationHealthPolicy() { return std::move(healthPolicy_); }

        // The serialization is for upgrade progress query so that files and credentials are not needed.
        FABRIC_FIELDS_09(
            deploymentName_,
            applicationName_,
            upgradeType_,
            rollingUpgradeMode_,
            replicaSetCheckTimeout_,
            forceRestart_,
            monitoringPolicy_,
            healthPolicy_,
            isHealthPolicyValid_);

    private:
        std::wstring deploymentName_;
        std::wstring applicationName_;

        ServiceModel::UpgradeType::Enum upgradeType_;
        // Rolling upgrade policy
        
        ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
        Common::TimeSpan replicaSetCheckTimeout_;
        bool forceRestart_;
        ServiceModel::RollingUpgradeMonitoringPolicy monitoringPolicy_;
        ServiceModel::ApplicationHealthPolicy healthPolicy_;
        bool isHealthPolicyValid_;
    };
}
