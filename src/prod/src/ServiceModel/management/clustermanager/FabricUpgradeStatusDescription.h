// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class FabricUpgradeStatusDescription : public Serialization::FabricSerializable
        {
            DENY_COPY(FabricUpgradeStatusDescription)

        public:
            FabricUpgradeStatusDescription();

            FabricUpgradeStatusDescription(FabricUpgradeStatusDescription &&);

            FabricUpgradeStatusDescription & operator=(FabricUpgradeStatusDescription &&);

            FabricUpgradeStatusDescription(
                Common::FabricVersion const & targetVersion,
                FabricUpgradeState::Enum upgradeState,
                std::wstring && inProgressUpgradeDomain,
                std::vector<std::wstring> && completedUpgradeDomains,
                std::vector<std::wstring> && pendingUpgradeDomains,
                uint64 upgradeInstance, 
                ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode,
                std::shared_ptr<FabricUpgradeDescription> && upgradeDescription,
                Common::TimeSpan const upgradeDuration,
                Common::TimeSpan const currentUpgradeDomainDuration,
                std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations, 
                Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress,
                CommonUpgradeContextData && commonUpgradeContextData);

            __declspec(property(get=get_TargetVersion)) Common::FabricVersion const & TargetVersion;
            __declspec(property(get=get_UpgradeState)) FabricUpgradeState::Enum UpgradeState;
            __declspec(property(get=get_InProgressUpgradeDomain)) std::wstring const & InProgressUpgradeDomain;
            __declspec(property(get=get_NextUpgradeDomain)) std::wstring const & NextUpgradeDomain;
            __declspec(property(get=get_CompletedUpgradeDomains)) std::vector<std::wstring> const & CompletedUpgradeDomains;
            __declspec(property(get=get_PendingUpgradeDomains)) std::vector<std::wstring> const & PendingUpgradeDomains;
            __declspec(property(get=get_UpgradeInstance)) uint64 UpgradeInstance;
            __declspec(property(get=get_RollingUpgradeMode)) ServiceModel::RollingUpgradeMode::Enum const & RollingUpgradeMode;
            __declspec(property(get=get_UpgradeDescription)) std::shared_ptr<FabricUpgradeDescription> const & UpgradeDescription;
            __declspec(property(get=get_UpgradeDuration)) Common::TimeSpan const UpgradeDuration;
            __declspec(property(get=get_CurrentUpgradeDomainDuration)) Common::TimeSpan const CurrentUpgradeDomainDuration;
            __declspec(property(get=get_UnhealthyEvaluations)) std::vector<ServiceModel::HealthEvaluation> const & UnhealthyEvaluations;
            __declspec(property(get=get_CurrentUpgradeDomainProgress)) Reliability::UpgradeDomainProgress const& CurrentUpgradeDomainProgress;
            __declspec(property(get=get_CommonUpgradeContextData)) ClusterManager::CommonUpgradeContextData const& CommonUpgradeContextData;

            Common::FabricVersion const & get_TargetVersion() const { return targetVersion_; }
            FabricUpgradeState::Enum const & get_UpgradeState() const { return upgradeState_; }
            std::wstring const & get_InProgressUpgradeDomain() const { return inProgressUpgradeDomain_; }
            std::wstring const & get_NextUpgradeDomain() const { return nextUpgradeDomain_; }
            std::vector<std::wstring> const & get_CompletedUpgradeDomains() const { return completedUpgradeDomains_; }
            std::vector<std::wstring> const & get_PendingUpgradeDomains() const { return pendingUpgradeDomains_; }
            uint64 get_UpgradeInstance() const { return upgradeInstance_; }
            ServiceModel::RollingUpgradeMode::Enum const & get_RollingUpgradeMode() const { return rollingUpgradeMode_; }
            std::shared_ptr<FabricUpgradeDescription> const & get_UpgradeDescription() const { return upgradeDescription_; }
            Common::TimeSpan const get_UpgradeDuration() const { return upgradeDuration_; }
            Common::TimeSpan const get_CurrentUpgradeDomainDuration() const { return currentUpgradeDomainDuration_; }
            std::vector<ServiceModel::HealthEvaluation> const & get_UnhealthyEvaluations() const { return unhealthyEvaluations_; }
            Reliability::UpgradeDomainProgress const& get_CurrentUpgradeDomainProgress() const { return currentUpgradeDomainProgress_; }
            ClusterManager::CommonUpgradeContextData const& get_CommonUpgradeContextData() const { return commonUpgradeContextData_; }

        public: // public API conversions

            FABRIC_UPGRADE_STATE ToPublicUpgradeState() const;

            void ToPublicUpgradeDomains(
                __in Common::ScopedHeap &,
                __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const;

            Common::ErrorCode ToPublicChangedUpgradeDomains(
                __in Common::ScopedHeap &, 
                Common::FabricCodeVersion const & prevTargetCodeVersion,
                Common::FabricConfigVersion const & prevTargetConfigVersion,
                std::wstring const & prevInProgress,
                std::vector<std::wstring> const & prevCompleted,
                __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const;

            LPCWSTR ToPublicNextUpgradeDomain() const;

            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_UPGRADE_PROGRESS &) const;

        public: // To be called by Api::IUpgradeProgressResult

            Common::FabricCodeVersion const& GetTargetCodeVersion() const;
            Common::FabricConfigVersion const& GetTargetConfigVersion() const;
            FabricUpgradeState::Enum GetUpgradeState() const;
           
            void GetUpgradeDomains(
               __out std::wstring & inProgressDomain,
               __out std::vector<std::wstring> & pendingDomains, 
               __out std::vector<std::wstring> & completedDomains) const;

            void GetChangedUpgradeDomains(
               std::wstring const & prevInProgress,
               std::vector<std::wstring> const & prevCompleted,
               __out std::wstring & changedInProgressDomain,
               __out std::vector<std::wstring> & changedCompletedDomains) const;

            ServiceModel::RollingUpgradeMode::Enum GetRollingUpgradeMode() const;
            std::wstring const& GetNextUpgradeDomain() const;
            uint64 GetUpgradeInstance() const;
            std::shared_ptr<FabricUpgradeDescription> const & GetUpgradeDescription() const;
            Common::TimeSpan const GetUpgradeDuration() const;
            Common::TimeSpan const GetCurrentUpgradeDomainDuration() const;

            std::vector<ServiceModel::HealthEvaluation> const & GetHealthEvaluations() const;
            Reliability::UpgradeDomainProgress const& GetCurrentUpgradeDomainProgress() const;

            Common::DateTime GetStartTime() const;

            Common::DateTime GetFailureTime() const;

            Management::ClusterManager::UpgradeFailureReason::Enum GetFailureReason() const;

            Reliability::UpgradeDomainProgress const& GetUpgradeDomainProgressAtFailure() const;

        public:

            FABRIC_FIELDS_14(
                targetVersion_, 
                upgradeState_,
                inProgressUpgradeDomain_,
                completedUpgradeDomains_,
                pendingUpgradeDomains_,
                upgradeInstance_,
                nextUpgradeDomain_,
                rollingUpgradeMode_,
                upgradeDescription_,
                upgradeDuration_,
                currentUpgradeDomainDuration_,
                unhealthyEvaluations_,
                currentUpgradeDomainProgress_,
                commonUpgradeContextData_);

        private:
            Common::FabricVersion targetVersion_;
            FabricUpgradeState::Enum upgradeState_;
            std::wstring inProgressUpgradeDomain_;
            std::wstring nextUpgradeDomain_;
            std::vector<std::wstring> completedUpgradeDomains_;
            std::vector<std::wstring> pendingUpgradeDomains_;
            uint64 upgradeInstance_;
            ServiceModel::RollingUpgradeMode::Enum rollingUpgradeMode_;
            std::shared_ptr<FabricUpgradeDescription> upgradeDescription_;
            Common::TimeSpan upgradeDuration_;
            Common::TimeSpan currentUpgradeDomainDuration_;
            std::vector<ServiceModel::HealthEvaluation> unhealthyEvaluations_;
            Reliability::UpgradeDomainProgress currentUpgradeDomainProgress_;
            ClusterManager::CommonUpgradeContextData commonUpgradeContextData_;
        };
    }
}
