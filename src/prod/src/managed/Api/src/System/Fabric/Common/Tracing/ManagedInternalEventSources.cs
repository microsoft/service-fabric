// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*
 *  For Adding new Events :
 *      * Follow the existing template of events while adding new events and make sure the eventId is unique.
 *      (For the sake of convenience a gap in EventId's have been left between modules to allow easy addition of Events)
 *      * Run the build of Microsoft.ServiceFabric.Internal.csproj - This would generate manifest file with your events,
 *        in case there are any errors in the way you've defined the events, there would be a build error in the afterbuild step during generation of manifest.
 *        Listing some common causes for failure -
 *          ? EventId wasn't unique
 *          ? There is already an event with same name
 *          ? Arguments mismatch
 *          ? Any other syntax error
 *      * Test that your event is visible in traces.
 *
 *  For Editing existing Events :
 *      * One of the primary advantages of EventSource over 'print-style' logging is that it can be processed mechanically by the consumers of the events with relative ease.
 *      However this advantage does come at a cost.   The minute you have mechanical processing it is now possible to break that processing as you EventSource
 *      changes unless you follow some rules to insure compatibility.
 *      * While this can be done in many ways, we strongly suggest following these rules by default.
 *          1. Once you add an event with certain set of payload properties, you cannot rename the event, remove any properties,
 *             or change the meaning of the existing properties.
 *          2. You ARE however allowed to add new properties AT THE END YOUR EVENT PAYLOAD. When you add new payloads you should use
 *             the Version property of the event attribute to increment the version for that event (by default the version is 0 so
 *             if the event did not have a Version property you can bump it to 1).
 *          3. If you need to change semantics of the payload properties you should simply make up a new event with a different event name and Event ID.
 *             Events with the same ID should only be changed with the two rules above.
 *      These rules are really VERY easy to follow (only add things (at the end), and if you need to do more, make up entirely new events.
 *      They also are completely analogous to how we version APIs. In the same way that you don't change the meaning of existing methods after shipping a class,
 *      you also don't change existing events (and their payloads). You are free to ADD things (either more properties (at the end)) or more events, or new providers),
 *      but changing existing things may break people.
 *      * Post changes, Run the build of Microsoft.ServiceFabric.Internal.csproj to check for sanity.
 *      * Test that your event is visible in traces.
*/

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

    /// <summary>
    /// Maintains structured trace events logged via Microsoft.ServiceFabric.Internal (MSFI).
    /// MSFI goes in SDK, so Service Fabric events that need to be logged from sdk can use this class.
    /// </summary>
    [EventSource(Name = "Microsoft-ServiceFabric-Internal-Events", Guid = "fa8ea6ea-9bf3-4630-b3ef-2b01ebb2a69b")]

    internal sealed class FabricEvents : FabricEventSource
    {
        // Need to give an opportunity for static configuration properties to be set before constructing singleton.
        private static readonly Lazy<FabricEvents> SingletonEvents = new Lazy<FabricEvents>(() => new FabricEvents());

        private FabricEvents()
        {
            IVariantEventWriter variantEventWriter;

            variantEventWriter = VariantEventWriterOverride ?? this;
            this.eventDescriptors = GenerateEventDescriptors(variantEventWriter);
        }

        public static FabricEvents Events
        {
            get
            {
                return SingletonEvents.Value;
            }
        }

        public static IVariantEventWriter VariantEventWriterOverride { set; private get; }

        #region Events

        [Event(59394, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op11, Level = EventLevel.Informational, Keywords = Keywords.Default)]
        public void SystemFabric_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59394], id, type, message);
        }

        [Event(59393, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op12, Level = EventLevel.Warning, Keywords = Keywords.Default)]
        public void SystemFabric_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59393], id, type, message);
        }

        [Event(59392, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op13, Level = EventLevel.Error, Keywords = Keywords.Default)]
        public void SystemFabric_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59392], id, type, message);
        }

        [Event(59395, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op14, Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        public void SystemFabric_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59395], id, type, message);
        }

        [Event(65026, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op11, Level = EventLevel.Informational, Keywords = Keywords.Default)]
        public void ServiceFramework_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65026], id, type, message);
        }

        [Event(65025, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op12, Level = EventLevel.Warning, Keywords = Keywords.Default)]
        public void ServiceFramework_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65025], id, type, message);
        }

        [Event(65024, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op13, Level = EventLevel.Error, Keywords = Keywords.Default)]
        public void ServiceFramework_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65024], id, type, message);
        }

        [Event(65027, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op14, Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        public void ServiceFramework_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65027], id, type, message);
        }

        #endregion

        #region NonEvents

        [NonEvent]
        internal EventTask GetEventTask(string taskName)
        {
            try
            {
                return this.taskMap[taskName];
            }
            catch (KeyNotFoundException)
            {
                return Tasks.Max;
            }
        }

        [NonEvent]
        protected override ReadOnlyDictionary<string, EventTask> GenerateTaskMap()
        {
            var taskMap = new Dictionary<string, EventTask>
            {
                { "SystemFabric", FabricEvents.Tasks.SystemFabric },
                { "ServiceFramework", FabricEvents.Tasks.ServiceFramework },
            };

            return new ReadOnlyDictionary<string, EventTask>(taskMap);
        }

        #endregion // NonEvents

        public class ExtensionsEvents : ExtensionsEventsInternal
        {
            public ExtensionsEvents(EventTask code) : base(code)
            {
            }

            /// <summary>
            /// Gets object instance which can be used for writing events via EventSource
            /// </summary>
            /// <param name="code">TaskCode to be used for the events.</param>
            public static ExtensionsEvents GetEventSource(EventTask code)
            {
                return new ExtensionsEvents(code);
            }

            static ExtensionsEvents()
            {
                actionMap = new WriteVariants[(int)FabricEvents.Tasks.Max];
                actionMap[(int)FabricEvents.Tasks.SystemFabric] = new WriteVariants(Events.SystemFabric_InfoText, Events.SystemFabric_ErrorText,
                    Events.SystemFabric_WarningText, Events.SystemFabric_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ServiceFramework] = new WriteVariants(Events.ServiceFramework_InfoText, Events.ServiceFramework_ErrorText,
                    Events.ServiceFramework_WarningText, Events.ServiceFramework_NoiseText);
            }
        }

        #region Keywords / Tasks / Opcodes

        public static class Keywords
        {
            public const EventKeywords Default = (EventKeywords)0x01;
            public const EventKeywords AppInstance = (EventKeywords)0x02;
            public const EventKeywords ForQuery = (EventKeywords)0x04;
            public const EventKeywords CustomerInfo = (EventKeywords)0x08;
            public const EventKeywords DataMessaging = (EventKeywords)0x10;
            public const EventKeywords DataMessagingAll = (EventKeywords)0x20;
			/// <summary>
            /// This keyword is special because it will not show up in the Microsoft-ServiceFabric-Events.man
            /// This is used to circumvent the fact that with old version of diagnostics library the Event attribute
            /// does not expose Channel property. This keyword is used to decide which channel to pass in when TraceEvent
            /// object is created in FabricEventSource. ManifestGeneratorManaged, if finds this keyword, puts channel == "Operational" in the manifest.
            /// </summary>
            public const EventKeywords Operational = (EventKeywords)0x40;
        }

        public static class Tasks
        {
            /// <summary>
            /// The trace code for System.Fabric
            /// </summary>
            public const EventTask SystemFabric = (EventTask)1;

            /// <summary>
            /// Service Framework (fabsrv).
            /// </summary>
            public const EventTask ServiceFramework = (EventTask)2;

            /// <summary>
            /// All valid task id must be below this number. Increase when adding tasks.
            /// Max value is 2**16.
            /// </summary>
            public const EventTask Max = (EventTask)3;
        }

        // Eventsource mandates that all events should have unique taskcode/opcode pair. Since taskcode is shared across module we are defining Opcodes here to make the pair unique.
        // Eventsource also limites range for user defined Opcodes within the range of 11-238
        public static class Opcodes
        {
            public const EventOpcode Op11 = (EventOpcode)11;
            public const EventOpcode Op12 = (EventOpcode)12;
            public const EventOpcode Op13 = (EventOpcode)13;
            public const EventOpcode Op14 = (EventOpcode)14;
        }

        #endregion
    }
}