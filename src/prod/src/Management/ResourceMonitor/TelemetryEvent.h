// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ResourceMonitor
{
    class TelemetryEvent
    {
    public:
        TelemetryEvent(std::wstring const & partitionId, std::vector<Metric> const & eventMetrics);

        _declspec(property(get = get_PartitionId)) std::wstring const & PartitionId;
        inline std::wstring const & get_PartitionId() const { return partitionId_; }

        _declspec(property(get = get_Metrics)) std::vector<Metric> const & Metrics;
        inline std::vector<Metric> const & get_Metrics() const { return eventMetrics_; }

    private:
        std::wstring partitionId_;
        std::vector<Metric> eventMetrics_;
    };
}
