// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ClusterHealthStatisticsFilter
        : public HealthStatisticsFilter
    {
        DENY_COPY(ClusterHealthStatisticsFilter)

    public:
        ClusterHealthStatisticsFilter();
        explicit ClusterHealthStatisticsFilter(bool excludeHealthStatistics);
        ClusterHealthStatisticsFilter(bool excludeHealthStatistics, bool includeSystemApplicationHealthStatistics);
        
        ClusterHealthStatisticsFilter(ClusterHealthStatisticsFilter && other) = default;
        ClusterHealthStatisticsFilter & operator = (ClusterHealthStatisticsFilter && other) = default;

        virtual ~ClusterHealthStatisticsFilter();

        __declspec(property(get=get_IncludeSystemApplicationHealthStatistics, put=set_IncludeSystemApplicationHealthStatistics)) bool IncludeSystemApplicationHealthStatistics;
        bool get_IncludeSystemApplicationHealthStatistics() const { return includeSystemApplicationHealthStatistics_; }
        void set_IncludeSystemApplicationHealthStatistics(bool value) { includeSystemApplicationHealthStatistics_ = value; }

        Common::ErrorCode FromPublicApi(
            FABRIC_CLUSTER_HEALTH_STATISTICS_FILTER const & publicFilter);

        std::wstring ToString() const override;
        static Common::ErrorCode FromString(
            std::wstring const & str,
            __out ClusterHealthStatisticsFilter & filter);

        Common::ErrorCode Validate() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(Constants::IncludeSystemApplicationHealthStatistics, includeSystemApplicationHealthStatistics_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        bool includeSystemApplicationHealthStatistics_;
    };

    using ClusterHealthStatisticsFilterUPtr = std::unique_ptr<ClusterHealthStatisticsFilter>;
}

