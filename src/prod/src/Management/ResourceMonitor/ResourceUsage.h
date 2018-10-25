// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ResourceMonitor
{
    class ResourceUsage
    {
    public:
        //at least this many measurements should be made before sending out that info
        //needs to be at least 2 as we need at least 2 measures to figure out the cpu rate
        static const int MinTimeMeasuredValid = 2;

#if !defined (PLATFORM_UNIX)
        //per online documentation the total cpu time is expressed in 100 nanosecond ticks
        static const int64 CpuRateTicksPerSecond = 10000000;
#else
        //per online documentation the total cpu time is expressed in nanosecond ticks on Linux
        static const int64 CpuRateTicksPerSecond = 1000000000;
#endif 

        ResourceUsage();
        ~ResourceUsage();

        void Update(Management::ResourceMonitor::ResourceMeasurement const & measurement);

        bool IsValid() { return timeMeasured_ >= MinTimeMeasuredValid; }

        //check if we need to measure again for this host
        bool ShouldCheckUsage(Common::DateTime now);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        //these are adjusted with weighted moving average
        uint64 memoryUsage_;
        double cpuRate_;

        //these are the last raw data we have
        uint64 memoryUsageCurrent_;
        double cpuRateCurrent_;

        uint64 totalCpuTime_;
        Common::DateTime lastRead_;
        int timeMeasured_;
    };
}
