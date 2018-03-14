// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ApplicationDescription.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Service;
        class ServicePackageDescription;

        class Application
        {
            DENY_COPY(Application);

        public:
            explicit Application(ApplicationDescription && appDesc);

            Application(Application && other);

            static Common::GlobalWString const FormatHeader;

            __declspec (property(get = get_ApplicationDescription)) ApplicationDescription const& ApplicationDesc;
            ApplicationDescription const& get_ApplicationDescription() const { return applicationDesc_; }

            __declspec (property(get = get_Services)) std::set<std::wstring> const& Services;
            std::set<std::wstring> const& get_Services() const { return services_; }

            __declspec (property(get = get_UpgradeInProgess)) bool UpgradeInProgess;
            bool get_UpgradeInProgess() const { return ApplicationDesc.UpgradeInProgess; }

            __declspec (property(get = get_UpgradeCompletedUDs)) std::set<std::wstring> const& UpgradeCompletedUDs;
            std::set<std::wstring> const& get_UpgradeCompletedUDs() const { return ApplicationDesc.UpgradeCompletedUDs; }

            bool UpdateDescription(ApplicationDescription && desc);

            void AddService(std::wstring const& serviceName);

            void AddServicePackage(ServicePackageDescription const & servicePackage);

            size_t RemoveService(std::wstring const& serviceName);

            int64 GetReservedCapacity(std::wstring const& metricName) const;

            void GetChangedServicePackages(ApplicationDescription const& newDescription,
                std::set<ServiceModel::ServicePackageIdentifier> & deletedSPs,
                std::set<ServiceModel::ServicePackageIdentifier> & updatedSPs);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            ApplicationDescription applicationDesc_;
            //It is important that this be an ordered set. The order is useful while adding metric connections.
            std::set<std::wstring> services_;

        };
    }
}
