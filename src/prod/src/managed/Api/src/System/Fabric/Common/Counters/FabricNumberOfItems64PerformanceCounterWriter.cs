// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    // This class modifies the value of a performance counter of type AverageCount64
    internal class FabricNumberOfItems64PerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal FabricNumberOfItems64PerformanceCounterWriter(
            FabricPerformanceCounterSetInstance instance,
            string counterName
            )
            : base(
                instance,
                counterName)
        {
        }

        internal void UpdateCounterValue(long delta)
        {
            this.Counter.IncrementBy(delta);
        }
    }
}