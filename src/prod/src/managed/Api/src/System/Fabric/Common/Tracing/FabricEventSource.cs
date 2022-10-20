// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Diagnostics.Tracing;
    using System.Globalization;
    using System.Linq;
    using System.Reflection;
    using System.Text;
#if DotNetCoreClrLinux
    using System.Fabric.Interop;
#endif

    using ServiceFabricEventChannel = System.Fabric.Common.Tracing.EventChannel;
#if DotNetCoreClr
    using SystemEventChannel = System.Diagnostics.Tracing.EventChannel;
#endif

    /// <summary>
    /// EventSource class that abstracts most of EventSource and VariantEventWriter logic of
    /// System.Fabric and Microsoft.ServiceFabric.Internal.
    /// </summary>
    internal abstract class FabricEventSource
#if DotNetCoreClr || MANAGED_INTERNAL_EVENTSOURCE_ENABLE
        : EventSource, IVariantEventWriter
#endif
    {
        protected ReadOnlyDictionary<int, TraceEvent> eventDescriptors;
        protected readonly ReadOnlyDictionary<string, EventTask> taskMap;

        protected FabricEventSource()
        {
            this.taskMap = GenerateTaskMap();
        }

        #region NonEvents

        [NonEvent]
        internal string GetEventTaskName(EventTask eventTask)
        {
            try
            {
                return this.taskMap.Single(t => t.Value == eventTask).Key;
            }
            catch (Exception)
            {
                return "UnSpecified";
            }
        }

        [NonEvent]
        protected abstract ReadOnlyDictionary<string, EventTask> GenerateTaskMap();

        [NonEvent]
        protected ReadOnlyDictionary<int, TraceEvent> GenerateEventDescriptors(IVariantEventWriter variantEventWriter)
        {
            var eventDescriptors = new Dictionary<int, TraceEvent>();

            foreach (var eventMethod in this.GetType().GetMethods())
            {
                EventAttribute eventAttribute = (EventAttribute)eventMethod.GetCustomAttributes(typeof(EventAttribute), false).SingleOrDefault();

                if (eventAttribute == null)
                {
                    continue;
                }

                var hasId = false;
                int typeFieldIndex = -1;
                var methodParams = eventMethod.GetParameters().ToList();
                if (methodParams.Any())
                {
                    typeFieldIndex = methodParams.FindIndex(e => e.Name == "type");
                    hasId = methodParams[0].Name == "id";
                }

                var eventId = (ushort)eventAttribute.EventId;

                ProvisionalMetadataAttribute provisional = (ProvisionalMetadataAttribute)CustomAttributeExtensions.GetCustomAttributes(eventMethod, typeof(ProvisionalMetadataAttribute), false).SingleOrDefault();
                EventExtendedMetadataAttribute extended = (EventExtendedMetadataAttribute)CustomAttributeExtensions.GetCustomAttributes(eventMethod, typeof(EventExtendedMetadataAttribute), false).SingleOrDefault();

                string eventName = eventMethod.Name;

                if (extended != null && extended.TableEntityKind != TableEntityKind.Unknown)
                {
                    eventName = string.Format(FabricEventSourceConstants.OperationalChannelEventNameFormat, extended.TableEntityKind, FabricEventSourceConstants.OperationalChannelTableNameSuffix, eventName);
                }

                eventDescriptors[eventId] = new TraceEvent(
                    variantEventWriter,
                    eventAttribute.Task,
                    eventId,
                    eventName,
                    eventAttribute.Level,
                    eventAttribute.Opcode,
                    ChannelToUse(eventAttribute, extended),
                    eventAttribute.Keywords,
                    FormatStringToUse(eventAttribute, extended),
                    hasId,
                    provisional,
                    typeFieldIndex,
                    extended);

                // Wire up TraceConfig filtering to event enabled status
                TraceConfig.OnFilterUpdate += sinkType =>
                {
                    eventDescriptors[eventId].UpdateSinkEnabledStatus(sinkType, TraceConfig.GetEventEnabledStatus(
                        sinkType,
                        eventDescriptors[eventId].Level,
                        eventDescriptors[eventId].TaskId,
                        eventDescriptors[eventId].EventName));

                    eventDescriptors[eventId].UpdateSinkSamplingRatio(sinkType, TraceConfig.GetEventSamplingRatio(
                        sinkType,
                        eventDescriptors[eventId].Level,
                        eventDescriptors[eventId].TaskId,
                        eventDescriptors[eventId].EventName));

                    eventDescriptors[eventId].UpdateProvisionalFeatureStatus(sinkType, TraceConfig.GetEventProvisionalFeatureStatus(sinkType));
                };
            }

            return new ReadOnlyDictionary<int, TraceEvent>(eventDescriptors);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category);
                return;
            }

            WriteEventInternal(eventDescriptor);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0);
                return;
            }

            WriteEventInternal(eventDescriptor, param0);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5,
                    param6);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5,
                    param6,
                    param7);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5,
                    param6,
                    param7,
                    param8);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7, param8);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5,
                    param6,
                    param7,
                    param8,
                    param9);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7, param8, param9);
        }


        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5,
                    param6,
                    param7,
                    param8,
                    param9,
                    param10);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
        }

        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10, Variant param11)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                WriteEventInternal(
                    eventDescriptor,
                    eventDescriptor.ExtendedMetadata.PublicEventName,
                    eventDescriptor.ExtendedMetadata.Category,
                    param0,
                    param1,
                    param2,
                    param3,
                    param4,
                    param5,
                    param6,
                    param7,
                    param8,
                    param9,
                    param10,
                    param11);
                return;
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11);
        }
 
        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10, Variant param11, Variant param12)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                throw new ArgumentException("eventDescriptor.ExtendedMetadata: Too many params, ExtendedMetadata can not be included.");
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12);
        }
 
        [NonEvent]
        protected static void WriteEvent(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10, Variant param11, Variant param12, Variant param13)
        {
            if (eventDescriptor.ExtendedMetadata != null)
            {
                throw new ArgumentException("eventDescriptor.ExtendedMetadata: Too many params, ExtendedMetadata can not be included.");
            }

            WriteEventInternal(eventDescriptor, param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13);
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink();
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks();
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7, param8);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7, param8);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9);
            }
        }


        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10)
        {

            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10, Variant param11)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10, Variant param11, Variant param12)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12);
            }
        }

        [NonEvent]
        private static void WriteEventInternal(TraceEvent eventDescriptor, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8, Variant param9, Variant param10, Variant param11, Variant param12, Variant param13)
        {
            if (eventDescriptor.IsEtwSinkEnabled())
            {
                eventDescriptor.WriteToEtwSink(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13);
            }

            if (eventDescriptor.AreFlatSinksEnabled())
            {
                eventDescriptor.WriteToFlatEventSinks(param0, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, param11, param12, param13);
            }
        }

#if DotNetCoreClr || MANAGED_INTERNAL_EVENTSOURCE_ENABLE
        [NonEvent]
        public unsafe void VariantWrite(ref GenericEventDescriptor genericEventDescriptor, int argCount,
            Variant v1 = default(Variant),  Variant v2 = default(Variant),  Variant v3 = default(Variant),
            Variant v4 = default(Variant),  Variant v5 = default(Variant),  Variant v6 = default(Variant),
            Variant v7 = default(Variant),  Variant v8 = default(Variant),  Variant v9 = default(Variant),
            Variant v10 = default(Variant), Variant v11 = default(Variant), Variant v12 = default(Variant),
            Variant v13 = default(Variant), Variant v14 = default(Variant))
        {
            if (argCount == 0)
            {
#if !DotNetCoreClrLinux
                this.WriteEvent(genericEventDescriptor.EventId);
#else
                TraceViaNative.WriteStructured(ref genericEventDescriptor, 0, null);
#endif
                return;
            }

            Tracing.EventData* eventSourceData = stackalloc Tracing.EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edb = new EventDataArrayBuilder(eventSourceData, dataBuffer);

            if (!edb.AddEventDataWithTruncation(argCount, ref v1, ref v2, ref v3, ref v4, ref v5, ref v6, ref v7, ref v8, ref v9, ref v10, ref v11, ref v12, ref v13, ref v14))
            {
                Debug.Fail(string.Format("EventData for eventid {0} is invalid. Check the total size of the event.",
                    genericEventDescriptor.EventId));
                return;
            }

            fixed (char* s1 = v1.ConvertToString(),
                s2 = v2.ConvertToString(),
                s3 = v3.ConvertToString(),
                s4 = v4.ConvertToString(),
                s5 = v5.ConvertToString(),
                s6 = v6.ConvertToString(),
                s7 = v7.ConvertToString(),
                s8 = v8.ConvertToString(),
                s9 = v9.ConvertToString(),
                s10 = v10.ConvertToString(),
                s11 = v11.ConvertToString(),
                s12 = v12.ConvertToString(),
                s13 = v13.ConvertToString(),
                s14 = v14.ConvertToString())
            {
                var tracingEventDataPtr = edb.ToEventDataArray(s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14);
                // Convert to EventSource.EventData from Tracing.EventData.
#if !DotNetCoreClrLinux
                EventData* eventDataPtr = stackalloc EventData[argCount];
                for (uint i = 0; i < argCount; ++i)
                {
                    eventDataPtr[i].DataPointer = (IntPtr)tracingEventDataPtr[i].DataPointer;
                    eventDataPtr[i].Size = (int)tracingEventDataPtr[i].Size;
                }

                this.WriteEventCore(genericEventDescriptor.EventId, argCount, eventDataPtr);
#else
                EventDataDescriptor *eventDataPtr = stackalloc EventDataDescriptor[argCount];
                for (uint i = 0; i < argCount; ++i)
                {
                    eventDataPtr[i].DataPointer = (UInt64)tracingEventDataPtr[i].DataPointer;
                    eventDataPtr[i].Size = tracingEventDataPtr[i].Size;
                }

                TraceViaNative.WriteStructured(ref genericEventDescriptor, (uint)argCount, eventDataPtr);
#endif
            }
        }

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        [NonEvent]
        public void VariantWriteLinuxUnstructured(
            ref GenericEventDescriptor genericEventDescriptor,
            int argCount,
            Variant v0 = default(Variant),
            Variant v1 = default(Variant),
            Variant v2 = default(Variant),
            Variant v3 = default(Variant),
            Variant v4 = default(Variant),
            Variant v5 = default(Variant),
            Variant v6 = default(Variant),
            Variant v7 = default(Variant),
            Variant v8 = default(Variant),
            Variant v9 = default(Variant),
            Variant v10 = default(Variant),
            Variant v11 = default(Variant),
            Variant v12 = default(Variant),
            Variant v13 = default(Variant))
        {
            int eventId = genericEventDescriptor.EventId;
            string text = string.Format(
                this.eventDescriptors[eventId].Message, new object[]
                { v0.ToObject(), v1.ToObject(), v2.ToObject(), v3.ToObject(), v4.ToObject(), v5.ToObject(), v6.ToObject(), v7.ToObject(), v8.ToObject(), v9.ToObject(), v10.ToObject(), v11.ToObject(), v12.ToObject(), v13.ToObject() }
            );

            string id = this.eventDescriptors[eventId].hasId && !string.IsNullOrWhiteSpace(v0.ConvertToString()) ? v0.ConvertToString() : eventId.ToString();
            int typeFieldIndex = this.eventDescriptors[eventId].typeFieldIndex;
            string type = this.eventDescriptors[eventId].EventName;
            if (this.eventDescriptors[eventId].typeFieldIndex != -1)
            {
                string typeField = GetTypeFromIndex(typeFieldIndex, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13);
                if (!string.IsNullOrWhiteSpace(typeField))
                {
                    type = typeField;
                }
            }

            TraceViaNative.WriteUnstructured(FabricEvents.Events.GetEventTaskName((EventTask)genericEventDescriptor.Task), type, id, genericEventDescriptor.Level, text);
        }

        private string GetTypeFromIndex(int index, Variant v0, Variant v1, Variant v2, Variant v3, Variant v4,
            Variant v5, Variant v6, Variant v7, Variant v8, Variant v9, Variant v10, Variant v11, Variant v12, Variant v13)
        {
            switch(index)
            {
                case 0:  return v0.ConvertToString();
                case 1:  return v1.ConvertToString();
                case 2:  return v2.ConvertToString();
                case 3:  return v3.ConvertToString();
                case 4:  return v4.ConvertToString();
                case 5:  return v5.ConvertToString();
                case 6:  return v6.ConvertToString();
                case 7:  return v7.ConvertToString();
                case 8:  return v8.ConvertToString();
                case 9:  return v9.ConvertToString();
                case 10: return v10.ConvertToString();
                case 11: return v11.ConvertToString();
                case 12: return v12.ConvertToString();
                case 13: return v13.ConvertToString();
            }

            return null;
        }
#endif

        [NonEvent]
        public bool IsEnabled(byte level, long keywords)
        {
            return this.IsEnabled((EventLevel)level, (EventKeywords)keywords);
        }
#endif

        #endregion // NonEvents

        #region Utilities

        /// <summary>
        ///
        /// </summary>
        /// <param name="eventAttribute"></param>
        /// <param name="extended"></param>
        /// <returns></returns>
        public static ServiceFabricEventChannel ChannelToUse(EventAttribute eventAttribute, EventExtendedMetadataAttribute extended)
        {
            bool hasKeywordOperational = extended != null;

            if (hasKeywordOperational)
            {
                return ServiceFabricEventChannel.Operational;
            }
            else
            {
                bool moreSevereThanWarningLevel = eventAttribute.Level < EventLevel.Warning;

                if (moreSevereThanWarningLevel)
                {
                    return ServiceFabricEventChannel.Admin;
                }
                else
                {
                    return ServiceFabricEventChannel.Debug;
                }
            }
        }

        public static string FormatStringToUse(EventAttribute eventAttribute, EventExtendedMetadataAttribute extended)
        {
            if (extended == null)
            {
                return eventAttribute.Message;
            }

            var newFormat = new StringBuilder();
            foreach (var field in EventExtendedMetadataAttribute.MetadataFields)
            {
                newFormat.Append(field.FormatString);
                newFormat.Append(", ");
            }

            newFormat.Append(
                ShiftFormatArguments(
                    eventAttribute.Message,
                    (ushort)EventExtendedMetadataAttribute.MetadataFields.Count));
            return newFormat.ToString();
        }

        private static ushort GetFormatArgumentsCount(string format)
        {
            var maxIndex = 0;
            for (var i = 0; i < TraceEvent.MaxFieldsPerEvent; ++i)
            {
                var arg = string.Format("{{{0}}}", i);
                var hexArg = string.Format("{{{0}:x}}", i);
                if (format.Contains(arg) || format.Contains(hexArg))
                {
                    maxIndex = i + 1;
                }
            }

            return (ushort)maxIndex;
        }

        private static string ShiftFormatArguments(string format, ushort indicesDifference)
        {
            var newFormatString = new StringBuilder(format);
            var currentArgsCount = GetFormatArgumentsCount(format);
            for (var i = currentArgsCount - 1; i >= 0; --i)
            {
                var newIndex = i + indicesDifference;

                var arg = string.Format("{{{0}}}", i);
                var newArg = string.Format("{{{0}}}", newIndex);
                newFormatString.Replace(arg, newArg);

                var hexArg = string.Format("{{{0}:x}}", i);
                var hexNewArg = string.Format("{{{0}:x}}", newIndex);
                newFormatString.Replace(hexArg, hexNewArg);
            }

            return newFormatString.ToString();
        }

        #endregion

        #region Extensions

        public class ExtensionsEventsInternal
        {
            //Using a fixed sized array instead of dictionary for better performance.
            protected static WriteVariants[] actionMap;
            private readonly int taskCode;

            protected struct WriteVariants
            {
                public delegate void WriteAction(string id, string type, string message);

                public WriteAction WriteInfo;
                public WriteAction WriteError;
                public WriteAction WriteWarning;
                public WriteAction WriteNoise;

                public WriteVariants(WriteAction info, WriteAction error, WriteAction warning, WriteAction noise)
                {
                    WriteInfo = info;
                    WriteError = error;
                    WriteWarning = warning;
                    WriteNoise = noise;
                }
            }

            /// <summary>
            /// Constructor for ExtensionsEventsInternal
            /// </summary>
            /// <param name="code">TaskCode to be used for the events.</param>
            public ExtensionsEventsInternal(EventTask code)
            {
                this.taskCode = (int)code;
            }

            protected virtual void WriteInfo(string id, string type, string message)
            {
                actionMap[taskCode].WriteInfo(id, type, message);
            }

            protected virtual void WriteError(string id, string type, string message)
            {
                actionMap[taskCode].WriteError(id, type, message);
            }

            protected virtual void WriteWarning(string id, string type, string message)
            {
                actionMap[taskCode].WriteWarning(id, type, message);
            }

            protected virtual void WriteNoise(string id, string type, string message)
            {
                actionMap[taskCode].WriteNoise(id, type, message);
            }

            /// <summary>
            /// Write an error text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteError(string type, string format, params object[] args)
            {
                this.WriteErrorWithId(type, string.Empty, format, args);
            }

            /// <summary>
            /// Write an error text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteErrorWithId(string type, string id, string format, params object[] args)
            {
                if (null == args || 0 == args.Length)
                {
                    this.WriteError(id, type, format);
                }
                else
                {
                    this.WriteError(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
                }
            }

            /// <summary>
            /// Writes a message with an exception
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="ex">The exception to trace</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteExceptionAsError(string type, Exception ex, string format, params object[] args)
            {
                WriteExceptionAsErrorWithId(type, string.Empty, ex, format, args);
            }

            /// <summary>
            /// Writes a message with an exception
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="ex">The exception to trace</param>
            public void WriteExceptionAsError(string type, Exception ex)
            {
                this.WriteExceptionAsError(type, ex, string.Empty);
            }

            /// <summary>
            /// Writes a message with an exception  and an id
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="ex">The exception to trace</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteExceptionAsErrorWithId(
                string type,
                string id,
                Exception ex,
                string format,
                params object[] args)
            {
                this.WriteError(id, type, GetExceptionStringForTrace(ex, format, args));
            }

            /// <summary>
            /// Write a warning text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteWarning(string type, string format, params object[] args)
            {
                this.WriteWarningWithId(type, string.Empty, format, args);
            }

            /// <summary>
            /// Write a warning text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteWarningWithId(string type, string id, string format, params object[] args)
            {
                if (args == null || args.Length == 0)
                {
                    this.WriteWarning(id, type, format);
                }
                else
                {
                    this.WriteWarning(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
                }
            }

            /// <summary>
            /// Writes a message with an exception
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="ex">The exception to trace</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteExceptionAsWarning(string type, Exception ex, string format, params object[] args)
            {
                WriteExceptionAsWarningWithId(type, string.Empty, ex, format, args);
            }

            /// <summary>
            /// Writes a message with an exception
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="ex">The exception to trace</param>
            public void WriteExceptionAsWarning(string type, Exception ex)
            {
                this.WriteExceptionAsWarning(type, ex, string.Empty);
            }

            /// <summary>
            /// Writes a message with an exception  and an id
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="ex">The exception to trace</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteExceptionAsWarningWithId(
                string type,
                string id,
                Exception ex,
                string format,
                params object[] args)
            {
                this.WriteWarning(id, type, GetExceptionStringForTrace(ex, format, args));
            }

            /// <summary>
            /// Writes a message with an exception
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="ex">The exception to trace</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteExceptionAsInfo(string type, Exception ex, string format, params object[] args)
            {
                WriteExceptionAsInfoWithId(type, string.Empty, ex, format, args);
            }

            /// <summary>
            /// Writes a message with an exception
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="ex">The exception to trace</param>
            public void WriteExceptionAsInfo(string type, Exception ex)
            {
                this.WriteExceptionAsInfo(type, ex, string.Empty);
            }

            /// <summary>
            /// Writes a message with an exception  and an id
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="ex">The exception to trace</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteExceptionAsInfoWithId(
                string type,
                string id,
                Exception ex,
                string format,
                params object[] args)
            {
                this.WriteInfo(id, type, GetExceptionStringForTrace(ex, format, args));
            }

            private static string GetExceptionStringForTrace(
                Exception ex,
                string format,
                params object[] args)
            {
                StringBuilder output = new StringBuilder();

                if (args == null || args.Length == 0)
                {
                    output.Append(format);
                }
                else
                {
                    output.AppendFormat(format, args);
                }

                if (ex != null)
                {
                    output.AppendLine();
                    output.Append(ex);
                }

                return output.ToString();
            }

            /// <summary>
            /// Write an informational text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteInfo(string type, string format, params object[] args)
            {
                WriteInfoWithId(type, string.Empty, format, args);
            }

            /// <summary>
            /// Write an informational text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            public void WriteInfo(string type)
            {
                this.WriteInfo(string.Empty, type, string.Empty);
            }

            /// <summary>
            /// Write an informational text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteInfoWithId(string type, string id, string format, params object[] args)
            {
                if (null == args || 0 == args.Length)
                {
                    this.WriteInfo(id, type, format);
                }
                else
                {
                    this.WriteInfo(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
                }
            }

            /// <summary>
            /// Write a noise text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteNoise(string type, string format, params object[] args)
            {
                this.WriteNoiseWithId(type, string.Empty, format, args);
            }

            /// <summary>
            /// Write a noise text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            public void WriteNoise(string type)
            {
                this.WriteNoise(string.Empty, type, string.Empty);
            }

            /// <summary>
            /// Write a noise text event.
            /// </summary>
            /// <param name="type">Type of the event.</param>
            /// <param name="id">The id of the event.</param>
            /// <param name="format">Format string.</param>
            /// <param name="args">The arguments</param>
            public void WriteNoiseWithId(string type, string id, string format, params object[] args)
            {
                if (null == args || 0 == args.Length)
                {
                    this.WriteNoise(id, type, format);
                }
                else
                {
                    this.WriteNoise(id, type, string.Format(CultureInfo.InvariantCulture, format, args));
                }

            }
        }

#endregion
    }
}