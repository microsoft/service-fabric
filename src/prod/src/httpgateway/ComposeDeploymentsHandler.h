// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class ComposeDeploymentsHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ComposeDeploymentsHandler)

    public:
        ComposeDeploymentsHandler(HttpGatewayImpl & server);
        virtual ~ComposeDeploymentsHandler() {}

        Common::ErrorCode Initialize();

    private:
        class RegistryCredential : public Common::IFabricJsonSerializable
        {
        public:
            RegistryCredential() : passwordEncrypted_(false) {}

            __declspec(property(get = get_RegistryUserName)) std::wstring const& RegistryUserName;
            std::wstring const& get_RegistryUserName() const { return registryUserName_; }

            __declspec(property(get = get_RegistryPassword)) std::wstring const& RegistryPassword;
            std::wstring const& get_RegistryPassword() const { return registryPassword_; }

            __declspec(property(get = get_PasswordEncrypted)) bool PasswordEncrypted;
            bool get_PasswordEncrypted() const { return passwordEncrypted_; }

            std::wstring && TakeUserName() { return std::move(registryUserName_); }
            std::wstring && TakePassword() { return std::move(registryPassword_); }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RegistryUserName, registryUserName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RegistryPassword, registryPassword_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PasswordEncrypted, passwordEncrypted_)
                // Deprecated
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RepositoryUserName, registryUserName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RepositoryPassword, registryPassword_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring registryUserName_;
            std::wstring registryPassword_;
            bool passwordEncrypted_;
        };

        class CreateComposeDeploymentData : public Common::IFabricJsonSerializable
        {
        public:
            CreateComposeDeploymentData() {};

            __declspec(property(get = get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get = get_ComposeFileContent)) std::wstring const& ComposeFileContent;
            std::wstring const& get_ComposeFileContent() const { return composeFileContent_; }

            __declspec(property(get = get_RegistryCredential)) RegistryCredential const& RegistryCredentialObj;
            RegistryCredential const& get_RegistryCredential() const { return registryCredential_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::DeploymentName, deploymentName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ComposeFileContent, composeFileContent_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RegistryCredential, registryCredential_)
                // Deprecated
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RepositoryCredential, registryCredential_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring deploymentName_;
            std::wstring composeFileContent_;
            RegistryCredential registryCredential_;
        };

        class UpgradeComposeDeploymentData : public Common::IFabricJsonSerializable
        {
        public:
            UpgradeComposeDeploymentData() {};

            __declspec(property(get = get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get = get_ComposeFileContent)) std::wstring const& ComposeFileContent;
            std::wstring const& get_ComposeFileContent() const { return composeFileContent_; }

            __declspec(property(get = get_RegistryCredential)) RegistryCredential & RegistryCredentialObj;
            RegistryCredential & get_RegistryCredential() { return registryCredential_; }
    
            __declspec(property(get=get_UpgradeKind)) ServiceModel::UpgradeType::Enum &UpgradeKind;
            ServiceModel::UpgradeType::Enum const& get_UpgradeKind() const { return upgradeType_; }

            __declspec(property(get=get_UpgradeMode)) ServiceModel::RollingUpgradeMode::Enum &UpgradeMode;
            ServiceModel::RollingUpgradeMode::Enum const& get_UpgradeMode() const { return rollingUpgradeMode_; }

            __declspec(property(get=get_ForceRestart)) bool ForceRestart;
            bool get_ForceRestart() const { return forceRestart_; }

            __declspec(property(get=get_ReplicaChkTimeout)) ULONG ReplicaSetTimeoutInSec;
            ULONG get_ReplicaChkTimeout() const { return replicaSetCheckTimeoutInSeconds_; }
            
            __declspec(property(get=get_MonitoringPolicy)) ServiceModel::RollingUpgradeMonitoringPolicy const & MonitoringPolicy;
            ServiceModel::RollingUpgradeMonitoringPolicy const& get_MonitoringPolicy() const { return monitoringPolicy_; }

            __declspec(property(get=get_HealthPolicy)) std::shared_ptr<ServiceModel::ApplicationHealthPolicy> const & HealthPolicy;
            std::shared_ptr<ServiceModel::ApplicationHealthPolicy> const& get_HealthPolicy() const { return healthPolicy_; }

            ServiceModel::RollingUpgradeMonitoringPolicy && TakeMonitoringPolicy() { return std::move(monitoringPolicy_); }
            std::wstring && TakeDeploymentName() { return std::move(deploymentName_); }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::DeploymentName, deploymentName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ComposeFileContent, composeFileContent_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RegistryCredential, registryCredential_)
                // Deprecated
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RepositoryCredential, registryCredential_)

                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::UpgradeKind, upgradeType_)
                SERIALIZABLE_PROPERTY_ENUM_IF(ServiceModel::Constants::RollingUpgradeMode, rollingUpgradeMode_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::UpgradeReplicaSetCheckTimeoutInSeconds, replicaSetCheckTimeoutInSeconds_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::ForceRestart, forceRestart_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::MonitoringPolicy, monitoringPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::ApplicationHealthPolicy, healthPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring deploymentName_;
            std::wstring composeFileContent_;
            RegistryCredential registryCredential_;
            ServiceModel::UpgradeType::Enum upgradeType_;
            ULONG replicaSetCheckTimeoutInSeconds_;
            ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
            bool forceRestart_;
            ServiceModel::RollingUpgradeMonitoringPolicy monitoringPolicy_;
            std::shared_ptr<ServiceModel::ApplicationHealthPolicy> healthPolicy_;
        };

    private:
        void GetComposeDeploymentApiHandlers(std::vector<HandlerUriTemplate> & handlerUris);

        void GetAllComposeDeployments(Common::AsyncOperationSPtr const & thisSPtr);
        void OnGetAllComposeDeploymentsComplete(Common::AsyncOperationSPtr const &, bool);

        void GetComposeDeploymentById(Common::AsyncOperationSPtr const & thisSPtr);
        void OnGetComposeDeploymentByIdComplete(Common::AsyncOperationSPtr const &, bool);

        void DeleteComposeDeployment(Common::AsyncOperationSPtr const & thisSPtr);
        void OnDeleteComposeDeploymentComplete(Common::AsyncOperationSPtr const &, bool);

        void CreateComposeDeployment(Common::AsyncOperationSPtr const & thisSPtr);
        void OnCreateComposeDeploymentComplete(Common::AsyncOperationSPtr const &, bool);

        void UpgradeComposeDeployment(Common::AsyncOperationSPtr const & thisSPtr);
        void OnUpgradeComposeDeploymentComplete(Common::AsyncOperationSPtr const &, bool);

        void GetUpgradeProgress(Common::AsyncOperationSPtr const & thisSPtr);
        void OnGetUpgradeProgressComplete(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void RollbackComposeDeployment(Common::AsyncOperationSPtr const & thisSPtr);
        void OnRollbackComposeDeploymentComplete(Common::AsyncOperationSPtr const &, bool);
    };
}
