// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationHealthStatisticsFilter
        : public HealthStatisticsFilter
    {
        DENY_COPY(DeployedApplicationHealthStatisticsFilter)

    public:
        DeployedApplicationHealthStatisticsFilter();
        explicit DeployedApplicationHealthStatisticsFilter(bool excludeHealthStatistics);

        DeployedApplicationHealthStatisticsFilter(DeployedApplicationHealthStatisticsFilter && other);
        DeployedApplicationHealthStatisticsFilter & operator = (DeployedApplicationHealthStatisticsFilter && other);

        virtual ~DeployedApplicationHealthStatisticsFilter();

        Common::ErrorCode FromPublicApi(
            FABRIC_DEPLOYED_APPLICATION_HEALTH_STATISTICS_FILTER const & publicFilter);

        std::wstring ToString() const override;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out DeployedApplicationHealthStatisticsFilter & filter);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()
    };

    using DeployedApplicationHealthStatisticsFilterUPtr = std::unique_ptr<DeployedApplicationHealthStatisticsFilter>;
}

