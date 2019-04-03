// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

PerformanceCounterAverageTimerData::PerformanceCounterAverageTimerData(PerformanceCounterData & counter)
    : counter_(counter)
    , operationCount_(0)
    , averageTicks_(0)
{
}

PerformanceCounterAverageTimerData::~PerformanceCounterAverageTimerData()
{
}

void PerformanceCounterAverageTimerData::Update(int64 operationStartTime)
{
    int64 operationEndTime = Stopwatch::GetTimestamp();
    
    ++operationCount_;

    auto elapsedTicks = operationEndTime - operationStartTime;
    auto delta = (elapsedTicks - averageTicks_) / operationCount_;
    averageTicks_  += delta;

    counter_.Value = TimeSpan::FromTicks(averageTicks_).TotalMilliseconds();
}

void PerformanceCounterAverageTimerData::UpdateWithNormalizedTime(int64 elapsedTicks)
{
    ++operationCount_;
    auto delta = (elapsedTicks - averageTicks_) / operationCount_;
    averageTicks_ += delta;

    counter_.Value = TimeSpan::FromTicks(averageTicks_).TotalMilliseconds();
}
