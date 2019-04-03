// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

INITIALIZE_COUNTER_SET(InterlockedPoolPerfCounters)

InterlockedPoolPerfCountersSPtr InterlockedPoolPerfCounters::CreateInstance(
    wstring const &componentName,
    wstring const &uniqueId)
{
    std::wstring id;
    Common::StringWriter writer(id);
    writer.Write("{0}:{1}:{2}", componentName, uniqueId, SequenceNumber::GetNext());
    return InterlockedPoolPerfCounters::CreateInstance(id);
}

void InterlockedPoolPerfCounters::UpdatePerfCounters(ULONG poolObjectsUsed, ULONGLONG bytesAllocated)
{
    AverageBufferPoolAllocatorsBase.Increment();
    AverageBytesAllocated.IncrementBy(bytesAllocated);
    AverageBuffersUsed.IncrementBy(poolObjectsUsed);
}
