// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ComposeDeploymentUpgradeProgress
        : public Serialization::FabricSerializable
    {
        public:
            ComposeDeploymentUpgradeProgress();

            ComposeDeploymentUpgradeProgress(
                std::wstring deploymentName,
                Common::NamingUri && applicationName,
                RollingUpgradeMode::Enum rollingUpgradeMode,
                ULONG replicaSetCheckTimeoutInSeconds,
                RollingUpgradeMonitoringPolicy const & monitoringPolicy,
                std::shared_ptr<ApplicationHealthPolicy> healthPolicy,
                wstring const & targetAppTypeVersion,
                ComposeDeploymentUpgradeState::Enum upgradeState,
                std::wstring && statusDetails);
        
            ComposeDeploymentUpgradeProgress(
                std::wstring const & deploymentName,
                Common::NamingUri && applicationName,
                UpgradeType::Enum const upgradeType,
                RollingUpgradeMode::Enum const rollingUpgradeMode,
                ULONG const replicaSetCheckTimeoutInSeconds,
                RollingUpgradeMonitoringPolicy const & monitoringPolicy,
                std::shared_ptr<ApplicationHealthPolicy> healthPolicy,
                std::wstring const & targetAppTypeVersion,
                ComposeDeploymentUpgradeState::Enum const upgradeState,
                std::wstring && inProgressUpgradeDomain,
                std::vector<std::wstring> completedUpgradeDomains,
                std::vector<std::wstring> pendingUpgradeDomains,
                uint64 upgradeInstance,
                Common::TimeSpan upgradeDuration,
                Common::TimeSpan currentUpgradeDomainDuration,
                std::vector<HealthEvaluation> && unhealthyEvaluations,
                Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress,
                std::wstring && statusDetails,
                Management::ClusterManager::CommonUpgradeContextData && commonUpgradeContextData);

            __declspec(property(get=get_DeploymentName)) std::wstring const & DeploymentName;
            std::wstring const & get_DeploymentName() const { return deploymentName_; }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_UpgradeKind)) UpgradeType::Enum UpgradeKind;
            UpgradeType::Enum get_UpgradeKind() const { return upgradeType_; }

            __declspec(property(get=get_RollingUpgradeMode)) RollingUpgradeMode::Enum RollingUpgradeMode;
            RollingUpgradeMode::Enum get_RollingUpgradeMode() const { return rollingUpgradeMode_; }

            __declspec(property(get=get_ForceRestart)) bool ForceRestart;
            bool get_ForceRestart() const { return forceRestart_; }

            __declspec(property(get=get_ReplicaSetCheckTimeoutInSeconds)) ULONG ReplicaSetCheckTimeoutInSeconds;
            ULONG get_ReplicaSetCheckTimeoutInSeconds() const { return replicaSetCheckTimeoutInSeconds_; }

            __declspec(property(get=get_HealthPolicy)) std::shared_ptr<ApplicationHealthPolicy> HealthPolicy;
            std::shared_ptr<ApplicationHealthPolicy> get_HealthPolicy() const { return healthPolicy_; }

            __declspec(property(get=get_TargetApplicationTypeVersion)) std::wstring const & TargetApplicationTypeVersion;
            std::wstring const & get_TargetApplicationTypeVersion() const { return targetAppTypeVersion_; }

            __declspec(property(get=get_ComposeDeploymentUpgradeState)) ComposeDeploymentUpgradeState::Enum const UpgradeState;
            ComposeDeploymentUpgradeState::Enum const get_ComposeDeploymentUpgradeState() const { return composeDeploymentUpgradeState_; }

            __declspec(property(get=get_InProgressUpgradeDomain)) std::wstring const & InProgressUpgradeDomain;
            std::wstring const & get_InProgressUpgradeDomain() const { return inProgressUpgradeDomain_; }

            __declspec(property(get=get_NextUpgradeDomain)) std::wstring const & NextUpgradeDomain;
            std::wstring const & get_NextUpgradeDomain() const { return nextUpgradeDomain_; }

            __declspec(property(get=get_CompletedUpgradeDomains)) std::vector<std::wstring> const & CompletedUpgradeDomains;
            std::vector<std::wstring> const & get_CompletedUpgradeDomains() const { return completedUpgradeDomains_; }

            __declspec(property(get=get_PendingUpgradeDomains)) std::vector<std::wstring> const & PendingUpgradeDomains;
            std::vector<std::wstring> const & get_PendingUpgradeDomains() const { return pendingUpgradeDomains_; }

            __declspec(property(get=get_UpgradeDuration)) Common::TimeSpan const & UpgradeDuration;
            Common::TimeSpan const & get_UpgradeDuration() const { return upgradeDuration_; }

            __declspec(property(get=get_CurrentUpgradeDomainDuration)) Common::TimeSpan const & CurrentUpgradeDomainDuration;
            Common::TimeSpan const & get_CurrentUpgradeDomainDuration() const { return currentUpgradeDomainDuration_; }

            FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE ToPublicUpgradeState() const;

            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS &) const;

            FABRIC_FIELDS_21(
                deploymentName_,
                applicationName_,
                upgradeType_,
                rollingUpgradeMode_,
                forceRestart_,
                replicaSetCheckTimeoutInSeconds_,
                monitoringPolicy_,
                healthPolicy_,
                targetAppTypeVersion_,
                composeDeploymentUpgradeState_,
                inProgressUpgradeDomain_,
                nextUpgradeDomain_,
                completedUpgradeDomains_,
                pendingUpgradeDomains_,
                upgradeDuration_,
                currentUpgradeDomainDuration_,
                unhealthyEvaluations_,
                currentUpgradeDomainProgress_,
                statusDetails_,
                commonUpgradeContextData_,
                publicUpgradeState_)
    
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(Constants::DeploymentName, deploymentName_)
                SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
                SERIALIZABLE_PROPERTY_ENUM(Constants::UpgradeState, publicUpgradeState_)
                SERIALIZABLE_PROPERTY(Constants::UpgradeStatusDetails, statusDetails_)
                SERIALIZABLE_PROPERTY_ENUM(Constants::UpgradeKind, upgradeType_)
                SERIALIZABLE_PROPERTY_ENUM_IF(Constants::RollingUpgradeMode, rollingUpgradeMode_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(Constants::ForceRestart, forceRestart_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(Constants::UpgradeReplicaSetCheckTimeoutInSeconds, replicaSetCheckTimeoutInSeconds_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(Constants::MonitoringPolicy, monitoringPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY_IF(Constants::ApplicationHealthPolicy, healthPolicy_, upgradeType_ == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
                SERIALIZABLE_PROPERTY(Constants::TargetApplicationTypeVersion, targetAppTypeVersion_)
                SERIALIZABLE_PROPERTY(Constants::UpgradeDuration, upgradeDuration_)
                SERIALIZABLE_PROPERTY(Constants::CurrentUpgradeDomainDuration, currentUpgradeDomainDuration_)
                SERIALIZABLE_PROPERTY(Constants::ApplicationUnhealthyEvaluations, unhealthyEvaluations_);
                SERIALIZABLE_PROPERTY(Constants::CurrentUpgradeDomainProgress, currentUpgradeDomainProgress_)
                SERIALIZABLE_PROPERTY(Constants::StartTimestampUtc, startTime_)
                SERIALIZABLE_PROPERTY(Constants::FailureTimestampUtc, failureTime_)
                SERIALIZABLE_PROPERTY(Constants::UpgradeDomainProgressAtFailure, commonUpgradeContextData_.UpgradeProgressAtFailure)
                SERIALIZABLE_PROPERTY(Constants::ApplicationUpgradeStatusDetails, applicationStatusDetails_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring deploymentName_;
            Common::NamingUri applicationName_;
            UpgradeType::Enum upgradeType_;
            
            // upgrade policy
            RollingUpgradeMode::Enum rollingUpgradeMode_;
            bool forceRestart_;
            ULONG replicaSetCheckTimeoutInSeconds_;
            RollingUpgradeMonitoringPolicy monitoringPolicy_;
            std::shared_ptr<ApplicationHealthPolicy> healthPolicy_;

            std::wstring targetAppTypeVersion_;
            ComposeDeploymentUpgradeState::Enum composeDeploymentUpgradeState_;
            FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE publicUpgradeState_;

            std::wstring inProgressUpgradeDomain_;
            std::wstring nextUpgradeDomain_;
            std::vector<std::wstring> completedUpgradeDomains_;
            std::vector<std::wstring> pendingUpgradeDomains_;
            uint64 upgradeInstance_;
            Common::TimeSpan upgradeDuration_;
            Common::TimeSpan currentUpgradeDomainDuration_;
            std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations_;
            Reliability::UpgradeDomainProgress currentUpgradeDomainProgress_;
            std::wstring statusDetails_;
            
            Management::ClusterManager::CommonUpgradeContextData commonUpgradeContextData_;

            // For JSON macro
            // TODO: Start time is not accurate yet. Only in "Application Upgrade" scope.
            Common::DateTime startTime_;
            Common::DateTime failureTime_;
            std::wstring applicationStatusDetails_;
    };
}
