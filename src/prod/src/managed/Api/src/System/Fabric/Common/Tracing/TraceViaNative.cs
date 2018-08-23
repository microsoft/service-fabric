// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Fabric.Interop;
    using System.Diagnostics.Tracing;

    /// <summary>
    /// Sink that writes trace records through Native layer.
    /// </summary>
    internal static class TraceViaNative
    {
        public static void WriteUnstructured(string taskName, string eventName, string id, ushort level, string text)
        {
            try
            {
                using (var pin = new PinCollection())
                {
                    NativeCommon.WriteManagedTrace(
                        pin.AddBlittable(taskName),
                        pin.AddBlittable(eventName),
                        pin.AddBlittable(id),
                        level,
                        pin.AddBlittable(text));
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Exception during interop trace writing: {0}", ex);
                Console.WriteLine("{0}, {1}, {2}, {3}, {4}", taskName, eventName, id, level, text);
            }
            
        }

#if DotNetCoreClrLinux
        public static unsafe void WriteStructured(ref GenericEventDescriptor eventDescriptor, ulong userDataCount, EventDataDescriptor *eventDataPtr)
        {
            try
            {
                //using (var pin = new PinCollection())
                fixed (GenericEventDescriptor *eventDescriptorPtr = &eventDescriptor)
                {
                    var traceEvent = new NativeTypes.FABRIC_ETW_TRACE_EVENT_PAYLOAD()
                    {
                        EventDescriptor = (IntPtr)eventDescriptorPtr,//pin.AddBlittable(eventDescriptor),
                        EventDataDescriptorList = new NativeTypes.FABRIC_ETW_TRACE_EVENT_DATA_DESCRIPTOR_LIST()
                        {
                            Count = userDataCount,
                            UserDataDescriptor = (IntPtr)eventDataPtr
                        },
                        Reserved = IntPtr.Zero
                    };

                    NativeCommon.WriteManagedStructuredTrace(ref traceEvent);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Exception during interop structured trace writing: {0}", ex);
                Console.WriteLine("EventId: {0}, UserDataCount {1}", eventDescriptor.EventId, userDataCount);
            }
        }
#endif
    }
}