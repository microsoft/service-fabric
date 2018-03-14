// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct PerformanceCounterData
    {
        PerformanceCounterValue RawValue;

        inline void Increment()
        {
            ::InterlockedIncrement64(&RawValue);
        }

        inline void Decrement()
        {
            ::InterlockedDecrement64(&RawValue);
        }

        inline void IncrementBy(PerformanceCounterValue value)
        {
            ::InterlockedExchangeAdd64(&RawValue, value);
        }

        __declspec(property(get=get_Value, put=set_Value)) PerformanceCounterValue Value;
        
        inline PerformanceCounterValue get_Value(void) 
        { 
            return ::InterlockedCompareExchange64(&RawValue, 0, 0); 
        }

        inline PerformanceCounterValue set_Value(PerformanceCounterValue value) 
        { 
            return ::InterlockedExchange64(&RawValue, value); 
        }
    };
}

