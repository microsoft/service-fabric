// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class HealthStateFilterHelper
    {
    public:
        static bool StateRespectsFilter(DWORD filter, FABRIC_HEALTH_STATE state, bool returnAllOnDefault = true);
    };

    class HealthEventsFilter
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(HealthEventsFilter)

    public:
        HealthEventsFilter();

        explicit HealthEventsFilter(DWORD healthStateFilter);

        HealthEventsFilter(HealthEventsFilter && other) = default;
        HealthEventsFilter & operator = (HealthEventsFilter && other) = default;

        ~HealthEventsFilter();

        __declspec(property(get=get_HealthStateFilter)) DWORD HealthStateFilter;
        DWORD get_HealthStateFilter() const { return healthStateFilter_; }

        bool IsRespected(HealthEvent const & event) const;
        bool IsRespected(FABRIC_HEALTH_STATE eventState) const;

        Common::ErrorCode FromPublicApi(FABRIC_HEALTH_EVENTS_FILTER const & publicHealthEventsFilter);

        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & str, 
            __out HealthEventsFilter & eventsFilter);

        FABRIC_FIELDS_01(healthStateFilter_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::HealthStateFilter, healthStateFilter_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        DWORD healthStateFilter_;
    };
}

