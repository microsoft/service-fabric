// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedServicePacakgeHealthQueryDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(DeployedServicePacakgeHealthQueryDescription)

    public:
        DeployedServicePacakgeHealthQueryDescription();

        DeployedServicePacakgeHealthQueryDescription(
               Common::NamingUri const & applicationName,
               std::wstring const & nodeName,
               std::wstring const & serviceManifestName,
               ApplicationHealthPolicy const & healthPolicy,
               std::unique_ptr<HealthEventsFilter> const & healthEventsFilter);

        DeployedServicePacakgeHealthQueryDescription(DeployedServicePacakgeHealthQueryDescription && other);
        DeployedServicePacakgeHealthQueryDescription & operator = (DeployedServicePacakgeHealthQueryDescription && other);

        ~DeployedServicePacakgeHealthQueryDescription();

        __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }

        __declspec(property(get=get_HealthPolicy)) ApplicationHealthPolicy const & HealthPolicy;
        ApplicationHealthPolicy const & get_HealthPolicy() const { return healthPolicy_; }

        __declspec(property(get=get_HealthEventsFilter)) std::unique_ptr<HealthEventsFilter> const & std::unique_ptr<HealthEventsFilter>;
        std::unique_ptr<HealthEventsFilter> const & get_HealthEventsFilter() const { return healthEventsFilter_; }

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACAKGE_HEALTH_QUERY_DESCRIPTION const & publicDeployedServicePacakgeHealthQueryDescription);

        Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __inout FABRIC_DEPLOYED_SERVICE_PACAKGE_HEALTH_QUERY_DESCRIPTION & publicDeployedServicePacakgeHealthQueryDescription) const;

        FABRIC_FIELDS_05(applicationName_, nodeName_, serviceManifestName_, healthPolicy_, healthEventsFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::NodeName, nodeName_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::HealthPolicy, healthPolicy_)
            SERIALIZABLE_PROPERTY(Constants::std::unique_ptr<HealthEventsFilter>, healthEventsFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::NamingUri applicationName_;
        std::wstring nodeName_;
        std::wstring serviceManifestName_;
        ApplicationHealthPolicy healthPolicy_;
        std::unique_ptr<HealthEventsFilter> healthEventsFilter_;
    };
}

