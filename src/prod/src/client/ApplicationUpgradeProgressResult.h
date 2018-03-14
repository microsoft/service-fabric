// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ApplicationUpgradeProgressResult
        : public Api::IApplicationUpgradeProgressResult
        , public Common::ComponentRoot
    {
        DENY_COPY(ApplicationUpgradeProgressResult)

    public:
        ApplicationUpgradeProgressResult(
            __in Management::ClusterManager::ApplicationUpgradeStatusDescription &&desc);

        Common::NamingUri const& GetApplicationName();

        std::wstring const& GetApplicationTypeName();

        std::wstring const& GetTargetApplicationTypeVersion();

        Management::ClusterManager::ApplicationUpgradeState::Enum GetUpgradeState();

        Common::ErrorCode GetUpgradeDomains(
            __out std::wstring &inProgressDomain,
            __out std::vector<std::wstring> &pendingDomains, 
            __out std::vector<std::wstring> &completedDomains);

        Common::ErrorCode GetChangedUpgradeDomains(
            Api::IApplicationUpgradeProgressResultPtr const &previousProgress,
            __out std::wstring &changedInProgressDomain,
            __out std::vector<std::wstring> &changedCompletedDomains);

        ServiceModel::RollingUpgradeMode::Enum GetRollingUpgradeMode();

        std::wstring const& GetNextUpgradeDomain();

        uint64 GetUpgradeInstance();

        std::shared_ptr<Management::ClusterManager::ApplicationUpgradeDescription> const & GetUpgradeDescription();

        Common::TimeSpan const GetUpgradeDuration();

        Common::TimeSpan const GetCurrentUpgradeDomainDuration();

        std::vector<ServiceModel::HealthEvaluation> const & GetHealthEvaluations() const;

        Reliability::UpgradeDomainProgress const& GetCurrentUpgradeDomainProgress() const;

        // Expose CommonContextData members

        Common::DateTime GetStartTime() const;

        Common::DateTime GetFailureTime() const;

        Management::ClusterManager::UpgradeFailureReason::Enum GetFailureReason() const;

        Reliability::UpgradeDomainProgress const& GetUpgradeDomainProgressAtFailure() const;

        std::wstring const & GetUpgradeStatusDetails() const;

        // Public API conversions

        FABRIC_APPLICATION_UPGRADE_STATE ToPublicUpgradeState() const;

        void ToPublicUpgradeDomains(
            __in Common::ScopedHeap &,
            __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const;

        Common::ErrorCode ToPublicChangedUpgradeDomains(
            __in Common::ScopedHeap &, 
            Api::IApplicationUpgradeProgressResultPtr const &,
            __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const;

        LPCWSTR ToPublicNextUpgradeDomain() const;

        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_APPLICATION_UPGRADE_PROGRESS &) const;

    private:
        Management::ClusterManager::ApplicationUpgradeStatusDescription upgradeDescription_;
    };
}
