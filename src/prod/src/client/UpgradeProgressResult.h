// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class UpgradeProgressResult
        : public Api::IUpgradeProgressResult
        , public Common::ComponentRoot
    {
        DENY_COPY(UpgradeProgressResult);

    public:
        UpgradeProgressResult(
            __in Management::ClusterManager::FabricUpgradeStatusDescription &&desc);

        Common::FabricCodeVersion const& GetTargetCodeVersion();

        Common::FabricConfigVersion const& GetTargetConfigVersion();

        Management::ClusterManager::FabricUpgradeState::Enum GetUpgradeState();

        Common::ErrorCode GetUpgradeDomains(
            __out std::wstring &inProgressDomain,
            __out std::vector<std::wstring> &pendingDomains, 
            __out std::vector<std::wstring> &completedDomains);

        Common::ErrorCode GetChangedUpgradeDomains(
            Api::IUpgradeProgressResultPtr const &previousProgress,
            __out std::wstring &changedInProgressDomain,
            __out std::vector<std::wstring> &changedCompletedDomains);

        ServiceModel::RollingUpgradeMode::Enum GetRollingUpgradeMode();

        std::wstring const& GetNextUpgradeDomain();

        uint64 GetUpgradeInstance();

        std::shared_ptr<Management::ClusterManager::FabricUpgradeDescription> const & GetUpgradeDescription();

        Common::TimeSpan const GetUpgradeDuration();

        Common::TimeSpan const GetCurrentUpgradeDomainDuration();

        std::vector<ServiceModel::HealthEvaluation> const & GetHealthEvaluations() const;

        Reliability::UpgradeDomainProgress const& GetCurrentUpgradeDomainProgress() const;

        // Expose CommonContextData members

        Common::DateTime GetStartTime() const;

        Common::DateTime GetFailureTime() const;

        Management::ClusterManager::UpgradeFailureReason::Enum GetFailureReason() const;

        Reliability::UpgradeDomainProgress const& GetUpgradeDomainProgressAtFailure() const;

        // Public API conversions

        FABRIC_UPGRADE_STATE ToPublicUpgradeState() const;

        void ToPublicUpgradeDomains(
            __in Common::ScopedHeap &,
            __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const;

        Common::ErrorCode ToPublicChangedUpgradeDomains(
            __in Common::ScopedHeap &, 
            Api::IUpgradeProgressResultPtr const &,
            __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const;

        LPCWSTR ToPublicNextUpgradeDomain() const;

        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_UPGRADE_PROGRESS &) const;

    private:
        Management::ClusterManager::FabricUpgradeStatusDescription upgradeDescription_;
    };
}
