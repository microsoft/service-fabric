// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicaHealthStateFilter
        : public Common::IFabricJsonSerializable
        , public Serialization::FabricSerializable
    {
        DENY_COPY(ReplicaHealthStateFilter)

    public:
        ReplicaHealthStateFilter();

        explicit ReplicaHealthStateFilter(
            DWORD healthStateFilter);

        ReplicaHealthStateFilter(ReplicaHealthStateFilter && other) = default;
        ReplicaHealthStateFilter & operator = (ReplicaHealthStateFilter && other) = default;

        virtual ~ReplicaHealthStateFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        __declspec(property(get=get_ReplicaOrInstanceIdFilter,put=set_ReplicaOrInstanceIdFilter)) FABRIC_REPLICA_ID ReplicaOrInstanceIdFilter;
        FABRIC_REPLICA_ID get_ReplicaOrInstanceIdFilter() const { return replicaOrInstanceIdFilter_; }
        void set_ReplicaOrInstanceIdFilter(FABRIC_REPLICA_ID value) { replicaOrInstanceIdFilter_ = value; }

        std::wstring ToJsonString() const;
        static Common::ErrorCode FromJsonString(std::wstring const & str, __inout ReplicaHealthStateFilter & filter);

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_REPLICA_HEALTH_STATE_FILTER const & publicReplicaHealthStateFilter);

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPLICA_HEALTH_STATE_FILTER & publicFilter) const;

        FABRIC_FIELDS_02(healthStateFilter_, replicaOrInstanceIdFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
            SERIALIZABLE_PROPERTY(Constants::ReplicaOrInstanceIdFilter, replicaOrInstanceIdFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
        FABRIC_REPLICA_ID replicaOrInstanceIdFilter_;
    };

    using ReplicaHealthStateFilterList = std::vector<ReplicaHealthStateFilter>;
    using ReplicaHealthStateFilterListWrapper = HealthStateFilterList<ReplicaHealthStateFilter, FABRIC_REPLICA_HEALTH_STATE_FILTER, FABRIC_REPLICA_HEALTH_STATE_FILTER_LIST>;
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ReplicaHealthStateFilter)
