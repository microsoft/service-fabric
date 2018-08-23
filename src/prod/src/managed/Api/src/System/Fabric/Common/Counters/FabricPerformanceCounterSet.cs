// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Runtime.InteropServices;

    internal class FabricPerformanceCounterSet : SafeHandle
    {
        public FabricPerformanceCounterSetDefinition CounterSetDefinition;
        internal IntPtr CounterSetDefinitionPtr;
        internal Dictionary<string, int> CounterNameToId;

        public FabricPerformanceCounterSet(
            FabricPerformanceCounterSetDefinition setDefinition,
            IEnumerable<FabricPerformanceCounterDefinition> counters) : base(IntPtr.Zero, true)
        {
            CounterSetDefinition = setDefinition;
            CounterNameToId = new Dictionary<string, int>();

            foreach (var ctr in counters)
            {
                CounterNameToId[ctr.Name] = ctr.Id;
            }
            CounterSetDefinitionPtr = FabricPerformanceCounterInterop.CreateCounterSet(CounterSetDefinition, counters);
            SetHandle(CounterSetDefinitionPtr);
        }

        protected override bool ReleaseHandle()
        {
            FabricPerformanceCounterInterop.DeleteCounterSet(this.handle);
            return true;
        }

        public override bool IsInvalid
        {
            get { return this.handle == IntPtr.Zero; }
        }

        public FabricPerformanceCounterSetInstance CreateCounterSetInstance(string instanceName)
        {
            return new FabricPerformanceCounterSetInstance(this, instanceName);
        }
    }
}