// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    // Classes that modify values of performance counters with no base should derive from
    // this class
    internal abstract class FabricBaselessPerformanceCounterWriter
    {
        protected readonly FabricPerformanceCounter Counter;

        protected FabricBaselessPerformanceCounterWriter(FabricPerformanceCounterSetInstance instance, string counterName)
        {
            Counter = instance.GetPerformanceCounter(counterName);
        }
        protected virtual bool IsInitialized
        {
            get { return this.Counter != null; }
        }
    }
}