// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    internal enum FabricPerformanceCounterType
    {
        NumberOfItemsHEX32 = 1,

        NumberOfItemsHEX64 = 2,

        NumberOfItems32 = 3,

        NumberOfItems64 = 4,

        CounterDelta32 = 5,

        CounterDelta64 = 6,

        SampleCounter = 7,

        CountPerTimeInterval32 = 8,

        CountPerTimeInterval64 = 9,

        RateOfCountsPerSecond32 = 10,

        RateOfCountsPerSecond64 = 11,

        RawFraction = 12,

        CounterTimer = 13,

        Timer100Ns = 14,

        SampleFraction = 15,

        CounterTimerInverse = 16,

        Timer100NsInverse = 17,

        CounterMultiTimer = 18,

        CounterMultiTimer100Ns = 19,

        CounterMultiTimerInverse = 20,

        CounterMultiTimer100NsInverse = 21,

        AverageTimer32 = 22,

        ElapsedTime = 23,

        AverageCount64 = 24,

        SampleBase = 25,

        AverageBase = 26,

        RawBase = 27,

        CounterMultiBase = 28,
    }
}