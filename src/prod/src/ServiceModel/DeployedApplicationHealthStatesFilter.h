// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationHealthStatesFilter
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(DeployedApplicationHealthStatesFilter)

    public:
        DeployedApplicationHealthStatesFilter();

        explicit DeployedApplicationHealthStatesFilter(DWORD healthStateFilter);

        DeployedApplicationHealthStatesFilter(DeployedApplicationHealthStatesFilter && other) = default;
        DeployedApplicationHealthStatesFilter & operator = (DeployedApplicationHealthStatesFilter && other) = default;

        ~DeployedApplicationHealthStatesFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_APPLICATION_HEALTH_STATES_FILTER const & publicDeployedApplicationHealthStatesFilter);

        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out DeployedApplicationHealthStatesFilter & filter);

        FABRIC_FIELDS_01(healthStateFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
    };
}

