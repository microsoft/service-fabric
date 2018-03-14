// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace PerformanceCounterType
    {
        enum Enum
        {
            QueueLength  = PERF_COUNTER_QUEUELEN_TYPE,
            LargeQueueLength = PERF_COUNTER_LARGE_QUEUELEN_TYPE,
            QueueLength100Ns  = PERF_COUNTER_100NS_QUEUELEN_TYPE,
            QueueLengthObjectTime = PERF_COUNTER_QUEUELEN_TYPE,  
            RawData32 = PERF_COUNTER_RAWCOUNT,  
            RawData64 = PERF_COUNTER_LARGE_RAWCOUNT,  
            RawDataHex32 = PERF_COUNTER_RAWCOUNT_HEX,  
            RawDataHex64 = PERF_COUNTER_LARGE_RAWCOUNT_HEX,  
            RateOfCountPerSecond32 = PERF_COUNTER_COUNTER,  
            RateOfCountPerSecond64 = PERF_COUNTER_BULK_COUNT,  
            RawFraction32 = PERF_RAW_FRACTION,  
            RawFraction64 = PERF_LARGE_RAW_FRACTION,
            RawBase32 = PERF_RAW_BASE,  
            RawBase64 = PERF_LARGE_RAW_BASE,  
            SampleFraction = PERF_SAMPLE_FRACTION,  
            SampleCounter = PERF_SAMPLE_COUNTER,  
            SampleBase = PERF_SAMPLE_BASE,
            AverageTimer32 = PERF_AVERAGE_TIMER,  
            AverageBase = PERF_AVERAGE_BASE,  
            AverageCount64 = PERF_AVERAGE_BULK,  
            PercentageActive = PERF_COUNTER_TIMER,  
            PercentageNotActive = PERF_COUNTER_TIMER_INV,  
            PercentageActive100Ns = PERF_100NSEC_TIMER,  
            PercentageNotActive100Ns = PERF_100NSEC_TIMER_INV,  
            ElapsedTime = PERF_ELAPSED_TIME,  
            MultiTimerPercentageActive = PERF_COUNTER_MULTI_TIMER,  
            MultiTimerPercentageNotActive = PERF_COUNTER_MULTI_TIMER_INV,  
            MultiTimerPercentageActive100Ns = PERF_100NSEC_MULTI_TIMER,  
            MultiTimerPercentageNotActive100Ns = PERF_100NSEC_MULTI_TIMER_INV,  
            MultiTimerBase = PERF_COUNTER_MULTI_BASE,
            Delta32 = PERF_COUNTER_DELTA,  
            Delta64 = PERF_COUNTER_LARGE_DELTA,  
            ObjectSpecificTimer = PERF_OBJ_TIME_TIMER,  
            PrecisionSystemTimer = PERF_PRECISION_SYSTEM_TIMER,  
            PrecisionTimer100Ns = PERF_PRECISION_100NS_TIMER,  
            PrecisionObjectSpecificTimer = PERF_PRECISION_OBJECT_TIMER,  
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}

