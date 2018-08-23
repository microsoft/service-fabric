// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedServicePackageHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(DeployedServicePackageHealthStateFilter)

    public:
        DeployedServicePackageHealthStateFilter();

        explicit DeployedServicePackageHealthStateFilter(
            DWORD healthStateFilter);

        DeployedServicePackageHealthStateFilter(DeployedServicePackageHealthStateFilter && other) = default;
        DeployedServicePackageHealthStateFilter & operator = (DeployedServicePackageHealthStateFilter && other) = default;

        virtual ~DeployedServicePackageHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_ServiceManifestNameFilter, put=set_ServiceManifestNameFilter)) std::wstring const & ServiceManifestNameFilter;
        std::wstring const & get_ServiceManifestNameFilter() const { return serviceManifestNameFilter_; }
        void set_ServiceManifestNameFilter(std::wstring && value) { serviceManifestNameFilter_ = std::move(value); }

        __declspec(property(get = get_ServicePackageActivationIdFilterSPtr)) std::shared_ptr<std::wstring> const & ServicePackageActivationIdFilterSPtr;
		std::shared_ptr<std::wstring> const & get_ServicePackageActivationIdFilterSPtr() const { return servicePackageActivationIdFilterSPtr_; }

        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout DeployedServicePackageHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

		bool IsDefaultFilter() const;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER const & publicDeployedServicePackageHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_03(healthStateFilter_, serviceManifestNameFilter_, servicePackageActivationIdFilterSPtr_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestNameFilter, serviceManifestNameFilter_)
            SERIALIZABLE_PROPERTY(Constants::ServicePackageActivationIdFilter, servicePackageActivationIdFilterSPtr_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        std::wstring serviceManifestNameFilter_;
        std::shared_ptr<std::wstring> servicePackageActivationIdFilterSPtr_;
    };

    using DeployedServicePackageHealthStateFilterList = std::vector<DeployedServicePackageHealthStateFilter>;
    using DeployedServicePackageHealthStateFilterListWrapper = HealthStateFilterList<DeployedServicePackageHealthStateFilter, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER, FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DeployedServicePackageHealthStateFilter)
