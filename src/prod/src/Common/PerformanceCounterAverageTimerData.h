// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class PerformanceCounterAverageTimerData
    {
    private:
        DENY_COPY(PerformanceCounterAverageTimerData)

    public:
        explicit PerformanceCounterAverageTimerData(PerformanceCounterData & counter);
        ~PerformanceCounterAverageTimerData();

        void Update(int64 operationStartTime);

        void UpdateWithNormalizedTime(int64 elapsedTicks);

        __declspec(property(get=get_Value, put=set_Value)) PerformanceCounterValue Value;
        inline PerformanceCounterValue get_Value(void) const { return counter_.Value; }
        inline void set_Value(PerformanceCounterValue value) { counter_.Value = value; }

    private:
        PerformanceCounterData & counter_;
        int64 operationCount_;
        int64 averageTicks_;
    };
}

