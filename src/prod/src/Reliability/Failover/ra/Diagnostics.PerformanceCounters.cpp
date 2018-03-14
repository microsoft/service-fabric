// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace Diagnostics;

INITIALIZE_COUNTER_SET(RAPerformanceCounters);

void RAPerformanceCounters::OnUpgradeCompleted()
{
    NumberOfCompletedUpgrades.Increment();
}

void RAPerformanceCounters::UpdateAverageDurationCounter(
    StopwatchTime startTime,
    IClock & clock,
    PerformanceCounterData & base,
    PerformanceCounterData & counter)
{
    auto elapsed = clock.Now() - startTime;
    UpdateAverageDurationCounter(elapsed, base, counter);
}

void RAPerformanceCounters::UpdateAverageDurationCounter(
    StopwatchTime startTime,
    PerformanceCounterData & base,
    PerformanceCounterData & counter)
{
    auto elapsed = Stopwatch::Now() - startTime;
    UpdateAverageDurationCounter(elapsed, base, counter);
}

void RAPerformanceCounters::UpdateAverageDurationCounter(
    TimeSpan elapsed,
    PerformanceCounterData & base,
    PerformanceCounterData & counter)
{
    base.Increment();
    counter.IncrementBy(elapsed.TotalMilliseconds());
}
