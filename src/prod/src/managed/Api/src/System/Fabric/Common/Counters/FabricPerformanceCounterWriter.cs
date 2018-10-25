// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    // Classes that modify values of performance counters with a base should derive from
    // this class
    internal abstract class FabricPerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        protected readonly FabricPerformanceCounter CounterBase;

        protected FabricPerformanceCounterWriter(
            FabricPerformanceCounterSetInstance instance,
            string counterName,
            string baseCounterName
            )
            : base(
                instance,
                counterName)
        {
            this.CounterBase = instance.GetPerformanceCounter(baseCounterName);
        }

        protected override bool IsInitialized
        {
            get { return this.Counter != null && this.CounterBase != null; }
        }
    }
}