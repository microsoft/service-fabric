// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ApplicationCapacitiesDescription.h"
#include "ServicePackageDescription.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ApplicationDescription
        {
            DENY_COPY_ASSIGNMENT(ApplicationDescription);
        public:
            friend class Application;

            static Common::GlobalWString const FormatHeader;

            ApplicationDescription(std::wstring && name);

            ApplicationDescription(
                std::wstring && name,
                std::map<std::wstring, ApplicationCapacitiesDescription> && capacities,
                int scaleoutCount);

            ApplicationDescription(
                std::wstring && name,
                std::map<std::wstring, ApplicationCapacitiesDescription> && capacities,
                int minimumNodes,
                int scaleoutCount,
                std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> && servicePackages = std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>(),
                std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> && upgradingServicePackages = std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>(),
                bool isUpgradeInProgress = false,
                std::set<std::wstring> && completedUDs = std::set<std::wstring>(),
                ServiceModel::ApplicationIdentifier applicationIdentifier = ServiceModel::ApplicationIdentifier());

            ApplicationDescription(ApplicationDescription const & other);

            ApplicationDescription(ApplicationDescription && other);

            ApplicationDescription & operator = (ApplicationDescription && other);

            bool operator == (ApplicationDescription const& other) const;
            bool operator != (ApplicationDescription const& other) const;

            __declspec (property(get = get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return appName_; }

            __declspec (property(get = get_AppCapacities)) std::map<std::wstring, ApplicationCapacitiesDescription> const& AppCapacities;
            std::map<std::wstring, ApplicationCapacitiesDescription> const& get_AppCapacities() const { return capacities_; }

            __declspec(property(get = get_MinimumNodes)) int MinimumNodes;
            int get_MinimumNodes() const { return minimumNodes_; }

            __declspec (property(get = get_ScaleoutCount)) int ScaleoutCount;
            int get_ScaleoutCount() const { return scaleoutCount_; }

            __declspec (property(get = get_ApplicationId, put = set_ApplicationId)) uint64 ApplicationId;
            uint64 get_ApplicationId() const { return applicationId_; }
            void set_ApplicationId(uint64 id) { applicationId_ = id; }

            __declspec (property(get = get_UpgradeInProgess)) bool UpgradeInProgess;
            bool get_UpgradeInProgess() const { return upgradeInProgess_; }

            __declspec (property(get = get_UpgradeCompletedUDs)) std::set<std::wstring> const& UpgradeCompletedUDs;
            std::set<std::wstring> const& get_UpgradeCompletedUDs() const { return upgradeCompletedUDs_; }

            __declspec(property(get = get_ServicePackages)) std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> const& ServicePackages;
            std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> const& get_ServicePackages() const { return servicePackages_; }

            __declspec(property(get = get_IsPLBSafetyCheckNeeded)) bool IsPLBSafetyCheckRequiredForUpgrade;
            bool get_IsPLBSafetyCheckNeeded() const { return packagesToCheck_.size() > 0; }

            __declspec(property(get = get_ServicePackageIDsToCheck)) std::set<ServiceModel::ServicePackageIdentifier> const& ServicePackageIDsToCheck;
            std::set<ServiceModel::ServicePackageIdentifier> const& get_ServicePackageIDsToCheck() const { return packagesToCheck_; }

            __declspec(property(get = get_ApplicationIdentifier)) ServiceModel::ApplicationIdentifier const& ApplicationIdentifier;
            ServiceModel::ApplicationIdentifier const& get_ApplicationIdentifier() const { return applicationIdentifer_; }

            bool HasScaleoutOrCapacity() const
            {
                return (scaleoutCount_ > 0 || !capacities_.empty());
            }

            bool HasCapacity() const
            {
                return (!capacities_.empty());
            }

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring appName_;
            // Metric to capacity map
            std::map<std::wstring, ApplicationCapacitiesDescription> capacities_;
            int scaleoutCount_;
            int minimumNodes_;
            uint64 applicationId_;

            // Service Packages that belong to this application
            std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> servicePackages_;

            // Service Packages that are in upgrade
            std::map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> upgradingServicePackages_;

            // Service Packages that need check for upgrade.
            std::set<ServiceModel::ServicePackageIdentifier> packagesToCheck_;

            bool upgradeInProgess_;
            std::set<std::wstring> upgradeCompletedUDs_;

            ServiceModel::ApplicationIdentifier applicationIdentifer_;
        };
    }
}
