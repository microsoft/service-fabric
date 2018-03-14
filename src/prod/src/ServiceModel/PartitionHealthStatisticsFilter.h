// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PartitionHealthStatisticsFilter
        : public HealthStatisticsFilter
    {
        DENY_COPY(PartitionHealthStatisticsFilter)

    public:
        PartitionHealthStatisticsFilter();
        explicit PartitionHealthStatisticsFilter(bool excludeHealthStatistics);

        PartitionHealthStatisticsFilter(PartitionHealthStatisticsFilter && other);
        PartitionHealthStatisticsFilter & operator = (PartitionHealthStatisticsFilter && other);

        virtual ~PartitionHealthStatisticsFilter();

        Common::ErrorCode FromPublicApi(
            FABRIC_PARTITION_HEALTH_STATISTICS_FILTER const & publicFilter);

        std::wstring ToString() const override;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out PartitionHealthStatisticsFilter & filter);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()
    };

    using PartitionHealthStatisticsFilterUPtr = std::unique_ptr<PartitionHealthStatisticsFilter>;
}

