// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedServicePackageHealthStatesFilter
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(DeployedServicePackageHealthStatesFilter)

    public:
        DeployedServicePackageHealthStatesFilter();

        explicit DeployedServicePackageHealthStatesFilter(DWORD healthStateFilter);

        DeployedServicePackageHealthStatesFilter(DeployedServicePackageHealthStatesFilter && other) = default;
        DeployedServicePackageHealthStatesFilter & operator = (DeployedServicePackageHealthStatesFilter && other) = default;

        ~DeployedServicePackageHealthStatesFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_STATES_FILTER const & publicDeployedServicePackageHealthStatesFilter);

        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out DeployedServicePackageHealthStatesFilter & filter);

        FABRIC_FIELDS_01(healthStateFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
    };
}

