// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(DeployedApplicationHealthStateFilter)

    public:
        DeployedApplicationHealthStateFilter();

        explicit DeployedApplicationHealthStateFilter(
            DWORD healthStateFilter);

        DeployedApplicationHealthStateFilter(DeployedApplicationHealthStateFilter && other) = default;
        DeployedApplicationHealthStateFilter & operator = (DeployedApplicationHealthStateFilter && other) = default;

        virtual ~DeployedApplicationHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_NodeNameFilter, put=set_NodeNameFilter)) std::wstring const & NodeNameFilter;
        std::wstring const & get_NodeNameFilter() const { return nodeNameFilter_; }
        void set_NodeNameFilter(std::wstring && value) { nodeNameFilter_ = std::move(value); }

        __declspec(property(get=get_DeployedServicePackageFilters)) DeployedServicePackageHealthStateFilterList const & DeployedServicePackageFilters;
        DeployedServicePackageHealthStateFilterList const & get_DeployedServicePackageFilters() const { return deployedServicePackageFilters_; }

        void AddDeployedServicePackageHealthStateFilter(DeployedServicePackageHealthStateFilter && dsp) { deployedServicePackageFilters_.push_back(std::move(dsp)); }

        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout DeployedApplicationHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER const & publicDeployedApplicationHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_03(healthStateFilter_, nodeNameFilter_, deployedServicePackageFilters_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::NodeNameFilter, nodeNameFilter_)
            SERIALIZABLE_PROPERTY(Constants::DeployedServicePackageFilters, deployedServicePackageFilters_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        std::wstring nodeNameFilter_;
        DeployedServicePackageHealthStateFilterList deployedServicePackageFilters_;
    };

    using DeployedApplicationHealthStateFilterList = std::vector<DeployedApplicationHealthStateFilter>;
    using DeployedApplicationHealthStateFilterListWrapper = HealthStateFilterList<DeployedApplicationHealthStateFilter, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER, FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::DeployedApplicationHealthStateFilter)
