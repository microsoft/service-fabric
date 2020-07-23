// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*
 *  For adding new events to below class follow the onenote guide for latest policy https://microsoft.sharepoint.com/teams/WindowsFabric/_layouts/OneNote.aspx?id=%2Fteams%2FWindowsFabric%2FNotebooks%2FWindows%20Fabric%20Design%20Notes&wd=target%28NanoServer%20compliance.one%7CAA560593-DB04-4D06-91B5-DBE8708646F4%2FGuide%20for%20adding%5C%2Fediting%20Events%20in%20Managed%20code%7CC9E1DD99-A606-4335-A678-0ACE1A7BD9B1%2F%29
 *  Pasting the content of one note here for quick reference.
 *
 *  History
 *      We've moved from legacy eventing system (TraceEventWriter) to EventSource
 *      With this change few files have been omitted and replaced by a central file for all events.
 *      The events have been taken from these their original files and translated to EventSource events under file
 *      WindowsFabric\src\prod\src\Managed\Api\src\System\Fabric\Common\Tracing\ManagedEventSources.cs
 *
 *      Going forward if there are any changes to be done to events please modify ManagedEventSources.cs
 *
 *  For Adding new Events
 *      � Follow the existing template of events while adding new events and make sure the eventId is unique. (For the sake of convenience a gap in EventId's have been left between modules to allow easy addition of Events)
 *      � Run the build of System.Fabric.csproj - This would generate manifest file with your events, in case there are any errors in the way you've defined the events, there would be a build error in the afterbuild step during generation of manifest. Listing some common causes for failure -
 *          ? EventId wasn't unique
 *          ? There is already an event with same name
 *          ? Arguments mismatch
 *          ? Any other syntax error
 *      � Test that your event is visible in traces.
 *
 *  For Editing existing Events
 *
 *      � One of the primary advantages of EventSource over �print-style� logging is that it can be processed mechanically by the consumers of the events with relative ease.    However this advantage does come at a cost.   The minute you have mechanical processing it is now possible to break that processing as you EventSource changes unless you follow some rules to insure compatibility.
 *      � While this can be done in many ways, we strongly suggest following these rules by default.
 *          1. Once you add an event with certain set of payload properties, you cannot rename the event, remove any properties, or change the meaning of the existing properties.
 *          2. You ARE however allowed to add new properties AT THE END YOUR EVENT PAYLOAD. When you add new payloads you should use the Version property of the event attribute to increment the version for that event (by default the version is 0 so if the event did not have a Version property you can bump it to 1).
 *          3. If you need to change semantics of the payload properties you should simply make up a new event with a different event name and Event ID. Events with the same ID should only be changed with the two rules above.
 *      These rules are really VERY easy to follow (only add things (at the end), and if you need to do more, make up entirely new events. They also are completely analogous to how we version APIs. In the same way that you don�t change the meaning of existing methods after shipping a class, you also don�t change existing events (and their payloads). You are free to ADD things (either more properties (at the end)) or more events, or new providers), but changing existing things may break people.
 *      � Post changes, Run the build of System.Fabric.csproj to check for sanity.
 *      � Test that your event is visible in traces.
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

    using ServiceFabricEventChannel = System.Fabric.Common.Tracing.EventChannel;
#if DotNetCoreClr
    using SystemEventChannel = System.Diagnostics.Tracing.EventChannel;
#endif

    /// <summary>
    /// Maintains structured trace events.
    /// </summary>
#if DotNetCoreClr
    [EventSource(Name = "Microsoft-ServiceFabric-Events", Guid = "cbd93bc2-71e5-4566-b3a7-595d8eeca6e8")]
#endif
    internal sealed class FabricEvents : FabricEventSource
    {
        // TaskId 65534 is designated for EventSource messages we don't want these messages (so filter these messages in DCA and TraceViewer)
        public const EventTask EventSourceTaskId = (EventTask)65534;

        internal const string ProviderName = "Microsoft-ServiceFabric";
        internal const string ProviderMessage = "Microsoft-Service Fabric";
        internal const string ProviderSymbol = "Microsoft_ServiceFabric";
        internal static readonly Guid ProviderId = Guid.Parse("cbd93bc2-71e5-4566-b3a7-595d8eeca6e8");

        // Need to give an opportunity for static configuration properties to be set before constructing singleton.
        private static readonly Lazy<FabricEvents> SingletonEvents = new Lazy<FabricEvents>(() => new FabricEvents());

        private FabricEvents()
        {
            IVariantEventWriter variantEventWriter;

#if (DotNetCoreClr)
            variantEventWriter = VariantEventWriterOverride ?? this;
            this.eventDescriptors = GenerateEventDescriptors(variantEventWriter);
#else
            variantEventWriter = VariantEventWriterOverride ?? new VariantEventProvider(ProviderId);
            this.eventDescriptors = GenerateEventDescriptors(variantEventWriter);
#endif
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

        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\DCA\product\host\MockHostingEvents.Test.cs***/
        /***********************/

        // This event is used for testing only.
        [ProvisionalMetadata(PositionOfIdElements = Position.Zero, ProvisionalTimeInMs = 1000)]
        [Event(23060, Message = "Test message {0};{1}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op52, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestProvisionalTraceChainStart(int arg1, string arg2)
        {
            WriteEvent(this.eventDescriptors[23060], arg1, arg2);
        }

        // This event is used for testing only.
        [ProvisionalMetadata(PositionOfIdElements = Position.Zero, FlushAndClose = true)]
        [Event(23063, Message = "Test message {0};{1},{2}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op53, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestProvisionalTraceChainEnd(int arg1, string arg2, string arg3)
        {
            WriteEvent(this.eventDescriptors[23063], arg1, arg2, arg3);
        }

        // This event is used for testing only.
        [Event(23064, Message = "Notify DCA: ServicePackage {1}:{3}:{5}:{6} is active.", Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op11, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.AppInstance)]
        public void ServicePackageActivatedNotifyDca(int version, string nodeName, string runLayoutRoot,
            string applicationId, string applicationVersion, string servicePackageName, string servicePackageVersion)
        {
            WriteEvent(this.eventDescriptors[23064], version, nodeName, runLayoutRoot, applicationId, applicationVersion, servicePackageName,
                    servicePackageVersion);
        }

        // This event is used for testing only.
        [Event(23065, Message = "Notify DCA: ServicePackage {1}:{2}:{3} has been deactivated.", Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op12, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.AppInstance)]
        public void ServicePackageDeactivatedNotifyDca(int version, string nodeName, string applicationId,
            string servicePackageName)
        {
            WriteEvent(this.eventDescriptors[23065], version, nodeName, applicationId, servicePackageName);
        }

        // This event is used for testing only.
        [Event(23066, Message = "Notify DCA: ServicePackage {1}:{3}:{5}:{6} is active after upgrade.",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op13, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.AppInstance)]
        public void ServicePackageUpgradedNotifyDca(int version, string nodeName, string runLayoutRoot,
            string applicationId, string applicationVersion, string servicePackageName, string servicePackageVersion)
        {
            WriteEvent(this.eventDescriptors[23066], version, nodeName, runLayoutRoot, applicationId, applicationVersion, servicePackageName,
                    servicePackageVersion);
        }

        // This event is used for testing only.
        [Event(23067, Message = "Test message",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op23, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace0()
        {
            WriteEvent(this.eventDescriptors[23067]);
        }

        // This event is used for testing only.
        [Event(23068, Message = "Test message {0}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op24, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace1(int arg1)
        {
            WriteEvent(this.eventDescriptors[23068], arg1);
        }

        // This event is used for testing only.
        [Event(23069, Message = "Test message {0};{1}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op25, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace2(int arg1, string arg2)
        {
            WriteEvent(this.eventDescriptors[23069], arg1, arg2);
        }

        // This event is used for testing only.
        [Event(23070, Message = "Test message {0};{1};{2}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op17, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace3(int arg1, string arg2, DateTime arg3)
        {
            WriteEvent(this.eventDescriptors[23070], arg1, arg2, arg3);
        }

        // This event is used for testing only.
        [Event(23071, Message = "Test message {0};{1};{2};{3}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op18, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace4(int arg1, string arg2, DateTime arg3,
            Guid arg4)
        {
            WriteEvent(this.eventDescriptors[23071], arg1, arg2, arg3, arg4);
        }

        // This event is used for testing only.
        [Event(23072, Message = "Test message {0};{1};{2};{3};{4}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op19, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace5(int arg1, string arg2, DateTime arg3,
            Guid arg4, ulong arg5)
        {
            WriteEvent(this.eventDescriptors[23072], arg1, arg2, arg3, arg4, arg5);
        }

        // This event is used for testing only.
        [Event(23073, Message = "Test message {0};{1};{2};{3};{4};{5}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op20, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace6(int arg1, string arg2, DateTime arg3,
            Guid arg4, ulong arg5, byte arg6)
        {
            WriteEvent(this.eventDescriptors[23073], arg1, arg2, arg3, arg4, arg5, arg6);
        }

        // This event is used for testing only.
        [Event(23074, Message = "Test message {0};{1};{2};{3};{4};{5};{6}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op21, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace7(int arg1, string arg2, DateTime arg3,
            Guid arg4, ulong arg5, byte arg6, bool arg7)
        {
            WriteEvent(this.eventDescriptors[23074], arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        }

        // This event is used for testing only.
        [Event(23075, Message = "Test message {0};{1};{2};{3};{4};{5};{6};{7}",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op22, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestStructuredTrace8(int arg1, string arg2, DateTime arg3,
            Guid arg4, ulong arg5, byte arg6, bool arg7,
            short arg8)
        {
            WriteEvent(this.eventDescriptors[23075], arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
        }

        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\DCA\product\system.fabric.dca\StructuredEvents.cs***/
        /***********************/

        [Event(59908, Message = "The plugin {0} has been disposed.", Task = Tasks.FabricDCA, Opcode = Opcodes.Op14,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PluginDisposed(string SectionName)
        {
            WriteEvent(this.eventDescriptors[59908], SectionName);
        }

        [Event(59909, Message = "Notify DCA: ServicePackage {1}:{2}:{3} has been deactivated.", Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op15, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.AppInstance)]
        public void ServicePackageInactive(int version, string nodeName, string applicationId, string servicePackageName)
        {
            WriteEvent(this.eventDescriptors[59909], version, nodeName, applicationId, servicePackageName);
        }

        [Event(59910, Message = "Crash dump for Microsoft Azure Service Fabric binary found. Dump file name: {0}.",
            Task = Tasks.FabricDCA, Opcode = Opcodes.Op16, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WindowsFabricCrashDumpFound(string id)
        {
            WriteEvent(this.eventDescriptors[59910], id);
        }

        [Event(
            59911,
            Message = "Plugins enabled: {0}.",
            Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op26,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PluginConfigurationTelemetry(
            long pluginsEnabled)
        {
            WriteEvent(this.eventDescriptors[59911], pluginsEnabled);
        }

        [Event(
            59912,
            Message = "App Plugins enabled for {0}: {1}.",
            Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op27,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AppPluginConfigurationTelemetry(
            string applicationInstanceIdHash,
            long pluginsEnabled)
        {
            WriteEvent(this.eventDescriptors[59912], applicationInstanceIdHash, pluginsEnabled);
        }

        [Event(
            59913,
            Message = "Global {0} policy deleted {1} of {2} attempted files, {3}B of goal {4}B, oldest file {5} minutes old.",
            Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op28,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DiskSpaceManagerGlobalPolicyStats(
            bool isSafeDeletion,
            long deleteSuccessfulFiles,
            long deleteAttemptedFiles,
            long bytesDeleted,
            long bytesGoal,
            double oldestFileInMinutes)
        {
            WriteEvent(this.eventDescriptors[59913], isSafeDeletion, deleteSuccessfulFiles, deleteAttemptedFiles, bytesDeleted, bytesGoal, oldestFileInMinutes);
        }

        [Event(
            59914,
            Message = "TraceType {0}, PassTimeInSeconds {1}, EtwEventsProcessed {2}",
            Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op29,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EtlPassPerformance(
            string traceType,
            double passTimeInSeconds,
            long etwEventsProcessed)
        {
            WriteEvent(this.eventDescriptors[59914], traceType, passTimeInSeconds, etwEventsProcessed);
        }

        [Event(
            59915,
            Message = "TraceType {0}, BacklogSize {1}",
            Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op30,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EtlPassBacklogPerformance(
            string traceType,
            long backlogSize)
        {
            WriteEvent(this.eventDescriptors[59915], traceType, backlogSize);
        }

        [Event(
            59916,
            Message = "Trace session name: {0}, Events lost: {1}, Trace session start time: {2}, Trace session end time: {3}",
            Task = Tasks.FabricDCA,
            Opcode = Opcodes.Op31,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TraceSessionStats(
            string traceSessionName,
            long eventsLostCount,
            DateTime startTime,
            DateTime endTime)
        {
            WriteEvent(this.eventDescriptors[59916], traceSessionName, eventsLostCount, startTime, endTime);
        }

        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\System.Fabric.Wave\Wave\StructuredEvents.cs***/
        /***********************/

        [Event(62212, Message = "wave={1} kind={2} txn={3}", Task = Tasks.Wave, Opcode = Opcodes.Op11,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Created(string id, string wave, string kind, string transaction)
        {
            WriteEvent(this.eventDescriptors[62212], id, wave, kind, transaction);
        }

        [Event(62213, Message = "wave={1} kind={2} outboundStream={3} target={4} partition={5} txn={6}",
            Task = Tasks.Wave, Opcode = Opcodes.Op12, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SendFeedback(string id, string wave, string kind, string outboundStream, string target,
            string partition, string transaction)
        {
            WriteEvent(this.eventDescriptors[62213], id, wave, kind, outboundStream, target, partition, transaction);
        }

        [Event(62214, Message = "wave={1} inboundStream={2} source={3} partition={4} txn={5}", Task = Tasks.Wave,
            Opcode = Opcodes.Op13, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReceiveFeedback(string id, string wave, string inboundStream, string source, string partition,
            string transaction)
        {
            WriteEvent(this.eventDescriptors[62214], id, wave, inboundStream, source, partition, transaction);
        }

        [Event(62215, Message = "wave={1} inboundStream={2} source={3} partition={4} txn={5}", Task = Tasks.Wave,
            Opcode = Opcodes.Op14, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReceiveWave(string id, string wave, string inboundStream, string source, string partition,
            string transaction)
        {
            WriteEvent(this.eventDescriptors[62215], id, wave, inboundStream, source, partition, transaction);
        }

        [Event(62216, Message = "wave={1} kind={2} txn={3}", Task = Tasks.Wave, Opcode = Opcodes.Op15,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Completed(string id, string wave, string kind, string transaction)
        {
            WriteEvent(this.eventDescriptors[62216], id, wave, kind, transaction);
        }

        [Event(62217, Message = "wave={1} kind={2} txn={3}", Task = Tasks.Wave, Opcode = Opcodes.Op16,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Process(string id, string wave, string kind, string transaction)
        {
            WriteEvent(this.eventDescriptors[62217], id, wave, kind, transaction);
        }

        [Event(62218, Message = "wave={1} outboundStream={2} target={3} partition={4} txn={5}", Task = Tasks.Wave,
            Opcode = Opcodes.Op17, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SendWave(string id, string wave, string outboundStream, string target, string partition,
            string transaction)
        {
            WriteEvent(this.eventDescriptors[62218], id, wave, outboundStream, target, partition, transaction);
        }

        [Event(62219, Message = "wave={1} state={2}", Task = Tasks.Wave, Opcode = Opcodes.Op18,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Restart(string id, string wave, string state)
        {
            WriteEvent(this.eventDescriptors[62219], id, wave, state);
        }

        [Event(62220, Message = "phase={1}", Task = Tasks.Wave, Opcode = Opcodes.Op19, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PrimaryProcessing(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[62220], id, phase);
        }

        [Event(62221, Message = "outboundStream={1} action={2} service={3} partition={4}", Task = Tasks.Wave,
            Opcode = Opcodes.Op20, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OutboundStream(string id, string outboundStream, string action, string target, string partition)
        {
            WriteEvent(this.eventDescriptors[62221], id, outboundStream, action, target, partition);
        }

        [Event(62222, Message = "inboundStream={1} action={2} service={3} partition={4}", Task = Tasks.Wave,
            Opcode = Opcodes.Op21, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InboundStream(string id, string inboundStream, string action, string source, string partition)
        {
            WriteEvent(this.eventDescriptors[62222], id, inboundStream, action, source, partition);
        }

        [Event(62223, Message = "wave={1} kind={2} txn={3}", Task = Tasks.Wave, Opcode = Opcodes.Op22,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Exists(string id, string wave, string kind, string transaction)
        {
            WriteEvent(this.eventDescriptors[62223], id, wave, kind, transaction);
        }

        [Event(62224, Message = "inboundStream={1} action={2} service={3} partition={4}", Task = Tasks.Wave,
            Opcode = Opcodes.Op23, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InboundStreamRestart(string id, string inboundStream, string action, string source, string partition)
        {
            WriteEvent(this.eventDescriptors[62224], id, inboundStream, action, source, partition);
        }

        [Event(62225, Message = "outboundStream={1} action={2} service={3} partition={4}", Task = Tasks.Wave,
            Opcode = Opcodes.Op24, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OutboundStreamRestart(string id, string outboundStream, string action, string target,
            string partition)
        {
            WriteEvent(this.eventDescriptors[62225], id, outboundStream, action, target, partition);
        }




        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\Microsoft.ServiceFabric.Data.Impl\Collections\StructuredEvents.cs***/
        /***********************/

        [Event(61700, Message = "keyType={1}, valueType={2}, keySerializerType={3}, valueSerializerType={4}",
            Task = Tasks.DistributedDictionary, Opcode = Opcodes.Op16, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Initialize_DistributedDictionary(string id, string keyType, string valueType,
            string keySerializerType, string valueSerializerType, string behavior)
        {
            WriteEvent(this.eventDescriptors[61700], id, keyType, valueType, keySerializerType, valueSerializerType, behavior);
        }

        [Event(61957, Message = "keySerializerType={1}, valueType={2}, valueSerializerType={3}",
            Task = Tasks.DistributedQueue, Opcode = Opcodes.Op17, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Initialize_DistributedQueue(string id, string keySerializerType, string valueType,
            string valueSerializerType)
        {
            WriteEvent(this.eventDescriptors[61957], id, keySerializerType, valueType, valueSerializerType);
        }

        [Event(61958, Message = "txn={1}, tailPointer={2}", Task = Tasks.DistributedQueue, Opcode = Opcodes.Op11,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EnqueueAsync(string id, long transactionId, long tailPointer)
        {
            WriteEvent(this.eventDescriptors[61958], id, transactionId, tailPointer);
        }

        [Event(61959, Message = "txn={1}, headPointer={2}", Task = Tasks.DistributedQueue, Opcode = Opcodes.Op12,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DequeueAsync(string id, long transactionId, long headPointer)
        {
            WriteEvent(this.eventDescriptors[61959], id, transactionId, headPointer);
        }

        [Event(61960, Message = "txn={1}, count={2}", Task = Tasks.DistributedQueue,
            Opcode = Opcodes.Op13, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetCountAsync(string id, long transactionId, long count)
        {
            WriteEvent(this.eventDescriptors[61960], id, transactionId, count);
        }

        [Event(61961, Message = "txn={1}, tailPointer={2}, exception={3}", Task = Tasks.DistributedQueue,
            Opcode = Opcodes.Op14, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void EnqueueAsyncCompensationFailed(string id, long transactionId, long tailPointer, string exception)
        {
            WriteEvent(this.eventDescriptors[61961], id, transactionId, tailPointer, exception);
        }

        [Event(61962, Message = "txn={1}, headPointerPointer={2}, exception={3}", Task = Tasks.DistributedQueue,
            Opcode = Opcodes.Op15, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void DequeueAsyncCompensationFailed(string id, long transactionId, long headPointer, string exception)
        {
            WriteEvent(this.eventDescriptors[61962], id, transactionId, headPointer, exception);
        }


        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\Microsoft.ServiceFabric.Data.Impl\ReliableMessaging\stream\StructuredTraceEvents.cs***/
        /***********************/


        [Event(62469, Message = "StreamName: {1} Quota: {2} Target: {3} StreamId::TransactionId: {4}",
            Task = Tasks.ReliableStream, Opcode = Opcodes.Op11, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CreateStream(string id, string StreamName, string Quota, string Target, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62469], id, StreamName, Quota, Target, StreamId);
        }

        [Event(62470, Message = "StreamName: {1} Target: {2} StreamId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op12, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void DuplicateStreamName(string id, string StreamName, string Target, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62470], id, StreamName, Target, StreamId);
        }

        [Event(62471, Message = "StreamName: {1} StreamId: {2} Source: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op13, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InboundStreamRequested(string id, string StreamName, string StreamId, string Source)
        {
            WriteEvent(this.eventDescriptors[62471], id, StreamName, StreamId, Source);
        }

        [Event(62472, Message = "StreamId: {1} Response: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op14,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteOpenStreamProtocol(string id, string StreamId, string Response)
        {
            WriteEvent(this.eventDescriptors[62472], id, StreamId, Response);
        }

        [Event(62473, Message = "StreamId: {1} Response: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op15,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCloseStreamProtocol(string id, string StreamId, string Response)
        {
            WriteEvent(this.eventDescriptors[62473], id, StreamId, Response);
        }

        [Event(62474, Message = "StreamName: {1} StreamId: {2} Source: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op16, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RegisterInboundStream(string id, string StreamName, string StreamId, string Source)
        {
            WriteEvent(this.eventDescriptors[62474], id, StreamName, StreamId, Source);
        }

        [Event(62475, Message = "Partner: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op17,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ResetPartnerStreams(string id, string Partner)
        {
            WriteEvent(this.eventDescriptors[62475], id, Partner);
        }

        [Event(62476, Message = "Partner: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op18,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PausePartnerStreams(string id, string Partner)
        {
            WriteEvent(this.eventDescriptors[62476], id, Partner);
        }

        [Event(62477, Message = "Partner: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op19,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestartPartnerStreams(string id, string Partner)
        {
            WriteEvent(this.eventDescriptors[62477], id, Partner);
        }

        [Event(62478, Message = "PartitionKey: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op20,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverPartnerStreams(string id, string PartitionKey)
        {
            WriteEvent(this.eventDescriptors[62478], id, PartitionKey);
        }

        [Event(62481, Message = "Partner: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op21,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SendingResetPartnerStreams(string id, string Partner)
        {
            WriteEvent(this.eventDescriptors[62481], id, Partner);
        }

        [Event(62482, Message = "StreamId: {1} SequenceNumber: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op22,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetLastSequenceNumberSent(string id, string StreamId, long SequenceNumber)
        {
            WriteEvent(this.eventDescriptors[62482], id, StreamId, SequenceNumber);
        }

        [Event(62484, Message = "StreamId: {1} Acceptance: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op24,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetAcceptanceOutcome(string id, string StreamId, string Acceptance)
        {
            WriteEvent(this.eventDescriptors[62484], id, StreamId, Acceptance);
        }

        [Event(62485, Message = "StreamId: {1} Target: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op25,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenStream(string id, string StreamId, string Target)
        {
            WriteEvent(this.eventDescriptors[62485], id, StreamId, Target);
        }

        [Event(62486, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op26,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseStream(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62486], id, StreamId);
        }

        [Event(62487, Message = "StreamId: {1} Target: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op27,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenStreamFailure(string id, string StreamId, string Target)
        {
            WriteEvent(this.eventDescriptors[62487], id, StreamId, Target);
        }

        [Event(62488, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op28,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseStreamFailure(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62488], id, StreamId);
        }

        [Event(62489, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op29, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Receive(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62489], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62490, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op30, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Send(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62490], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62491, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op31,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenRequestSent(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62491], id, StreamId);
        }

        [Event(62492, Message = "StreamId: {1} SequenceNumber: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op32,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseRequestSent(string id, string StreamId, long SequenceNumber)
        {
            WriteEvent(this.eventDescriptors[62492], id, StreamId, SequenceNumber);
        }

        [Event(62493, Message = "StreamId: {1} SequenceNumber: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op33,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AckReceived(string id, string StreamId, long SequenceNumber)
        {
            WriteEvent(this.eventDescriptors[62493], id, StreamId, SequenceNumber);
        }

        [Event(62494, Message = "StreamId: {1} SequenceNumber: {2} Kind: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op34, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WireMessageReceived(string id, string StreamId, long SequenceNumber, string MessageKind)
        {
            WriteEvent(this.eventDescriptors[62494], id, StreamId, SequenceNumber, MessageKind);
        }

        [Event(62495, Message = "StreamId: {1} SequenceNumber: {2} Kind: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op35, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WireMessageSent(string id, string StreamId, long SequenceNumber, string MessageKind)
        {
            WriteEvent(this.eventDescriptors[62495], id, StreamId, SequenceNumber, MessageKind);
        }

        [Event(62496, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op36, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DataMessageDelivery(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62496], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62497, Message = "StreamId: {1} SequenceNumber: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op37,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MessageDroppedAsDuplicate(string id, string StreamId, long SequenceNumber)
        {
            WriteEvent(this.eventDescriptors[62497], id, StreamId, SequenceNumber);
        }

        [Event(62498, Message = "StreamId: {1} Response: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op38,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteOpenStream(string id, string StreamId, string Response)
        {
            WriteEvent(this.eventDescriptors[62498], id, StreamId, Response);
        }

        [Event(62499, Message = "StreamId: {1} Response: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op39,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCloseStream(string id, string StreamId, string Response)
        {
            WriteEvent(this.eventDescriptors[62499], id, StreamId, Response);
        }

        [Event(62500, Message = "Key: {1} Timeout: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op40,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SyncPointWait(string id, string Key, string Timeout)
        {
            WriteEvent(this.eventDescriptors[62500], id, Key, Timeout);
        }

        [Event(62501, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op41, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseInboundStream(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62501], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62502, Message = "StreamId: {1} SequenceNumber: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op42,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InboundStreamClosed(string id, string StreamId, long SequenceNumber)
        {
            WriteEvent(this.eventDescriptors[62502], id, StreamId, SequenceNumber);
        }

        [Event(62503, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op43, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SetCloseMessageSequenceNumber(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62503], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62504, Message = "StreamId: {1} TransactionId: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op44,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void NotPrimaryException(string id, string StreamId, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62504], id, StreamId, TransactionId);
        }

        [Event(62505, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op45, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void ReceiveFailure(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62505], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62506, Message = "StreamId: {1} SequenceNumber: {2} TransactionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op46, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void SendFailure(string id, string StreamId, long SequenceNumber, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62506], id, StreamId, SequenceNumber, TransactionId);
        }

        [Event(62507, Message = "StreamId: {1} State: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op47,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Pause(string id, string StreamId, string State)
        {
            WriteEvent(this.eventDescriptors[62507], id, StreamId, State);
        }

        [Event(62508, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op48,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenOnRestart(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62508], id, StreamId);
        }

        [Event(62509, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op49,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseOnRestart(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62509], id, StreamId);
        }

        [Event(62510, Message = "StreamId: {1} State: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op50,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestartOutbound(string id, string StreamId, string State)
        {
            WriteEvent(this.eventDescriptors[62510], id, StreamId, State);
        }

        [Event(62511, Message = "StreamId: {1} State: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op51,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestartInbound(string id, string StreamId, string State)
        {
            WriteEvent(this.eventDescriptors[62511], id, StreamId, State);
        }

        [Event(62512, Message = "PartitionKey: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op52,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteEndpointProperty(string id, string PartitionKey)
        {
            WriteEvent(this.eventDescriptors[62512], id, PartitionKey);
        }

        [Event(62513, Message = "PartitionKey: {1} PartitionInfo: {2}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op53, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RegisterEndpointProperty(string id, string PartitionKey, string PartitionInfo)
        {
            WriteEvent(this.eventDescriptors[62513], id, PartitionKey, PartitionInfo);
        }

        [Event(62514, Message = "PartitionKey: {1} Endpoint: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op54,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetEndpointProperty(string id, string PartitionKey, string Endpoint)
        {
            WriteEvent(this.eventDescriptors[62514], id, PartitionKey, Endpoint);
        }

        [Event(62515, Message = "Target: {1} Endpoint: {2} SessionId: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op55, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SetupSession(string id, string PartitionKey, string Endpoint, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62515], id, PartitionKey, Endpoint, SessionId);
        }

        [Event(62516, Message = "Target: {1} SessionId: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op56,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ClearOutboundSession(string id, string PartitionKey, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62516], id, PartitionKey, SessionId);
        }

        [Event(62517, Message = "Source: {1} SessionId: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op57,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptInboundSession(string id, string PartitionKey, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62517], id, PartitionKey, SessionId);
        }

        [Event(62518, Message = "Target: {1} SessionId: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op58,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FindOrCreateOutboundSession(string id, string PartitionKey, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62518], id, PartitionKey, SessionId);
        }

        [Event(62519, Message = "PartnerKey: {1} SessionId: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op59,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SessionAbortedByPartner(string id, string PartitionKey, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62519], id, PartitionKey, SessionId);
        }

        [Event(62520, Message = "BaseKey: {1} ResolvedKey: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op60,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ResolveTargetPartition(string id, string BaseKey, string ResolvedKey)
        {
            WriteEvent(this.eventDescriptors[62520], id, BaseKey, ResolvedKey);
        }

        [Event(62521, Message = "Target: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op61,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FindOrCreateOutboundSessionDriver(string id, string PartitionKey)
        {
            WriteEvent(this.eventDescriptors[62521], id, PartitionKey);
        }

        [Event(62522, Message = "Target: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op62,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ClearOutboundSessionDriver(string id, string PartitionKey)
        {
            WriteEvent(this.eventDescriptors[62522], id, PartitionKey);
        }

        [Event(62523, Message = "SessionId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op63,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SessionSendFailure(string id, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62523], id, SessionId);
        }

        [Event(62524, Message = "SessionId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op64,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SessionReceiveFailure(string id, string SessionId)
        {
            WriteEvent(this.eventDescriptors[62524], id, SessionId);
        }

        [Event(62525, Message = "StreamId: {1} SequenceNumber: {2} Kind: {3}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op65, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void StreamNotFoundForInboundWireMessage(string id, string StreamId, long SequenceNumber,
            string MessageKind)
        {
            WriteEvent(this.eventDescriptors[62525], id, StreamId, SequenceNumber, MessageKind);
        }

        [Event(62526, Message = "{1} TransactionId: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op66,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReplicatorTransactionTransientException(string id, string Context, string TransactionId)
        {
            WriteEvent(this.eventDescriptors[62526], id, Context, TransactionId);
        }

        [Event(62527, Message = "{1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op67, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetStreamParameterException(string id, string Context)
        {
            WriteEvent(this.eventDescriptors[62527], id, Context);
        }

        [Event(62528, Message = "{1} -> {2} Era: {3}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op68,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChangeRole_ReliableStream(string id, string OldRole, string NewRole, string Era)
        {
            WriteEvent(this.eventDescriptors[62528], id, OldRole, NewRole, Era);
        }

        [Event(62529, Message = "Role: {1} Era: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op69,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenStreamManager(string id, string Role, string Era)
        {
            WriteEvent(this.eventDescriptors[62529], id, Role, Era);
        }

        [Event(62530, Message = "Role: {1} Era: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op70,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseStreamManager(string id, string Role, string Era)
        {
            WriteEvent(this.eventDescriptors[62530], id, Role, Era);
        }

        [Event(62531, Message = "Role: {1} Era: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op71,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AbortStreamManager(string id, string Role, string Era)
        {
            WriteEvent(this.eventDescriptors[62531], id, Role, Era);
        }

        [Event(62532, Message = "Context: {1} Era: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op72,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChangeRoleSetEventFailure(string id, string Context, string Era)
        {
            WriteEvent(this.eventDescriptors[62532], id, Context, Era);
        }

        [Event(62533, Message = "Era: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op73,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StartAsPrimary(string id, string Era)
        {
            WriteEvent(this.eventDescriptors[62533], id, Era);
        }

        [Event(62534, Message = "Era: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op74,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RegisterStores(string id, string Era)
        {
            WriteEvent(this.eventDescriptors[62534], id, Era);
        }

        [Event(62535, Message = "Era: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op75,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Register(string id, string Era)
        {
            WriteEvent(this.eventDescriptors[62535], id, Era);
        }

        [Event(62536, Message = "Era: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op76,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InitializeCommunication(string id, string Era)
        {
            WriteEvent(this.eventDescriptors[62536], id, Era);
        }

        [Event(62537, Message = "NewRole: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op77,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ShutdownCommunication(string id, string Role)
        {
            WriteEvent(this.eventDescriptors[62537], id, Role);
        }

        [Event(62538, Message = "Era: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op78,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetSingletonStreamManager(string id, string Era)
        {
            WriteEvent(this.eventDescriptors[62538], id, Era);
        }

        [Event(62539, Message = "StreamId::TransactionId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op79,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteStream(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62539], id, StreamId);
        }

        [Event(62540, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op80,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteStreamFailure(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62540], id, StreamId);
        }

        [Event(62541, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op81,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteOnRestart(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62541], id, StreamId);
        }

        [Event(62542, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op82,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteOutboundStreamFromConsolidatedStore(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62542], id, StreamId);
        }

        [Event(62543, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op83,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteInboundStreamFromConsolidatedStore(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62543], id, StreamId);
        }

        [Event(62544, Message = "StreamId: {1} Response: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op84,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteDeleteStreamProtocol(string id, string StreamId, string Response)
        {
            WriteEvent(this.eventDescriptors[62544], id, StreamId, Response);
        }

        [Event(62545, Message = "StreamId: {1} Response: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op85,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteDeleteStream(string id, string StreamId, string Response)
        {
            WriteEvent(this.eventDescriptors[62545], id, StreamId, Response);
        }

        [Event(62546, Message = "StreamId: {1} Source: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op86,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteInboundStreamRequested(string id, string StreamId, string Source)
        {
            WriteEvent(this.eventDescriptors[62546], id, StreamId, Source);
        }

        [Event(62547, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op87,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteInboundStream(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62547], id, StreamId);
        }

        [Event(62548, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op88,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DeleteRequestSent(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62548], id, StreamId);
        }

        [Event(62549, Message = "StreamId: {1}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op89,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OutboundStreamDeleted(string id, string StreamId)
        {
            WriteEvent(this.eventDescriptors[62549], id, StreamId);
        }

        [Event(62550, Message = "Era: {1} Stream: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op90,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetOutboundStream(string id, string Era, string Stream)
        {
            WriteEvent(this.eventDescriptors[62550], id, Era, Stream);
        }

        [Event(62551, Message = "Era: {1} Stream: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op91,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetInboundStream(string id, string Era, string Stream)
        {
            WriteEvent(this.eventDescriptors[62551], id, Era, Stream);
        }

        [Event(62552, Message = "StreamId: {1} DeleteSequenceNumber: {2}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op92, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetDeleteSequenceNumber(string id, string StreamId, long DeleteSequenceNumber)
        {
            WriteEvent(this.eventDescriptors[62552], id, StreamId, DeleteSequenceNumber);
        }

        [Event(62553, Message = "Era: {1} Stream: {2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op93,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DoesOutboundStreamExistsInConsolidatedStore(string id, string Era, string Stream)
        {
            WriteEvent(this.eventDescriptors[62553], id, Era, Stream);
        }

        /***********************
        Generated from file = %SRCROOT%\prod\src\managed\Microsoft.ServiceFabric.Data.Impl\ReplicatedStore\DifferentialStore\StructuredEvents.cs
        For all Task = Tasks.TStore 240 << 8 (61440) <= eventId < 241 << 8 (61696).
        /***********************/

        [Event(61688, Message = "Loading values as part of recovery. Parallelism: {1}. Starting: {2} Duration: {3} ms",
            Task = Tasks.TStore, Opcode = Opcodes.Op126, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PreloadValues(string id, int numberOfParallelTasks, bool isStarting, long durationInMs)
        {
            WriteEvent(this.eventDescriptors[61688], id, numberOfParallelTasks, isStarting, durationInMs);
        }

        [Event(61689, Message = "Rebuild notification: Starting: {1} Duration: {2} ms",
            Task = Tasks.TStore, Opcode = Opcodes.Op127, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RebuildNotification(string id, bool isStarting, long durationInMs)
        {
            WriteEvent(this.eventDescriptors[61689], id, isStarting, durationInMs);
        }

        [Event(61690, Message = "Txn: {1} IsolationLevel: {2} ReadMode: {3} VisibilitySequenceNumber: {4}", Task = Tasks.TStore, Opcode = Opcodes.Op134, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CreateComponentEnumerableAsync(string id, long txnId, byte isolationLevel, int readMode, long visibilitySequenceNumber)
        {
            WriteEvent(this.eventDescriptors[61690], id, txnId, isolationLevel, readMode, visibilitySequenceNumber);
        }

        [Event(61691, Message = "Count: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op201, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PrepareCheckpointAsyncCompleted(string id, long count)
        {
            WriteEvent(this.eventDescriptors[61691], id, count);
        }

        [Event(61692, Message = "Last FileId: {1} Duration: {2} ms ItemCount: {3} DiskSize: {4}", Task = Tasks.TStore, Opcode = Opcodes.Op202, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PerformCheckpointAsyncCompleted(string id, uint fileId, double durationInMs, long itemCount, long diskSize)
        {
            WriteEvent(this.eventDescriptors[61692], id, fileId, durationInMs, itemCount, diskSize);
        }

        [Event(61693, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op203, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointAsync(string id, string message)
        {
            WriteEvent(this.eventDescriptors[61693], id, message);
        }

        [Event(61694, Message = "Too slow. Total: {1} ms Replace: {2} ms Swap: {3} ms ComputeToBeDeleted: {4} ms Delete: {5} ms", Task = Tasks.TStore, Opcode = Opcodes.Op135, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointAsyncWarning(string id, long totalInMs, long replaceTimeInMs, long swapTimeInMs, long computeTimeInMs, long deleteTimeInMs)
        {
            WriteEvent(this.eventDescriptors[61694], id, totalInMs, replaceTimeInMs, swapTimeInMs, swapTimeInMs, computeTimeInMs, deleteTimeInMs);
        }

        [Event(61695, Message = "Total: {1} ms Replace: {2} ms Swap: {3} ms ComputeToBeDeleted: {4} ms Delete: {5} ms", Task = Tasks.TStore, Opcode = Opcodes.Op136, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointAsyncCompleted(string id, long totalInMs, long replaceTimeInMs, long swapTimeInMs, long computeTimeInMs, long deleteTimeInMs)
        {
            WriteEvent(this.eventDescriptors[61695], id, totalInMs, replaceTimeInMs, swapTimeInMs, swapTimeInMs, computeTimeInMs, deleteTimeInMs);
        }

        [Event(61444, Message = "keyType={1} valueType={2} behavior={3} allowSecondaryReads={4}", Task = Tasks.TStore,
            Opcode = Opcodes.Op208, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Constructor(string id, string keyType, string valueType, string behavior, bool allowSecondaryReads)
        {
            WriteEvent(this.eventDescriptors[61444], id, keyType, valueType, behavior, allowSecondaryReads);
        }

        [Event(61445, Message = "key={1} cannot be added in txn={2}", Task = Tasks.TStore, Opcode = Opcodes.Op11,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_AddAsync(string id, ulong key, long transaction)
        {
            WriteEvent(this.eventDescriptors[61445], id, key, transaction);
        }

        [Event(61446, Message = "Txn: {1} Key: {2} Value={3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op12, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptAddAsync(string id, long transaction, ulong key, int value)
        {
            WriteEvent(this.eventDescriptors[61446], id, transaction, key, value);
        }

        [Event(61447, Message = "Txn: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op13,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptClearAsync(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61447], id, transaction);
        }

        [Event(61448, Message = "key={1} cannot be updated in txn={2} reason={3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op14, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void Error_ConditionalUpdateAsync(string id, ulong key, long transaction, string reason)
        {
            WriteEvent(this.eventDescriptors[61448], id, key, transaction, reason);
        }

        [Event(61451, Message = "acquire lock={1} key={2} txn={3} timeout={4}", Task = Tasks.TStore,
            Opcode = Opcodes.Op15, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void GetOrAddAsync(string id, byte lock_value, ulong key, long txn, int timeout)
        {
            WriteEvent(this.eventDescriptors[61451], id, lock_value, key, txn, timeout);
        }

        [Event(61454, Message = "Txn: {1} Key: {2} Value: {3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op16, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptGetOrAddAsync(string id, long transaction, ulong key,
            int value)
        {
            WriteEvent(this.eventDescriptors[61454], id, transaction, key, value);
        }

        [Event(61455, Message = "key={1} cannot be updated in txn={2} reason={3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op17, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void Error_UpdateWithOutputAsync(string id, ulong key, long transaction, string reason)
        {
            WriteEvent(this.eventDescriptors[61455], id, key, transaction, reason);
        }

        [Event(61459, Message = "LSN: {1} CommitLSN: {2} Txn: {3} Role: {4} Kind: {5}", Task = Tasks.TStore, Opcode = Opcodes.Op18,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyAsync(string id, long LSN, long commitLSN, long txn, long role, long context)
        {
            WriteEvent(this.eventDescriptors[61459], id, LSN, commitLSN, txn, role, context);
        }

        [Event(61460, Message = "starting Txn: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op19,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Unlock(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61460], id, transaction);
        }

        [Event(61461, Message = "committing Txn: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op20,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Commit_Unlock(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61461], id, transaction);
        }

        [Event(61464, Message = "committed Txn: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op21,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Done_Unlock(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61464], id, transaction);
        }

        [Event(61465, Message = "aborted Txn: {1} Count: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op22,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Abort_Unlock(string id, long transaction, long primaryOperationCount)
        {
            WriteEvent(this.eventDescriptors[61465], id, transaction, primaryOperationCount);
        }

        [Event(61466, Message = "LSN: {1} Txn: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op23, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnPrimaryApplyAsync(string id, long sequenceNumber, long transaction)
        {
            WriteEvent(this.eventDescriptors[61466], id, sequenceNumber, transaction);
        }

        [Event(61467, Message = "{1} LSN: {2} Txn: {3}", Task = Tasks.TStore, Opcode = Opcodes.Op24,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnSecondaryUndoFalseProgressAsync(string id, string transactionFound, long sequenceNumber,
            long transaction)
        {
            WriteEvent(this.eventDescriptors[61467], id, transactionFound, sequenceNumber, transaction);
        }

        [Event(61468, Message = "removing version chain lsn={1} txn={2} key={3} storeModificationType={4}",
            Task = Tasks.TStore, Opcode = Opcodes.Op25, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnUndoFalseProgressAsync(string id, long sequenceNumber, long transaction, ulong key,
            int storeModificationType)
        {
            WriteEvent(this.eventDescriptors[61468], id, sequenceNumber, transaction, key, storeModificationType);
        }

        [Event(61470, Message = "{1} LSN: {2} Txn: {3}", Task = Tasks.TStore, Opcode = Opcodes.Op26,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnSecondaryApplyAsync(string id, string transactionFound, long sequenceNumber, long transaction)
        {
            WriteEvent(this.eventDescriptors[61470], id, transactionFound, sequenceNumber, transaction);
        }

        [Event(61471, Message = "LSN: {1} Txn: {2} Key: {3} Value: {4} Count: {5} Type: {6}", Task = Tasks.TStore, Opcode = Opcodes.Op27,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyAddAsync(string id, long sequenceNumber, long transaction, ulong key, int value, long count, string applyType)
        {
            WriteEvent(this.eventDescriptors[61471], id, sequenceNumber, transaction, key, value, count, applyType);
        }

        [Event(61472, Message = "LSN: {1} Txn: {2} Key: {3} Value: {4} Type: {5}", Task = Tasks.TStore, Opcode = Opcodes.Op28,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyUpdateAsync(string id, long sequenceNumber, long transaction, ulong key, int value, string applyType)
        {
            WriteEvent(this.eventDescriptors[61472], id, sequenceNumber, transaction, key, value, applyType);
        }

        [Event(61473, Message = "LSN: {1} Txn: {2} Key: {3} Count: {4} Type: {5}", Task = Tasks.TStore, Opcode = Opcodes.Op29,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyRemoveAsync(string id, long sequenceNumber, long transaction, ulong key, long count, string applyType)
        {
            WriteEvent(this.eventDescriptors[61473], id, sequenceNumber, transaction, key, count, applyType);
        }

        [Event(61474, Message = "LSN: {1} Txn: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op30,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyClearAsync(string id, long sequenceNumber, long transaction)
        {
            WriteEvent(this.eventDescriptors[61474], id, sequenceNumber, transaction);
        }

        [Event(61475, Message = "{1} lsn={2} txn={3}", Task = Tasks.TStore, Opcode = Opcodes.Op31,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnRecoveryApplyAsync(string id, string transactionFound, long sequenceNumber, long transaction)
        {
            WriteEvent(this.eventDescriptors[61475], id, transactionFound, sequenceNumber, transaction);
        }

        [Event(61476, Message = "txn={1}", Task = Tasks.TStore, Opcode = Opcodes.Op32, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FixupDanglingStoreTransactionIfNeeded(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61476], id, transaction);
        }

        [Event(61480, Message = "key={1} cannot be removed in txn={2} reason={3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op33, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void Error_ConditionalRemoveAsync(string id, ulong key, long transaction, string reason)
        {
            WriteEvent(this.eventDescriptors[61480], id, key, transaction, reason);
        }

        [Event(61481, Message = "Txn: {1} Key: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op235,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptConditionalRemoveAsync(string id, long transaction, ulong key)
        {
            WriteEvent(this.eventDescriptors[61481], id, transaction, key);
        }

        [Event(61485, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op35, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61485], id, phase);
        }

        [Event(61486, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op36, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Error_OpenAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61486], id, phase);
        }

        [Event(61487, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op37, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61487], id, phase);
        }

        [Event(61488, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op38, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Abort_TStore(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61488], id, phase);
        }

        [Event(61489, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op234, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnCleanup(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61489], id, phase);
        }

        [Event(61490, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op40, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CheckpointAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61490], id, phase);
        }

        [Event(61576, Message = "TotalCount={1}, ActiveCount={2}, DeletedKeyCount={3}, FileId={4}", Task = Tasks.TStore, Opcode = Opcodes.Op205, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
        Keywords = Keywords.Default)]
        public void CheckpointDataAsync(string id, long totalCount, long activeCount, long deletedKeyCount, long fileId)
        {
            WriteEvent(this.eventDescriptors[61576], id, totalCount, activeCount, deletedKeyCount, fileId);
        }

        [Event(61491, Message = "Phase: {1} Count: {2} LSN: {3}", Task = Tasks.TStore, Opcode = Opcodes.Op41, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverCheckpointAsync(string id, string phase, long count, long checkpointLSN)
        {
            WriteEvent(this.eventDescriptors[61491], id, phase, count, checkpointLSN);
        }

        [Event(61492, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op42, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnRecoveryCompletedAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61492], id, phase);
        }

        [Event(61493, Message = "{1} stateproviderid={2}", Task = Tasks.TStore, Opcode = Opcodes.Op43,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveStateAsync(string id, string phase, long stateproviderid)
        {
            WriteEvent(this.eventDescriptors[61493], id, phase, stateproviderid);
        }

        [Event(61494, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op44, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnDataLossAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61494], id, phase);
        }

        [Event(61495, Message = "{1} stream={2}", Task = Tasks.TStore, Opcode = Opcodes.Op45,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetCurrentState_TStore(string id, string phase, int copyStream)
        {
            WriteEvent(this.eventDescriptors[61495], id, phase, copyStream);
        }

        [Event(61496, Message = "{1} stream={2} bytes={3}", Task = Tasks.TStore, Opcode = Opcodes.Op46,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Bytes_GetCurrentState(string id, string phase, int copyStream, long byteCount)
        {
            WriteEvent(this.eventDescriptors[61496], id, phase, copyStream, byteCount);
        }

        [Event(61497, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op47, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BeginSettingCurrentState(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61497], id, phase);
        }

        [Event(61498, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op48, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EndSettingCurrentState(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61498], id, phase);
        }

        [Event(61590, Message = "Copy aborted", Task = Tasks.TStore, Opcode = Opcodes.Op140, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EndSettingCurrentStateCopyAborted(string id)
        {
            WriteEvent(this.eventDescriptors[61590], id);
        }

        [Event(61499, Message = "{1} bytes={2}", Task = Tasks.TStore, Opcode = Opcodes.Op49,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Bytes_SetCurrentState(string id, string phase, int byteCount)
        {
            WriteEvent(this.eventDescriptors[61499], id, phase, byteCount);
        }

        [Event(61500, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op50, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61500], id, phase);
        }

        [Event(61501, Message = "consolidating vsn={1} dlsn={2} lsn={3}", Task = Tasks.TStore, Opcode = Opcodes.Op51,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Info_CopyAsync(string id, long visibility, long dataLossSequenceNumber,
            long replicationSequenceNumber)
        {
            WriteEvent(this.eventDescriptors[61501], id, visibility, dataLossSequenceNumber, replicationSequenceNumber);
        }

        [Event(61502, Message = "Phase: {1} Count: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op52, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverCopyStateAsync(string id, string phase, long count)
        {
            WriteEvent(this.eventDescriptors[61502], id, phase, count);
        }

        [Event(61503, Message = "{1} txn={2} is still open", Task = Tasks.TStore, Opcode = Opcodes.Op53,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StillOpen_CheckpointAsync(string id, string kind, long transaction)
        {
            WriteEvent(this.eventDescriptors[61503], id, kind, transaction);
        }

        [Event(61504, Message = "consolidating vsn={1} lsn={2}", Task = Tasks.TStore, Opcode = Opcodes.Op54,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Info_CheckpointAsync(string id, long visibility, long replicationSequenceNumber)
        {
            WriteEvent(this.eventDescriptors[61504], id, visibility, replicationSequenceNumber);
        }

        [Event(61505, Message = "{1} txn={2} status={3} role={4}", Task = Tasks.TStore, Opcode = Opcodes.Op55,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ThrowIfNot(string id, string kind, long tracer, string status, string role)
        {
            WriteEvent(this.eventDescriptors[61505], id, kind, tracer, status, role);
        }

        [Event(61506, Message = "starting kind={1} at {2}", Task = Tasks.TStore, Opcode = Opcodes.Op56,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DehydrateAsync(string id, string kind, long replicationSequenceNumber)
        {
            WriteEvent(this.eventDescriptors[61506], id, kind, replicationSequenceNumber);
        }

        [Event(61508, Message = "reason={1} msg={2}", Task = Tasks.TStore, Opcode = Opcodes.Op57,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_DehydrateAsync(string id, string reason, string msg)
        {
            WriteEvent(this.eventDescriptors[61508], id, reason, msg);
        }

        [Event(61509, Message = "starting kind={1} from ({2},{3},{4})", Task = Tasks.TStore, Opcode = Opcodes.Op58,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void HydrateAsync(string id, string kind, Guid partionId, long replicaId, long replicationSequenceNumber)
        {
            WriteEvent(this.eventDescriptors[61509], id, kind, partionId, replicaId, replicationSequenceNumber);
        }

        [Event(61511, Message = "Reason: {1} Msg: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op59,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Error_HydrateAsync(string id, string reason, string msg)
        {
            WriteEvent(this.eventDescriptors[61511], id, reason, msg);
        }

        [Event(61512, Message = "Txn: {1} Msg: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op60, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_ReplicateOperationAsync(string id, long transaction, string msg)
        {
            WriteEvent(this.eventDescriptors[61512], id, transaction, msg);
        }

        [Event(61513, Message = "Txn: {1} Backoff: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op61,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Retry_ReplicateOperationAsync(string id, long transaction, int backoff)
        {
            WriteEvent(this.eventDescriptors[61513], id, transaction, backoff);
        }

        [Event(61514, Message = "Txn: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op62, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FireNotifications(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61514], id, transaction);
        }

        [Event(61515, Message = "msg={1}", Task = Tasks.TStore, Opcode = Opcodes.Op63, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_FireNotifications(string id, string msg)
        {
            WriteEvent(this.eventDescriptors[61515], id, msg);
        }

        [Event(61516, Message = "kind={1}", Task = Tasks.TStore, Opcode = Opcodes.Op64, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FireEarlyNotifications(string id, bool kind)
        {
            WriteEvent(this.eventDescriptors[61516], id, kind);
        }

        [Event(61517, Message = "msg={1}", Task = Tasks.TStore, Opcode = Opcodes.Op65, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_FireEarlyNotifications(string id, string msg)
        {
            WriteEvent(this.eventDescriptors[61517], id, msg);
        }

        [Event(61518, Message = "writing file={1} msg={2}", Task = Tasks.TStore, Opcode = Opcodes.Op66,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Error_CheckpointAsync(string id, string file, string msg)
        {
            WriteEvent(this.eventDescriptors[61518], id, file, msg);
        }

        [Event(61519, Message = "Txn: {1} Reason: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op67,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_OnUndoFalseProgressAsync(string id, long transaction, string reason)
        {
            WriteEvent(this.eventDescriptors[61519], id, transaction, reason);
        }

        [Event(61520, Message = "Txn: {1} Reason: {2} Type: {3}", Task = Tasks.TStore, Opcode = Opcodes.Op68,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_OnApplyAddAsync(string id, long transaction, string reason, string applyType)
        {
            WriteEvent(this.eventDescriptors[61520], id, transaction, reason, applyType);
        }

        [Event(61521, Message = "Txn: {1} Reason: {2} Type: {3}", Task = Tasks.TStore, Opcode = Opcodes.Op69,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_OnApplyRemoveAsync(string id, long transaction, string reason, string applyType)
        {
            WriteEvent(this.eventDescriptors[61521], id, transaction, reason, applyType);
        }

        [Event(61522, Message = "Txn: {1} Reason: {2} Type: {3}", Task = Tasks.TStore, Opcode = Opcodes.Op70,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Error_OnApplyUpdateAsync(string id, long transaction, string reason, string applyType)
        {
            WriteEvent(this.eventDescriptors[61522], id, transaction, reason, applyType);
        }

        [Event(61523, Message = "txn={1}", Task = Tasks.TStore, Opcode = Opcodes.Op71, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ClearLocks(string id, long transaction)
        {
            WriteEvent(this.eventDescriptors[61523], id, transaction);
        }

        [Event(61524, Message = "acquire lock={1} txn={2} timeout={3}", Task = Tasks.TStore, Opcode = Opcodes.Op72,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PrimeLocksAsync(string id, byte lock_value, long txn, int timeout)
        {
            WriteEvent(this.eventDescriptors[61524], id, lock_value, txn, timeout);
        }

        [Event(61525, Message = "{1} role={2}", Task = Tasks.TStore, Opcode = Opcodes.Op73,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChangeRoleAsync(string id, string phase, string role)
        {
            WriteEvent(this.eventDescriptors[61525], id, phase, role);
        }

        [Event(61526, Message = "system timer resolution {1}", Task = Tasks.TStore, Opcode = Opcodes.Op74,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ConditionalGetAsyncError(string id, double checkSystemTimerResolution)
        {
            WriteEvent(this.eventDescriptors[61526], id, checkSystemTimerResolution);
        }

        [Event(61530, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op75, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveAllAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61530], id, phase);
        }

        [Event(61531, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op76, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TrimAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61531], id, phase);
        }

        [Event(61532, Message = "trimmed version for key={1} vsn={2} versions={3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op77, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void TrimVersion(string id, string key, long vsn, string versions)
        {
            WriteEvent(this.eventDescriptors[61532], id, key, vsn, versions);
        }

        [Event(61533, Message = "waiting for count={1} of in-flight unlock calls", Task = Tasks.TStore,
            Opcode = Opcodes.Op78, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WaitUnlock(string id, long count)
        {
            WriteEvent(this.eventDescriptors[61533], id, count);
        }

        [Event(61534, Message = "Folder: {1} {2}", Task = Tasks.TStore, Opcode = Opcodes.Op79,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupCheckpointAsync(string id, string folder, string message)
        {
            WriteEvent(this.eventDescriptors[61534], id, folder, message);
        }

        [Event(61535, Message = "Folder: {1} {2}", Task = Tasks.TStore, Opcode = Opcodes.Op80,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestoreCheckpointAsync(string id, string folder, string message)
        {
            WriteEvent(this.eventDescriptors[61535], id, folder, message);
        }

        [Event(61536, Message = "reason={1} msg={2}", Task = Tasks.TStore, Opcode = Opcodes.Op81,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Error_CopyAsync(string id, string reason, string msg)
        {
            WriteEvent(this.eventDescriptors[61536], id, reason, msg);
        }

        [Event(61537, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op82, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Complete_CopyAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61537], id, phase);
        }

        [Event(61538, Message = "size={1}", Task = Tasks.TStore, Opcode = Opcodes.Op83, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Size_CopyAsync(string id, int size)
        {
            WriteEvent(this.eventDescriptors[61538], id, size);
        }

        [Event(61539, Message = "filename={1} state={2}", Task = Tasks.TStore, Opcode = Opcodes.Op84,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OpenKeyCheckpointFile(string id, string filename, string state)
        {
            WriteEvent(this.eventDescriptors[61539], id, filename, state);
        }

        [Event(61540, Message = "directory={1} bytes={2}", Task = Tasks.TStore, Opcode = Opcodes.Op85,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessVersionCopyOperationData(string id, string directory, int bytes)
        {
            WriteEvent(this.eventDescriptors[61540], id, directory, bytes);
        }

        [Event(61541, Message = "Unknown copy protocol version={1}", Task = Tasks.TStore, Opcode = Opcodes.Op86,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessVersionCopyOperationMsg(string id, int version)
        {
            WriteEvent(this.eventDescriptors[61541], id, version);
        }

        [Event(61542, Message = "directory={1} version={2}", Task = Tasks.TStore, Opcode = Opcodes.Op87,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessVersionCopyOperationProtocol(string id, string directory, int version)
        {
            WriteEvent(this.eventDescriptors[61542], id, directory, version);
        }

        [Event(61543, Message = "directory={1} bytes={2}", Task = Tasks.TStore, Opcode = Opcodes.Op88,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessMetadataTableCopyOperation(string id, string directory, int bytes)
        {
            WriteEvent(this.eventDescriptors[61543], id, directory, bytes);
        }

        [Event(61544, Message = "directory={1} filename={2} bytes={3}", Task = Tasks.TStore, Opcode = Opcodes.Op89,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessStartKeyFileCopyOperation(string id, string directory, string filename, int bytes)
        {
            WriteEvent(this.eventDescriptors[61544], id, directory, filename, bytes);
        }

        [Event(61545, Message = "directory={1} filename={2} bytes={3}", Task = Tasks.TStore, Opcode = Opcodes.Op90,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessWriteKeyFileCopyOperation(string id, string directory, string filename, int bytes)
        {
            WriteEvent(this.eventDescriptors[61545], id, directory, filename, bytes);
        }

        [Event(61546, Message = "directory={1} filename={2} filesize={3}", Task = Tasks.TStore, Opcode = Opcodes.Op91,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessEndKeyFileCopyOperation(string id, string directory, string filename, long size)
        {
            WriteEvent(this.eventDescriptors[61546], id, directory, filename, size);
        }

        [Event(61547, Message = "directory={1} filename={2} bytes={3}", Task = Tasks.TStore, Opcode = Opcodes.Op92,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessStartValueFileCopyOperation(string id, string directory, string filename, int bytes)
        {
            WriteEvent(this.eventDescriptors[61547], id, directory, filename, bytes);
        }

        [Event(61548, Message = "directory={1} filename={2} bytes={3}", Task = Tasks.TStore, Opcode = Opcodes.Op93,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessWriteValueFileCopyOperation(string id, string directory, string filename, int bytes)
        {
            WriteEvent(this.eventDescriptors[61548], id, directory, filename, bytes);
        }

        [Event(61549, Message = "directory={1} diskwrite={2}bytes/sec", Task = Tasks.TStore, Opcode = Opcodes.Op94,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessCompleteCopyOperation(string id, string directory, long avgDiskWriteBytesPerSec)
        {
            WriteEvent(this.eventDescriptors[61549], id, directory, avgDiskWriteBytesPerSec);
        }

        [Event(61550, Message = "directory={1} filename={2} filesize={3}", Task = Tasks.TStore, Opcode = Opcodes.Op95,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessEndValueFileCopyOperation(string id, string directory, string filename, long size)
        {
            WriteEvent(this.eventDescriptors[61550], id, directory, filename, size);
        }

        [Event(61551, Message = "Assert={1} at Time={2}", Task = Tasks.TStore, Opcode = Opcodes.Op96,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void DiagnosticsError(string id, string message, DateTime TimeUTC)
        {
            WriteEvent(this.eventDescriptors[61551], id, message, TimeUTC);
        }

        [Event(61552, Message = "Errormsg={1} Assert={2} at Time={3}", Task = Tasks.TStore, Opcode = Opcodes.Op97,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void DiagnosticsMsg(string id, string message, string stackTrace, DateTime TimeUTC)
        {
            WriteEvent(this.eventDescriptors[61552], id, message, stackTrace, TimeUTC);
        }

        [Event(61553, Message = "starting consolidating={1}", Task = Tasks.TStore, Opcode = Opcodes.Op98,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StartingConsolidating(string id, int consolidating)
        {
            WriteEvent(this.eventDescriptors[61553], id, consolidating);
        }

        [Event(61554, Message = "completed consolidation oldcount={1}, deltacount={2}, newcount={3}", Task = Tasks.TStore, Opcode = Opcodes.Op99,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompletedConsolidating(string id, long oldcount, long deltacount, long newcount)
        {
            WriteEvent(this.eventDescriptors[61554], id, oldcount, deltacount, newcount);
        }

        [Event(61555, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op125,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MergeAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61555], id, phase);
        }

        [Event(61556, Message = "unexpected exception ex={1} exType={2}", Task = Tasks.TStore, Opcode = Opcodes.Op101,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SafeFileReplace(string id, string ex, string exType)
        {
            WriteEvent(this.eventDescriptors[61556], id, ex, exType);
        }

        [Event(61557, Message = "status={1}. filename={2}", Task = Tasks.TStore, Opcode = Opcodes.Op102,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ValueCheckpointFileAsyncOpen(string id, string status, string filename)
        {
            WriteEvent(this.eventDescriptors[61557], id, status, filename);
        }

        [Event(61558, Message = "message={1} exception={2}", Task = Tasks.TStore, Opcode = Opcodes.Op103,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessExceptionWarning(string id, string message, string exception)
        {
            WriteEvent(this.eventDescriptors[61558], id, message, exception);
        }

        [Event(61559, Message = "message={1} exception={2}", Task = Tasks.TStore, Opcode = Opcodes.Op104,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ProcessExceptionError(string id, string message, string exception)
        {
            WriteEvent(this.eventDescriptors[61559], id, message, exception);
        }

        [Event(61560, Message = "txn={1} msg={2}", Task = Tasks.TStore, Opcode = Opcodes.Op105,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Warning_ReplicateOperationAsync(string id, long transaction, string msg)
        {
            WriteEvent(this.eventDescriptors[61560], id, transaction, msg);
        }

        [Event(61561, Message = "Deleting key file: {1} and value file: {2}", Task = Tasks.TStore, Opcode = Opcodes.Op106,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FileMetadataDeleteKeyValue(string id, string KeyFile, string ValueFile)
        {
            WriteEvent(this.eventDescriptors[61561], id, KeyFile, ValueFile);
        }

        [Event(61562, Message = "start. directory:{1}", Task = Tasks.TStore, Opcode = Opcodes.Op107,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataTableTrimFilesStart(string id, string directory)
        {
            WriteEvent(this.eventDescriptors[61562], id, directory);
        }

        [Event(61563, Message = "found file to delete:{1}", Task = Tasks.TStore, Opcode = Opcodes.Op108,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataTableTrimFilesToDelete(string id, string file)
        {
            WriteEvent(this.eventDescriptors[61563], id, file);
        }

        [Event(61564, Message = "deleting file:{1}", Task = Tasks.TStore, Opcode = Opcodes.Op109,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataTableTrimFilesDeleting(string id, string TableFile)
        {
            WriteEvent(this.eventDescriptors[61564], id, TableFile);
        }

        [Event(61565, Message = "complete. directory:{1}", Task = Tasks.TStore, Opcode = Opcodes.Op110,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataTableTrimFilesComplete(string id, string directory)
        {
            WriteEvent(this.eventDescriptors[61565], id, directory);
        }

        [Event(61566, Message = "Merged filename={1}, FileId={2}, TotalCount={3}, ActiveCount={4}, DeletedKeyCount={5}, CheckpointFileWrite={6}bytes/sec", Task = Tasks.TStore, Opcode = Opcodes.Op100,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MergeFile(string id, string filename, long fileId, long totalCount, long activeCount, long deletedKeyCount, long fileWriteBytesPerSec)
        {
            WriteEvent(this.eventDescriptors[61566], id, filename, fileId, totalCount, activeCount, deletedKeyCount, fileWriteBytesPerSec);
        }

        [Event(61570, Message = "{1}: Filename={2}, FileId={3}, TotalCount={4}, DeletedKeyCount={5}, InvalidKeyCount={6}", Task = Tasks.TStore, Opcode = Opcodes.Op112,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CheckpointFileMetadata(string id, string phase, string filename, long fileId, long totalCount, long deletedKeyCount, long invalidKeyCount)
        {
            WriteEvent(this.eventDescriptors[61570], id, phase, filename, fileId, totalCount, deletedKeyCount, invalidKeyCount);
        }

        [Event(61571, Message = "MergePolicy={1}", Task = Tasks.TStore, Opcode = Opcodes.Op113,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MergePolicy(string id, string mergePolicy)
        {
            WriteEvent(this.eventDescriptors[61571], id, mergePolicy);
        }

        [Event(61572, Message = "CheckpointFileWrite={1}bytes/sec", Task = Tasks.TStore, Opcode = Opcodes.Op128,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CheckpointFileWriteBytesPerSec(string id, long fileWriteBytesPerSec)
        {
            WriteEvent(this.eventDescriptors[61572], id, fileWriteBytesPerSec);
        }

        [Event(61573, Message = "ItemCount={1} DiskSize={2}", Task = Tasks.TStore, Opcode = Opcodes.Op129,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StoreSize(string id, long itemCount, long diskSize)
        {
            WriteEvent(this.eventDescriptors[61573], id, itemCount, diskSize);
        }

        [Event(61482, Message = "{1} txn={2} key={3} isolation={4} lcsn={5}", Task = Tasks.TStore, Opcode = Opcodes.Op111,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ConditionalGetAsync(string id, string lockMode, long transaction, ulong key, byte isolation,
            long lcsn)
        {
            WriteEvent(this.eventDescriptors[61482], id, lockMode, transaction, key, isolation, lcsn);
        }

        [Event(61567, Message = "EnableSweep={1} SweepThreshold={2} EnableBackgroundConsolidation={3} EnableStrict2PL={4}", Task = Tasks.TStore,
            Opcode = Opcodes.Op238, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStoreEnableFlags(string id, bool EnableSweep, string SweepThreshold, bool EnableBackgroundConsolidation, bool EnableStrict2PL)
        {
            WriteEvent(this.eventDescriptors[61567], id, EnableSweep, SweepThreshold, EnableBackgroundConsolidation, EnableStrict2PL);
        }

        [Event(61568, Message = "Starting", Task = Tasks.TStore, Opcode = Opcodes.Op34,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SweepStarting(string id)
        {
            WriteEvent(this.eventDescriptors[61568], id);
        }

        [Event(61569, Message = "Completed", Task = Tasks.TStore, Opcode = Opcodes.Op39, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SweepCompleted(string id)
        {
            WriteEvent(this.eventDescriptors[61569], id);
        }

        [Event(61574, Message = "AppendToDeltaDifferentialState high delta count {1} and the default count is {2} indicating slow consolidation", Task = Tasks.TStore,
            Opcode = Opcodes.Op218, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AppendToDeltaDifferentialState_SlowConsolidation(string id, int Index, int DefaultNumberOfDeltasTobeConsolidated)
        {
            WriteEvent(this.eventDescriptors[61574], id, Index, DefaultNumberOfDeltasTobeConsolidated);
        }

        [Event(61575, Message = "Next metadata table has been reset to the current metadata table. Current metadata table file count={1}", Task = Tasks.TStore, Opcode = Opcodes.Op141,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointAsync_ResetNextMetadataTableDone(string id, long fileCount)
        {
            WriteEvent(this.eventDescriptors[61575], id, fileCount);
        }

        [Event(62480, Message = "StreamName: {1} StreamId: {2} State: {3} Target: {4}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op112, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverOutboundStream(string id, string StreamName, string StreamId, string State, string Target)
        {
            WriteEvent(this.eventDescriptors[62480], id, StreamName, StreamId, State, Target);
        }

        [Event(62479, Message = "StreamName: {1} StreamId: {2} State: {3} Source: {4}", Task = Tasks.ReliableStream,
            Opcode = Opcodes.Op113, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverInboundStream(string id, string StreamName, string StreamId, string State, string Source)
        {
            WriteEvent(this.eventDescriptors[62479], id, StreamName, StreamId, State, Source);
        }

        [Event(61479, Message = "acquire lock={1} store={2} txn={3} timeout={4} hints={5} isolation={6}",
            Task = Tasks.TStore, Opcode = Opcodes.Op114, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcquireStoreModificationLocksAsync(string id, byte lock_value, ulong store, long txn, int timeout,
            long hints, byte isolation)
        {
            WriteEvent(this.eventDescriptors[61479], id, lock_value, store, txn, timeout, hints, isolation);
        }

        [Event(61483, Message = "{1} txn={2} key={3} isolation={4} lcsn={5} reason={6}", Task = Tasks.TStore,
            Opcode = Opcodes.Op115, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void NotFound_ConditionalGetAsync(string id, string lockMode, long transaction, ulong key, byte isolation,
            long lcsn, string reason)
        {
            WriteEvent(this.eventDescriptors[61483], id, lockMode, transaction, key, isolation, lcsn, reason);
        }

        [Event(61484, Message = "{1} txn={2} key={3} isolation={4} lcsn={5} reason=keyfound", Task = Tasks.TStore,
            Opcode = Opcodes.Op116, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Found_ConditionalGetAsync(string id, string lockMode, long transaction, ulong key, byte isolation,
            long lcsn)
        {
            WriteEvent(this.eventDescriptors[61484], id, lockMode, transaction, key, isolation, lcsn);
        }

        [Event(61449, Message = "Key: {1} OldValue: {2} NewValue: {3} Txn: {4}", Task = Tasks.TStore,
            Opcode = Opcodes.Op117, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Skip_ConditionalUpdateAsync(string id, ulong key, int oldValue, int newValue, long transaction)
        {
            WriteEvent(this.eventDescriptors[61449], id, key, oldValue, newValue, transaction);
        }

        [Event(61450, Message = "Txn: {1} Key: {2} Value: {3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op118, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptConditionalUpdateAsync(string id, long transaction, ulong key, int value)
        {
            WriteEvent(this.eventDescriptors[61450], id, transaction, key, value);
        }

        [Event(61457, Message = "Txn: {1} Key: {2} Value: {3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op119, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptUpdateWithOutputAsync(string id, long transaction, ulong key, int value)
        {
            WriteEvent(this.eventDescriptors[61457], id, transaction, key, value);
        }

        [Event(61458, Message = "Type: {1} Txn: {2} Key: {3} Value: {4}", Task = Tasks.TStore,
            Opcode = Opcodes.Op120, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptAddOrUpdateAsync(string id, string operationType, long transaction, ulong key, int value)
        {
            WriteEvent(this.eventDescriptors[61458], id, operationType, transaction, key, value);
        }

        [Event(61452, Message = "acquire context={1} lock={2} key={3} txn={4} timeout={5} hints={6}", Task = Tasks.TStore,
            Opcode = Opcodes.Op121, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcquireKeyModificationLocksAsync(string id, string context, byte lock_value, ulong key, long txn,
            int timeout, long hints)
        {
            WriteEvent(this.eventDescriptors[61452], id, context, lock_value, key, txn, timeout, hints);
        }

        [Event(61453, Message = "acquire no lock context={1} key={2} txn={3}", Task = Tasks.TStore,
            Opcode = Opcodes.Op122, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void NoLock_AcquireKeyModificationLocksAsync(string id, string context, ulong key, long txn)
        {
            WriteEvent(this.eventDescriptors[61453], id, context, key, txn);
        }

        [Event(61478, Message = "acquire no lock context={1} key={2} txn={3} timeout={4} hints={5} isolation={6}",
            Task = Tasks.TStore, Opcode = Opcodes.Op123, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void NoLock_AcquireKeyReadLocksAsync(string id, string context, ulong key, long txn, int timeout,
            long hints, byte isolation)
        {
            WriteEvent(this.eventDescriptors[61478], id, context, key, txn, timeout, hints, isolation);
        }

        [Event(61477, Message = "acquire context={1} lock={2} key={3} txn={4} timeout={5} hints={6} isolation={7}",
            Task = Tasks.TStore, Opcode = Opcodes.Op124, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcquireKeyReadLocksAsync(string id, string context, byte lock_value, ulong key, long txn,
            int timeout, long hints, byte isolation)
        {
            WriteEvent(this.eventDescriptors[61477], id, context, lock_value, key, txn, timeout, hints, isolation);
        }

        [Event(61507, Message = "{1} Txn: {2} Status: {3} Role: {4} IsConsistent: {5}",
            Task = Tasks.TStore, Opcode = Opcodes.Op204, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ThrowIfNotReadable(string id, string kind, long txnId, string status, string role, bool isConsistent)
        {
            WriteEvent(this.eventDescriptors[61507], id, kind, txnId, status, role, isConsistent);
        }

        [Event(61577, Message = "File count: {1}", Task = Tasks.TStore, Opcode = Opcodes.Op137,
        Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverConsolidatedAsync(string id, int fileCount)
        {
            WriteEvent(this.eventDescriptors[61577], id, fileCount);
        }

        [Event(61578, Message = "RecoveryStoreComponet merge keyfiles. phase={1}", Task = Tasks.TStore, Opcode = Opcodes.Op138,
        Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MergeKeyCheckpointFilesAsync(string id, string phase)
        {
            WriteEvent(this.eventDescriptors[61578], id, phase);
        }

        [Event(61579, Message = "{1}", Task = Tasks.TStore, Opcode = Opcodes.Op139, Level = EventLevel.Error,
Keywords = Keywords.DataMessaging)]
        public void StoreDiagnosticError(string id, string errMsg)
        {
            WriteEvent(this.eventDescriptors[61579], id, errMsg);
        }

        [Event(61583,
            Message = "ReplicaId: {1} StoreCount: {2}",
            Task = Tasks.TStore, Opcode = Opcodes.Op144, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
# endif
            Keywords = Keywords.Default)]
        public void StoreCountTelemetry(Guid id, long replicaId, long numberOfStores)
        {
            WriteEvent(this.eventDescriptors[61583], id, replicaId, numberOfStores);
        }

        [Event(61584,
    Message = "ReplicaId: {1} TotalItemCount: {2}",
    Task = Tasks.TStore, Opcode = Opcodes.Op145, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ItemCountTelemetry(Guid id, long replicaId, long totalItemCount)
        {
            WriteEvent(this.eventDescriptors[61584], id, replicaId, totalItemCount);
        }

        [Event(61585,
            Message = "ReplicaId: {1} TotalDiskSize: {2}",
            Task = Tasks.TStore, Opcode = Opcodes.Op146, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
# endif
            Keywords = Keywords.Default)]
        public void DiskSizeTelemetry(Guid id, long replicaId, long totalDiskSize)
        {
            WriteEvent(this.eventDescriptors[61585], id, replicaId, totalDiskSize);
        }

        [Event(61586,
            Message = "ReplicaId: {1} TotalKeySize: {2} NumKeys: {3} TotalValueSize: {4} NumValues: {5}",
            Task = Tasks.TStore, Opcode = Opcodes.Op147, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
# endif
            Keywords = Keywords.Default)]
        public void KeyValueSizeTelemetry(Guid id, long replicaId, long totalKeySize, long numberOfKeys, long totalValueSize, long numberOfValues)
        {
            WriteEvent(this.eventDescriptors[61586], id, replicaId, totalKeySize, numberOfKeys, totalValueSize, numberOfValues);
        }

        [Event(61587,
            Message = "ReplicaId: {1} CustomKeyTypePercentage: {2}",
            Task = Tasks.TStore, Opcode = Opcodes.Op148, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
# endif
            Keywords = Keywords.Default)]
        public void KeyTypeTelemetry(Guid id, long replicaId, double customKeyTypePercentage)
        {
            WriteEvent(this.eventDescriptors[61587], id, replicaId, customKeyTypePercentage);
        }

        [Event(61588,
            Message = "ReplicaId: {1} NumReads: {2} NumWrites: {3}",
            Task = Tasks.TStore, Opcode = Opcodes.Op149, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
# endif
            Keywords = Keywords.Default)]
        public void ReadWriteTelemetry(Guid id, long replicaId, long numberOfReads, long numberOfWrites)
        {
            WriteEvent(this.eventDescriptors[61588], id, replicaId, numberOfReads, numberOfWrites);
        }

        [Event(61589,
            Message = "ReplicaId: {1} TotalReadLatency (us): {2} NumReads: {3} LastReadLatency (us): {4}",
            Task = Tasks.TStore, Opcode = Opcodes.Op150, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
# endif
            Keywords = Keywords.Default)]
        public void ReadLatencyTelemetry(Guid id, long replicaId, long totalReadLatencyInUs, long numberOfReads, long lastReadLatencyInUs)
        {
            WriteEvent(this.eventDescriptors[61589], id, replicaId, totalReadLatencyInUs, numberOfReads, lastReadLatencyInUs);
        }

        [Event(61591, Message = "starting", Task = Tasks.TStore, Opcode  = Opcodes.Op151, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SweepOnReads(string id)
        {
            WriteEvent(this.eventDescriptors[61591], id);
        }

        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\Microsoft.ServiceFabric.Data.Impl\Replicator\StateManagerStructuredEvents.cs***/
        /***********************/

        [Event(64481, Message = "abort failed for stateprovider {1}, message {2}, stack {3}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op11, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void AbortFailed(string id, long stateProviderId, string message, string stacktrace)
        {
            WriteEvent(this.eventDescriptors[64481], id, stateProviderId, message, stacktrace);
        }

        [Event(64282, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op12,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Abort_TStateManager(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64282], id, message);
        }

        [Event(64288, Message = "state provider {1} is being deleted in this transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op13, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void AddOperation(string id, string stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64288], id, stateProvider, transactionId);
        }

        [Event(64417, Message = "state provider {1} added as transient for transaction {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op14, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AddSingleStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64417], id, stateProvider, transactionId);
        }

        [Event(64406, Message = "request to register state provider {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op15, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AddSingleStateProviderBegin(string id, string stateProviderName)
        {
            WriteEvent(this.eventDescriptors[64406], id, stateProviderName);
        }

        [Event(64416, Message = "state provider registered: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op16,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AddSingleStateProviderEnd(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64416], id, stateProvider);
        }

        [Event(64418, Message = "failed to add state provider {1} in transaction {2}: key already exists",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op17, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void AddSingleStateProviderKeyExists(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64418], id, stateProvider, transactionId);
        }

        [Event(64281, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op18,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiAbort(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64281], id, message);
        }

        [Event(64266, Message = "change role from {1} -> {2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op19,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiChangeRole(string id, string oldRole, string newRole)
        {
            WriteEvent(this.eventDescriptors[64266], id, oldRole, newRole);
        }

        [Event(64267, Message = "change role completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op20,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiChangeRoleEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64267], id);
        }

        [Event(64276, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op21,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiClose(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64276], id, message);
        }

        [Event(64262, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op22,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiOpen(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64262], id, message);
        }

        [Event(64323, Message = "Deletion of state provider {1} committed on primary with transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op23, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnPrimaryDeleteStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64323], id, stateProvider, transactionId);
        }

        [Event(64322, Message = "Creation of state provider {1} committed on primary with transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op24, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnPrimaryInsertStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64322], id, stateProvider, transactionId);
        }

        [Event(64355, Message = "Deletion of state provider {1} committed on replica in transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op25, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnRecoveryDeleteStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64355], id, stateProvider, transactionId);
        }

        [Event(64344, Message = "Creation of state provider {1} committed on replica in transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op26, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnRecoveryInsertStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64344], id, stateProvider, transactionId);
        }

        [Event(64345, Message = "StateProviderCount {1}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op135, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderCount(string id, int activeCounter)
        {
            WriteEvent(this.eventDescriptors[64345], id, activeCounter);
        }

        [Event(64339,
            Message =
                "Insertion of state provider {1} is ignored because it has been deleted from the locally recovered data",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op27, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnRecoveryInsertStateProviderSkipped(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64339], id, stateProvider);
        }

        [Event(64329, Message = "Deletion of state provider {1} committed on secondary with transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op28, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnSecondaryDeleteStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64329], id, stateProvider, transactionId);
        }

        [Event(64333, Message = "Undoing deletion of state provider {1} in transaction: {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op29, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnSecondaryFalseProgressUndoDeleteStateProvider(string id, long stateProvider,
            long transactionId)
        {
            WriteEvent(this.eventDescriptors[64333], id, stateProvider, transactionId);
        }

        [Event(64332, Message = "Undoing creation of state provider {1} in transaction: {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op30, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnSecondaryFalseProgressUndoInsertStateProvider(string id, long stateProvider,
            long transactionId)
        {
            WriteEvent(this.eventDescriptors[64332], id, stateProvider, transactionId);
        }

        [Event(64324, Message = "Creation of state provider {1} committed on secondary with transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op31, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnSecondaryInsertStateProvider(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64324], id, stateProvider, transactionId);
        }

        [Event(64328, Message = "Creation of state provider {1} skipped on secondary as it has been deleted",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op32, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnSecondaryInsertStateProviderSkipped(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64328], id, stateProvider);
        }

        [Event(64334, Message = "Soft deleted state provider {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op33,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyOnSecondarySoftDeleteStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64334], id, stateProvider);
        }

        [Event(64340, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op34,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiBackupBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64340], id);
        }

        [Event(64341, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op35,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiBackupEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64341], id);
        }

        [Event(64367, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op36,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BeginSetCurrentLocalState(string id)
        {
            WriteEvent(this.eventDescriptors[64367], id);
        }

        [Event(64390, Message = "State provider added: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op37,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BeginSetCurrentLocalStateAddStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64390], id, stateProvider);
        }

        [Event(64350, Message = "set state started", Task = Tasks.TStateManager, Opcode = Opcodes.Op38,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiBeginSettingCurrentState(string id)
        {
            WriteEvent(this.eventDescriptors[64350], id);
        }

        [Event(64289, Message = "state provider {1} is being deleted in this transaction: {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op39, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void BeginTransaction(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64289], id, stateProvider, transactionId);
        }

        [Event(64268, Message = "completed logging replicator change role", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op40, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChangeRole_TStateManager(string id)
        {
            WriteEvent(this.eventDescriptors[64268], id);
        }

        [Event(64429, Message = "changing replica role for state providers to {1} after lock acquisition",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op41, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChangeRoleOnStateProviders(string id, string replicaRole)
        {
            WriteEvent(this.eventDescriptors[64429], id, replicaRole);
        }

        [Event(64428, Message = "acquiring lock to change replica role for state providers to {1}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op42, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChangeRoleOnStateProvidersAcquireLock(string id, string replicaRole)
        {
            WriteEvent(this.eventDescriptors[64428], id, replicaRole);
        }

        [Event(64362, Message = "Earliest lsn before which state providers can be cleaned up is {1}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op43, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CleanupMetadataLsn(string id, long lsn)
        {
            WriteEvent(this.eventDescriptors[64362], id, lsn);
        }

        [Event(64389, Message = "calling remove state on state provider {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op44, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CleanupMetadataRemoveStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64389], id, stateProvider);
        }

        [Event(64427, Message = "calling close on state provider {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op45,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CloseStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64427], id, stateProvider);
        }

        [Event(64335, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op46,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiCompleteCheckpointBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64335], id);
        }

        [Event(64336, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op47,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiCompleteCheckpointEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64336], id);
        }

        [Event(64382, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op48,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointOnLocalStateBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64382], id);
        }

        [Event(64383, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op49,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointOnLocalStateEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64383], id);
        }

        [Event(64384, Message = "no checkpoint file rename needed", Task = Tasks.TStateManager, Opcode = Opcodes.Op50,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CompleteCheckpointOnLocalStateNoRenameNeeded(string id)
        {
            WriteEvent(this.eventDescriptors[64384], id);
        }

        [Event(64391, Message = "state provider {1} added from old list.", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op51, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyToLocalStateAddStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64391], id, stateProvider);
        }

        [Event(64392, Message = "state provider {1} added to active list.", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op52, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyToLocalStateCreateStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64392], id, stateProvider);
        }

        [Event(64393, Message = "state provider {1} added to delete list.", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op53, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyToLocalStateDeleteStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64393], id, stateProvider);
        }

        [Event(64351, Message = "set state completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op54,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiEndSettingCurrentState(string id)
        {
            WriteEvent(this.eventDescriptors[64351], id);
        }

        [Event(64260, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op55, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Exception_TStateManager(string id, string exception)
        {
            WriteEvent(this.eventDescriptors[64260], id, exception);
        }

        [Event(64261, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op56, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ExceptionWarning(string id, string exception)
        {
            WriteEvent(this.eventDescriptors[64261], id, exception);
        }

        [Event(64424, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op57,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetChildrenBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64424], id);
        }

        [Event(64426, Message = "duplicate children are not allowed: {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op58, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetChildrenDuplicateFound(string id, string childName)
        {
            WriteEvent(this.eventDescriptors[64426], id, childName);
        }

        [Event(64425, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op59,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetChildrenEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64425], id);
        }

        [Event(64366, Message = "State Provider Id: {1}, Metadata Mode: {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op60, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetCurrentState_TStateManager(string id, long stateProvider, int metadataMode)
        {
            WriteEvent(this.eventDescriptors[64366], id, stateProvider, metadataMode);
        }

        [Event(64346, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op61,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiGetCurrentStateBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64346], id);
        }

        [Event(64347, Message = "completed. Active state provider copy stream count:{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op62,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiGetCurrentStateEnd(string id, long countOfStream)
        {
            WriteEvent(this.eventDescriptors[64347], id, countOfStream);
        }

        [Event(64375, Message = "get current state returned null for state provider: {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op63, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetCurrentStateNull(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64375], id, stateProvider);
        }

        [Event(64462, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op64, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void MetadataSerializerGetMetadataError(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64462], id, message);
        }

        [Event(64423, Message = "state provider was found: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op66,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetOrAddStateProvider(string id, string stateProvider)
        {
            WriteEvent(this.eventDescriptors[64423], id, stateProvider);
        }

        [Event(64415, Message = "moving state provider {1} to current role {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op67, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InitializeStateProvider(string id, string stateProvider, string replicaRole)
        {
            WriteEvent(this.eventDescriptors[64415], id, stateProvider, replicaRole);
        }

        [Event(64422,
            Message = "acquiring lock to call change role on state provider {1}, state manager's current role is {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op68, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InitializeStateProvidersAcquireLock(string id, long stateProvider, int replicaRole)
        {
            WriteEvent(this.eventDescriptors[64422], id, stateProvider, replicaRole);
        }

        [Event(64374, Message = "file size: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op69,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateCheckpoint(string id, long fileSize)
        {
            WriteEvent(this.eventDescriptors[64374], id, fileSize);
        }

        [Event(64372, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op70,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateCheckpointBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64372], id);
        }

        [Event(64373, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op71,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateCheckpointEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64373], id);
        }

        [Event(64394, Message = "state provider {1} added to active state.", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op72, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateRecoverCheckpointAddStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64394], id, stateProvider);
        }

        [Event(64376, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op73,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateRecoverCheckpointBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64376], id);
        }

        [Event(64395, Message = "state provider {1} added to deleted state.", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op74, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateRecoverCheckpointDeleteStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64395], id, stateProvider);
        }

        [Event(64378, Message = "Checkpoint file is empty", Task = Tasks.TStateManager, Opcode = Opcodes.Op75,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateRecoverCheckpointEmptyFile(string id)
        {
            WriteEvent(this.eventDescriptors[64378], id);
        }

        [Event(64377, Message = "recovery completed. CheckpointLSN: {1}.", Task = Tasks.TStateManager, Opcode = Opcodes.Op76,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LocalStateRecoverCheckpointEnd(string id, long checkpointLSN)
        {
            WriteEvent(this.eventDescriptors[64377], id, checkpointLSN);
        }

        [Event(64480, Message = "key {1}, tx {2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op77,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LockContextReleaseReadLock(string id, string key, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64480], id, key, transactionId);
        }

        [Event(64479, Message = "key {1}, tx {2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op78,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LockContextReleaseWriteLock(string id, string key, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64479], id, key, transactionId);
        }

        [Event(64458, Message = "metadata count on deserialization is: {1} activeStateProviders: {2} deletedStateProviders: {3}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op79, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataSerializerDeserializeCount(string id, int metadataCount, int activeCount, int deleteCount)
        {
            WriteEvent(this.eventDescriptors[64458], id, metadataCount, activeCount, deleteCount);
        }

        [Event(64459, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op80, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void MetadataSerializerDeserializeError(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64459], id, message);
        }

        [Event(64472, Message = "lock acquistion on key {1}, tx {2} failed with exception {3} and stack {4}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op81, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataManagerAcquireReadLockException(string id, string key, long tx, string exceptionMessage,
            string stackTrace)
        {
            WriteEvent(this.eventDescriptors[64472], id, key, tx, exceptionMessage, stackTrace);
        }

        [Event(64471, Message = "lock acquistion on key {1}, tx {2} failed with exception {3} and stack {4}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op82, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataManagerAcquireWriteLockException(string id, string key, long tx, string exceptionMessage,
            string stackTrace)
        {
            WriteEvent(this.eventDescriptors[64471], id, key, tx, exceptionMessage, stackTrace);
        }

        [Event(64474, Message = "requested lock on key {1}, sp {2}, tx {3}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op83, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderMetadataManagerLockForRead(string id, string key, long stateProviderId,
            long transactionId)
        {
            WriteEvent(this.eventDescriptors[64474], id, key, stateProviderId, transactionId);
        }

        [Event(64473, Message = "requested lock on key {1}, sp {2}, tx {3}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op84, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderMetadataManagerLockForWrite(string id, string key, long stateProviderId,
            long transactionId)
        {
            WriteEvent(this.eventDescriptors[64473], id, key, stateProviderId, transactionId);
        }

        [Event(64478, Message = "key released {1} as a part of tx {2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op85,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderMetadataManagerReleaseLock(string id, string key, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64478], id, key, transactionId);
        }

        [Event(64477, Message = "key removed {1} as a part of tx: {2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op86,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderMetadataManagerRemoveLock(string id, string key, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64477], id, key, transactionId);
        }

        [Event(64475, Message = "lock released for tx: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op87,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderMetadataManagerUnlock(string id, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64475], id, transactionId);
        }

        [Event(64476, Message = "request to delete lock {1} for tx {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op88, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateProviderMetadataManagerUnlockDelete(string id, string key, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64476], id, key, transactionId);
        }

        [Event(64456, Message = "metadata count on serialization is: {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op89, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetadataSerializerSerializeCount(string id, int metadataCount)
        {
            WriteEvent(this.eventDescriptors[64456], id, metadataCount);
        }

        [Event(64468, Message = "copy completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op90,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void NamedOperationDataCollectionGetNext(string id)
        {
            WriteEvent(this.eventDescriptors[64468], id);
        }

        [Event(64470, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op91, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void NamedOperationDataDeserializeError(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64470], id, message);
        }

        [Event(64306, Message = "apply called for state provider {1} with transaction: {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op92, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void OnApply(string id, string stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64306], id, stateProvider, transactionId);
        }

        [Event(64309,
            Message =
                "insertion of state provider {1} is ignored because it has been deleted from the locally recovered data",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op93, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyInsertStateProviderSkipped(string id, long stateProviderId)
        {
            WriteEvent(this.eventDescriptors[64309], id, stateProviderId);
        }

        [Event(64308, Message = "deserialization error(parameter 'name' is null) for transaction: {1}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op94, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyNameError(string id, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64308], id, transactionId);
        }

        [Event(64310, Message = "unsupported version {1} on deserialization, current version is {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op95, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void OnApplyVersionError(string id, int deserializedVersion, int currentVersion)
        {
            WriteEvent(this.eventDescriptors[64310], id, deserializedVersion, currentVersion);
        }

        [Event(64284, Message = "data loss status: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op96,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void OnDataLoss(string id, bool shouldRestore)
        {
            WriteEvent(this.eventDescriptors[64284], id, shouldRestore);
        }

        [Event(64337, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op97,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiOnRecoveryCompletedBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64337], id);
        }

        [Event(64338, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op98,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiOnRecoveryCompletedEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64338], id);
        }

        [Event(64263, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op99,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Open(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64263], id, message);
        }

        [Event(64388, Message = "Metadata for state provider {1} added to copyOrCheckpointSnapshot",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op100, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PerformCheckpointAddStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64388], id, stateProvider);
        }

        [Event(64330, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op101,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiPerformCheckpointBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64330], id);
        }

        [Event(64331, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op102,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiPerformCheckpointEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64331], id);
        }

        [Event(64326, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op103,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiPrepareCheckpointBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64326], id);
        }

        [Event(64327, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op104,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiPrepareCheckpointEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64327], id);
        }

        [Event(64386, Message = "recover checkpoint called for state provider {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op105, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64386], id, stateProvider);
        }

        [Event(64357, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op106,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecoverStateProviders(string id)
        {
            WriteEvent(this.eventDescriptors[64357], id);
        }

        [Event(64363, Message = "state remove called on: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op107,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveStateOnStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64363], id, stateProvider);
        }

        [Event(64412, Message = "before replication", Task = Tasks.TStateManager, Opcode = Opcodes.Op108,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveSingleStateProviderBeforeReplication(string id)
        {
            WriteEvent(this.eventDescriptors[64412], id);
        }

        [Event(64410, Message = "request to unregister state provider {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op109, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveSingleStateProviderBegin(string id, string stateProviderName)
        {
            WriteEvent(this.eventDescriptors[64410], id, stateProviderName);
        }

        [Event(64419, Message = "state provider unregistered: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op110,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveSingleStateProviderEnd(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64419], id, stateProvider);
        }

        [Event(64421, Message = "state provider {1} does not exist in transaction {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op111, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)
        ]
        public void RemoveSingleStateProviderKeyNotFound(string id, long stateProvider, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64421], id, stateProvider, transactionId);
        }

        [Event(64325, Message = "Retrying replication operation for transaction {1} with back off interval {2}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op112, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReplicateOperationRetry(string id, long transactionId, int backOffDelay)
        {
            WriteEvent(this.eventDescriptors[64325], id, transactionId, backOffDelay);
        }

        [Event(64464, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op113, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ReplicationMetadataDeserializeError(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64464], id, message);
        }

        [Event(64342, Message = "started", Task = Tasks.TStateManager, Opcode = Opcodes.Op115,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiRestoreBegin(string id)
        {
            WriteEvent(this.eventDescriptors[64342], id);
        }

        [Event(64343, Message = "completed", Task = Tasks.TStateManager, Opcode = Opcodes.Op116,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiRestoreEnd(string id)
        {
            WriteEvent(this.eventDescriptors[64343], id);
        }

        [Event(64387, Message = "restore checkpoint called for state provider {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op117, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestoreStateProvider(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64387], id, stateProvider);
        }

        [Event(64359, Message = "completed from backup directory: {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op118, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestoreStateProviders(string id, string backupDirectory)
        {
            WriteEvent(this.eventDescriptors[64359], id, backupDirectory);
        }

        [Event(64381, Message = "State Provider Id: {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op119,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SetCurrentState(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64381], id, stateProvider);
        }

        [Event(64354, Message = "deserialization error", Task = Tasks.TStateManager, Opcode = Opcodes.Op120,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void SetCurrentStateDeserializationError(string id)
        {
            WriteEvent(this.eventDescriptors[64354], id);
        }

        [Event(64385, Message = "set current state skipped for state provider as it is in the deleted list: {1}",
            Task = Tasks.TStateManager, Opcode = Opcodes.Op121, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SetCurrentStateSkipped(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64385], id, stateProvider);
        }

        [Event(64467, Message = "copied state provider count is {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op122,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateManagerCopyStreamGetNext(string id, int stateProviderCount)
        {
            WriteEvent(this.eventDescriptors[64467], id, stateProviderCount);
        }

        [Event(64466, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op123,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void StateManagerRestoreCheckpointAsync(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64466], id, message);
        }

        [Event(64431, Message = "state provider {1} is not registered in state manager {2}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op124, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)
        ]
        public void ThrowIfStateProviderIsNotRegistered(string id, string stateProviderName, string StateManagerName)
        {
            WriteEvent(this.eventDescriptors[64431], id, stateProviderName, StateManagerName);
        }

        [Event(64432, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op125, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ThrowIfNotValid(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64432], id, message);
        }

        [Event(64290, Message = "unlock called for state provider: {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op126, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Unlock_TStateManager(string id, long stateProvider)
        {
            WriteEvent(this.eventDescriptors[64290], id, stateProvider);
        }

        [Event(64364, Message = "transactionContext's owner transaction: {1}", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op127, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UnlockOnLocalState(string id, long transactionId)
        {
            WriteEvent(this.eventDescriptors[64364], id, transactionId);
        }

        [Event(64365, Message = "Invalid operation context for local state on unlock", Task = Tasks.TStateManager,
            Opcode = Opcodes.Op128, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)
        ]
        public void UnlockOnLocalStateInvalidContext(string id)
        {
            WriteEvent(this.eventDescriptors[64365], id);
        }

        [Event(64258, Message = "{2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStateManager_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64258], id, type, message);
        }

        [Event(64482, Message = "start. directory:{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op129,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveUnreferencedStateProvidersStart(string id, string directory)
        {
            WriteEvent(this.eventDescriptors[64482], id, directory);
        }

        [Event(64483, Message = "complete. directory:{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op130,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RemoveUnreferencedStateProvidersEnd(string id, string directory)
        {
            WriteEvent(this.eventDescriptors[64483], id, directory);
        }

        [Event(64484, Message = "found directory to delete:{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op131,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UnreferencedStateProvidersToBeDeleted(string id, string directory)
        {
            WriteEvent(this.eventDescriptors[64484], id, directory);
        }

        [Event(64485, Message = "deleting directory:{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op132,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UnreferencedStateProvidersBeingDeleted(string id, string directory)
        {
            WriteEvent(this.eventDescriptors[64485], id, directory);
        }

        [Event(64486, Message = "Time taken in seconds for UnreferencedStateProvider to be deleted : {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op133,
                Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UnreferencedStateProvidersTimer(string id, double timedifference)
        {
            WriteEvent(this.eventDescriptors[64486], id, timedifference);
        }

        [Event(64433, Message = "Service change role failed", Task = Tasks.TStateManager, Opcode = Opcodes.Op134,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ServiceChangeRoleFailed(string id)
        {
            WriteEvent(this.eventDescriptors[64433], id);
        }

        [Event(64434, Message = "Change state provider id to {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op235,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DynamicStateManagerAddStateProvider(string id, long StateProviderId)
        {
            WriteEvent(this.eventDescriptors[64434], id, StateProviderId);
        }

        [Event(64435, Message = "Change state provider id to {1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op236,
    Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DynamicStateManagerGetOrAddStateProvider(string id, long StateProviderId)
        {
            WriteEvent(this.eventDescriptors[64435], id, StateProviderId);
        }

        [Event(64436, Message = "{1}", Task = Tasks.TStateManager, Opcode = Opcodes.Op139, Level = EventLevel.Error,
        Keywords = Keywords.DataMessaging)]
        public void DiagnosticError(string id, string errMsg)
        {
            WriteEvent(this.eventDescriptors[64436], id, errMsg);
        }

        [Event(64437, Message = "SPId: {1} Api: Function: {2}.", Task = Tasks.TStateManager, Opcode = Opcodes.Op140, Level = EventLevel.Error,
            Keywords = Keywords.DataMessaging)]
        public void ISP2_ApiError(string id, long stateProviderId, string functionName)
        {
            WriteEvent(this.eventDescriptors[64437], id, stateProviderId, functionName);
        }

        /***********************/
        /***Generated from file = %SRCROOT%\prod\src\managed\Microsoft.ServiceFabric.Data.Impl\Replicator\StructuredEvents.cs***/
        /***********************/

        [Event(63755, Message = "{1}.\n Type: {2} LSN: {3} PSN: {4} Position: {5} Txn: {6}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op11, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AbortTx(string id, string message, string recordType, long LSN, long PSN, ulong Position, long Tx)
        {
            WriteEvent(this.eventDescriptors[63755], id, message, recordType, LSN, PSN, Position, Tx);
        }

        [Event(63869, Message = "BackupId: {1} Option: {2} Folder: {3}", Task = Tasks.TReplicator, Opcode = Opcodes.Op12,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptBackup(string id, Guid backupId, int backupOption, string localBackupFolder)
        {
            WriteEvent(this.eventDescriptors[63869], id, backupId, backupOption, localBackupFolder);
        }

        [Event(63819, Message = "LSN: {1}", Task = Tasks.TReplicator, Opcode = Opcodes.Op13,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptBarrier(string id, long lsn)
        {
            WriteEvent(this.eventDescriptors[63819], id, lsn);
        }

        [Event(63792, Message = "Txn: {1} LSN: {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op14,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptBeginTransaction(string id, long txid, long lsn)
        {
            WriteEvent(this.eventDescriptors[63792], id, txid, lsn);
        }

        [Event(63823, Message = "Txn: {1} LSN: {2} IsCommitted: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op15, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void AcceptEndTransaction(string id, long txid, long lsn, bool committed)
        {
            WriteEvent(this.eventDescriptors[63823], id, txid, lsn, committed);
        }

        [Event(63793, Message = "Txn: {1} LSN: {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op16,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptOperation(string id, long txid, long lsn)
        {
            WriteEvent(this.eventDescriptors[63793], id, txid, lsn);
        }

        [Event(63870,
            Message = "RestoreId: {1}\r\n Starting Version: {2} {3}\r\n Ending Version: {4} {5}\r\n Number of Backups {6}\r\n BackupFolder: {7}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op17, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptRestore(string id, Guid restoreId, string indexingRecordEpoch, long indexingRecordLsn,
            string highestBackupRecordEpoch, long highestBackupRecordLsn, int numberOfBackups, string backupFolder)
        {
            WriteEvent(this.eventDescriptors[63870], id, restoreId, indexingRecordEpoch, indexingRecordLsn, highestBackupRecordEpoch,
                    highestBackupRecordLsn, numberOfBackups, backupFolder);
        }

        [Event(63867, Message = "Txn: {1} LSN: {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op18,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AcceptSingleOperationTransaction(string id, long txid, long lsn)
        {
            WriteEvent(this.eventDescriptors[63867], id, txid, lsn);
        }

        [Event(63750, Message = "{1}", Task = Tasks.TReplicator, Opcode = Opcodes.Op19,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Api(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63750], id, message);
        }

        [Event(63804,
            Message = "Atomic Operation applied Stream: {1} LSN: {2} PSN: {3} Position: {4} Txn: {5} RedoOnly: {6}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op20, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyCallbackAtomicOperationRecord(string id, int Stream, long lsn, long psn, ulong position,
            long tx, bool redoonly)
        {
            WriteEvent(this.eventDescriptors[63804], id, Stream, lsn, psn, position, tx, redoonly);
        }

        [Event(63805, Message = "Record applied Stream: {1} LSN: {2} PSN: {3} Position: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op21, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyCallbackBarrierRecord(string id, int Stream, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63805], id, Stream, lsn, psn, position);
        }

        [Event(63866,
            Message = "Single Operation transaction applied Stream: {1} LSN: {2} PSN: {3} Position: {4} Txn: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op22, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyCallbackSingleOperationTransaction(string id, int Stream, long lsn, long psn, ulong position,
            long tx)
        {
            WriteEvent(this.eventDescriptors[63866], id, Stream, lsn, psn, position, tx);
        }

        [Event(63802, Message = "Transaction applied Stream: {1} PSN: {2} Txn: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op23, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyCallbackTransactionRecord(string id, int Stream, long psn, long tx)
        {
            WriteEvent(this.eventDescriptors[63802], id, Stream, psn, tx);
        }

        [Event(63803, Message = "Record applied Stream: {1} Type: {2} LSN: {3} PSN: {4} Position: {5} Txn: {6}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op24, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApplyCallbackTransactionRecordNoise(string id, int Stream, uint recordType, long lsn, long psn,
            ulong position, long tx)
        {
            WriteEvent(this.eventDescriptors[63803], id, Stream, recordType, lsn, psn, position, tx);
        }

        [Event(63784, Message = "BackupId: {1}. {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op25,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupAsync(string id, Guid backupId, string message)
        {
            WriteEvent(this.eventDescriptors[63784], id, backupId, message);
        }

        [Event(63786, Message = "BackupId: {1} Option: {2} Folder: {3} Epoch: <{4}, {5}> LSN: {6}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op26, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupAsyncCompleted(string id, Guid backupId, string option, string folder, long dataLossNumber,
            long configurationNumber, long lsn)
        {
            WriteEvent(this.eventDescriptors[63786], id, backupId, option, folder, dataLossNumber, configurationNumber, lsn);
        }

        [Event(63895, Message = "BackupId: {1} Type: {2} Message: {3} Stack: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op27, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void BackupException(string id, Guid backupId, string exceptionType, string message, string stack)
        {
            WriteEvent(this.eventDescriptors[63895], id, backupId, exceptionType, message, stack);
        }

        [Event(63785, Message = "BackupId: {1} Index: {2}, Highest Epoch: <{3}, {4}> Highest LSN: {5}, Count: {6} Size: {7} kb Elapsed Time: {8} ms",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op28, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupReplicatorCompleted(string id, Guid backupId, long indexLsn, long lastDN, long lastCN,
            long lastLsn, uint count, uint size, long time)
        {
            WriteEvent(this.eventDescriptors[63785], id, backupId, indexLsn, lastDN, lastCN, lastLsn, count, size, time);
        }

        [Event(63797,
            Message =
                "Action: {1} CheckpointRecordState: {2} LSN: {3} PSN: {4} Position: {5} EarliestPendingTxLSN: {6}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op29, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Checkpoint(string id, string action, int state, long lsn, long psn, ulong position,
            long earliestpendinglsn)
        {
            WriteEvent(this.eventDescriptors[63797], id, action, state, lsn, psn, position, earliestpendinglsn);
        }

        [Event(63818,
            Message =
                "Attempting to abort {1} transactions from LSN {2} to LSN {3} as it prevented a checkpoint at LSN:{4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op30, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CheckpointAbortOldTx(string id, int count, long stopLSN, long startLSN, long lSN)
        {
            WriteEvent(this.eventDescriptors[63818], id, count, stopLSN, startLSN, lSN);
        }

        [Event(63762, Message = "{1}. ", Task = Tasks.TReplicator, Opcode = Opcodes.Op31,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyOrBuildReplica(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63762], id, message);
        }

        [Event(63751, Message = "Log records copied: {1}", Task = Tasks.TReplicator, Opcode = Opcodes.Op32,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyStreamFinished(string id, int numberOfRecords)
        {
            WriteEvent(this.eventDescriptors[63751], id, numberOfRecords);
        }

        [Event(63768, Message = "{1}.", Task = Tasks.TReplicator, Opcode = Opcodes.Op33,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyStreamGetNext(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63768], id, message);
        }

        [Event(63769, Message = "{1}.", Task = Tasks.TReplicator, Opcode = Opcodes.Op34, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyStreamGetNextNoise(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63769], id, message);
        }

        [Event(63767, Message = "{1}.", Task = Tasks.TReplicator, Opcode = Opcodes.Op35,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void CopyStreamMetadata(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63767], id, message);
        }

        [Event(63815,
            Message =
                "Until pending operation (type:{1},LSN: {2}) completes. Bytes from log head:{3}. Log Usage Percent:{4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op36, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainReplicationBlocked(string id, string opType, long lsn, ulong bytes, long percent)
        {
            WriteEvent(this.eventDescriptors[63815], id, opType, lsn, bytes, percent);
        }

        [Event(63812,
            Message =
                "Drain of {1} stream completed. Drain status: {2}. Number of records: {3}. Last record Type: {4} LSN: {5} PSN: {6} Position: {7}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op37, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainCompleted(string id, string stream, string status, long number, uint recordType, long lsn, long psn,
            ulong position)
        {
            WriteEvent(this.eventDescriptors[63812], id, stream, status, number, recordType, lsn, psn, position);
        }

        [Event(63816, Message = "Continuing drain operation. Bytes from log head:{1}. Log Usage Percent:{2}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op38, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainReplicationContinue(string id, ulong bytes, long percent)
        {
            WriteEvent(this.eventDescriptors[63816], id, bytes, percent);
        }

        [Event(63774, Message = "Initiating flush at copied record {1} LSN: {2} AcksRemaining: {3}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op39, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainCopyFlush(string id, long number, long lsn, long acksRemaining)
        {
            WriteEvent(this.eventDescriptors[63774], id, number, lsn, acksRemaining);
        }

        [Event(63760, Message = "AckedLSN: {1} AcksPending: {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op40,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainCopyNoise(string id, long lsn, long acksPending)
        {
            WriteEvent(this.eventDescriptors[63760], id, lsn, acksPending);
        }

        [Event(63759, Message = "Received Copy record: {1} RecordType: {2} LSN: {3} AcksRemaining: {4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op41, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainCopyReceive(string id, long number, string recordType, long lsn, long acksRemaining)
        {
            WriteEvent(this.eventDescriptors[63759], id, number, recordType, lsn, acksRemaining);
        }

        [Event(63775,
            Message = "Initiating flush at replicated record {1} LSN: {2} AcksRemaining: {3} BytesRemaining: {4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op42, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainReplicationFlush(string id, long number, long lsn, long acksRemaining, long bytesRemaining)
        {
            WriteEvent(this.eventDescriptors[63775], id, number, lsn, acksRemaining, bytesRemaining);
        }

        [Event(63758, Message = "AckedLSN: {1} AcksPending: {2} BytesPending: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op43, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void DrainReplicationNoise(string id, long ackedLSN, long acksPending, long bytesPending)
        {
            WriteEvent(this.eventDescriptors[63758], id, ackedLSN, acksPending, bytesPending);
        }

        [Event(63801, Message = "Received Replication record: {1} RecordType: {2} LSN: {3} AcksRemaining: {4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op44, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainReplicationReceive(string id, long number, uint recordType, long lsn, long acksRemaining)
        {
            WriteEvent(this.eventDescriptors[63801], id, number, recordType, lsn, acksRemaining);
        }

        [Event(63776, Message = "{1}. ", Task = Tasks.TReplicator, Opcode = Opcodes.Op45,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainStart(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63776], id, message);
        }

        [Event(63761, Message = "{1}. \n {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op46,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DrainStateNoise(string id, string message, string record)
        {
            WriteEvent(this.eventDescriptors[63761], id, message, record);
        }

        [Event(63764, Message = "{1}. ", Task = Tasks.TReplicator, Opcode = Opcodes.Op47, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Error(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63764], id, message);
        }

        [Event(63798, Message = "Record Type: {1} LSN: {2} PSN: {3} Position: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op48, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FlushedRecordsCallback(string id, uint recordType, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63798], id, recordType, lsn, psn, position);
        }

        [Event(63806, Message = "Record Type: {1} LSN: {2} PSN: {3} Position: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op49, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void FlushedRecordsCallbackFailed(string id, uint recordType, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63806], id, recordType, lsn, psn, position);
        }

        [Event(63754, Message = "Starting record position: {1} Ending record position: {2}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op50, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GetLogRecordsToCopy(string id, ulong startingLSN, ulong endingLSN)
        {
            WriteEvent(this.eventDescriptors[63754], id, startingLSN, endingLSN);
        }

        [Event(63796,
            Message =
                "Encountered exception. Type: {1}.\n  HResult: 0x{2} Stack: {3} Type: {4} LSN: {5} PSN: {6} RecordStableLSN: {7} CurrentStableLSN: {8}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op51, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void GroupCommitWarning(string id, string exceptionandMessage, int HResult, string stacktrace,
            string recordtype, long LSN, long PSN, string RecordStableLSN, string CurrentStableLSN)
        {
            WriteEvent(this.eventDescriptors[63796], id, exceptionandMessage, HResult, stacktrace, recordtype, LSN, PSN, RecordStableLSN,
                    CurrentStableLSN);
        }

        [Event(63790, Message = "Lock: {1} RequesterName: {2}: Message: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op52, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void Lock(string id, string lockName, string requestersName, string message)
        {
            WriteEvent(this.eventDescriptors[63790], id, lockName, requestersName, message);
        }

        [Event(63770, Message = "{1}.", Task = Tasks.TReplicator, Opcode = Opcodes.Op53,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LogManager(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63770], id, message);
        }

        [Event(63771, Message = "Re-throwing Unexpected Exception in {1}. Type: {2} Message: {3} HResult: {4} Stack: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op54, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void LogManagerExceptionError(string id, string method, string recordType, string message, int hresult,
            string stack)
        {
            WriteEvent(this.eventDescriptors[63771], id, method, recordType, message, hresult, stack);
        }

        [Event(63772, Message = "Caught Benign Exception in {1}. Type: {2} Message: {3} HResult: {4} Stack: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op55, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LogManagerExceptionInfo(string id, string method, string recordType, string message, int hresult,
            string stack)
        {
            WriteEvent(this.eventDescriptors[63772], id, method, recordType, message, hresult, stack);
        }

        [Event(63813, Message = "Record Type: {1} LSN: {2} PSN: {3} Position: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op56, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LogManagerNoOpRecordsCallback(string id, string recordType, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63813], id, recordType, lsn, psn, position);
        }

        [Event(63814, Message = "Record Type: {1} LSN: {2} PSN: {3} Position: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op57, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void LogManagerNoOpRecordsCallbackFailed(string id, string recordType, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63814], id, recordType, lsn, psn, position);
        }

        [Event(63791, Message = "RecordType: {1} LSN: {2} Message: {3}", Task = Tasks.TReplicator, Opcode = Opcodes.Op58,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void LogRecordsMap(string id, string logRecordType, long lsn, string message)
        {
            WriteEvent(this.eventDescriptors[63791], id, logRecordType, lsn, message);
        }

        [Event(63773,
            Message = "{1} LSN: {2} PSN: {3} Position: {4}\nFlushed Record Type: {5} LSN: {6} PSN: {7} Position: {8}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op59, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PhysicalLogWriterFlushAsync(string id, string waitedRecordType, long waitedRecordLsn,
            long waitedRecordPsn, ulong waitedRecordPosition, string flushedType, long flushedLsn, long flushedPsn,
            ulong flushedPosition)
        {
            WriteEvent(this.eventDescriptors[63773], id, waitedRecordType, waitedRecordLsn, waitedRecordPsn, waitedRecordPosition,
                    flushedType, flushedLsn, flushedPsn, flushedPosition);
        }

        [Event(63782, Message = " Starting Flush of {1}: records from PSN: {2}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op60, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void PhysicalLogWriterFlushStart(string id, int count, long startingPSN)
        {
            WriteEvent(this.eventDescriptors[63782], id, count, startingPSN);
        }

        [Event(63783, Message = "{1}.", Task = Tasks.TReplicator, Opcode = Opcodes.Op61, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PrimaryFullCopyInitiated(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63783], id, message);
        }

        [Event(63794, Message = "Draining Stream: {1} PSN: {2} RecordStableLSN: {3} CurrentStableLSN: {4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op62, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessBarrierRecord(string id, int stream, long psn, long recordstablelsn, long currentstablelsn)
        {
            WriteEvent(this.eventDescriptors[63794], id, stream, psn, recordstablelsn, currentstablelsn);
        }

        [Event(63795,
            Message = "Draining Stream: {1} Message: {2} PSN: {3} RecordStableLSN: {4} CurrentStableLSN: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op63, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessBarrierRecordException(string id, string stream, int message, long psn, long recordstablelsn,
            long currentstablelsn)
        {
            WriteEvent(this.eventDescriptors[63795], id, stream, message, psn, recordstablelsn, currentstablelsn);
        }

        [Event(63808, Message = "Record processed. Stream: {1} PSN: {2}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op64, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void ProcessedLogicalRecord(string id, int stream, long psn)
        {
            WriteEvent(this.eventDescriptors[63808], id, stream, psn);
        }

        [Event(63809, Message = "Record processing skipped due to flush exception. Stream: {1} PSN: {2}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op65, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessedLogicalRecordSkip(string id, int stream, long psn)
        {
            WriteEvent(this.eventDescriptors[63809], id, stream, psn);
        }

        [Event(63810, Message = "Record processed. Stream: {1} Type: {2} LSN: {3} PSN: {4} Position: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op66, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessedPhysicalRecord(string id, int stream, string recordType, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63810], id, stream, recordType, lsn, psn, position);
        }

        [Event(63811, Message = "Record processing skipped due to flush exception. Stream: {1} PSN: {2}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op67, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessedPhysicalRecordSkip(string id, int stream, long psn)
        {
            WriteEvent(this.eventDescriptors[63811], id, stream, psn);
        }

        [Event(63763, Message = "Accumulated records. Count: {1} FromLSN: {2} ToLSN: {3} FromPSN: {4} ToPSN: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op68, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessLoggedRecords(string id, int count, long fromlsn, long tolsn, long frompsn, long topsn)
        {
            WriteEvent(this.eventDescriptors[63763], id, count, fromlsn, tolsn, frompsn, topsn);
        }

        [Event(63749, Message = "ReplicaRole:{1}\n {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op69,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void QuiesceReplicaActivityAsync(string id, string role, string message)
        {
            WriteEvent(this.eventDescriptors[63749], id, role, message);
        }

        [Event(63799, Message = "Record Processed. Draining Stream: {1} Info: {2} PSN: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op70, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RecordProcessedImmediately(string id, int stream, string info, long psn)
        {
            WriteEvent(this.eventDescriptors[63799], id, stream, info, psn);
        }

        [Event(63800, Message = "Record Applied. Draining Stream: {1} PSN: {2}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op71, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void RecordProcessedImmediatelyNoise(string id, int stream, long psn)
        {
            WriteEvent(this.eventDescriptors[63800], id, stream, psn);
        }

        [Event(63787, Message = "RestoreId: {1} {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op72,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestoreAsync(string id, Guid restoreId, string message)
        {
            WriteEvent(this.eventDescriptors[63787], id, restoreId, message);
        }

        [Event(63788,
            Message =
                "RestoreId: {1} Folder: {2}, Epoch: <{3}, {4}>, TailLSN: {5}\n StateManagerRestoreTime: {6} ms\n ReplicatorRestoreTime: {7} ms\n Total: {8} ms",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op73, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestoreCompleted(string id, Guid restoreId, string folder, long dataLossNumber,
            long configurationNumber, long tailLsn, long stateManagerRestoreTimeInMs, long replicatorRestoreTimeInMs,
            long totalRestoreTimeInMs)
        {
            WriteEvent(this.eventDescriptors[63788], id, restoreId, folder, dataLossNumber, configurationNumber, tailLsn,
                    stateManagerRestoreTimeInMs, replicatorRestoreTimeInMs, totalRestoreTimeInMs);
        }

        [Event(63896, Message = "RestoreId: {1} Type: {2} Message: {3} Stack: {4}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op74, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default
            )]
        public void RestoreException(string id, Guid restoreId, string exceptionType, string message, string stack)
        {
            WriteEvent(this.eventDescriptors[63896], id, restoreId, exceptionType, message, stack);
        }

        [Event(63789,
            Message = "Record Type: {1} BackupLogPosition: {2} LSN: {3} PSN: {4} LastPhysicalLogRecord.PSN: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op75, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RestoreOperationAsync(string id, string recordType, long backupLogPosition, long lsn, long psn,
            long lastPhysicalLogRecordPsn)
        {
            WriteEvent(this.eventDescriptors[63789], id, recordType, backupLogPosition, lsn, psn, lastPhysicalLogRecordPsn);
        }

        [Event(63897,
            Message = "LSN: {1} PSN: {2} RecordPosition: {3} HighestBackedupEpoch: <{4}, {5}> HighestBackedupLSN: {6}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op76, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SetBackupLogRecord(string id, long lsn, long psn, ulong recordPosition, long dataloss,
            long configuration, long highestBackedupLsn)
        {
            WriteEvent(this.eventDescriptors[63897], id, lsn, psn, recordPosition, dataloss, configuration, highestBackedupLsn);
        }

        [Event(63898,
            Message = "LSN: {1} PSN: {2} RecordPosition: {3} HighestBackedupEpoch: <{4}, {5}> HighestBackedupLSN: {6}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op200, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UndoBackupLogRecord(
            string id,
            long lsn,
            long psn,
            ulong recordPosition,
            long dataloss,
            long configuration,
            long highestBackedupLsn)
        {
            WriteEvent(this.eventDescriptors[63898], id, lsn, psn, recordPosition, dataloss, configuration, highestBackedupLsn);
        }

        [Event(64005, Message = "Unexpected Exception in {1}. Type: {2} Message: {3} HResult: {4} Stack: {5}",
            Task = Tasks.TStatefulServiceReplica, Opcode = Opcodes.Op77, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Exception_TStatefulServiceReplica(string id, string method, string recordType, string message,
            int hresult, string stack)
        {
            WriteEvent(this.eventDescriptors[64005], id, method, recordType, message, hresult, stack);
        }

        [Event(64004, Message = "{1}.", Task = Tasks.TStatefulServiceReplica, Opcode = Opcodes.Op78,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Lifecycle(string id, string message)
        {
            WriteEvent(this.eventDescriptors[64004], id, message);
        }

        [Event(63894, Message = "Setting: {1} Value: {2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op79,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TestSettingModified(string id, string setting, string value)
        {
            WriteEvent(this.eventDescriptors[63894], id, setting, value);
        }

        [Event(63817,
            Message =
                "TruncationState: {1} LSN: {2} PSN: {3} Position: {4} LogHead record LSN: {5} PSN: {6} Position: {7}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op80, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateHeadApply(string id, string state, long lsn, long psn, ulong position, long headlsn,
            long headpsn, ulong headposition)
        {
            WriteEvent(this.eventDescriptors[63817], id, state, lsn, psn, position, headlsn, headpsn, headposition);
        }

        [Event(63822,
            Message = "LSN: {1} PSN: {2} Position: {3} LogHead record LSN: {4} PSN: {5} Position: {6} IsStable: {7}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op81, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateHeadInitiate(string id, long lsn, long psn, ulong position, long headlsn, long headpsn,
            ulong headposition, bool isStable)
        {
            WriteEvent(this.eventDescriptors[63822], id, lsn, psn, position, headlsn, headpsn, headposition, isStable);
        }

        [Event(63820,
            Message =
                "LSN: {1} PSN: {2} Position: {3} LogHead record LSN: {4} PSN: {5} Position: {6} TruncationPerformed: {7} TruncationState: {8}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op82, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateHeadProcess(string id, long lsn, long psn, ulong position, long headlsn, long headpsn,
            ulong headposition, bool truncateperformed, string truncationstate)
        {
            WriteEvent(this.eventDescriptors[63820], id, lsn, psn, position, headlsn, headpsn, headposition, truncateperformed,
                    truncationstate);
        }

        [Event(63779, Message = "Atomic Operation undone. LSN: {1} PSN: {2} Position: {3} Txn: {4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op83, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailAtomicOperation(string id, long lsn, long psn, ulong position, long tx)
        {
            WriteEvent(this.eventDescriptors[63779], id, lsn, psn, position, tx);
        }

        [Event(63780, Message = "Barrier Deleted. LSN: {1} PSN: {2} Position: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op84, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailBarrier(string id, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63780], id, lsn, psn, position);
        }

        [Event(62483, Message = "Backup Deleted. LSN: {1} PSN: {2} Position: {3}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op199, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailBackup(string id, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[62483], id, lsn, psn, position);
        }

        [Event(63756, Message = "Tail is being truncated to {1}", Task = Tasks.TReplicator, Opcode = Opcodes.Op85,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTail(string id, long lsn)
        {
            WriteEvent(this.eventDescriptors[63756], id, lsn);
        }

        [Event(63821, Message = "Tail truncated to record Type: {1} LSN: {2} PSN: {3} Position: {4}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op86, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailDone(string id, string recordType, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63821], id, recordType, lsn, psn, position);
        }

        [Event(63778, Message = "Operation {1}. LSN: {2} PSN: {3} Position: {4} Txn: {5}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op87, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailOperationRecord(string id, string recordType, long lsn, long psn, ulong position,
            long tx)
        {
            WriteEvent(this.eventDescriptors[63778], id, recordType, lsn, psn, position, tx);
        }

        [Event(63868, Message = "Txn: {1}. LSN: {2} PSN: {3} Position: {4} Txn: {5}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op88, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailSingleOperationTransactionRecord(string id, string operation, long lsn, long psn,
            ulong position, long tx)
        {
            WriteEvent(this.eventDescriptors[63868], id, operation, lsn, psn, position, tx);
        }

        [Event(63777, Message = "Operation: {1}. LSN: {2} PSN: {3} Position: {4} Txn: {5}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op89, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailTransactionRecord(string id, string operation, long lsn, long psn, ulong position,
            long tx)
        {
            WriteEvent(this.eventDescriptors[63777], id, operation, lsn, psn, position, tx);
        }

        [Event(63781, Message = "UpdateEpoch Deleted. Epoch: {1},{2} LSN: {3} PSN: {4} Position: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op90, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TruncateTailUpdateEpoch(string id, long dataloss, long config, long lsn, long psn, ulong position)
        {
            WriteEvent(this.eventDescriptors[63781], id, dataloss, config, lsn, psn, position);
        }

        [Event(63766,
            Message =
                "Security settings are the only dynamically configurable setting. Cannot update other settings. \nCurrent Settings is {1}\nUpdate Request is {2}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op91, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void UpdatedReplicatorSettings(string id, string oldSettings, string newSettings)
        {
            WriteEvent(this.eventDescriptors[63766], id, oldSettings, newSettings);
        }

        [Event(63757, Message = "{1}: {2},{3} LSN: {4} ReplicaRole: {5}", Task = Tasks.TReplicator,
            Opcode = Opcodes.Op92, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UpdateEpoch(string id, string message, long dataloss, long config, long lsn, string role)
        {
            WriteEvent(this.eventDescriptors[63757], id, message, dataloss, config, lsn, role);
        }

        [Event(63752, Message = "{1}. AwaitLSN: {2} TailLSN: {3}", Task = Tasks.TReplicator, Opcode = Opcodes.Op93,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WaitForLogFlushUptoLsn(string id, string message, long awaitLSN, long tailLSN)
        {
            WriteEvent(this.eventDescriptors[63752], id, message, awaitLSN, tailLSN);
        }

        [Event(63748, Message = "{1}: {2} records", Task = Tasks.TReplicator, Opcode = Opcodes.Op94,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WaitForProcessing(string id, string recordType, long numberofRecords)
        {
            WriteEvent(this.eventDescriptors[63748], id, recordType, numberofRecords);
        }

        [Event(63807, Message = "{1}: Last processed record Type: {2} LSN: {3} PSN: {4} Position: {5}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op95, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WaitForProcessingDone(string id, string function, uint recordType, long lsn, long psn,
            ulong position)
        {
            WriteEvent(this.eventDescriptors[63807], id, function, recordType, lsn, psn, position);
        }

        [Event(63753, Message = "Wakeup awaiters. Flushed LSN: {1}", Task = Tasks.TReplicator, Opcode = Opcodes.Op96,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WakeupWaitingStreams(string id, long flushedLSN)
        {
            WriteEvent(this.eventDescriptors[63753], id, flushedLSN);
        }

        [Event(63765, Message = "{1}. ", Task = Tasks.TReplicator, Opcode = Opcodes.Op97, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Warning(string id, string message)
        {
            WriteEvent(this.eventDescriptors[63765], id, message);
        }

        [Event(63824,
            Message =
                "Flushed Bytes: {1} Records: {2} LatencySensitiveRecords: {3} FlushTime(ms): {4} SerializationTime(ms): {5}" +
                " Avg. Byte/sec: {6} Avg. Latency Milliseconds: {7}. WritePosition: {8}", Task = Tasks.TReplicator, Opcode = Opcodes.Op98,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PhysicalLogWriterFlushEnd(string id, ulong bytes, long count, int latencysensitive,
            long milliseconds, long serializationMilliseconds, double avgbytespersec, double avglatencyms, long writePosition)
        {
            WriteEvent(this.eventDescriptors[63824], id, bytes, count, latencysensitive, milliseconds, serializationMilliseconds, avgbytespersec, avglatencyms, writePosition);
        }

        [Event(63825,
            Message =
                "Flushed Bytes: {1} Records: {2} LatencySensitiveRecords: {3} FlushTime(ms): {4} SerializationTime(ms): {5}" +
                " Avg. Byte/sec: {6} Avg. Latency Milliseconds: {7}. WritePosition: {8}", Task = Tasks.TReplicator, Opcode = Opcodes.Op201,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PhysicalLogWriterFlushEndWarning(string id, ulong bytes, long count, int latencysensitive,
            long milliseconds, long serializationMilliseconds, double avgbytespersec, double avglatencyms, long writePosition)
        {
            WriteEvent(this.eventDescriptors[63825], id, bytes, count, latencysensitive, milliseconds, serializationMilliseconds, avgbytespersec, avglatencyms, writePosition);
        }

        [Event(63826,
            Message = "Replica Id: {1} Commit Rate: {2} Commit Count: {3}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op202, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Telemetry(Guid id, long replicaId, double commitRate, long commitCount)
        {
            WriteEvent(this.eventDescriptors[63826], id, replicaId, commitRate, commitCount);
        }

        [Event(63827,
            Message = "Replica Id: {1} Disk Size: {2} Number of State Providers: {3}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op203, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void MetricManagerTelemetry(Guid id, long replicaId, long diskSizeInMB, long numberOfStateProviders)
        {
            WriteEvent(this.eventDescriptors[63827], id, replicaId, diskSizeInMB, numberOfStateProviders);
        }

        [Event(63828,
            Message = "Head Truncation done at Lsn: {1} Psn: {2} Position: {3}. Invoked {4} freelink calls with {4} links older than the new log head Psn",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op204, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProcessLogHeadTruncationDone(string id, long LSN, long PSN, ulong position, long numFreeLinkCalls, long numTrueFreeLinkCalls)
        {
            WriteEvent(this.eventDescriptors[63828], id, LSN, PSN, position, numFreeLinkCalls, numTrueFreeLinkCalls);
        }

        [Event(63829,
            Message = "Starting full copy, ProgressVector validation failed. \r\nError Message: {1}\r\nSourceProgressVector: {2}\r\nSourceIndex: {3}.\r\nSourceProgressVectorEntry: {4} \r\nTargetProgressVector: {5}\r\nTargetIndex: {6}.\r\nTargetProgressVectorEntry: {7} ",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op205, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ProgressVectorValidationTelemetry(string id, string errorMessage, string sourceVector, long sourceIndex, string sourceVectorEntry, string targetVector, long targetIndex, string targetVectorEntry)
        {
            WriteEvent(this.eventDescriptors[63829], id, errorMessage, sourceVector, sourceIndex, sourceVectorEntry, targetVector, targetIndex, targetVectorEntry);
        }

        [Event(63830,
            Message = "State: {1}. Last Periodic Checkpoint : {2}. Last Periodic Truncation : {3}",
            Task = Tasks.TReplicator, Opcode = Opcodes.Op206, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void PeriodicCheckpointTruncation(string id, string periodicCheckpointTruncationState, string lastChkptTime, string lastTruncationTime)
        {
            WriteEvent(this.eventDescriptors[63830], id, periodicCheckpointTruncationState, lastChkptTime, lastTruncationTime);
        }

        [Event(57346, Message = "{2}", Task = Tasks.ManagedGeneral, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagedGeneral_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57346], id, type, message);
        }

        [Event(57602, Message = "{2}", Task = Tasks.ManagementCommon, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagementCommon_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57602], id, type, message);
        }

        [Event(57858, Message = "{2}", Task = Tasks.ImageStoreClient, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ImageStoreClient_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57858], id, type, message);
        }

        [Event(58114, Message = "{2}", Task = Tasks.FabricHost, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricHost_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58114], id, type, message);
        }

        [Event(58370, Message = "{2}", Task = Tasks.FabricDeployer, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricDeployer_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58370], id, type, message);
        }

        [Event(58882, Message = "{2}", Task = Tasks.Test, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Test_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58882], id, type, message);
        }

        [Event(59138, Message = "{2}", Task = Tasks.AzureLogCollector, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AzureLogCollector_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59138], id, type, message);
        }

        [Event(59394, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabric_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59394], id, type, message);
        }

        [Event(59650, Message = "{2}", Task = Tasks.ImageBuilder, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ImageBuilder_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59650], id, type, message);
        }

        [Event(59906, Message = "{2}", Task = Tasks.FabricDCA, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricDCA_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59906], id, type, message);
        }

        [Event(60162, Message = "{2}", Task = Tasks.FabricHttpGateway, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricHttpGateway_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60162], id, type, message);
        }

        [Event(60418, Message = "{2}", Task = Tasks.InfrastructureService, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InfrastructureService_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60418], id, type, message);
        }

        [Event(60674, Message = "{2}", Task = Tasks.ManagedTokenValidationService, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagedTokenValidationService_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60674], id, type, message);
        }

        [Event(60930, Message = "{2}", Task = Tasks.DSTSClient, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DSTSClient_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60930], id, type, message);
        }

        [Event(61186, Message = "{2}", Task = Tasks.FabricMonSvc, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricMonSvc_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61186], id, type, message);
        }

        [Event(61442, Message = "{2}", Task = Tasks.TStore, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStore_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61442], id, type, message);
        }

        [Event(61698, Message = "{2}", Task = Tasks.DistributedDictionary, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedDictionary_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61698], id, type, message);
        }

        [Event(61954, Message = "{2}", Task = Tasks.DistributedQueue, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedQueue_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61954], id, type, message);
        }

        [Event(62210, Message = "{2}", Task = Tasks.Wave, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Wave_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62210], id, type, message);
        }

        [Event(6658, Message = "{2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableStream_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[6658], id, type, message);
        }

        [Event(62722, Message = "{2}", Task = Tasks.DistributedVersionedDictionary, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedVersionedDictionary_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62722], id, type, message);
        }

        [Event(62978, Message = "{2}", Task = Tasks.Testability, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Testability_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62978], id, type, message);
        }

        [Event(63234, Message = "{2}", Task = Tasks.RandomActionGenerator, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RandomActionGenerator_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63234], id, type, message);
        }

        [Event(63490, Message = "{2}", Task = Tasks.FabricMdsAgentSvc, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricMdsAgentSvc_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63490], id, type, message);
        }

        [Event(63746, Message = "{2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TReplicator_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63746], id, type, message);
        }

        [Event(64002, Message = "{2}", Task = Tasks.TStatefulServiceReplica, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStatefulServiceReplica_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64002], id, type, message);
        }

        [Event(64514, Message = "{2}", Task = Tasks.ActorFramework, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ActorFramework_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64514], id, type, message);
        }

        [Event(64770, Message = "{2}", Task = Tasks.WRP, Opcode = Opcodes.Op230, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WRP_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64770], id, type, message);
        }

        [Event(65026, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ServiceFramework_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65026], id, type, message);
        }

        [Event(65282, Message = "{2}", Task = Tasks.FaultAnalysisService, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FaultAnalysisService_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65282], id, type, message);
        }

        [Event(65285, Message = "{2}", Task = Tasks.UpgradeOrchestrationService, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UpgradeOrchestrationService_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65285], id, type, message);
        }


        [Event(57345, Message = "{2}", Task = Tasks.ManagedGeneral, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagedGeneral_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57345], id, type, message);
        }

        [Event(57601, Message = "{2}", Task = Tasks.ManagementCommon, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagementCommon_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57601], id, type, message);
        }

        [Event(57857, Message = "{2}", Task = Tasks.ImageStoreClient, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ImageStoreClient_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57857], id, type, message);
        }

        [Event(58113, Message = "{2}", Task = Tasks.FabricHost, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricHost_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58113], id, type, message);
        }

        [Event(58369, Message = "{2}", Task = Tasks.FabricDeployer, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricDeployer_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58369], id, type, message);
        }

        [Event(58881, Message = "{2}", Task = Tasks.Test, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Test_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58881], id, type, message);
        }

        [Event(59137, Message = "{2}", Task = Tasks.AzureLogCollector, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AzureLogCollector_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59137], id, type, message);
        }

        [Event(59393, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabric_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59393], id, type, message);
        }

        [Event(59649, Message = "{2}", Task = Tasks.ImageBuilder, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ImageBuilder_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59649], id, type, message);
        }

        [Event(59905, Message = "{2}", Task = Tasks.FabricDCA, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricDCA_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59905], id, type, message);
        }

        [Event(60161, Message = "{2}", Task = Tasks.FabricHttpGateway, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricHttpGateway_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60161], id, type, message);
        }

        [Event(60417, Message = "{2}", Task = Tasks.InfrastructureService, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InfrastructureService_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60417], id, type, message);
        }

        [Event(60673, Message = "{2}", Task = Tasks.ManagedTokenValidationService, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagedTokenValidationService_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60673], id, type, message);
        }

        [Event(60929, Message = "{2}", Task = Tasks.DSTSClient, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DSTSClient_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60929], id, type, message);
        }

        [Event(61185, Message = "{2}", Task = Tasks.FabricMonSvc, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricMonSvc_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61185], id, type, message);
        }

        [Event(61441, Message = "{2}", Task = Tasks.TStore, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStore_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61441], id, type, message);
        }

        [Event(61697, Message = "{2}", Task = Tasks.DistributedDictionary, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedDictionary_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61697], id, type, message);
        }

        [Event(61953, Message = "{2}", Task = Tasks.DistributedQueue, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedQueue_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61953], id, type, message);
        }

        [Event(62209, Message = "{2}", Task = Tasks.Wave, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Wave_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62209], id, type, message);
        }

        [Event(6657, Message = "{2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableStream_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[6657], id, type, message);
        }

        [Event(62721, Message = "{2}", Task = Tasks.DistributedVersionedDictionary, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedVersionedDictionary_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62721], id, type, message);
        }

        [Event(62977, Message = "{2}", Task = Tasks.Testability, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Testability_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62977], id, type, message);
        }

        [Event(63233, Message = "{2}", Task = Tasks.RandomActionGenerator, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RandomActionGenerator_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63233], id, type, message);
        }

        [Event(63489, Message = "{2}", Task = Tasks.FabricMdsAgentSvc, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricMdsAgentSvc_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63489], id, type, message);
        }

        [Event(63745, Message = "{2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TReplicator_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63745], id, type, message);
        }

        [Event(64001, Message = "{2}", Task = Tasks.TStatefulServiceReplica, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStatefulServiceReplica_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64001], id, type, message);
        }

        [Event(64257, Message = "{2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStateManager_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64257], id, type, message);
        }

        [Event(64513, Message = "{2}", Task = Tasks.ActorFramework, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ActorFramework_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64513], id, type, message);
        }

        [Event(64769, Message = "{2}", Task = Tasks.WRP, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WRP_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64769], id, type, message);
        }

        [Event(65025, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op231, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ServiceFramework_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65025], id, type, message);
        }

        [Event(65281, Message = "{2}", Task = Tasks.FaultAnalysisService, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FaultAnalysisService_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65281], id, type, message);
        }

        [Event(65286, Message = "{2}", Task = Tasks.UpgradeOrchestrationService, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UpgradeOrchestrationService_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65286], id, type, message);
        }

        [Event(57344, Message = "{2}", Task = Tasks.ManagedGeneral, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ManagedGeneral_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57344], id, type, message);
        }

        [Event(57600, Message = "{2}", Task = Tasks.ManagementCommon, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ManagementCommon_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57600], id, type, message);
        }

        [Event(57856, Message = "{2}", Task = Tasks.ImageStoreClient, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ImageStoreClient_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57856], id, type, message);
        }

        [Event(58112, Message = "{2}", Task = Tasks.FabricHost, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FabricHost_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58112], id, type, message);
        }

        [Event(58368, Message = "{2}", Task = Tasks.FabricDeployer, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FabricDeployer_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58368], id, type, message);
        }

        [Event(58880, Message = "{2}", Task = Tasks.Test, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Test_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58880], id, type, message);
        }

        [Event(59136, Message = "{2}", Task = Tasks.AzureLogCollector, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void AzureLogCollector_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59136], id, type, message);
        }

        [Event(59392, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabric_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59392], id, type, message);
        }

        [Event(59648, Message = "{2}", Task = Tasks.ImageBuilder, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ImageBuilder_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59648], id, type, message);
        }

        [Event(59904, Message = "{2}", Task = Tasks.FabricDCA, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FabricDCA_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59904], id, type, message);
        }

        [Event(60160, Message = "{2}", Task = Tasks.FabricHttpGateway, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FabricHttpGateway_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60160], id, type, message);
        }

        [Event(60416, Message = "{2}", Task = Tasks.InfrastructureService, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void InfrastructureService_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60416], id, type, message);
        }

        [Event(60672, Message = "{2}", Task = Tasks.ManagedTokenValidationService, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ManagedTokenValidationService_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60672], id, type, message);
        }

        [Event(60928, Message = "{2}", Task = Tasks.DSTSClient, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void DSTSClient_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60928], id, type, message);
        }

        [Event(61184, Message = "{2}", Task = Tasks.FabricMonSvc, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FabricMonSvc_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61184], id, type, message);
        }

        [Event(61440, Message = "{2}", Task = Tasks.TStore, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void TStore_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61440], id, type, message);
        }

        [Event(61696, Message = "{2}", Task = Tasks.DistributedDictionary, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void DistributedDictionary_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61696], id, type, message);
        }

        [Event(61952, Message = "{2}", Task = Tasks.DistributedQueue, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void DistributedQueue_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61952], id, type, message);
        }

        [Event(62208, Message = "{2}", Task = Tasks.Wave, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Wave_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62208], id, type, message);
        }

        [Event(6656, Message = "{2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ReliableStream_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[6656], id, type, message);
        }

        [Event(62720, Message = "{2}", Task = Tasks.DistributedVersionedDictionary, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void DistributedVersionedDictionary_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62720], id, type, message);
        }

        [Event(62976, Message = "{2}", Task = Tasks.Testability, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void Testability_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62976], id, type, message);
        }

        [Event(63232, Message = "{2}", Task = Tasks.RandomActionGenerator, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void RandomActionGenerator_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63232], id, type, message);
        }

        [Event(63488, Message = "{2}", Task = Tasks.FabricMdsAgentSvc, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FabricMdsAgentSvc_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63488], id, type, message);
        }

        [Event(63744, Message = "{2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void TReplicator_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63744], id, type, message);
        }

        [Event(64000, Message = "{2}", Task = Tasks.TStatefulServiceReplica, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void TStatefulServiceReplica_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64000], id, type, message);
        }

        [Event(64256, Message = "{2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void TStateManager_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64256], id, type, message);
        }

        [Event(64512, Message = "{2}", Task = Tasks.ActorFramework, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ActorFramework_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64512], id, type, message);
        }

        [Event(64768, Message = "{2}", Task = Tasks.WRP, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void WRP_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64768], id, type, message);
        }

        [Event(65024, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ServiceFramework_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65024], id, type, message);
        }

        [Event(65280, Message = "{2}", Task = Tasks.FaultAnalysisService, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void FaultAnalysisService_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65280], id, type, message);
        }

        [Event(65287, Message = "{2}", Task = Tasks.UpgradeOrchestrationService, Opcode = Opcodes.Op232, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void UpgradeOrchestrationService_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65287], id, type, message);
        }

        [Event(57347, Message = "{2}", Task = Tasks.ManagedGeneral, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagedGeneral_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57347], id, type, message);
        }

        [Event(57603, Message = "{2}", Task = Tasks.ManagementCommon, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagementCommon_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57603], id, type, message);
        }

        [Event(57859, Message = "{2}", Task = Tasks.ImageStoreClient, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ImageStoreClient_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[57859], id, type, message);
        }

        [Event(58115, Message = "{2}", Task = Tasks.FabricHost, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricHost_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58115], id, type, message);
        }

        [Event(58371, Message = "{2}", Task = Tasks.FabricDeployer, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricDeployer_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58371], id, type, message);
        }

        [Event(58883, Message = "{2}", Task = Tasks.Test, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Test_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58883], id, type, message);
        }

        [Event(59139, Message = "{2}", Task = Tasks.AzureLogCollector, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void AzureLogCollector_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59139], id, type, message);
        }

        [Event(59395, Message = "{2}", Task = Tasks.SystemFabric, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabric_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59395], id, type, message);
        }

        [Event(59651, Message = "{2}", Task = Tasks.ImageBuilder, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ImageBuilder_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59651], id, type, message);
        }

        [Event(59907, Message = "{2}", Task = Tasks.FabricDCA, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricDCA_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[59907], id, type, message);
        }

        [Event(60163, Message = "{2}", Task = Tasks.FabricHttpGateway, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricHttpGateway_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60163], id, type, message);
        }

        [Event(60419, Message = "{2}", Task = Tasks.InfrastructureService, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InfrastructureService_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60419], id, type, message);
        }

        [Event(60675, Message = "{2}", Task = Tasks.ManagedTokenValidationService, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ManagedTokenValidationService_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60675], id, type, message);
        }

        [Event(60931, Message = "{2}", Task = Tasks.DSTSClient, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DSTSClient_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[60931], id, type, message);
        }

        [Event(61187, Message = "{2}", Task = Tasks.FabricMonSvc, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricMonSvc_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61187], id, type, message);
        }

        [Event(61443, Message = "{2}", Task = Tasks.TStore, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStore_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61443], id, type, message);
        }

        [Event(61699, Message = "{2}", Task = Tasks.DistributedDictionary, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedDictionary_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61699], id, type, message);
        }

        [Event(61955, Message = "{2}", Task = Tasks.DistributedQueue, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedQueue_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[61955], id, type, message);
        }

        [Event(62211, Message = "{2}", Task = Tasks.Wave, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Wave_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62211], id, type, message);
        }

        [Event(6659, Message = "{2}", Task = Tasks.ReliableStream, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableStream_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[6659], id, type, message);
        }

        [Event(62723, Message = "{2}", Task = Tasks.DistributedVersionedDictionary, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void DistributedVersionedDictionary_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62723], id, type, message);
        }

        [Event(62979, Message = "{2}", Task = Tasks.Testability, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void Testability_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[62979], id, type, message);
        }

        [Event(63235, Message = "{2}", Task = Tasks.RandomActionGenerator, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void RandomActionGenerator_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63235], id, type, message);
        }

        [Event(63491, Message = "{2}", Task = Tasks.FabricMdsAgentSvc, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricMdsAgentSvc_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63491], id, type, message);
        }

        [Event(63747, Message = "{2}", Task = Tasks.TReplicator, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TReplicator_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[63747], id, type, message);
        }

        [Event(64003, Message = "{2}", Task = Tasks.TStatefulServiceReplica, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStatefulServiceReplica_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64003], id, type, message);
        }

        [Event(64259, Message = "{2}", Task = Tasks.TStateManager, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void TStateManager_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64259], id, type, message);
        }

        [Event(64515, Message = "{2}", Task = Tasks.ActorFramework, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ActorFramework_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64515], id, type, message);
        }

        [Event(64771, Message = "{2}", Task = Tasks.WRP, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void WRP_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[64771], id, type, message);
        }

        [Event(65027, Message = "{2}", Task = Tasks.ServiceFramework, Opcode = Opcodes.Op233, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ServiceFramework_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65027], id, type, message);
        }

        [Event(65283, Message = "{2}", Task = Tasks.FaultAnalysisService, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FaultAnalysisService_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65283], id, type, message);
        }

        [Event(65284, Message = "{2}", Task = Tasks.UpgradeOrchestrationService, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void UpgradeOrchestrationService_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65284], id, type, message);
        }

        [Event(58626, Message = "{2}", Task = Tasks.SystemFabricDeployer, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabricDeployer_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58626], id, type, message);
        }

        [Event(58625, Message = "{2}", Task = Tasks.SystemFabricDeployer, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabricDeployer_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58625], id, type, message);
        }

        [Event(58624, Message = "{2}", Task = Tasks.SystemFabricDeployer, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabricDeployer_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58624], id, type, message);
        }

        [Event(58627, Message = "{2}", Task = Tasks.SystemFabricDeployer, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void SystemFabricDeployer_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[58627], id, type, message);
        }

        [Event(65288, Message = "{2}", Task = Tasks.BackupRestoreService, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        public void BackupRestoreService_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65288], id, type, message);
        }

        [Event(65289, Message = "{2}", Task = Tasks.BackupRestoreService, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational, Keywords = Keywords.Default)]
        public void BackupRestoreService_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65289], id, type, message);
        }

        [Event(65290, Message = "{2}", Task = Tasks.BackupRestoreService, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning, Keywords = Keywords.Default)]
        public void BackupRestoreService_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65290], id, type, message);
        }

        [Event(65291, Message = "{2}", Task = Tasks.BackupRestoreService, Opcode = Opcodes.Op232,
            Level = EventLevel.Error, Keywords = Keywords.Default)]
        public void BackupRestoreService_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65291], id, type, message);
        }

        [Event(65292, Message = "{2}", Task = Tasks.BackupCopier, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        public void BackupCopier_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65292], id, type, message);
        }

        [Event(65293, Message = "{2}", Task = Tasks.BackupCopier, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational, Keywords = Keywords.Default)]
        public void BackupCopier_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65293], id, type, message);
        }

        [Event(65294, Message = "{2}", Task = Tasks.BackupCopier, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning, Keywords = Keywords.Default)]
        public void BackupCopier_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65294], id, type, message);
        }

        [Event(65295, Message = "{2}", Task = Tasks.BackupCopier, Opcode = Opcodes.Op232,
            Level = EventLevel.Error, Keywords = Keywords.Default)]
        public void BackupCopier_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65295], id, type, message);
        }

        [Event(65296, Message = "All secrets updated using the new encryption certificate: {1}", Task = Tasks.BackupRestoreService, Opcode = Opcodes.Op11,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BRSSecretsUpdated(string id, string thumprint)
        {
            WriteEvent(this.eventDescriptors[65296], id, thumprint);
        }

        [Event(
            65298,
            Message = "Backup Restore is enabled for the cluster of type {0}",
            Task = Tasks.BackupRestoreService,
            Opcode = Opcodes.Op101,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupRestoreEnabled(string type
           )
        {
            WriteEvent(this.eventDescriptors[65298], type);
        }

        [Event(
            65299,
            Message = "Created Policy Name : {0} with incrementalBackupValue {1}, WithAutoRestored {2}, timeBaseSchedule {3}, frequencyBasedSchedule {4} , azureStorageBased {5} fileShareStorageBased {6}.",
            Task = Tasks.BackupRestoreService,
            Opcode = Opcodes.Op102,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupPolicyDetails(
            string name,
            int incrementalBackupValue,
            int isAutoRestored,
            int timeBaseSchedule,
            int frequencyBasedSchedule,
            int azureStorageBased,
            int fileShareStorageBased)
        {
            WriteEvent(this.eventDescriptors[65299], name, incrementalBackupValue, isAutoRestored, timeBaseSchedule, frequencyBasedSchedule, azureStorageBased, fileShareStorageBased);
        }

        [Event(
            65300,
            Message = "Created Incremental Backup of Size {0}KB ,zipped to Size {1}KB where TimeForLocal Backup is {2}s with upload time {3}s amd total time {4}s for partition Id {5}",
            Task = Tasks.BackupRestoreService,
            Opcode = Opcodes.Op103,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void IncrementalBackupDetails(
            long backupSizeInKB,
            long zipSizeInKB,
            double timeForLocalBackup,
            double timeForUpload,
            double totalTimeForBackup,
            string partitionId)
        {
            WriteEvent(this.eventDescriptors[65300], backupSizeInKB, zipSizeInKB, timeForLocalBackup,
                timeForUpload, totalTimeForBackup, partitionId);
        }

        [Event(
            65301,
            Message = "Created Full Backup of Size {0}KB ,zipped to Size {1}KB where TimeForLocal Backup is {2}s with upload time {3}s amd total time {4}s for partition Id {5}",
            Task = Tasks.BackupRestoreService,
            Opcode = Opcodes.Op104,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FullBackupDetails(
            long backupSizeInKB,
            long zipSizeInKB,
            double timeForLocalBackup,
            double timeForUpload,
            double totalTimeForBackup,
            string partitionId)
        {
            WriteEvent(this.eventDescriptors[65301], backupSizeInKB, zipSizeInKB, timeForLocalBackup,
                timeForUpload, totalTimeForBackup, partitionId);
        }

        [Event(
          65302,
          Message = "Backup was auto restore {0} with success {1} in  total time of {2}s with backupCounts {3} for partition Id {4}",
          Task = Tasks.BackupRestoreService,
          Opcode = Opcodes.Op105,
          Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void BackupRestoreDetails(
          int autoRestoredCount,
          int successfullRestore,
          double timeForRestore,
          int backupCounts,
          string partitionId)
        {
            WriteEvent(this.eventDescriptors[65302], autoRestoredCount, successfullRestore, timeForRestore,
                backupCounts, partitionId);
        }


        /***********************/
        /***Events corresponding to FabricApiMonitoringComponent
        /***********************/


        [Event(65152, Message = "{2}", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiMonitor_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65152], id, type, message);
        }

        [Event(65153, Message = "{2}", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiMonitor_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65153], id, type, message);
        }

        [Event(65154, Message = "{2}", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op232,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ApiMonitor_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65154], id, type, message);
        }

        [Event(65155, Message = "{2}", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ApiMonitor_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65155], id, type, message);
        }

        [Event(65156, Message = "Start {1} on partition {2} replica {3}", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op100,
           Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricApiStartMonitoring(string id, string apiName, Guid partitionId, long replicaId)
        {
            WriteEvent(this.eventDescriptors[65156], id, apiName, partitionId, replicaId);
        }

        [Event(65157, Message = "Finish {1} on partition {2} replica {3}. Elapsed = {4}ms", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op101,
           Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricApiStopMonitoring(string id, string apiName, Guid partitionId, long replicaId, double elapsedTime)
        {
            WriteEvent(this.eventDescriptors[65157], id, apiName, partitionId, replicaId, elapsedTime);
        }

        [Event(65158, Message = "Api {1} slow on partition {2} replica {3}. Elapsed = {4}ms", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op102,
           Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricApiSlowTrace(string id, string apiName, Guid partitionId, long replicaId, double elapsedTime)
        {
            WriteEvent(this.eventDescriptors[65158], id, apiName, partitionId, replicaId, elapsedTime);
        }

        [Event(65159, Message = "Invoking health report on api {1}, partition {2}", Task = Tasks.ApiMonitor, Opcode = Opcodes.Op103,
           Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void FabricApiInvokeHealthReport(string id, string apiName, Guid partitionId)
        {
            WriteEvent(this.eventDescriptors[65159], id, apiName, partitionId);
        }

        [Event(65160, Message = "state provider {1} is not registered in state manager {2}", Task = Tasks.TStateManager,
                Opcode = Opcodes.Op136, Level = EventLevel.Error, Keywords = Keywords.Default)
        ]
        public void ThrowIfStateProviderIdIsNotRegistered(string id, long stateProviderId, string StateManagerName)
        {
            WriteEvent(this.eventDescriptors[65160], id, stateProviderId, StateManagerName);
        }

#region ReliableConcurrentQueue events

        [Event(63616, Message = "Enqueue rejected as the queue is full.  Max size: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op11,
           Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_EnqueueQueueFull(string id, int capacity)
        {
            WriteEvent(this.eventDescriptors[63616], id, capacity);
        }

        [Event(63617, Message = "Enqueue accepted.  Transaction id: {1}  ListElement id: <{2}>  TimeTaken: {3}  LinkedCount: {4}  DataStoreSize: {5}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op12,
             Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_Enqueue(string id, long transactionId, long listElementId, long timeTaken, int linkedCount, int dataStoreSize)
        {
            WriteEvent(this.eventDescriptors[63617], id, transactionId, listElementId, timeTaken, linkedCount, dataStoreSize);
        }

        [Event(63618, Message = "Enqueue failed.  Transaction id: {1}  ListElement id: <{2}>  Exception: {3}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op13,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_EnqueueFailed(string id, long transactionId, string listElementId, string exception)
        {
            WriteEvent(this.eventDescriptors[63618], id, transactionId, listElementId, exception);
        }

        [Event(63619, Message = "Dequeue accepted.  Transaction id: {1}  ListElement id: <{2}>  TimeTaken: {3}  LinkedCount: {4}  DataStoreSize: {5}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op14,
           Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_Dequeue(string id, long transactionId, long listElementId, long timeTaken, int linkedCount, int dataStoreSize)
        {
            WriteEvent(this.eventDescriptors[63619], id, transactionId, listElementId, timeTaken, linkedCount, dataStoreSize);
        }

        [Event(63620, Message = "Dequeue failed.  Transaction id: {1}  ListElement: <{2}>  Exception: {3}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op15,
            Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_DequeueFailed(string id, long transactionId, string listElementId, string exception)
        {
            WriteEvent(this.eventDescriptors[63620], id, transactionId, listElementId, exception);
        }

        [Event(63621, Message = "Invalid listElement state.  ListElement: <{1}>,  UnlockSource: {2}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op16,
            Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_InvalidListElementState(string id, string listElement, string unlockSource)
        {
            WriteEvent(this.eventDescriptors[63621], id, listElement, unlockSource);
        }

        [Event(63622, Message = "Null transaction.", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op17, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_NullTransaction(string id)
        {
            WriteEvent(this.eventDescriptors[63622], id);
        }

        [Event(63623, Message = "Invalid transaction.  Transaction id: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op18, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_InvalidTransaction(string id, long transactionId)
        {
            WriteEvent(this.eventDescriptors[63623], id, transactionId);
        }

        [Event(63624, Message = "{1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op19, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ExceptionWarning(string id, string exception)
        {
            WriteEvent(this.eventDescriptors[63624], id, exception);
        }

        [Event(63625, Message = "{1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op20, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ExceptionError(string id, string exception)
        {
            WriteEvent(this.eventDescriptors[63625], id, exception);
        }

        [Event(63626, Message = "ReliableConcurrentQueue has closed or is closing.", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op21, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_Closing(string id)
        {
            WriteEvent(this.eventDescriptors[63626], id);
        }

        [Event(63627, Message = "ReliableConcurrentQueue.{1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op22, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ApiNoise(string id, string apiName)
        {
            WriteEvent(this.eventDescriptors[63627], id, apiName);
        }

        [Event(63628, Message = "ReliableConcurrentQueue.{1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op23, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ApiInfo(string id, string apiName)
        {
            WriteEvent(this.eventDescriptors[63628], id, apiName);
        }

        [Event(63629, Message = "ChangeRole - CurrentRole: {1} NewRole: {2}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op24,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ChangeRole(string id, string currentRole, string newRole)
        {
            WriteEvent(this.eventDescriptors[63629], id, currentRole, newRole);
        }

        [Event(63630, Message = "SetCurrentState - IsComplete: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op25, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_SetCurrentState(string id, bool complete)
        {
            WriteEvent(this.eventDescriptors[63630], id, complete);
        }

        [Event(63631, Message = "Ignoring {1} replay for listElement {2}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op26, Level = EventLevel.Verbose,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_IgnoreReplay(string id, string listElement, string scenario)
        {
            WriteEvent(this.eventDescriptors[63631], id, listElement, scenario);
        }

        [Event(63632, Message = "No current checkpoint file at {1}, recovering empty state.", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op27,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_NoCheckpoint(string id, string path)
        {
            WriteEvent(this.eventDescriptors[63632], id, path);
        }

        [Event(63633, Message = "Error reading checkpoint file: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op28, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ReadCheckpointError(string id, string exception)
        {
            WriteEvent(this.eventDescriptors[63633], id, exception);
        }

        [Event(63634, Message = "Error copying the next checkpoint frame: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op29, Level = EventLevel.Error,
#if DotNetCoreClr
			Channel = SystemEventChannel.Admin,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_CopyNextFrameError(string id, string exception)
        {
            WriteEvent(this.eventDescriptors[63634], id, exception);
        }

        [Event(63635, Message = "ApplyAsync : LSN={1} CommitLSN={2} txn={3} role={4} kind={5} operationType={6}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op30,
             Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ApplyAsync(string id, long lsn, long commitLsn, long txid, long role, long kind, short operationType)
        {
            WriteEvent(this.eventDescriptors[63635], id, lsn, commitLsn, txid, role, kind, operationType);
        }

        [Event(63636, Message = "{1}.{2} txn={3} status={4} role={5}", Task = Tasks.ReliableConcurrentQueue,
             Opcode = Opcodes.Op31, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_ThrowIfNot(string id, string kind, string api, long tracer, string status, string role)
        {
            WriteEvent(this.eventDescriptors[63636], id, kind, api, tracer, status, role);
        }

        [Event(63637, Message = "{1} => {2}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op32,
             Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_Info(string id, string api, string message)
        {
            WriteEvent(this.eventDescriptors[63637], id, api, message);
        }

        [Event(63638, Message = "UnlockSource: {1} TransactionId: {2}  ListElementId: {3}  ListElementState: {4}  TimeToUnlock: {5} ms", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op33,
           Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_Unlock(string id, short unlockSource, long transactionid, long listElementId, short listElementState, long totalTime)
        {
            WriteEvent(this.eventDescriptors[63638], id, unlockSource, transactionid, listElementId, listElementState, totalTime);
        }

        [Event(63639, Message = "LSN: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op34,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_PrepareCheckpointStarted(string id, long lsn)
        {
            WriteEvent(this.eventDescriptors[63639], id, lsn);
        }

        [Event(63640, Message = "LSN: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op35,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_PerformCheckpointStarted(string id, long lsn)
        {
            WriteEvent(this.eventDescriptors[63640], id, lsn);
        }

        [Event(63641, Message = "LSN: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op36,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_CompleteCheckpointStarted(string id, long lsn)
        {
            WriteEvent(this.eventDescriptors[63641], id, lsn);
        }

        [Event(63642, Message = "LSN: {1}, LinkedCount: {2}, DataStoreSize: {3}, IsNoop: {4}, TimeTaken: {5} ms", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op37,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_PrepareCheckpointCompleted(string id, long lsn, int linkedCount, int dataStoreSize, bool isNoop, long timeTaken)
        {
            WriteEvent(this.eventDescriptors[63642], id, lsn, linkedCount, dataStoreSize, isNoop, timeTaken);
        }

        [Event(63643, Message = "LSN: {1}, CountWritten: {2}, TimeTaken: {3} ms", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op38,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_PerformCheckpointCompleted(string id, long lsn, long countWritten, long timeTaken)
        {
            WriteEvent(this.eventDescriptors[63643], id, lsn, countWritten, timeTaken);
        }

        [Event(63644, Message = "LSN: {1}, TimeTaken: {2} ms", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op39,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_CompleteCheckpointCompleted(string id, long lsn, long timeTaken)
        {
            WriteEvent(this.eventDescriptors[63644], id, lsn, timeTaken);
        }

        [Event(63645, Message = "DataStoreSize: {1}, TimeTaken: {2} ms", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op40,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_RecoverCheckpointCompleted(string id, int dataStoreSize, long timeTaken)
        {
            WriteEvent(this.eventDescriptors[63645], id, dataStoreSize, timeTaken);
        }

        [Event(63646, Message = "Dequeue Timeout.  Transaction id: {1}", Task = Tasks.ReliableConcurrentQueue, Opcode = Opcodes.Op41,
            Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ReliableConcurrentQueue_DequeueTimeout(string id, long transactionId)
        {
            WriteEvent(this.eventDescriptors[63646], id, transactionId);
        }

#endregion

        [Event(60420, Message = "{1}", Task = Tasks.InfrastructureService, Opcode = Opcodes.Op11, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void InfrastructureServiceAggregatedEvent(string id, string message)
        {
            WriteEvent(this.eventDescriptors[60420], id, message);
        }

#region Chaos events

        [Event(64969, Message = "Found:{1}, Running:{2}, TimestampUtcOfLastStartInTicks:{3}", Task = Tasks.Testability, Opcode = Opcodes.Op11, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChaosFound(
            string id,
            bool foundChaos,
            bool isRunning,
            long timestampUtcOfLastStartInTicks)
        {
            WriteEvent(
                this.eventDescriptors[64969],
                id,
                foundChaos,
                isRunning,
                timestampUtcOfLastStartInTicks);
        }

        [Event(64997, Message = "TimestampUtcInTicks: {1}, FaultCount: {2}, FaultString: {3}", Task = Tasks.Testability, Opcode = Opcodes.Op12, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = (Keywords.Default | Keywords.CustomerInfo))]
        public void ChaosExecutingFaults(
            string id,
            long timestampUtcInTicks,
            long faultCount,
            string faultString)
        {
            WriteEvent(
                this.eventDescriptors[64997],
                id,
                timestampUtcInTicks,
                faultCount,
                faultString);
        }

        [Event(64951, Message = "{1}", Task = Tasks.Testability, Opcode = Opcodes.Op13, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = (Keywords.Default | Keywords.CustomerInfo))]
        public void ChaosEngineError(string id, string message)
        {
            WriteEvent(
                this.eventDescriptors[64951],
                id,
                message);
        }

        [Event(64937, Message = "NodeCount:{1}, AppCount:{2}, ServiceCount:{3}, PartitionCount:{4}, ReplicaCount:{5}, SnapshotTimeInSeconds:{6}, UnsafeMarkingTimeInSeconds:{7}, RetryCount:{8}", Task = Tasks.Testability, Opcode = Opcodes.Op14, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void ChaosSnapshot(
            string id,
            int nodeCount,
            int appCount,
            int serviceCount,
            int partitionCount,
            int replicaCount,
            double snapshotTimeInSeconds,
            double unsafeMarkingTimeInSeconds,
            int retryCount)
        {
            WriteEvent(
                this.eventDescriptors[64937],
                id,
                nodeCount,
                appCount,
                serviceCount,
                partitionCount,
                replicaCount,
                snapshotTimeInSeconds,
                unsafeMarkingTimeInSeconds,
                retryCount);
        }

        [Event(64927, Message = "TimeStampUtcInTicks:{1}, IsStoppedManually:{2}", Task = Tasks.Testability, Opcode = Opcodes.Op15, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = (Keywords.Default | Keywords.CustomerInfo))]
        public void ChaosStop(
            string id,
            long timeStampUtcInTicks,
            int manual)
        {
            WriteEvent(
                this.eventDescriptors[64927],
                id,
                timeStampUtcInTicks,
                manual);
        }

        [Event(64921, Message = "TimeStampUtcInTicks:{1}, MaxConcurrentFaults: {2}, TimeToRunInSeconds:{3}, MaxClusterStabilizationTimeoutInSeconds:{4}, WaitTimeBetweenIterations:{5}, WaitTimeBetweenFaults:{6}", Task = Tasks.Testability, Opcode = Opcodes.Op16, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = (Keywords.Default | Keywords.CustomerInfo))]
        public void ChaosStart(
            string id,
            long timeStampUtcInTicks,
            long maxConcurrentFaults,
            double timeToRunInSeconds,
            double maxClusterStabilizationTimeoutInSeconds,
            double waitTimeBetweenIterationsInSeconds,
            double waitTimeBetweenFaultsInSeconds)
        {
            WriteEvent(
                this.eventDescriptors[64921],
                id,
                timeStampUtcInTicks,
                maxConcurrentFaults,
                timeToRunInSeconds,
                maxClusterStabilizationTimeoutInSeconds,
                waitTimeBetweenIterationsInSeconds,
                waitTimeBetweenFaultsInSeconds);
        }

        [Event(64919, Message = "TimeStampUtcInTicks: {1}, Reason:{2}", Task = Tasks.Testability, Opcode = Opcodes.Op17, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = (Keywords.Default | Keywords.CustomerInfo))]
        public void ChaosValidationFailed(
            string id,
            long timeStampUtcInTicks,
            string reason)
        {
            WriteEvent(
                this.eventDescriptors[64919],
                id,
                timeStampUtcInTicks,
                reason);
        }

#endregion Chaos events

#region ClusterAnalysis Events

        /// <summary>
        /// When a primary replica moves, PrimaryMoveAnalysisAgent@ClusterAnalysis analyzes the traces to find out the reason
        /// for the primary move and emits this event.
        /// </summary>
        /// <param name="eventInstanceId"></param>
        /// <param name="partitionId"></param>
        /// <param name="whenMoveCompleted"></param>
        /// <param name="analysisDelayInSeconds"></param>
        /// <param name="analysisDurationInSeconds"></param>
        /// <param name="previousNode"></param>
        /// <param name="currentNode"></param>
        /// <param name="reason"></param>
        /// <param name="correlatedTraceRecords"></param>
        [Event(65003, Message = "EventInstanceId: {0}. Primary for partition {1} moved at {2}. Analysis delay: {3}. Analysis duration: {4}. PreviousNode: {5}. CurrentNode: {6}. Reason: {7}. Correlated Traces: {8}.", Task = Tasks.Testability, Opcode = Opcodes.Op18, Level = EventLevel.Informational,
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Partitions, "PartitionPrimaryMoveAnalysis", EventConstants.AnalysisCategory)]
        public void PrimaryMoveAnalysisEvent(
            Guid eventInstanceId,
            Guid partitionId,
            DateTime whenMoveCompleted,
            double analysisDelayInSeconds,
            double analysisDurationInSeconds,
            string previousNode,
            string currentNode,
            string reason,
            string correlatedTraceRecords)
        {
            WriteEvent(
                this.eventDescriptors[65003],
                eventInstanceId,
                partitionId,
                whenMoveCompleted,
                analysisDelayInSeconds,
                analysisDurationInSeconds,
                previousNode,
                currentNode,
                reason,
                correlatedTraceRecords);
        }

#endregion

#region Chaos Operational traces

        [Event(50021, Message = "EventInstanceId: {0}, MaxConcurrentFaults: {1}, TimeToRunInSeconds:{2}, MaxClusterStabilizationTimeoutInSeconds:{3}, WaitTimeBetweenIterationsInSeconds:{4}, WaitTimeBetweenFaultsInSeconds:{5}, MoveReplicaFaultEnabled: {6}, IncludedNodeTypeList: {7}, IncludedApplicationList: {8}, {9}, Context: {10}.", Task = Tasks.Testability, Opcode = Opcodes.Op22, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Cluster, "ChaosStarted", EventConstants.ChaosCategory)]
        public void ChaosStartedEvent(
            Guid eventInstanceId,
            long maxConcurrentFaults,
            double timeToRunInSeconds,
            double maxClusterStabilizationTimeoutInSeconds,
            double waitTimeBetweenIterationsInSeconds,
            double waitTimeBetweenFautlsInSeconds,
            bool moveReplicaFaultEnabled,
            string includedNodeTypeList,
            string includedApplicationList,
            string clusterHealthPolicy,
            string chaosContext
            )
        {
            WriteEvent(
                this.eventDescriptors[50021],
                eventInstanceId,
                maxConcurrentFaults,
                timeToRunInSeconds,
                maxClusterStabilizationTimeoutInSeconds,
                waitTimeBetweenIterationsInSeconds,
                waitTimeBetweenFautlsInSeconds,
                moveReplicaFaultEnabled,
                includedNodeTypeList,
                includedApplicationList,
                clusterHealthPolicy,
                chaosContext
                );
        }

        [Event(50023, Message = "EventInstanceId: {0}, StopReason: {1}.", Task = Tasks.Testability, Opcode = Opcodes.Op23, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Cluster, "ChaosStopped", EventConstants.ChaosCategory)]
        public void ChaosStoppedEvent(
            Guid eventInstanceId,
            string reason
            )
        {
            WriteEvent(
                this.eventDescriptors[50023],
                eventInstanceId,
                reason
                );
        }

        [Event(50033, Message = "EventInstanceId: {0}, NodeName: {1}, NodeInstanceId: {2}, FaultGroupId: {3}, FaultId: {4}.", Task = Tasks.Testability, Opcode = Opcodes.Op24, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Nodes, "ChaosNodeRestartScheduled", EventConstants.ChaosCategory)]
        public void ChaosRestartNodeFaultScheduledEvent(
            Guid eventInstanceId,
            string nodeName,
            long nodeInstanceId,
            Guid faultGroupId,
            Guid faultId
            )
        {
            WriteEvent(
                this.eventDescriptors[50033],
                eventInstanceId,
                nodeName,
                nodeInstanceId,
                faultGroupId,
                faultId
                );
        }

        [Event(50047, Message = "EventInstanceId: {0}, PartitionId: {1}, ReplicaId: {2}, FaultGroupId: {3}, FaultId: {4} ServiceUri: {5}.", Task = Tasks.Testability, Opcode = Opcodes.Op25, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Replicas, "ChaosReplicaRestartScheduled", EventConstants.ChaosCategory)]
        public void ChaosRestartReplicaFaultScheduledEvent(
            Guid eventInstanceId,
            Guid partitionId,
            long replicaId,
            Guid faultGroupId,
            Guid faultId,
            string serviceUri
            )
        {
            WriteEvent(
                this.eventDescriptors[50047],
                eventInstanceId,
                partitionId,
                replicaId,
                faultGroupId,
                faultId,
                serviceUri
                );
        }

        [Event(50051, Message = "EventInstanceId: {0}, PartitionId: {1}, ReplicaId: {2} FaultGroupId: {3}, FaultId: {4}, ServiceUri: {5}.", Task = Tasks.Testability, Opcode = Opcodes.Op26, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Replicas, "ChaosReplicaRemovalScheduled", EventConstants.ChaosCategory)]
        public void ChaosRemoveReplicaFaultScheduledEvent(
            Guid eventInstanceId,
            Guid partitionId,
            long replicaId,
            Guid faultGroupId,
            Guid faultId,
            string serviceUri
            )
        {
            WriteEvent(
                this.eventDescriptors[50051],
                eventInstanceId,
                partitionId,
                replicaId,
                faultGroupId,
                faultId,
                serviceUri
                );
        }

        [Event(50053, Message = "EventInstanceId: {0}, ApplicationName: {1}, FaultGroupId: {2}, FaultId: {3}, NodeName: {4}, ServiceManifestName: {5}, CodePackageName: {6}, ServicePackageActivationId: {7}.", Task = Tasks.Testability, Opcode = Opcodes.Op27, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Applications, "ChaosCodePackageRestartScheduled", EventConstants.ChaosCategory)]
        public void ChaosRestartCodePackageFaultScheduledEvent(
            Guid eventInstanceId,
            string applicationName,
            Guid faultGroupId,
            Guid faultId,
            string nodeName,
            string serviceManifestName,
            string codePackageName,
            string servicePackageActivationId
            )
        {
            WriteEvent(
                this.eventDescriptors[50053],
                eventInstanceId,
                applicationName,
                faultGroupId,
                faultId,
                nodeName,
                serviceManifestName,
                codePackageName,
                servicePackageActivationId
                );
        }

        [Event(50069, Message = "EventInstanceId: {0}, PartitionId: {1}, FaultGroupId: {2}, FaultId: {3}, ServiceUri: {4}, MoveDestinationNode: {5}, IsForcedMove: {6}.", Task = Tasks.Testability, Opcode = Opcodes.Op28, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = (Keywords.Default))]
        [EventExtendedMetadataAttribute(TableEntityKind.Partitions, "ChaosPartitionPrimaryMoveScheduled", EventConstants.ChaosCategory)]
        public void ChaosMovePrimaryFaultScheduledEvent(
            Guid eventInstanceId,
            Guid partitionId,
            Guid faultGroupId,
            Guid faultId,
            string serviceName,
            string nodeTo,
            bool forcedMove
            )
        {
            WriteEvent(
                this.eventDescriptors[50069],
                eventInstanceId,
                partitionId,
                faultGroupId,
                faultId,
                serviceName,
                nodeTo,
                forcedMove
                );
        }


        [Event(50077, Message = "EventInstanceId: {0}, PartitionId: {1}, FaultGroupId: {2}, FaultId: {3}, ServiceUri: {4}, MoveSourceNode: {5}, MoveDestinationNode: {6}, IsForcedMove: {7}", Task = Tasks.Testability, Opcode = Opcodes.Op29, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Partitions, "ChaosPartitionSecondaryMoveScheduled", EventConstants.ChaosCategory)]
        public void ChaosMoveSecondaryFaultScheduledEvent(
            Guid eventInstanceId,
            Guid partitionId,
            Guid faultGroupId,
            Guid faultId,
            string serviceName,
            string sourceNode,
            string destinationNode,
            bool forcedMove
            )
        {
            WriteEvent(
                this.eventDescriptors[50077],
                eventInstanceId,
                partitionId,
                faultGroupId,
                faultId,
                serviceName,
                sourceNode,
                destinationNode,
                forcedMove
                );
        }

#endregion

#region EventStoreReader Events

        [Event(65004, Message = "EntityType: {0}, ApiVersion: {1}, ParamStartTime: {2}, ParamEndTime {3}, ParamIsIdentifierPresent: {4}, ParamEventTypes: {5}, SkipCorrelationLookup: {6}, QueryExecutionTimeMs: {7}, queryItemCount: {8}", Task = Tasks.Testability, Opcode = Opcodes.Op19, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EventStoreQueryStat(
            string entityType,
            string apiVersion,
            DateTime paramStartTime,
            DateTime paramEndTime,
            bool paramIsIdentifierPresent,
            string paramEventTypes,
            bool skipCorrelationLookup,
            double queryExecutionTimeMs,
            int queryItemCount)
        {
            WriteEvent(this.eventDescriptors[65004], entityType, apiVersion, paramStartTime, paramEndTime, paramIsIdentifierPresent, paramEventTypes, skipCorrelationLookup, queryExecutionTimeMs, queryItemCount);
        }

        [Event(65005, Message = "UnsupportedUri. Message : {0}", Task = Tasks.Testability, Opcode = Opcodes.Op20, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EventStoreUnsupportedUri(string message)
        {
            WriteEvent(this.eventDescriptors[65005], message);
        }

        [Event(65006, Message = "Query Failure. QueryArgs: {0}, Failure: {1}", Task = Tasks.Testability, Opcode = Opcodes.Op21, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EventStoreFailed(string queryArgs, string failure)
        {
            WriteEvent(this.eventDescriptors[65006], queryArgs, failure);
        }

        [Event(65007, Message = "Cache Miss. CacheName: {0}, CacheStartTime: {1}, CacheEndTime: {2}, QueryStartTime: {3}, QueryEndTime: {4}", Task = Tasks.Testability, Opcode = Opcodes.Op33, Level = EventLevel.Warning,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EventStoreCacheMiss(string cacheName, DateTime cacheStartTime, DateTime cacheEndTime, DateTime queryStartTime, DateTime queryEndTime)
        {
            WriteEvent(this.eventDescriptors[65007], cacheName, cacheStartTime, cacheEndTime, queryStartTime, queryEndTime);
        }

        [Event(65008, Message = "Cache Telemetry. CacheName: {0}, CacheItemCount: {1}, CacheDurationHours: {2}", Task = Tasks.Testability, Opcode = Opcodes.Op36, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EventStoreCacheTelemetry(string cacheName, int cacheItemCount, double cacheDurationHours)
        {
            WriteEvent(this.eventDescriptors[65008], cacheName, cacheItemCount, cacheDurationHours);
        }

        [Event(65009, Message = "EventStore Perf telemetry. MemoryUsageMb: {0}", Task = Tasks.Testability, Opcode = Opcodes.Op37, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        public void EventStorePerfTelemetry(int memoryUsageMb)
        {
            WriteEvent(this.eventDescriptors[65009], memoryUsageMb);
        }

        #endregion

        #region Cluster Analysis Events

        [Event(65011, Message = "Correlation Id: EventInstance: {0}, RelatedFromId: {1}, RelatedFromType: {2}, RelatedToId: {3}, RelatedToType: {4}", Task = Tasks.Testability, Opcode = Opcodes.Op34, Level = EventLevel.Informational,
#if DotNetCoreClr
            Channel = SystemEventChannel.Debug,
#endif
            Keywords = Keywords.Default)]
        [EventExtendedMetadataAttribute(TableEntityKind.Correlation, "Correlation", EventConstants.CorrelationCategory)]
        public void CorrelationOperational(Guid eventInstanceId, Guid relatedFromId, string relatedFromType, Guid relatedToId, string relatedToType)
        {
            WriteEvent(this.eventDescriptors[65011], eventInstanceId, relatedFromId, relatedFromType, relatedToId, relatedToType);
        }

#endregion


#region FabricCM events

        [Event(65310, Message = "{2}", Task = Tasks.ManagedHosting, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        public void ManagedHosting_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65310], id, type, message);
        }

        [Event(65311, Message = "{2}", Task = Tasks.ManagedHosting, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational, Keywords = Keywords.Default)]
        public void ManagedHosting_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65311], id, type, message);
        }

        [Event(65312, Message = "{2}", Task = Tasks.ManagedHosting, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning, Keywords = Keywords.Default)]
        public void ManagedHosting_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65312], id, type, message);
        }

        [Event(65313, Message = "{2}", Task = Tasks.ManagedHosting, Opcode = Opcodes.Op232,
            Level = EventLevel.Error, Keywords = Keywords.Default)]
        public void ManagedHosting_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65313], id, type, message);
        }

        #endregion

        #region FabricGatewayResourceManager events

        [Event(65400, Message = "{2}", Task = Tasks.GatewayResourceManager, Opcode = Opcodes.Op233,
            Level = EventLevel.Verbose, Keywords = Keywords.Default)]
        public void GatewayResourceManager_NoiseText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65400], id, type, message);
        }

        [Event(65401, Message = "{2}", Task = Tasks.GatewayResourceManager, Opcode = Opcodes.Op230,
            Level = EventLevel.Informational, Keywords = Keywords.Default)]
        public void GatewayResourceManager_InfoText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65401], id, type, message);
        }

        [Event(65402, Message = "{2}", Task = Tasks.GatewayResourceManager, Opcode = Opcodes.Op231,
            Level = EventLevel.Warning, Keywords = Keywords.Default)]
        public void GatewayResourceManager_WarningText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65402], id, type, message);
        }

        [Event(65403, Message = "{2}", Task = Tasks.GatewayResourceManager, Opcode = Opcodes.Op232,
            Level = EventLevel.Error, Keywords = Keywords.Default)]
        public void GatewayResourceManager_ErrorText(string id, string type, string message)
        {
            WriteEvent(this.eventDescriptors[65403], id, type, message);
        }

        #endregion

        #endregion

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
                { "ManagedGeneral", FabricEvents.Tasks.ManagedGeneral },
                { "ManagementCommon", FabricEvents.Tasks.ManagementCommon },
                { "ImageStoreClient", FabricEvents.Tasks.ImageStoreClient },
                { "FabricHost", FabricEvents.Tasks.FabricHost },
                { "FabricDeployer", FabricEvents.Tasks.FabricDeployer },
                { "SystemFabricDeployer", FabricEvents.Tasks.SystemFabricDeployer },
                { "Test", FabricEvents.Tasks.Test },
                { "AzureLogCollector", FabricEvents.Tasks.AzureLogCollector },
                { "SystemFabric", FabricEvents.Tasks.SystemFabric },
                { "ImageBuilder", FabricEvents.Tasks.ImageBuilder },
                { "FabricDCA", FabricEvents.Tasks.FabricDCA },
                { "FabricHttpGateway", FabricEvents.Tasks.FabricHttpGateway },
                { "InfrastructureService", FabricEvents.Tasks.InfrastructureService },
                { "ManagedTokenValidationService", FabricEvents.Tasks.ManagedTokenValidationService },
                { "DSTSClient", FabricEvents.Tasks.DSTSClient },
                { "FabricMonSvc", FabricEvents.Tasks.FabricMonSvc },
                { "TStore", FabricEvents.Tasks.TStore },
                { "DistributedDictionary", FabricEvents.Tasks.DistributedDictionary },
                { "DistributedQueue", FabricEvents.Tasks.DistributedQueue },
                { "Wave", FabricEvents.Tasks.Wave },
                { "ReliableStream", FabricEvents.Tasks.ReliableStream },
                { "DistributedVersionedDictionary", FabricEvents.Tasks.DistributedVersionedDictionary },
                { "Testability", FabricEvents.Tasks.Testability },
                { "RandomActionGenerator", FabricEvents.Tasks.RandomActionGenerator },
                { "FabricMdsAgentSvc", FabricEvents.Tasks.FabricMdsAgentSvc },
                { "TReplicator", FabricEvents.Tasks.TReplicator },
                { "TStatefulServiceReplica", FabricEvents.Tasks.TStatefulServiceReplica },
                { "TStateManager", FabricEvents.Tasks.TStateManager },
                { "ActorFramework", FabricEvents.Tasks.ActorFramework },
                { "WRP", FabricEvents.Tasks.WRP },
                { "ServiceFramework", FabricEvents.Tasks.ServiceFramework },
                { "FaultAnalysisService", FabricEvents.Tasks.FaultAnalysisService },
                { "ApiMonitor", FabricEvents.Tasks.ApiMonitor },
                { "ReliableConcurrentQueue", FabricEvents.Tasks.ReliableConcurrentQueue },
                { "UpgradeOrchestrationService", FabricEvents.Tasks.UpgradeOrchestrationService },
                { "ManagedHosting", FabricEvents.Tasks.ManagedHosting }
            };

            return new ReadOnlyDictionary<string, EventTask>(taskMap);
        }

#region Extensions

        public class ExtensionsEvents : ExtensionsEventsInternal
        {
            /// <summary>
            /// Constructor for ExtensionsEvents
            /// </summary>
            /// <param name="code">TaskCode to be used for the events.</param>
            public ExtensionsEvents(EventTask code) : base(code)
            {
            }

            static ExtensionsEvents()
            {
                actionMap = new WriteVariants[(int)FabricEvents.Tasks.Max];
                actionMap[(int)FabricEvents.Tasks.ManagedGeneral] = new WriteVariants(Events.ManagedGeneral_InfoText, Events.ManagedGeneral_ErrorText,
                    Events.ManagedGeneral_WarningText, Events.ManagedGeneral_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ManagementCommon] = new WriteVariants(Events.ManagementCommon_InfoText, Events.ManagementCommon_ErrorText,
                    Events.ManagementCommon_WarningText, Events.ManagementCommon_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ImageStoreClient] = new WriteVariants(Events.ImageStoreClient_InfoText, Events.ImageStoreClient_ErrorText,
                    Events.ImageStoreClient_WarningText, Events.ImageStoreClient_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FabricHost] = new WriteVariants(Events.FabricHost_InfoText, Events.FabricHost_ErrorText,
                    Events.FabricHost_WarningText, Events.FabricHost_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FabricDeployer] = new WriteVariants(Events.FabricDeployer_InfoText, Events.FabricDeployer_ErrorText,
                    Events.FabricDeployer_WarningText, Events.FabricDeployer_NoiseText);
                actionMap[(int)FabricEvents.Tasks.SystemFabricDeployer] = new WriteVariants(Events.SystemFabricDeployer_InfoText, Events.SystemFabricDeployer_ErrorText,
                    Events.SystemFabricDeployer_WarningText, Events.SystemFabricDeployer_NoiseText);
                actionMap[(int)FabricEvents.Tasks.Test] = new WriteVariants(Events.Test_InfoText, Events.Test_ErrorText, Events.Test_WarningText,
                    Events.Test_NoiseText);
                actionMap[(int)FabricEvents.Tasks.AzureLogCollector] = new WriteVariants(Events.AzureLogCollector_InfoText, Events.AzureLogCollector_ErrorText,
                    Events.AzureLogCollector_WarningText, Events.AzureLogCollector_NoiseText);
                actionMap[(int)FabricEvents.Tasks.SystemFabric] = new WriteVariants(Events.SystemFabric_InfoText, Events.SystemFabric_ErrorText,
                    Events.SystemFabric_WarningText, Events.SystemFabric_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ImageBuilder] = new WriteVariants(Events.ImageBuilder_InfoText, Events.ImageBuilder_ErrorText,
                    Events.ImageBuilder_WarningText, Events.ImageBuilder_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FabricDCA] = new WriteVariants(Events.FabricDCA_InfoText, Events.FabricDCA_ErrorText,
                    Events.FabricDCA_WarningText, Events.FabricDCA_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FabricHttpGateway] = new WriteVariants(Events.FabricHttpGateway_InfoText, Events.FabricHttpGateway_ErrorText,
                    Events.FabricHttpGateway_WarningText, Events.FabricHttpGateway_NoiseText);
                actionMap[(int)FabricEvents.Tasks.InfrastructureService] = new WriteVariants(Events.InfrastructureService_InfoText,
                    Events.InfrastructureService_ErrorText, Events.InfrastructureService_WarningText,
                    Events.InfrastructureService_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ManagedTokenValidationService] = new WriteVariants(Events.ManagedTokenValidationService_InfoText,
                    Events.ManagedTokenValidationService_ErrorText, Events.ManagedTokenValidationService_WarningText,
                    Events.ManagedTokenValidationService_NoiseText);
                actionMap[(int)FabricEvents.Tasks.DSTSClient] = new WriteVariants(Events.DSTSClient_InfoText, Events.DSTSClient_ErrorText,
                    Events.DSTSClient_WarningText, Events.DSTSClient_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FabricMonSvc] = new WriteVariants(Events.FabricMonSvc_InfoText, Events.FabricMonSvc_ErrorText,
                    Events.FabricMonSvc_WarningText, Events.FabricMonSvc_NoiseText);
                actionMap[(int)FabricEvents.Tasks.TStore] = new WriteVariants(Events.TStore_InfoText, Events.TStore_ErrorText,
                    Events.TStore_WarningText, Events.TStore_NoiseText);
                actionMap[(int)FabricEvents.Tasks.DistributedDictionary] = new WriteVariants(Events.DistributedDictionary_InfoText,
                    Events.DistributedDictionary_ErrorText, Events.DistributedDictionary_WarningText,
                    Events.DistributedDictionary_NoiseText);
                actionMap[(int)FabricEvents.Tasks.DistributedQueue] = new WriteVariants(Events.DistributedQueue_InfoText, Events.DistributedQueue_ErrorText,
                    Events.DistributedQueue_WarningText, Events.DistributedQueue_NoiseText);
                actionMap[(int)FabricEvents.Tasks.Wave] = new WriteVariants(Events.Wave_InfoText, Events.Wave_ErrorText, Events.Wave_WarningText,
                    Events.Wave_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ReliableStream] = new WriteVariants(Events.ReliableStream_InfoText, Events.ReliableStream_ErrorText,
                    Events.ReliableStream_WarningText, Events.ReliableStream_NoiseText);
                actionMap[(int)FabricEvents.Tasks.DistributedVersionedDictionary] = new WriteVariants(Events.DistributedVersionedDictionary_InfoText,
                    Events.DistributedVersionedDictionary_ErrorText, Events.DistributedVersionedDictionary_WarningText,
                    Events.DistributedVersionedDictionary_NoiseText);
                actionMap[(int)FabricEvents.Tasks.Testability] = new WriteVariants(Events.Testability_InfoText, Events.Testability_ErrorText,
                    Events.Testability_WarningText, Events.Testability_NoiseText);
                actionMap[(int)FabricEvents.Tasks.RandomActionGenerator] = new WriteVariants(Events.RandomActionGenerator_InfoText,
                    Events.RandomActionGenerator_ErrorText, Events.RandomActionGenerator_WarningText,
                    Events.RandomActionGenerator_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FabricMdsAgentSvc] = new WriteVariants(Events.FabricMdsAgentSvc_InfoText, Events.FabricMdsAgentSvc_ErrorText,
                    Events.FabricMdsAgentSvc_WarningText, Events.FabricMdsAgentSvc_NoiseText);
                actionMap[(int)FabricEvents.Tasks.TReplicator] = new WriteVariants(Events.TReplicator_InfoText, Events.TReplicator_ErrorText,
                    Events.TReplicator_WarningText, Events.TReplicator_NoiseText);
                actionMap[(int)FabricEvents.Tasks.TStatefulServiceReplica] = new WriteVariants(Events.TStatefulServiceReplica_InfoText,
                    Events.TStatefulServiceReplica_ErrorText, Events.TStatefulServiceReplica_WarningText,
                    Events.TStatefulServiceReplica_NoiseText);
                actionMap[(int)FabricEvents.Tasks.TStateManager] = new WriteVariants(Events.TStateManager_InfoText, Events.TStateManager_ErrorText,
                    Events.TStateManager_WarningText, Events.TStateManager_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ActorFramework] = new WriteVariants(Events.ActorFramework_InfoText, Events.ActorFramework_ErrorText,
                    Events.ActorFramework_WarningText, Events.ActorFramework_NoiseText);
                actionMap[(int)FabricEvents.Tasks.WRP] = new WriteVariants(Events.WRP_InfoText, Events.WRP_ErrorText, Events.WRP_WarningText,
                    Events.WRP_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ServiceFramework] = new WriteVariants(Events.ServiceFramework_InfoText, Events.ServiceFramework_ErrorText,
                    Events.ServiceFramework_WarningText, Events.ServiceFramework_NoiseText);
                actionMap[(int)FabricEvents.Tasks.FaultAnalysisService] = new WriteVariants(Events.FaultAnalysisService_InfoText,
                    Events.FaultAnalysisService_ErrorText, Events.FaultAnalysisService_WarningText,
                    Events.FaultAnalysisService_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ApiMonitor] = new WriteVariants(Events.ApiMonitor_InfoText,
                    Events.ApiMonitor_ErrorText, Events.ApiMonitor_WarningText,
                    Events.ApiMonitor_NoiseText);
                actionMap[(int)FabricEvents.Tasks.UpgradeOrchestrationService] = new WriteVariants(Events.UpgradeOrchestrationService_InfoText,
                    Events.UpgradeOrchestrationService_ErrorText, Events.UpgradeOrchestrationService_WarningText,
                    Events.UpgradeOrchestrationService_NoiseText);
                actionMap[(int)FabricEvents.Tasks.BackupRestoreService] = new WriteVariants(Events.BackupRestoreService_InfoText,
                   Events.BackupRestoreService_ErrorText, Events.BackupRestoreService_WarningText,
                   Events.BackupRestoreService_NoiseText);
                actionMap[(int)FabricEvents.Tasks.BackupCopier] = new WriteVariants(Events.BackupCopier_InfoText,
                   Events.BackupCopier_ErrorText, Events.BackupCopier_WarningText,
                   Events.BackupCopier_NoiseText);
                actionMap[(int)FabricEvents.Tasks.ManagedHosting] = new WriteVariants(Events.ManagedHosting_InfoText,Events.ManagedHosting_ErrorText,
                   Events.ManagedHosting_WarningText, Events.ManagedHosting_NoiseText);
                actionMap[(int)FabricEvents.Tasks.GatewayResourceManager] = new WriteVariants(Events.GatewayResourceManager_InfoText,
                   Events.GatewayResourceManager_ErrorText, Events.GatewayResourceManager_WarningText,
                   Events.GatewayResourceManager_NoiseText);
            }

            /// <summary>
            /// Gets object instance which can be used for writing events via EventSource
            /// </summary>
            /// <param name="code">TaskCode to be used for the events.</param>
            public static ExtensionsEvents GetEventSource(EventTask code)
            {
                return new ExtensionsEvents(code);
            }
        }

#endregion

#region Keywords / Tasks / Opcodes

        public static class Keywords
        {
            public const EventKeywords Default            = (EventKeywords)0x01;
            public const EventKeywords AppInstance        = (EventKeywords)0x02;
            public const EventKeywords ForQuery           = (EventKeywords)0x04;
            public const EventKeywords CustomerInfo       = (EventKeywords)0x08;
            public const EventKeywords DataMessaging      = (EventKeywords)0x10;
            public const EventKeywords DataMessagingAll   = (EventKeywords)0x20;
        }

        //!!!
        //!!! DO NOT ADD MORE TASK CODES INTO THIS LIST AS TASK CODES ARE DISTRIBUTED IN THE RANGE OF 1-223 IN NATIVE AND 224-256 IN MANAGED
        //!!!
        public static class Tasks
        {
            /// <summary>
            /// General task for managed code.
            /// </summary>
            public const EventTask ManagedGeneral = (EventTask)224;

            /// <summary>
            /// The trace code for management common library.
            /// </summary>
            public const EventTask ManagementCommon = (EventTask)225;

            /// <summary>
            /// The trace code for the Image Store code.
            /// </summary>
            public const EventTask ImageStoreClient = (EventTask)226;

            /// <summary>
            /// The trace code for the Fabric Host code.
            /// </summary>
            public const EventTask FabricHost = (EventTask)227;

            /// <summary>
            /// The trace code for the Cluster Setup code.
            /// </summary>
            public const EventTask FabricDeployer = (EventTask)228;

            /// <summary>
            /// System Traces for Fabric Deployer
            /// </summary>
            public const EventTask SystemFabricDeployer = (EventTask)229;

            /// <summary>
            /// Used by tests which write to Fabric trace session
            /// </summary>
            public const EventTask Test = (EventTask)230;

            /// <summary>
            /// The trace code for the Azure Log Collector code.
            /// </summary>
            public const EventTask AzureLogCollector = (EventTask)231;

            /// <summary>
            /// The trace code for System.Fabric
            /// </summary>
            public const EventTask SystemFabric = (EventTask)232;

            /// <summary>
            /// The trace code for ImageBuilder
            /// </summary>
            public const EventTask ImageBuilder = (EventTask)233;

            /// <summary>
            /// The trace code for DCA
            /// </summary>
            public const EventTask FabricDCA = (EventTask)234;

            /// <summary>
            /// The trace code for FabricHttpGateway
            /// </summary>
            public const EventTask FabricHttpGateway = (EventTask)235;

            /// <summary>
            /// The trace code for InfrastructureService
            /// </summary>
            public const EventTask InfrastructureService = (EventTask)236;

            /// <summary>
            /// The trace code for ManagedTokenValidationService
            /// </summary>
            public const EventTask ManagedTokenValidationService = (EventTask)237;

            /// <summary>
            /// The trace code for DSTS Client
            /// </summary>
            public const EventTask DSTSClient = (EventTask)238;

            /// <summary>
            /// The trace code for Monitoring Service
            /// </summary>
            public const EventTask FabricMonSvc = (EventTask)239;

            /// <summary>
            /// Differential store
            /// </summary>
            public const EventTask TStore = (EventTask)240;

            /// <summary>
            /// Distributed Dictionary
            /// </summary>
            public const EventTask DistributedDictionary = (EventTask)241;

            /// <summary>
            /// Distributed Queue
            /// </summary>
            public const EventTask DistributedQueue = (EventTask)242;

            /// <summary>
            /// Stream based waves
            /// </summary>
            public const EventTask Wave = (EventTask)243;

            /// <summary>
            /// Reliable Streams
            /// </summary>
            public const EventTask ReliableStream = (EventTask)244;

            /// <summary>
            /// Distributed Versioned Dictionary
            /// </summary>
            public const EventTask DistributedVersionedDictionary = (EventTask)245;

            /// <summary>
            /// Testability
            /// </summary>
            public const EventTask Testability = (EventTask)246;

            /// <summary>
            /// RandomActionGenerator
            /// </summary>
            public const EventTask RandomActionGenerator = (EventTask)247;

            /// <summary>
            /// The trace code for MDS agent service
            /// </summary>
            /// <remarks>
            /// This task's last EventId is 63615
            /// </remarks>
            public const EventTask FabricMdsAgentSvc = (EventTask)248;

            /// <summary>
            /// Transactional replicator
            /// </summary>
            public const EventTask TReplicator = (EventTask)249;

            /// <summary>
            /// Stateful service replica that is part of the transactional replicator
            /// </summary>
            public const EventTask TStatefulServiceReplica = (EventTask)250;

            /// <summary>
            /// State Manager that is part of the transactional replicator
            /// </summary>
            public const EventTask TStateManager = (EventTask)251;

            /// <summary>
            /// Actor framework
            /// </summary>
            public const EventTask ActorFramework = (EventTask)252;

            /// <summary>
            /// State Manager that is part of the transactional replicator
            /// </summary>
            public const EventTask WRP = (EventTask)253;

            /// <summary>
            /// Service Framework (fabsrv).  Shares EventId space with ApiMonitor. Ends at 65151.
            /// </summary>
            public const EventTask ServiceFramework = (EventTask)254;

            /// <summary>
            /// Fabric Analysis Service
            /// </summary>
            public const EventTask FaultAnalysisService = (EventTask)255;

            /// <summary>
            /// API Monitor. Shares EventId space with ServiceFramework. Starts at 65152.
            /// </summary>
            public const EventTask ApiMonitor = (EventTask)256;

            /// <summary>
            /// ReliableConcurrentQueue
            /// </summary>
            /// <remarks>
            /// Task range is shared with FabricMdsAgentService and starts at 63616
            /// </remarks>
            public const EventTask ReliableConcurrentQueue = (EventTask)257;

            /// <summary>
            /// UpgradeOrchestrationService
            /// </summary>
            /// <remarks>
            /// Task range starts at 65284
            /// </remarks>
            public const EventTask UpgradeOrchestrationService = (EventTask)258;

            /// <summary>
            /// BackupRestoreService
            /// </summary>
            public const EventTask BackupRestoreService = (EventTask)259;

            /// <summary>
            /// BackupCopier
            /// </summary>
            public const EventTask BackupCopier = (EventTask)260;

            /// <summary>
            /// Used by FabricCAS (Fabric Container Activator Service)
            /// </summary>
            public const EventTask ManagedHosting = (EventTask)261;

            /// <summary>
            /// Used by GatewayResourceManager
            /// </summary>
            public const EventTask GatewayResourceManager = (EventTask)262;

            /// <summary>
            /// All valid task id must be below this number. Increase when adding tasks.
            /// Max value is 2**16.
            /// </summary>
            public const EventTask Max = (EventTask)263;
        }

        // Eventsource mandates that all events should have unique taskcode/opcode pair. Since taskcode is shared across module we are defining Opcodes here to make the pair unique.
        // Eventsource also limites range for user defined Opcodes within the range of 11-238
        public static class Opcodes
        {
            public const EventOpcode Op11 = (EventOpcode)11;
            public const EventOpcode Op12 = (EventOpcode)12;
            public const EventOpcode Op13 = (EventOpcode)13;
            public const EventOpcode Op14 = (EventOpcode)14;
            public const EventOpcode Op15 = (EventOpcode)15;
            public const EventOpcode Op16 = (EventOpcode)16;
            public const EventOpcode Op17 = (EventOpcode)17;
            public const EventOpcode Op18 = (EventOpcode)18;
            public const EventOpcode Op19 = (EventOpcode)19;
            public const EventOpcode Op20 = (EventOpcode)20;
            public const EventOpcode Op21 = (EventOpcode)21;
            public const EventOpcode Op22 = (EventOpcode)22;
            public const EventOpcode Op23 = (EventOpcode)23;
            public const EventOpcode Op24 = (EventOpcode)24;
            public const EventOpcode Op25 = (EventOpcode)25;
            public const EventOpcode Op26 = (EventOpcode)26;
            public const EventOpcode Op27 = (EventOpcode)27;
            public const EventOpcode Op28 = (EventOpcode)28;
            public const EventOpcode Op29 = (EventOpcode)29;
            public const EventOpcode Op30 = (EventOpcode)30;
            public const EventOpcode Op31 = (EventOpcode)31;
            public const EventOpcode Op32 = (EventOpcode)32;
            public const EventOpcode Op33 = (EventOpcode)33;
            public const EventOpcode Op34 = (EventOpcode)34;
            public const EventOpcode Op35 = (EventOpcode)35;
            public const EventOpcode Op36 = (EventOpcode)36;
            public const EventOpcode Op37 = (EventOpcode)37;
            public const EventOpcode Op38 = (EventOpcode)38;
            public const EventOpcode Op39 = (EventOpcode)39;
            public const EventOpcode Op40 = (EventOpcode)40;
            public const EventOpcode Op41 = (EventOpcode)41;
            public const EventOpcode Op42 = (EventOpcode)42;
            public const EventOpcode Op43 = (EventOpcode)43;
            public const EventOpcode Op44 = (EventOpcode)44;
            public const EventOpcode Op45 = (EventOpcode)45;
            public const EventOpcode Op46 = (EventOpcode)46;
            public const EventOpcode Op47 = (EventOpcode)47;
            public const EventOpcode Op48 = (EventOpcode)48;
            public const EventOpcode Op49 = (EventOpcode)49;
            public const EventOpcode Op50 = (EventOpcode)50;
            public const EventOpcode Op51 = (EventOpcode)51;
            public const EventOpcode Op52 = (EventOpcode)52;
            public const EventOpcode Op53 = (EventOpcode)53;
            public const EventOpcode Op54 = (EventOpcode)54;
            public const EventOpcode Op55 = (EventOpcode)55;
            public const EventOpcode Op56 = (EventOpcode)56;
            public const EventOpcode Op57 = (EventOpcode)57;
            public const EventOpcode Op58 = (EventOpcode)58;
            public const EventOpcode Op59 = (EventOpcode)59;
            public const EventOpcode Op60 = (EventOpcode)60;
            public const EventOpcode Op61 = (EventOpcode)61;
            public const EventOpcode Op62 = (EventOpcode)62;
            public const EventOpcode Op63 = (EventOpcode)63;
            public const EventOpcode Op64 = (EventOpcode)64;
            public const EventOpcode Op65 = (EventOpcode)65;
            public const EventOpcode Op66 = (EventOpcode)66;
            public const EventOpcode Op67 = (EventOpcode)67;
            public const EventOpcode Op68 = (EventOpcode)68;
            public const EventOpcode Op69 = (EventOpcode)69;
            public const EventOpcode Op70 = (EventOpcode)70;
            public const EventOpcode Op71 = (EventOpcode)71;
            public const EventOpcode Op72 = (EventOpcode)72;
            public const EventOpcode Op73 = (EventOpcode)73;
            public const EventOpcode Op74 = (EventOpcode)74;
            public const EventOpcode Op75 = (EventOpcode)75;
            public const EventOpcode Op76 = (EventOpcode)76;
            public const EventOpcode Op77 = (EventOpcode)77;
            public const EventOpcode Op78 = (EventOpcode)78;
            public const EventOpcode Op79 = (EventOpcode)79;
            public const EventOpcode Op80 = (EventOpcode)80;
            public const EventOpcode Op81 = (EventOpcode)81;
            public const EventOpcode Op82 = (EventOpcode)82;
            public const EventOpcode Op83 = (EventOpcode)83;
            public const EventOpcode Op84 = (EventOpcode)84;
            public const EventOpcode Op85 = (EventOpcode)85;
            public const EventOpcode Op86 = (EventOpcode)86;
            public const EventOpcode Op87 = (EventOpcode)87;
            public const EventOpcode Op88 = (EventOpcode)88;
            public const EventOpcode Op89 = (EventOpcode)89;
            public const EventOpcode Op90 = (EventOpcode)90;
            public const EventOpcode Op91 = (EventOpcode)91;
            public const EventOpcode Op92 = (EventOpcode)92;
            public const EventOpcode Op93 = (EventOpcode)93;
            public const EventOpcode Op94 = (EventOpcode)94;
            public const EventOpcode Op95 = (EventOpcode)95;
            public const EventOpcode Op96 = (EventOpcode)96;
            public const EventOpcode Op97 = (EventOpcode)97;
            public const EventOpcode Op98 = (EventOpcode)98;
            public const EventOpcode Op99 = (EventOpcode)99;
            public const EventOpcode Op100 = (EventOpcode)100;
            public const EventOpcode Op101 = (EventOpcode)101;
            public const EventOpcode Op102 = (EventOpcode)102;
            public const EventOpcode Op103 = (EventOpcode)103;
            public const EventOpcode Op104 = (EventOpcode)104;
            public const EventOpcode Op105 = (EventOpcode)105;
            public const EventOpcode Op106 = (EventOpcode)106;
            public const EventOpcode Op107 = (EventOpcode)107;
            public const EventOpcode Op108 = (EventOpcode)108;
            public const EventOpcode Op109 = (EventOpcode)109;
            public const EventOpcode Op110 = (EventOpcode)110;
            public const EventOpcode Op111 = (EventOpcode)111;
            public const EventOpcode Op112 = (EventOpcode)112;
            public const EventOpcode Op113 = (EventOpcode)113;
            public const EventOpcode Op114 = (EventOpcode)114;
            public const EventOpcode Op115 = (EventOpcode)115;
            public const EventOpcode Op116 = (EventOpcode)116;
            public const EventOpcode Op117 = (EventOpcode)117;
            public const EventOpcode Op118 = (EventOpcode)118;
            public const EventOpcode Op119 = (EventOpcode)119;
            public const EventOpcode Op120 = (EventOpcode)120;
            public const EventOpcode Op121 = (EventOpcode)121;
            public const EventOpcode Op122 = (EventOpcode)122;
            public const EventOpcode Op123 = (EventOpcode)123;
            public const EventOpcode Op124 = (EventOpcode)124;
            public const EventOpcode Op125 = (EventOpcode)125;
            public const EventOpcode Op126 = (EventOpcode)126;
            public const EventOpcode Op127 = (EventOpcode)127;
            public const EventOpcode Op128 = (EventOpcode)128;
            public const EventOpcode Op129 = (EventOpcode)129;
            public const EventOpcode Op130 = (EventOpcode)130;
            public const EventOpcode Op131 = (EventOpcode)131;
            public const EventOpcode Op132 = (EventOpcode)132;
            public const EventOpcode Op133 = (EventOpcode)133;
            public const EventOpcode Op134 = (EventOpcode)134;
            public const EventOpcode Op135 = (EventOpcode)135;
            public const EventOpcode Op136 = (EventOpcode)136;
            public const EventOpcode Op137 = (EventOpcode)137;
            public const EventOpcode Op138 = (EventOpcode)138;
            public const EventOpcode Op139 = (EventOpcode)139;
            public const EventOpcode Op140 = (EventOpcode)140;
            public const EventOpcode Op141 = (EventOpcode)141;
            public const EventOpcode Op142 = (EventOpcode)142;
            public const EventOpcode Op143 = (EventOpcode)143;
            public const EventOpcode Op144 = (EventOpcode)144;
            public const EventOpcode Op145 = (EventOpcode)145;
            public const EventOpcode Op146 = (EventOpcode)146;
            public const EventOpcode Op147 = (EventOpcode)147;
            public const EventOpcode Op148 = (EventOpcode)148;
            public const EventOpcode Op149 = (EventOpcode)149;
            public const EventOpcode Op150 = (EventOpcode)150;
            public const EventOpcode Op151 = (EventOpcode)151;
            public const EventOpcode Op152 = (EventOpcode)152;
            public const EventOpcode Op153 = (EventOpcode)153;
            public const EventOpcode Op154 = (EventOpcode)154;
            public const EventOpcode Op155 = (EventOpcode)155;
            public const EventOpcode Op156 = (EventOpcode)156;
            public const EventOpcode Op157 = (EventOpcode)157;
            public const EventOpcode Op158 = (EventOpcode)158;
            public const EventOpcode Op159 = (EventOpcode)159;
            public const EventOpcode Op160 = (EventOpcode)160;
            public const EventOpcode Op161 = (EventOpcode)161;
            public const EventOpcode Op162 = (EventOpcode)162;
            public const EventOpcode Op163 = (EventOpcode)163;
            public const EventOpcode Op164 = (EventOpcode)164;
            public const EventOpcode Op165 = (EventOpcode)165;
            public const EventOpcode Op166 = (EventOpcode)166;
            public const EventOpcode Op167 = (EventOpcode)167;
            public const EventOpcode Op168 = (EventOpcode)168;
            public const EventOpcode Op169 = (EventOpcode)169;
            public const EventOpcode Op170 = (EventOpcode)170;
            public const EventOpcode Op171 = (EventOpcode)171;
            public const EventOpcode Op172 = (EventOpcode)172;
            public const EventOpcode Op173 = (EventOpcode)173;
            public const EventOpcode Op174 = (EventOpcode)174;
            public const EventOpcode Op175 = (EventOpcode)175;
            public const EventOpcode Op176 = (EventOpcode)176;
            public const EventOpcode Op177 = (EventOpcode)177;
            public const EventOpcode Op178 = (EventOpcode)178;
            public const EventOpcode Op179 = (EventOpcode)179;
            public const EventOpcode Op180 = (EventOpcode)180;
            public const EventOpcode Op181 = (EventOpcode)181;
            public const EventOpcode Op182 = (EventOpcode)182;
            public const EventOpcode Op183 = (EventOpcode)183;
            public const EventOpcode Op184 = (EventOpcode)184;
            public const EventOpcode Op185 = (EventOpcode)185;
            public const EventOpcode Op186 = (EventOpcode)186;
            public const EventOpcode Op187 = (EventOpcode)187;
            public const EventOpcode Op188 = (EventOpcode)188;
            public const EventOpcode Op189 = (EventOpcode)189;
            public const EventOpcode Op190 = (EventOpcode)190;
            public const EventOpcode Op191 = (EventOpcode)191;
            public const EventOpcode Op192 = (EventOpcode)192;
            public const EventOpcode Op193 = (EventOpcode)193;
            public const EventOpcode Op194 = (EventOpcode)194;
            public const EventOpcode Op195 = (EventOpcode)195;
            public const EventOpcode Op196 = (EventOpcode)196;
            public const EventOpcode Op197 = (EventOpcode)197;
            public const EventOpcode Op198 = (EventOpcode)198;
            public const EventOpcode Op199 = (EventOpcode)199;
            public const EventOpcode Op200 = (EventOpcode)200;
            public const EventOpcode Op201 = (EventOpcode)201;
            public const EventOpcode Op202 = (EventOpcode)202;
            public const EventOpcode Op203 = (EventOpcode)203;
            public const EventOpcode Op204 = (EventOpcode)204;
            public const EventOpcode Op205 = (EventOpcode)205;
            public const EventOpcode Op206 = (EventOpcode)206;
            public const EventOpcode Op207 = (EventOpcode)207;
            public const EventOpcode Op208 = (EventOpcode)208;
            public const EventOpcode Op209 = (EventOpcode)209;
            public const EventOpcode Op210 = (EventOpcode)210;
            public const EventOpcode Op211 = (EventOpcode)211;
            public const EventOpcode Op212 = (EventOpcode)212;
            public const EventOpcode Op213 = (EventOpcode)213;
            public const EventOpcode Op214 = (EventOpcode)214;
            public const EventOpcode Op215 = (EventOpcode)215;
            public const EventOpcode Op216 = (EventOpcode)216;
            public const EventOpcode Op217 = (EventOpcode)217;
            public const EventOpcode Op218 = (EventOpcode)218;
            public const EventOpcode Op219 = (EventOpcode)219;
            public const EventOpcode Op220 = (EventOpcode)220;
            public const EventOpcode Op221 = (EventOpcode)221;
            public const EventOpcode Op222 = (EventOpcode)222;
            public const EventOpcode Op223 = (EventOpcode)223;
            public const EventOpcode Op224 = (EventOpcode)224;
            public const EventOpcode Op225 = (EventOpcode)225;
            public const EventOpcode Op226 = (EventOpcode)226;
            public const EventOpcode Op227 = (EventOpcode)227;
            public const EventOpcode Op228 = (EventOpcode)228;
            public const EventOpcode Op229 = (EventOpcode)229;
            public const EventOpcode Op230 = (EventOpcode)230;
            public const EventOpcode Op231 = (EventOpcode)231;
            public const EventOpcode Op232 = (EventOpcode)232;
            public const EventOpcode Op233 = (EventOpcode)233;
            public const EventOpcode Op234 = (EventOpcode)234;
            public const EventOpcode Op235 = (EventOpcode)235;
            public const EventOpcode Op236 = (EventOpcode)236;
            public const EventOpcode Op237 = (EventOpcode)237;
            public const EventOpcode Op238 = (EventOpcode)238;
        }

#endregion
    }
}