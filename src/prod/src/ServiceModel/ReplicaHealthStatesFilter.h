// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ReplicaHealthStatesFilter
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(ReplicaHealthStatesFilter)

    public:
        ReplicaHealthStatesFilter();

        explicit ReplicaHealthStatesFilter(DWORD healthStateFilter);

        ReplicaHealthStatesFilter(ReplicaHealthStatesFilter && other) = default;
        ReplicaHealthStatesFilter & operator = (ReplicaHealthStatesFilter && other) = default;

        ~ReplicaHealthStatesFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_REPLICA_HEALTH_STATES_FILTER const & publicReplicaHealthStatesFilter);

        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out ReplicaHealthStatesFilter & filter);

        FABRIC_FIELDS_01(healthStateFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
    };
}

