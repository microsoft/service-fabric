// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class UpgradeComposeDeploymentRequestHeaderBase :
        public Serialization::FabricSerializable
    {
    public:
        UpgradeComposeDeploymentRequestHeaderBase()
            : deploymentName_()
            , applicationName_()
            , composeFileSizes_()
            , sfSettingsFileSizes_()
            , registryUserName_()
            , registryPassword_()
            , isPasswordEncrypted_(false)
            , isHealthPolicyValid_(false)
            , forceRestart_(false)
            , upgradeType_(ServiceModel::UpgradeType::Enum::Invalid)
            , rollingUpgradeMode_(ServiceModel::RollingUpgradeMode::Enum::Invalid)
            , monitoringPolicy_()
        {
        }

        UpgradeComposeDeploymentRequestHeaderBase(
            std::vector<size_t> &&composeFileSizes,
            std::vector<size_t> &&sfSettingsFileSizes,
            ServiceModel::ComposeDeploymentUpgradeDescription const &description)
            : composeFileSizes_(move(composeFileSizes))
            , sfSettingsFileSizes_(move(sfSettingsFileSizes))
        {
            deploymentName_ = description.TakeDeploymentName();
            applicationName_ = description.TakeApplicationName();
            registryUserName_ = description.TakeRepoUserName();
            registryPassword_ = description.TakeRepoPassword();
            isPasswordEncrypted_ = description.PasswordEncrypted;
            isHealthPolicyValid_ = description.HealthPolicyValid;
            forceRestart_ = description.ForceRestart;
            upgradeType_ = description.UpgradeType;
            rollingUpgradeMode_ = description.UpgradeMode;
            replicaSetCheckTimeout_ = description.ReplicaSetCheckTimeout;
            monitoringPolicy_ = description.TakeUpgradeMonitoringPolicy();
            healthPolicy_ = description.TakeApplicationHealthPolicy();
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const 
        {
            w.Write("UpgradeComposeDeploymentRequestHeader{ ");
            w.Write("DeploymentName : {0},", deploymentName_);
            w.Write("ApplicationName : {0},", applicationName_);
            w.Write("# ComposeFiles : {0},", composeFileSizes_.size());
            w.Write("# Sf settings files : {0}", sfSettingsFileSizes_.size());
            w.Write("Repo username : {0}", registryUserName_);
            w.Write("Upgrade type : {0}", upgradeType_);
            w.Write("Rolling upgrade mode : {0}", rollingUpgradeMode_);
            w.Write("force restart : {0}", forceRestart_);
            w.Write("Health policy valid : {0}", isHealthPolicyValid_);
            w.Write(" }");
        }

        __declspec(property(get=get_ComposeFileSizes)) std::vector<size_t> const &ComposeFileSizes;
        inline std::vector<size_t> const& get_ComposeFileSizes() const { return composeFileSizes_; }

        __declspec(property(get=get_SfSettingsFileSizes)) std::vector<size_t> const &SFSettingsFileSizes;
        inline std::vector<size_t> const& get_SfSettingsFileSizes() const { return sfSettingsFileSizes_; }

        __declspec(property(get=get_ApplicationName)) std::wstring const &ApplicationName;
        inline std::wstring const& get_ApplicationName() const { return applicationName_; }
 
        __declspec(property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum UpgradeType;
        __declspec(property(get=get_UpgradeMode)) ServiceModel::RollingUpgradeMode::Enum UpgradeMode;
        __declspec(property(get=get_ForceRestart)) bool ForceRestart;
        __declspec(property(get=get_HealthPolicyValid)) bool HealthPolicyValid;
        __declspec(property(get=get_ReplicaSetCheckTimeout)) Common::TimeSpan const & ReplicaSetCheckTimeout;
        __declspec(property(get=get_PasswordEncrypted)) bool PasswordEncrypted;

        ServiceModel::UpgradeType::Enum get_UpgradeType() const { return upgradeType_; }
        ServiceModel::RollingUpgradeMode::Enum get_UpgradeMode() const { return rollingUpgradeMode_; }
        bool get_ForceRestart() const { return forceRestart_; }
        bool get_HealthPolicyValid() const { return isHealthPolicyValid_; }
        Common::TimeSpan const & get_ReplicaSetCheckTimeout() const { return replicaSetCheckTimeout_; }
        bool get_PasswordEncrypted() const { return isPasswordEncrypted_; }

        std::wstring && TakeDeploymentName() { return std::move(deploymentName_); }
        std::wstring && TakeApplicationName() { return std::move(applicationName_); }
        std::wstring && TakeRepoUserName() { return std::move(registryUserName_); }
        std::wstring && TakeRepoPassword() { return std::move(registryPassword_); }
        ServiceModel::RollingUpgradeMonitoringPolicy && TakeUpgradeMonitoringPolicy() { return std::move(monitoringPolicy_); }
        ServiceModel::ApplicationHealthPolicy && TakeApplicationHealthPolicy() { return std::move(healthPolicy_); }

        FABRIC_FIELDS_14(deploymentName_,
                         applicationName_,
                         composeFileSizes_,
                         sfSettingsFileSizes_,
                         registryUserName_,
                         registryPassword_,
                         isPasswordEncrypted_,
                         upgradeType_,
                         rollingUpgradeMode_,
                         forceRestart_,
                         replicaSetCheckTimeout_,
                         monitoringPolicy_,
                         healthPolicy_,
                         isHealthPolicyValid_);

    protected:
        std::wstring deploymentName_;
        std::wstring applicationName_;
        std::vector<size_t> composeFileSizes_;
        std::vector<size_t> sfSettingsFileSizes_;
        std::wstring registryUserName_;
        std::wstring registryPassword_;
        bool isPasswordEncrypted_;

        ServiceModel::UpgradeType::Enum upgradeType_;

        // Rolling upgrade policy
        ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
        Common::TimeSpan replicaSetCheckTimeout_;
        bool forceRestart_;
        ServiceModel::RollingUpgradeMonitoringPolicy monitoringPolicy_;
        ServiceModel::ApplicationHealthPolicy healthPolicy_;
        bool isHealthPolicyValid_;
    };

    class UpgradeComposeDeploymentRequestHeader :
        public Transport::MessageHeader<Transport::MessageHeaderId::UpgradeComposeDeploymentRequest>,
        public UpgradeComposeDeploymentRequestHeaderBase
    {
    public:
        UpgradeComposeDeploymentRequestHeader()
            : UpgradeComposeDeploymentRequestHeaderBase()
        {
        }

        UpgradeComposeDeploymentRequestHeader(
            std::vector<size_t> &&composeFileSizes,
            std::vector<size_t> &&sfSettingsFileSizes,
            ServiceModel::ComposeDeploymentUpgradeDescription const &description)
            : UpgradeComposeDeploymentRequestHeaderBase(
                move(composeFileSizes),
                move(sfSettingsFileSizes),
                description)
        {
        }
    };

    // v6.0 accepts UpgradeComposeDeploymentRequest_Compatibility header which we shipped as a conflicted one.
    class UpgradeComposeDeploymentRequestCompatibilityHeader :
        public Transport::MessageHeader<Transport::MessageHeaderId::UpgradeComposeDeploymentRequest_Compatibility>,
        public UpgradeComposeDeploymentRequestHeaderBase
    {
    public:
        UpgradeComposeDeploymentRequestCompatibilityHeader()
            : UpgradeComposeDeploymentRequestHeaderBase()
        {
        }

        UpgradeComposeDeploymentRequestCompatibilityHeader(
            std::vector<size_t> &&composeFileSizes,
            std::vector<size_t> &&sfSettingsFileSizes,
            ServiceModel::ComposeDeploymentUpgradeDescription const &description)
            : UpgradeComposeDeploymentRequestHeaderBase(
                move(composeFileSizes),
                move(sfSettingsFileSizes),
                description)
        {
        }
    };
}
