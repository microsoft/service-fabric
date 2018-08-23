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

    internal class FabricPerformanceCounterSetInstance : SafeHandle
    {
        private Dictionary<string, FabricPerformanceCounter> counterNameToCounter;

        internal FabricPerformanceCounterSetInstance(FabricPerformanceCounterSet counterSet, string instanceName) : base(IntPtr.Zero, true)
        {
            IntPtr counterInstancePtr = FabricPerformanceCounterInterop.CreateCounterSetInstance(counterSet.CounterSetDefinitionPtr, instanceName);
            SetHandle(counterInstancePtr);
            counterNameToCounter = new Dictionary<string, FabricPerformanceCounter>();

            foreach (var key in counterSet.CounterNameToId.Keys)
            {
                int id = counterSet.CounterNameToId[key];
                try
                {
                    //Get the reference of individual counters.
                    FabricPerformanceCounter perfCtr = new FabricPerformanceCounter(counterInstancePtr, id);
                    counterNameToCounter.Add(key, perfCtr);
                }
                catch
                {
                    //Dispose counter set instance.
                    Dispose();
                    throw;
                }
            }
        }
        protected override bool ReleaseHandle()
        {
            //Do not release individual counters to avoid possible race condition where one thread is disposing the counter instance while other thread
            //is incrementing the counter. All counter related resources are released in the counter object finalizer method.
			
            //Remove this perf-counter instance
            FabricPerformanceCounterInterop.DeleteCounterSetInstance(this.handle);
            return true;
        }

        public override bool IsInvalid
        {
            get { return this.handle == IntPtr.Zero; }
        }

        public FabricPerformanceCounter GetPerformanceCounter(string counterName)
        {
            if (counterNameToCounter.ContainsKey(counterName))
            {
                return counterNameToCounter[counterName];
            }
            throw new ArgumentException(String.Format(StringResources.Error_PerformanceCounterUnavailable, counterName));
        }

    }
}