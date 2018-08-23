// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PartitionHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(PartitionHealthStateFilter)

    public:
        PartitionHealthStateFilter();

        explicit PartitionHealthStateFilter(
            DWORD healthStateFilter);

        PartitionHealthStateFilter(PartitionHealthStateFilter && other) = default;
        PartitionHealthStateFilter & operator = (PartitionHealthStateFilter && other) = default;

        virtual ~PartitionHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_PartitionIdFilter, put=set_PartitionIdFilter)) Common::Guid PartitionIdFilter;
        Common::Guid get_PartitionIdFilter() const { return partitionIdFilter_; }
        void set_PartitionIdFilter(Common::Guid value) { partitionIdFilter_ = value; }

        __declspec(property(get=get_ReplicaFilters)) ReplicaHealthStateFilterList const & ReplicaFilters;
        ReplicaHealthStateFilterList const & get_ReplicaFilters() const { return replicaFilters_; }

        void AddReplicaHealthStateFilter(ReplicaHealthStateFilter && replica) { replicaFilters_.push_back(std::move(replica)); }
        
        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout PartitionHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_PARTITION_HEALTH_STATE_FILTER const & publicPartitionHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_PARTITION_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_03(healthStateFilter_, partitionIdFilter_, replicaFilters_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::PartitionIdFilter, partitionIdFilter_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaFilters, replicaFilters_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        Common::Guid partitionIdFilter_;
        ReplicaHealthStateFilterList replicaFilters_;
    };

    using PartitionHealthStateFilterList = std::vector<PartitionHealthStateFilter>;
    using PartitionHealthStateFilterListWrapper = HealthStateFilterList<PartitionHealthStateFilter, FABRIC_PARTITION_HEALTH_STATE_FILTER, FABRIC_PARTITION_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::PartitionHealthStateFilter)
