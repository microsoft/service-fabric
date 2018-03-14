// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IUpgradeProgressResult
    {
    public:
        virtual ~IUpgradeProgressResult() {};

        virtual Common::FabricCodeVersion const& GetTargetCodeVersion() = 0;

        virtual Common::FabricConfigVersion const& GetTargetConfigVersion() = 0;

        virtual Management::ClusterManager::FabricUpgradeState::Enum GetUpgradeState() = 0;

        virtual Common::ErrorCode GetUpgradeDomains(
            __out std::wstring &inProgressDomain,
            __out std::vector<std::wstring> &pendingDomains, 
            __out std::vector<std::wstring> &completedDomains) = 0;

        virtual Common::ErrorCode GetChangedUpgradeDomains(
            IUpgradeProgressResultPtr const &previousProgress,
            __out std::wstring &changedInProgressDomain,
            __out std::vector<std::wstring> &changedCompletedDomains) = 0;

        virtual ServiceModel::RollingUpgradeMode::Enum GetRollingUpgradeMode() = 0;

        virtual std::wstring const& GetNextUpgradeDomain() = 0;

        virtual uint64 GetUpgradeInstance() = 0;

        virtual std::shared_ptr<Management::ClusterManager::FabricUpgradeDescription> const & GetUpgradeDescription() = 0;

        virtual Common::TimeSpan const GetUpgradeDuration() = 0;

        virtual Common::TimeSpan const GetCurrentUpgradeDomainDuration() = 0;

        virtual std::vector<ServiceModel::HealthEvaluation> const & GetHealthEvaluations() const = 0;

        virtual Reliability::UpgradeDomainProgress const& GetCurrentUpgradeDomainProgress() const = 0;
        
        // Expose CommonContextData members

        virtual Common::DateTime GetStartTime() const = 0;

        virtual Common::DateTime GetFailureTime() const = 0;

        virtual Management::ClusterManager::UpgradeFailureReason::Enum GetFailureReason() const = 0;

        virtual Reliability::UpgradeDomainProgress const& GetUpgradeDomainProgressAtFailure() const = 0;

        // Public API conversions

        virtual FABRIC_UPGRADE_STATE ToPublicUpgradeState() const = 0;

        virtual void ToPublicUpgradeDomains(
            __in Common::ScopedHeap &,
            __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const = 0;

        virtual Common::ErrorCode ToPublicChangedUpgradeDomains(
            __in Common::ScopedHeap &, 
            IUpgradeProgressResultPtr const &,
            __out Common::ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> &) const = 0;

        virtual LPCWSTR ToPublicNextUpgradeDomain() const = 0;

        virtual void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_UPGRADE_PROGRESS &) const = 0;
    };
}
