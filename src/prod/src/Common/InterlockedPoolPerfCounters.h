// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class InterlockedPoolPerfCounters
    {
        DENY_COPY(InterlockedPoolPerfCounters)
    public:

        BEGIN_COUNTER_SET_DEFINITION(
            L"E1E76246-F779-4BF2-AD5A-4BB8E535F7F4",
            L"Interlocked Bufferpool",
            L"Counters for the interlocked bufferpool used by ServiceFabric components",
            PerformanceCounterSetInstanceType::Multiple)
            COUNTER_DEFINITION(
            1,
            Common::PerformanceCounterType::AverageBase,
            L"# of bufferpool allocators created from the bufferpool/Operation Base",
            L"Base counter for measuring the average bytes allocated by the bufferpool allocators created from the bufferpool",
            noDisplay)
            COUNTER_DEFINITION_WITH_BASE(
            2,
            1,
            PerformanceCounterType::AverageCount64,
            L"Avg. # bytes allocated by the bufferpool allocators created from the bufferpool/Operation",
            L"Counter for measuring the average bytes allocated by the bufferpool allocators created from the bufferpool")
            COUNTER_DEFINITION_WITH_BASE(
            3,
            1,
            PerformanceCounterType::AverageCount64,
            L"Avg. # buffers used by the bufferpool allocators created from the bufferpool/Operation",
            L"Counter for measuring the average number of pools used by the bufferpool allocators created from the bufferpool")
        END_COUNTER_SET_DEFINITION()

        DECLARE_COUNTER_INSTANCE(AverageBufferPoolAllocatorsBase)
        DECLARE_COUNTER_INSTANCE(AverageBytesAllocated)
        DECLARE_COUNTER_INSTANCE(AverageBuffersUsed)

        BEGIN_COUNTER_SET_INSTANCE(InterlockedPoolPerfCounters)
            DEFINE_COUNTER_INSTANCE(AverageBufferPoolAllocatorsBase, 1)
            DEFINE_COUNTER_INSTANCE(AverageBytesAllocated, 2)
            DEFINE_COUNTER_INSTANCE(AverageBuffersUsed, 3)
        END_COUNTER_SET_INSTANCE()

        static std::shared_ptr<InterlockedPoolPerfCounters> InterlockedPoolPerfCounters::CreateInstance(
            std::wstring const &bufferPoolComponentName,
            std::wstring const &uniqueId);

        void UpdatePerfCounters(ULONG poolObjectsUsed, ULONGLONG bytesAllocated);
    };

    typedef std::shared_ptr<InterlockedPoolPerfCounters> InterlockedPoolPerfCountersSPtr;
}
