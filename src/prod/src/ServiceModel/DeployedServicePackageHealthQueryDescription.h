// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedServicePackageHealthQueryDescription
    {
        DENY_COPY(DeployedServicePackageHealthQueryDescription)

    public:
        DeployedServicePackageHealthQueryDescription();

        DeployedServicePackageHealthQueryDescription(
            Common::NamingUri && applicationName,
            std::wstring && serviceManifestName,
            std::wstring && servicePackageActivationId,
            std::wstring && nodeName,
            std::unique_ptr<ApplicationHealthPolicy> && healthPolicy);

        DeployedServicePackageHealthQueryDescription(DeployedServicePackageHealthQueryDescription && other) = default;
        DeployedServicePackageHealthQueryDescription & operator = (DeployedServicePackageHealthQueryDescription && other) = default;

        ~DeployedServicePackageHealthQueryDescription();

        __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
        std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }

        __declspec(property(get=get_HealthPolicy)) std::unique_ptr<ApplicationHealthPolicy> const & HealthPolicy;
        std::unique_ptr<ApplicationHealthPolicy> const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_EventsFilter)) std::unique_ptr<HealthEventsFilter> const & EventsFilter;
        std::unique_ptr<HealthEventsFilter> const & get_EventsFilter() const { return eventsFilter_; }

        void SetEventsFilter(std::unique_ptr<HealthEventsFilter> && eventsFilter) { eventsFilter_ = move(eventsFilter); }

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION const & publicDeployedServicePackageHealthQueryDescription);

        void SetQueryArguments(__in QueryArgumentMap & argMap) const;

    private:
        Common::NamingUri applicationName_;
        std::wstring nodeName_;
        std::wstring serviceManifestName_;
        std::wstring servicePackageActivationId_;
        std::unique_ptr<ApplicationHealthPolicy> healthPolicy_;
        std::unique_ptr<HealthEventsFilter> eventsFilter_;
    };
}

