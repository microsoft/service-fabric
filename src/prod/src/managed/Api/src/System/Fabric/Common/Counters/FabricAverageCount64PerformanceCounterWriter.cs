// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    // This class modifies the value of a performance counter of type AverageCount64
    internal class FabricAverageCount64PerformanceCounterWriter : FabricPerformanceCounterWriter
    {
        internal FabricAverageCount64PerformanceCounterWriter(
            FabricPerformanceCounterSetInstance instance,
            string counterName,
            string baseCounterName
            )
            : base(
                instance,
                counterName,
                baseCounterName)
        {
        }

        internal void UpdateCounterValue(long delta)
        {
            this.Counter.IncrementBy(delta);
            this.CounterBase.Increment();
        }
    }
}