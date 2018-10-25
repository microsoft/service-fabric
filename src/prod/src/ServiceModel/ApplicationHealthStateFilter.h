// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(ApplicationHealthStateFilter)

    public:
        ApplicationHealthStateFilter();

        explicit ApplicationHealthStateFilter(
            DWORD healthStateFilter);

        ApplicationHealthStateFilter(ApplicationHealthStateFilter && other) = default;
        ApplicationHealthStateFilter & operator = (ApplicationHealthStateFilter && other) = default;

        virtual ~ApplicationHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_ApplicationNameFilter, put=set_ApplicationNameFilter)) std::wstring const & ApplicationNameFilter;
        std::wstring const & get_ApplicationNameFilter() const { return applicationNameFilter_; }
        void set_ApplicationNameFilter(std::wstring const & value) { applicationNameFilter_ = value; }

        __declspec(property(get = get_ApplicationTypeNameFilter, put = set_ApplicationTypeNameFilter)) std::wstring const & ApplicationTypeNameFilter;
        std::wstring const & get_ApplicationTypeNameFilter() const { return applicationTypeNameFilter_; }
        void set_ApplicationTypeNameFilter(std::wstring const & value) { applicationTypeNameFilter_ = value; }

        __declspec(property(get=get_ServiceFilters)) ServiceHealthStateFilterList const & ServiceFilters;
        ServiceHealthStateFilterList const & get_ServiceFilters() const { return serviceFilters_; }
        
        void AddServiceHealthStateFilter(ServiceHealthStateFilter && service) { serviceFilters_.push_back(std::move(service)); }

        __declspec(property(get=get_DeployedApplicationFilters)) DeployedApplicationHealthStateFilterList const & DeployedApplicationFilters;
        DeployedApplicationHealthStateFilterList const & get_DeployedApplicationFilters() const { return deployedApplicationFilters_; }
        
        void AddDeployedApplicationHealthStateFilter(DeployedApplicationHealthStateFilter && deployedApplication) { deployedApplicationFilters_.push_back(std::move(deployedApplication)); }

        std::wstring ToJsonString() const;

        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout ApplicationHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_STATE_FILTER const & publicApplicationHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_APPLICATION_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_05(healthStateFilter_, applicationNameFilter_, serviceFilters_, deployedApplicationFilters_, applicationTypeNameFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationNameFilter, applicationNameFilter_)
            SERIALIZABLE_PROPERTY(Constants::ServiceFilters, serviceFilters_)
            SERIALIZABLE_PROPERTY(Constants::DeployedApplicationFilters, deployedApplicationFilters_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationTypeNameFilter, applicationTypeNameFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        std::wstring applicationNameFilter_;
        ServiceHealthStateFilterList serviceFilters_;
        DeployedApplicationHealthStateFilterList deployedApplicationFilters_;
        std::wstring applicationTypeNameFilter_;
    };

    using ApplicationHealthStateFilterList = std::vector<ApplicationHealthStateFilter>;
    using ApplicationHealthStateFilterListWrapper = HealthStateFilterList<ApplicationHealthStateFilter, FABRIC_APPLICATION_HEALTH_STATE_FILTER, FABRIC_APPLICATION_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ApplicationHealthStateFilter)
