// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.Diagnostics.Tracing;
    using System.Linq;
    using System.Reflection;
    using Config;
    using Writer;

    /// <summary>
    /// ServiceFabricEventSource class is a wrapper on top of EventSource with following benefits
    /// 1) It contains stack-allocated WriteEvent overloads which are faster than ones available under EventSource
    /// 2) It has custom filtering for events through Settings.xml
    /// </summary>
    internal abstract class ServiceFabricEventSource : EventSource
    {
        private const string DefaultPackageName = "Config";
        private readonly ReadOnlyDictionary<int, TraceEvent> eventDescriptors;
        private readonly TraceConfig configMgr;
        private readonly string eventSourceName;

        /// <summary>
        /// Constructor which populates the events descriptor data for all events defined
        /// </summary>
        protected ServiceFabricEventSource() : this(DefaultPackageName)
        {
        }

        /// <summary>
        /// Constructor which populates the events descriptor data for all events defined
        /// </summary>
        protected ServiceFabricEventSource(string configPackageName)
        {
            Type eventSourceType = this.GetType();

            EventSourceAttribute eventSourceAttribute = (EventSourceAttribute)eventSourceType.GetCustomAttributes(typeof(EventSourceAttribute), true).SingleOrDefault();

            this.eventSourceName = eventSourceType.Name;
            if (eventSourceAttribute != null && !string.IsNullOrEmpty(eventSourceAttribute.Name))
            {
                this.eventSourceName = eventSourceAttribute.Name;
            }

            this.configMgr = new TraceConfig(this.eventSourceName, configPackageName);
            this.eventDescriptors = this.GenerateEventDescriptors(eventSourceType);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        [NonEvent]
        protected new void WriteEvent(int eventId)
        {
            this.VariantWrite(eventId, 0);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0)
        {
            this.VariantWrite(eventId, 1, param0);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1)
        {
            this.VariantWrite(eventId, 2, param0, param1);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2)
        {
            this.VariantWrite(eventId, 3, param0, param1, param2);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        /// <param name="param3"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2, Variant param3)
        {
            this.VariantWrite(eventId, 4, param0, param1, param2, param3);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        /// <param name="param3"></param>
        /// <param name="param4"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4)
        {
            this.VariantWrite(eventId, 5, param0, param1, param2, param3, param4);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        /// <param name="param3"></param>
        /// <param name="param4"></param>
        /// <param name="param5"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5)
        {
            this.VariantWrite(eventId, 6, param0, param1, param2, param3, param4, param5);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        /// <param name="param3"></param>
        /// <param name="param4"></param>
        /// <param name="param5"></param>
        /// <param name="param6"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6)
        {
            this.VariantWrite(eventId, 7, param0, param1, param2, param3, param4, param5, param6);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        /// <param name="param3"></param>
        /// <param name="param4"></param>
        /// <param name="param5"></param>
        /// <param name="param6"></param>
        /// <param name="param7"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7)
        {
            this.VariantWrite(eventId, 8, param0, param1, param2, param3, param4, param5, param6, param7);
        }

        /// <summary>
        /// Overriding WriteEvent of EventSource for optimization
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="param0"></param>
        /// <param name="param1"></param>
        /// <param name="param2"></param>
        /// <param name="param3"></param>
        /// <param name="param4"></param>
        /// <param name="param5"></param>
        /// <param name="param6"></param>
        /// <param name="param7"></param>
        /// <param name="param8"></param>
        [NonEvent]
        protected void WriteEvent(int eventId, Variant param0, Variant param1, Variant param2, Variant param3, Variant param4, Variant param5, Variant param6, Variant param7, Variant param8)
        {
            this.VariantWrite(eventId, 9, param0, param1, param2, param3, param4, param5, param6, param7, param8);
        }

        /// <summary>
        /// Generates descriptors for all the events inside a specified class type
        /// </summary>
        /// <param name="eventSourceType">type of the class deriving from ServiceFabricEventSource</param>
        /// <returns>Dictionary of events</returns>
        [NonEvent]
        private ReadOnlyDictionary<int, TraceEvent> GenerateEventDescriptors(Type eventSourceType)
        {
            var eventDescriptors = new Dictionary<int, TraceEvent>();
            MethodInfo[] methods = eventSourceType.GetMethods(BindingFlags.DeclaredOnly | BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);

            foreach (var eventMethod in methods)
            {
                EventAttribute eventAttribute = (EventAttribute)eventMethod.GetCustomAttributes(typeof(EventAttribute), false).SingleOrDefault();
                if (eventAttribute == null)
                {
                    continue;
                }

                var hasId = false;
                int typeFieldIndex = -1;
#if DotNetCoreClrLinux
                var methodParams = eventMethod.GetParameters().ToList();
                if (methodParams.Any())
                {
                    typeFieldIndex = methodParams.FindIndex(e => e.Name == "type");
                    hasId = methodParams[0].Name == "id";
                }
#endif

                var eventId = eventAttribute.EventId;

                eventDescriptors[eventId] = new TraceEvent(
                    eventMethod.Name,
                    eventAttribute.Level,
                    eventAttribute.Keywords,
                    eventAttribute.Message,
                    this.configMgr,
                    hasId,
                    typeFieldIndex);
            }

            return new ReadOnlyDictionary<int, TraceEvent>(eventDescriptors);
        }
#if DotNetCoreClrLinux
        [NonEvent]
        public void VariantWriteViaNative(
            int eventId,
            int argCount,
            Variant v0 = default(Variant),
            Variant v1 = default(Variant),
            Variant v2 = default(Variant),
            Variant v3 = default(Variant),
            Variant v4 = default(Variant),
            Variant v5 = default(Variant),
            Variant v6 = default(Variant),
            Variant v7 = default(Variant),
            Variant v8 = default(Variant))
        {
            string text = string.Format(this.eventDescriptors[eventId].Message,
                new object[] { v0.ToObject(), v1.ToObject(), v2.ToObject(), v3.ToObject(), v4.ToObject(), v5.ToObject(), v6.ToObject(), v7.ToObject(), v8.ToObject() }
            );

            string id = this.eventDescriptors[eventId].hasId && !string.IsNullOrWhiteSpace(v0.ConvertToString()) ? v0.ConvertToString() : eventId.ToString();
            string type = this.eventDescriptors[eventId].EventName;
            if (this.eventDescriptors[eventId].typeFieldIndex != -1)
            {
                string typeField = GetTypeFromIndex(this.eventDescriptors[eventId].typeFieldIndex, v0, v1, v2, v3, v4, v5, v6, v7, v8);
                if (!string.IsNullOrWhiteSpace(typeField))
                {
                    type = typeField;
                }
            }

            System.Fabric.Common.Tracing.TraceViaNative.WriteUnstructured(
                this.eventSourceName,
                type,
                id,
                (ushort)this.eventDescriptors[eventId].Level,
                text);
        }

        [NonEvent]
        private string GetTypeFromIndex(int index, Variant v0, Variant v1, Variant v2, Variant v3, Variant v4,
            Variant v5, Variant v6, Variant v7, Variant v8)
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
            }

            return null;
        }

#endif
        [NonEvent]
        private unsafe void VariantWrite(
        int eventId,
        int argCount,
        Variant v0 = default(Variant),
        Variant v1 = default(Variant),
        Variant v2 = default(Variant),
        Variant v3 = default(Variant),
        Variant v4 = default(Variant),
        Variant v5 = default(Variant),
        Variant v6 = default(Variant),
        Variant v7 = default(Variant),
        Variant v8 = default(Variant))
        {
            if (this.eventDescriptors[eventId].IsEventSinkEnabled())
            {
                if (argCount == 0)
                {
                    this.WriteEventCore(eventId, argCount, null);
                }
                else
                {
                    EventData* eventSourceData = stackalloc EventData[argCount]; // allocation for the data descriptors
                    byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];

                    // 16 byte for non-string argument
                    var edb = new EventDataArrayBuilder((Writer.EventData*)eventSourceData, dataBuffer);

                    // The block below goes through all the arguments and fills in the data
                    // descriptors.
                    edb.AddEventData(v0);
                    if (argCount > 1)
                    {
                        edb.AddEventData(v1);
                        if (argCount > 2)
                        {
                            edb.AddEventData(v2);
                            if (argCount > 3)
                            {
                                edb.AddEventData(v3);
                                if (argCount > 4)
                                {
                                    edb.AddEventData(v4);
                                    if (argCount > 5)
                                    {
                                        edb.AddEventData(v5);
                                        if (argCount > 6)
                                        {
                                            edb.AddEventData(v6);
                                            if (argCount > 7)
                                            {
                                                edb.AddEventData(v7);
                                                if (argCount > 8)
                                                {
                                                    edb.AddEventData(v8);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (!edb.IsValid())
                    {
                        Debug.Fail(string.Format(
                            "EventData for eventid {0} is invalid. Check the total size of the event.", eventId));
                        return;
                    }

                    fixed (
                        char* s0 = v0.ConvertToString(),
                        s1 = v1.ConvertToString(),
                        s2 = v2.ConvertToString(),
                        s3 = v3.ConvertToString(),
                        s4 = v4.ConvertToString(),
                        s5 = v5.ConvertToString(),
                        s6 = v6.ConvertToString(),
                        s7 = v7.ConvertToString(),
                        s8 = v8.ConvertToString())
                    {
                        var eventDataPtr = edb.ToEventDataArray(
                            s0,
                            s1,
                            s2,
                            s3,
                            s4,
                            s5,
                            s6,
                            s7,
                            s8);
                        this.WriteEventCore(eventId, argCount, (EventData*)eventDataPtr);
                    }
                }
            }
#if DotNetCoreClrLinux
            this.VariantWriteViaNative(eventId, 9, v0, v1, v2, v3, v4, v5, v6, v7, v8);
#endif
        }
    }
}
