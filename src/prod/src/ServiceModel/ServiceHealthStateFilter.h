// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(ServiceHealthStateFilter)

    public:
        ServiceHealthStateFilter();

        explicit ServiceHealthStateFilter(
            DWORD healthStateFilter);

        ServiceHealthStateFilter(ServiceHealthStateFilter && other) = default;
        ServiceHealthStateFilter & operator = (ServiceHealthStateFilter && other) = default;

        virtual ~ServiceHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_ServiceNameFilter, put=set_ServiceNameFilter)) std::wstring const & ServiceNameFilter;
        std::wstring const & get_ServiceNameFilter() const { return serviceNameFilter_; }
        void set_ServiceNameFilter(std::wstring const & value) { serviceNameFilter_ = value; }

        __declspec(property(get=get_PartitionFilters)) PartitionHealthStateFilterList const & PartitionFilters;
        PartitionHealthStateFilterList const & get_PartitionFilters() const { return partitionFilters_; }

        void AddPartitionHealthStateFilter(PartitionHealthStateFilter && partition) { partitionFilters_.push_back(std::move(partition)); }

        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout ServiceHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_HEALTH_STATE_FILTER const & publicServiceHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_03(healthStateFilter_, serviceNameFilter_, partitionFilters_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::ServiceNameFilter, serviceNameFilter_)
            SERIALIZABLE_PROPERTY(Constants::PartitionFilters, partitionFilters_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        std::wstring serviceNameFilter_;
        PartitionHealthStateFilterList partitionFilters_;
    };

    using ServiceHealthStateFilterList = std::vector<ServiceHealthStateFilter>;
    using ServiceHealthStateFilterListWrapper = HealthStateFilterList<ServiceHealthStateFilter, FABRIC_SERVICE_HEALTH_STATE_FILTER, FABRIC_SERVICE_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServiceHealthStateFilter)
