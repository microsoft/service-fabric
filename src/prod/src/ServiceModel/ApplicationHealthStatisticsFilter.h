// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationHealthStatisticsFilter
        : public HealthStatisticsFilter
    {
        DENY_COPY(ApplicationHealthStatisticsFilter)

    public:
        ApplicationHealthStatisticsFilter();
        explicit ApplicationHealthStatisticsFilter(bool excludeHealthStatistics);

        ApplicationHealthStatisticsFilter(ApplicationHealthStatisticsFilter && other);
        ApplicationHealthStatisticsFilter & operator = (ApplicationHealthStatisticsFilter && other);

        virtual ~ApplicationHealthStatisticsFilter();

        Common::ErrorCode FromPublicApi(
            FABRIC_APPLICATION_HEALTH_STATISTICS_FILTER const & publicFilter);

        std::wstring ToString() const override;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out ApplicationHealthStatisticsFilter & filter);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()
    };

    using ApplicationHealthStatisticsFilterUPtr = std::unique_ptr<ApplicationHealthStatisticsFilter>;
}

