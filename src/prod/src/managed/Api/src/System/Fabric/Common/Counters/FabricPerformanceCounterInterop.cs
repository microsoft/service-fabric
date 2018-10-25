// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Fabric.Interop;

    internal sealed class FabricPerformanceCounterInterop
    {
        public static IntPtr CreateCounterSet(FabricPerformanceCounterSetDefinition counterSetDefinition,
                IEnumerable<FabricPerformanceCounterDefinition> counters)

        {
            IntPtr counterSetHandle = IntPtr.Zero;
#if !DotNetCoreClrLinux
            NativeTypes.FABRIC_COUNTER_SET_INITIALIZER nativeCounterSet = new NativeTypes.FABRIC_COUNTER_SET_INITIALIZER();            
            using (var pin = new PinCollection())
            {
                nativeCounterSet.CounterSetId = pin.AddBlittable(counterSetDefinition.Guid.ToString());
                nativeCounterSet.CounterSetName = pin.AddBlittable(counterSetDefinition.Name);
                nativeCounterSet.CounterSetDescription = pin.AddBlittable(counterSetDefinition.Description);
                nativeCounterSet.CounterSetInstanceType = (UInt32)counterSetDefinition.CategoryType;
                nativeCounterSet.NumCountersInSet = (UInt32)counters.Count();

                var nativeCounters = new NativeTypes.FABRIC_COUNTER_INITIALIZER[counters.Count()];
                int index = 0;
                foreach (FabricPerformanceCounterDefinition ctr in counters)
                {
                    nativeCounters[index].CounterId = (UInt32)ctr.Id;
                    nativeCounters[index].BaseCounterId = (UInt32)ctr.BaseId;
                    nativeCounters[index].CounterType = (UInt32)ctr.CounterType;
                    nativeCounters[index].CounterName = pin.AddBlittable(ctr.Name);
                    nativeCounters[index].CounterDescription = pin.AddBlittable(ctr.Description);
                    index++;
                }
                nativeCounterSet.Counters = pin.AddBlittable(nativeCounters);
                Utility.WrapNativeSyncInvokeInMTA(() => { NativeCommon.FabricPerfCounterCreateCounterSet(pin.AddBlittable(nativeCounterSet), out counterSetHandle); }, "PerformanceCountersInterop.CreateCounterSet");                
            }
#endif
            return counterSetHandle;
        }

        public static void DeleteCounterSet(IntPtr counterSetHandle)
        {
#if !DotNetCoreClrLinux
            Utility.WrapNativeSyncInvokeInMTA(() => { NativeCommon.FabricPerfCounterDeleteCounterSet(counterSetHandle); }, "PerformanceCountersInterop.DeleteCounterSet");
#endif
        }

        public static IntPtr CreateCounterSetInstance(IntPtr counterSetHandle, string instanceName)
        {
            IntPtr counterSetInstanceHandle = IntPtr.Zero;
#if !DotNetCoreClrLinux
            using (var pin = new PinCollection())
            {
                Utility.WrapNativeSyncInvokeInMTA(
                    () =>
                    {
                        NativeCommon.FabricPerfCounterCreateCounterSetInstance(counterSetHandle, pin.AddBlittable(instanceName),
                            out counterSetInstanceHandle);
                    }, "PerformanceCountersInterop.CreateCounterSetInstance");
            }
#endif
            return counterSetInstanceHandle;
        }

        public static void SetCounterRefValue(IntPtr counterInstanceHandle, int id, IntPtr counterAddress)
        {
#if !DotNetCoreClrLinux
            Utility.WrapNativeSyncInvokeInMTA(() => { NativeCommon.FabricPerfCounterSetPerformanceCounterRefValue(counterInstanceHandle, id, counterAddress); }, "PerformanceCountersInterop.SetCounterRefValue");
#endif
        }

        public static void DeleteCounterSetInstance(IntPtr counterInstanceHandle)
        {
#if !DotNetCoreClrLinux
            Utility.WrapNativeSyncInvokeInMTA(() => { NativeCommon.FabricPerfCounterDeleteCounterSetInstance(counterInstanceHandle); }, "PerformanceCountersInterop.DeleteCounterSetInstance");
#endif
        }
    }
}