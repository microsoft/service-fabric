// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // Base class for all health statistics filters
    class HealthStatisticsFilter
        : public Common::IFabricJsonSerializable
    {
        DENY_COPY(HealthStatisticsFilter)

    public:
        HealthStatisticsFilter();
        explicit HealthStatisticsFilter(bool excludeHealthStatistics);

        HealthStatisticsFilter(HealthStatisticsFilter && other) = default;
        HealthStatisticsFilter & operator = (HealthStatisticsFilter && other) = default;

        virtual ~HealthStatisticsFilter();

        __declspec(property(get=get_ExcludeHealthStatistics)) bool ExcludeHealthStatistics;
        bool get_ExcludeHealthStatistics() const { return excludeHealthStatistics_; }

        virtual std::wstring ToString() const = 0;
        
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ExcludeHealthStatistics, excludeHealthStatistics_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    protected:
        void SetExcludeHealthStatistics(BOOLEAN value) { excludeHealthStatistics_ = (value == TRUE); }

    private:
        bool excludeHealthStatistics_;
    };
}

