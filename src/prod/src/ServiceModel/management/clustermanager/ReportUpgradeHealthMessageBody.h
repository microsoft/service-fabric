// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ReportUpgradeHealthMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            ReportUpgradeHealthMessageBody()
            {
            }

            explicit ReportUpgradeHealthMessageBody(
                Common::NamingUri const & applicationName)
                : applicationName_(applicationName) 
                , healthyUpgradeDomains_()
                , upgradeInstance_(0)
            {
            }

            ReportUpgradeHealthMessageBody(
                Common::NamingUri const & applicationName,
                std::vector<std::wstring> && healthyUpgradeDomains,
                uint64 upgradeInstance)
                : applicationName_(applicationName) 
                , healthyUpgradeDomains_(std::move(healthyUpgradeDomains))
                , upgradeInstance_(upgradeInstance)
            {
            }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_UpgradeDomains)) std::vector<std::wstring> & UpgradeDomains;
            std::vector<std::wstring> & get_UpgradeDomains() { return healthyUpgradeDomains_; }

            __declspec(property(get=get_UpgradeInstance)) uint64 UpgradeInstance;
            uint64 get_UpgradeInstance() { return upgradeInstance_; }

            FABRIC_FIELDS_03(applicationName_, healthyUpgradeDomains_, upgradeInstance_);

        private:
            Common::NamingUri applicationName_;
            std::vector<std::wstring> healthyUpgradeDomains_;
            uint64 upgradeInstance_;
        };
    }
}
