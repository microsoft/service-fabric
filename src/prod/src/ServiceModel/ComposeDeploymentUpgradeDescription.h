// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ComposeDeploymentUpgradeDescription : public ServiceModel::ClientServerMessageBody
    {
        DENY_COPY(ComposeDeploymentUpgradeDescription)

    public:
        ComposeDeploymentUpgradeDescription()
            : deploymentName_()
            , applicationName_()
            , composeFiles_()
            , sfSettingsFiles_()
            , repositoryUserName_()
            , repositoryPassword_()
            , isPasswordEncrypted_(false)
            , isHealthPolicyValid_(false)
            , forceRestart_(false)
            , upgradeType_(ServiceModel::UpgradeType::Enum::Invalid)
            , rollingUpgradeMode_(ServiceModel::RollingUpgradeMode::Enum::Invalid)
            , monitoringPolicy_()
        {}

        ComposeDeploymentUpgradeDescription(
            std::wstring &&deploymentName,
            std::wstring &&applicationName,
            std::vector<Common::ByteBuffer> &&composeFiles,
            std::vector<Common::ByteBuffer> &&sfSettingsFiles,
            std::wstring &&repositoryUserName,
            std::wstring &&repositoryPassword,
            bool isPasswordEncrypted,
            ServiceModel::UpgradeType::Enum,
            ServiceModel::RollingUpgradeMode::Enum,
            bool forceRestart,
            ServiceModel::RollingUpgradeMonitoringPolicy &&,
            bool isHealthPolicyValid,
            ServiceModel::ApplicationHealthPolicy &&,
            Common::TimeSpan const &replicaSetCheckTimeout);

        Common::ErrorCode FromPublicApi(FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION const &upgradeDescription);
        Common::ErrorCode Validate();

        __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum UpgradeType;
        __declspec(property(get=get_UpgradeMode)) ServiceModel::RollingUpgradeMode::Enum UpgradeMode;
        __declspec(property(get=get_ForceRestart)) bool ForceRestart;
        __declspec(property(get=get_HealthPolicyValid)) bool HealthPolicyValid;
        __declspec(property(get=get_ReplicaSetCheckTimeout)) Common::TimeSpan const & ReplicaSetCheckTimeout;
        __declspec(property(get=get_PasswordEncrypted)) bool PasswordEncrypted;
        __declspec(property(get=get_RepoUserName)) std::wstring const &RegistryUserName;
        __declspec(property(get=get_RepoPassword)) std::wstring const &RegistryPassword;
        __declspec(property(get=get_DeploymentName)) std::wstring const &DeploymentName;
        __declspec(property(get=get_ComposeFiles)) std::vector<Common::ByteBuffer> const &ComposeFiles;
        __declspec(property(get=get_SfSettingsFiles)) std::vector<Common::ByteBuffer> const &SFSettingsFiles;
        __declspec(property(get=get_MonitoringPolicy)) ServiceModel::RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
        __declspec(property(get=get_HealthPolicy)) ServiceModel::ApplicationHealthPolicy const & HealthPolicy;

        ServiceModel::UpgradeType::Enum get_UpgradeType() const { return upgradeType_; }
        ServiceModel::RollingUpgradeMode::Enum get_UpgradeMode() const { return rollingUpgradeMode_; }
        bool get_ForceRestart() const { return forceRestart_; }
        bool get_HealthPolicyValid() const { return isHealthPolicyValid_; }
        Common::TimeSpan const & get_ReplicaSetCheckTimeout() const { return replicaSetCheckTimeout_; }
        std::wstring const & get_DeploymentName() const { return deploymentName_; }
        std::wstring const & get_RepoUserName() const { return repositoryUserName_; }
        std::wstring const & get_RepoPassword() const { return repositoryPassword_; }
        std::vector<Common::ByteBuffer> const & get_ComposeFiles() const { return composeFiles_; }
        std::vector<Common::ByteBuffer> const & get_SfSettingsFiles() const { return sfSettingsFiles_; }
        bool get_PasswordEncrypted() const { return isPasswordEncrypted_; }
        ServiceModel::RollingUpgradeMonitoringPolicy const & get_MonitoringPolicy() const { return monitoringPolicy_; }
        ServiceModel::ApplicationHealthPolicy const & get_HealthPolicy() const { return healthPolicy_; }

        std::wstring && TakeDeploymentName() const { return std::move(deploymentName_); }
        std::wstring && TakeApplicationName() const { return std::move(applicationName_); }
        std::wstring && TakeRepoUserName() const { return std::move(repositoryUserName_); }
        std::wstring && TakeRepoPassword() const { return std::move(repositoryPassword_); }
        std::vector<Common::ByteBuffer> && TakeComposeFiles() const { return std::move(composeFiles_); }
        std::vector<Common::ByteBuffer> && TakeSettingsFiles() const { return std::move(sfSettingsFiles_); }
        ServiceModel::RollingUpgradeMonitoringPolicy && TakeUpgradeMonitoringPolicy() const { return std::move(monitoringPolicy_); }
        ServiceModel::ApplicationHealthPolicy && TakeApplicationHealthPolicy() const { return std::move(healthPolicy_); }

        // The serialization if for upgrade progress query so that files and credentials are not needed.
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
        
        Common::ErrorCode GetFileContents(std::wstring const &filePath, __out Common::ByteBuffer &fileContents);

        mutable std::wstring deploymentName_;
        mutable std::wstring applicationName_;
        mutable std::vector<Common::ByteBuffer> composeFiles_;
        mutable std::vector<Common::ByteBuffer> sfSettingsFiles_;
        mutable std::wstring repositoryUserName_;
        mutable std::wstring repositoryPassword_;
        bool isPasswordEncrypted_;

        ServiceModel::UpgradeType::Enum upgradeType_;

        // Rolling upgrade policy
        ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
        Common::TimeSpan replicaSetCheckTimeout_;
        bool forceRestart_;
        mutable ServiceModel::RollingUpgradeMonitoringPolicy monitoringPolicy_;
        mutable ServiceModel::ApplicationHealthPolicy healthPolicy_;
        bool isHealthPolicyValid_;
    };
}
