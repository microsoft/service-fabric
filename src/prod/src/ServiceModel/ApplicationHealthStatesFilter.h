// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationHealthStatesFilter
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(ApplicationHealthStatesFilter)

    public:
        ApplicationHealthStatesFilter();

        explicit ApplicationHealthStatesFilter(DWORD healthStateFilter);

        ApplicationHealthStatesFilter(ApplicationHealthStatesFilter && other) = default;
        ApplicationHealthStatesFilter & operator = (ApplicationHealthStatesFilter && other) = default;

        ~ApplicationHealthStatesFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        bool IsRespected(FABRIC_HEALTH_STATE healthState) const;

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_HEALTH_STATES_FILTER const & publicApplicationHealthStatesFilter);

        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out ApplicationHealthStatesFilter & filter);

        FABRIC_FIELDS_01(healthStateFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
    };

    using ApplicationHealthStatesFilterUPtr = std::unique_ptr<ApplicationHealthStatesFilter>;
}

