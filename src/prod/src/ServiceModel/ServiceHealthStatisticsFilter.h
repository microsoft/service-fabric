// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceHealthStatisticsFilter
        : public HealthStatisticsFilter
    {
        DENY_COPY(ServiceHealthStatisticsFilter)

    public:
        ServiceHealthStatisticsFilter();
        explicit ServiceHealthStatisticsFilter(bool excludeHealthStatistics);

        ServiceHealthStatisticsFilter(ServiceHealthStatisticsFilter && other);
        ServiceHealthStatisticsFilter & operator = (ServiceHealthStatisticsFilter && other);

        virtual ~ServiceHealthStatisticsFilter();

        Common::ErrorCode FromPublicApi(
            FABRIC_SERVICE_HEALTH_STATISTICS_FILTER const & publicFilter);

        std::wstring ToString() const override;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out ServiceHealthStatisticsFilter & filter);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()
    };

    using ServiceHealthStatisticsFilterUPtr = std::unique_ptr<ServiceHealthStatisticsFilter>;
}

