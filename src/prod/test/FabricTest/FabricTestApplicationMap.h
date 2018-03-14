// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestApplicationMap
    {
        DENY_COPY(FabricTestApplicationMap)

    public:     
        FabricTestApplicationMap() 
        {
        }

        void AddApplication(std::wstring const& appName, std::wstring const& appTypeName, std::wstring const& appBuilderName);
        void UpdateApplicationId(wstring const& appName, Management::ClusterManager::ServiceModelApplicationId appId);
        void DeleteApplication(std::wstring const& appName);

        void StartUpgradeApplication(std::wstring const& appName, std::wstring const& newAppBuilderName, bool isForceRestart);
        void MarkUpgradeComplete(std::wstring const& appName);

        void RollbackUpgrade(std::wstring const& appName, std::wstring const& oldAppBuilderName);
        void MarkRollbackComplete(std::wstring const& appName);

        bool IsUpgrading(std::wstring const& appName);
        bool IsRollingback(std::wstring const& appName);
        bool IsUpgradeOrRollbackPending(std::wstring const& appName);
        bool IsUpgradeOrRollbackPending(ServiceModel::ApplicationIdentifier const& appId, bool & isForceRestart);

        void InterruptUpgrade(std::wstring const& appName);
        bool HasUpgradeBeenInterrupted(std::wstring const& appName);

        void SetRollingUpgradeMode(std::wstring const& appName, FABRIC_ROLLING_UPGRADE_MODE);
        void SetUpgradeState(std::wstring const& appName, FABRIC_APPLICATION_UPGRADE_STATE);
        void AddCompletedUpgradeDomain(std::wstring const& appName, std::wstring const& upgradeDomain);
        bool AreUpgradeDomainsComplete(std::wstring const& appName, std::vector<std::wstring> const& upgradeDomains);
        bool VerifyUpgrade(
            std::wstring const& appName,
            std::vector<std::wstring> const & upgradeDomains, 
            FABRIC_ROLLING_UPGRADE_MODE,
            std::vector<FABRIC_APPLICATION_UPGRADE_STATE> const &);


        ServiceModel::ApplicationIdentifier GetAppId(std::wstring const& appName);

        std::wstring GetCurrentApplicationBuilderName(std::wstring const& appName);

        bool TryGetAppName(ServiceModel::ApplicationIdentifier const& appId, std::wstring & appName);

        bool IsDeleted(std::wstring & appName);

    private:

        class TesApplicationData;
        typedef std::shared_ptr<TesApplicationData> TestApplicationDataSPtr;
        std::map<std::wstring, TestApplicationDataSPtr> applicationDataMap_;
        Common::RwLock applicationDataMapLock_;
    };
};
