// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.IO;
    using System.Reflection;
    using Tools.EtlReader;

    // This class processes ETW events that contain information about the application
    // instances on the node
    internal class AppInstanceEtwDataReader
    {
        // Constants
        private const string TraceType = "AppInstanceEtwDataReader";
        private const string WindowsFabricManifestFileName = "Microsoft-ServiceFabric-Events.man";
        private const string DcaTaskName = "FabricDCA";
        private const string HostingTaskName = "Hosting";
        private const string ServicePackageActivatedEventName = "ServicePackageActivatedNotifyDca";
        private const string ServicePackageDeactivatedEventName = "ServicePackageDeactivatedNotifyDca";
        private const string ServicePackageUpgradedEventName = "ServicePackageUpgradedNotifyDca";
        private const string ServicePackageInactiveEventName = "ServicePackageInactive";
        private const int MaxServicePackageActivatedEventVersion = 0;
        private const int MaxServicePackageDeactivatedEventVersion = 0;
        private const int MaxServicePackageUpgradedEventVersion = 0;

        // Manifest cache for the ETL reader library
        private ManifestCache manifestCache;

        internal AppInstanceEtwDataReader(
            ServicePackageTableManager.AddOrUpdateHandler addOrUpdateServicePackageHandler,
            ServicePackageTableManager.RemoveHandler removeServicePackageHandler,
            ManifestCache manifestCache)
        {
            this.AddOrUpdateServicePackageHandler = addOrUpdateServicePackageHandler;
            this.RemoveServicePackageHandler = removeServicePackageHandler;
            this.manifestCache = manifestCache;
        }

        protected AppInstanceEtwDataReader(
                     ServicePackageTableManager.AddOrUpdateHandler addOrUpdateServicePackageHandler,
                     ServicePackageTableManager.RemoveHandler removeServicePackageHandler)
        {
            this.LoadWindowsFabricEtwManifest();
            this.AddOrUpdateServicePackageHandler = addOrUpdateServicePackageHandler;
            this.RemoveServicePackageHandler = removeServicePackageHandler;
            this.ResetMostRecentEtwEventTimestamp();
        }

        // Timestamp of the latest ETW event that we have processed.
        protected EtwEventTimestamp LatestProcessedEtwEventTimestamp { get; set; }

        // Method to invoke when a service package needs to be added or updated
        protected ServicePackageTableManager.AddOrUpdateHandler AddOrUpdateServicePackageHandler { get; set; }

        // Method to invoke when a service package needs to be added or updated
        protected ServicePackageTableManager.RemoveHandler RemoveServicePackageHandler { get; set; }

        // Timestamp of the ETW event that we received most recently
        protected EtwEventTimestamp MostRecentEtwEventTimestamp { get; set; }

        // Flag that indicates whether the DCA is stopping
        protected bool Stopping { get; set; }

        protected void ResetMostRecentEtwEventTimestamp()
        {
            this.MostRecentEtwEventTimestamp = new EtwEventTimestamp { Differentiator = 0, Timestamp = DateTime.MinValue };
        }

        protected void OnEtwEventReceived(object sender, EventRecordEventArgs e)
        {
            if (this.Stopping)
            {
                // DCA is stopping. Don't process any more events.
                return;
            }

            DateTime eventDateTime = DateTime.FromFileTimeUtc(e.Record.EventHeader.TimeStamp);
            EtwEventTimestamp eventTimestamp = new EtwEventTimestamp();
            eventTimestamp.Timestamp = eventDateTime;
            if (eventDateTime.Equals(this.MostRecentEtwEventTimestamp.Timestamp))
            {
                // This event has the same timestamp as the previous one. Increment the
                // differentiator so that we can keep track of how far we've read.
                eventTimestamp.Differentiator = this.MostRecentEtwEventTimestamp.Differentiator + 1;
            }
            else
            {
                // This event has a later timestamp than the previous one. Initialize
                // the differentiator to 0.
                Debug.Assert(
                    eventDateTime.CompareTo(
                        this.MostRecentEtwEventTimestamp.Timestamp) > 0,
                        "Event timestamps should only move forward in time.");
                eventTimestamp.Differentiator = 0;
            }

            Utility.TraceSource.WriteInfo(
                TraceType,
                "ETW event received. Timestamp: {0} ({1}, {2}).",
                eventTimestamp.Timestamp,
                eventTimestamp.Timestamp.Ticks,
                eventTimestamp.Differentiator);

            if (eventTimestamp.CompareTo(this.LatestProcessedEtwEventTimestamp) > 0)
            {
                // This event occurred after our last processed event. So we need to
                // process it.
                this.ProcessEtwEvent(e.Record, eventTimestamp);

                // Update our latest processed event
                this.LatestProcessedEtwEventTimestamp = eventTimestamp;
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "The latest timestamp upto which we have processed ETW events is {0} ({1}, {2}).",
                    this.LatestProcessedEtwEventTimestamp.Timestamp,
                    this.LatestProcessedEtwEventTimestamp.Timestamp.Ticks,
                    this.LatestProcessedEtwEventTimestamp.Differentiator);
            }
            else
            {
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Ignoring an ETW event because its timestamp {0} ({1}, {2}) is older than the latest timestamp {3} ({4}, {5}) that we have already processed. Ignored event ID: {6}.",
                    eventTimestamp.Timestamp,
                    eventTimestamp.Timestamp.Ticks,
                    eventTimestamp.Differentiator,
                    this.LatestProcessedEtwEventTimestamp.Timestamp,
                    this.LatestProcessedEtwEventTimestamp.Timestamp.Ticks,
                    this.LatestProcessedEtwEventTimestamp.Differentiator,
                    e.Record.EventHeader.EventDescriptor.Id);
            }

            this.MostRecentEtwEventTimestamp = eventTimestamp;
        }

        private void LoadWindowsFabricEtwManifest()
        {
            // Figure out where the ETW manifests are located
            string assemblyLocation = typeof(AppInstanceEtwDataReader).GetTypeInfo().Assembly.Location;
            string manifestFileDirectory = Path.GetDirectoryName(assemblyLocation);

            // Get the Windows Fabric manifest file
            string manifestFile = Path.Combine(
                                      manifestFileDirectory,
                                      WindowsFabricManifestFileName);

            // Load the manifest
            this.manifestCache = new ManifestCache(Utility.DcaWorkFolder);
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        this.manifestCache.LoadManifest(
                            manifestFile);
                    });
                Utility.TraceSource.WriteInfo(
                    TraceType,
                    "Loaded manifest {0}",
                    manifestFile);
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteExceptionAsError(
                    TraceType,
                    e,
                    "Failed to load manifest {0}.",
                    manifestFile);
            }
        }

        private void ProcessEtwEvent(EventRecord eventRecord, EtwEventTimestamp eventTimestamp)
        {
            try
            {
                // Get the event definition
                EventDefinition eventDefinition = this.manifestCache.GetEventDefinition(
                                                      eventRecord);

                // Ignore event source manifests and heartbeat events
                if ((eventDefinition == null && eventRecord.EventHeader.EventDescriptor.Id == 0) || 
                    EventSourceHelper.CheckForDynamicManifest(eventRecord.EventHeader.EventDescriptor))
                {
                    return;
                }

                bool unexpectedEvent = false;
                if (eventDefinition.TaskName.Equals(HostingTaskName))
                {
                    // Verify event name
                    if (eventDefinition.EventName.Equals(ServicePackageActivatedEventName))
                    {
                        this.ProcessServicePackageActiveEvent(
                            eventRecord,
                            eventDefinition.TaskName,
                            eventDefinition.EventName,
                            eventTimestamp,
                            MaxServicePackageActivatedEventVersion);
                    }
                    else if (eventDefinition.EventName.Equals(ServicePackageUpgradedEventName))
                    {
                        this.ProcessServicePackageActiveEvent(
                            eventRecord,
                            eventDefinition.TaskName,
                            eventDefinition.EventName,
                            eventTimestamp,
                            MaxServicePackageUpgradedEventVersion);
                    }
                    else if (eventDefinition.EventName.Equals(ServicePackageDeactivatedEventName))
                    {
                        this.ProcessServicePackageInactiveEvent(
                            eventRecord,
                            eventDefinition.TaskName,
                            eventDefinition.EventName,
                            eventTimestamp,
                            false);
                    }
                    else
                    {
                        unexpectedEvent = true;
                    }
                }
                else if (eventDefinition.TaskName.Equals(DcaTaskName))
                {
                    if (eventDefinition.EventName.Equals(ServicePackageInactiveEventName))
                    {
                        this.ProcessServicePackageInactiveEvent(
                            eventRecord,
                            eventDefinition.TaskName,
                            eventDefinition.EventName,
                            eventTimestamp,
                            true);
                    }
                    else
                    {
                        unexpectedEvent = true;
                    }
                }
                else
                {
                    unexpectedEvent = true;
                }

                if (unexpectedEvent)
                {
                    Utility.TraceSource.WriteWarning(
                         TraceType,
                         "Unexpected event (task name {0}, event name {1}) encountered. Event will be ignored.",
                         eventDefinition.TaskName,
                         eventDefinition.EventName);
                }
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteError(
                    TraceType,
                    "Exception encountered while processing event with ID {0}, Task {1}. Exception information: {2}.",
                    eventRecord.EventHeader.EventDescriptor.Id,
                    eventRecord.EventHeader.EventDescriptor.Task,
                    e);
            }
        }

        private void ProcessServicePackageActiveEvent(EventRecord eventRecord, string taskName, string eventName, EtwEventTimestamp eventTimestamp, int maxVersion)
        {
            ApplicationDataReader reader = new ApplicationDataReader(
                                                   eventRecord.UserData,
                                                   eventRecord.UserDataLength);

            // Verify event version
            int eventVersion = reader.ReadInt32();
            if (eventVersion > maxVersion)
            {
                Utility.TraceSource.WriteError(
                     TraceType,
                     "Unexpected version {0} encountered for event name {1}. Event will be ignored.",
                     eventVersion,
                     eventName);
                return;
            }

            // Get the node name
            string nodeName = reader.ReadUnicodeString();

            // Get the root directory for the run layout
            string runLayoutRoot = reader.ReadUnicodeString();

            // Get the application instance ID
            string appInstanceId = reader.ReadUnicodeString();

            // Get the application rollout version
            string appRolloutVersion = reader.ReadUnicodeString();

            // Get the service package name
            string servicePackageName = reader.ReadUnicodeString();

            // Get the service package rollout version
            string serviceRolloutVersion = reader.ReadUnicodeString();

            // Create a record for the application data table
            ServicePackageTableRecord record = new ServicePackageTableRecord(
                nodeName,
                appInstanceId,
                appRolloutVersion,
                servicePackageName,
                serviceRolloutVersion,
                runLayoutRoot,
                default(DateTime));

            Utility.TraceSource.WriteInfo(
                TraceType,
                "ETW event received. Task {0}, event {1}, node name: {2}, application instance ID: {3}, application rollout version: {4}, service package name: {5}, service rollout version: {6}.",
                taskName,
                eventName,
                nodeName,
                record.ApplicationInstanceId,
                record.ApplicationRolloutVersion,
                record.ServicePackageName,
                record.ServiceRolloutVersion);

            // Add or update record in service package table
            this.AddOrUpdateServicePackageHandler(
                nodeName,
                appInstanceId,
                eventTimestamp.Timestamp,
                servicePackageName,
                record,
                eventTimestamp,
                true);
        }

        private void ProcessServicePackageInactiveEvent(
                        EventRecord eventRecord,
                        string taskName,
                        string eventName,
                        EtwEventTimestamp eventTimestamp,
                        bool implicitlyInactive)
        {
            ApplicationDataReader reader = new ApplicationDataReader(
                                                   eventRecord.UserData,
                                                   eventRecord.UserDataLength);

            // Verify event version
            int eventVersion = reader.ReadInt32();
            if (eventVersion > MaxServicePackageDeactivatedEventVersion)
            {
                Utility.TraceSource.WriteError(
                     TraceType,
                     "Unexpected version {0} encountered for event name {1}. Event will be ignored.",
                     eventVersion,
                     eventName);
                return;
            }

            // Get the node name
            string nodeName = reader.ReadUnicodeString();

            // Get the application instance ID
            string appInstanceId = reader.ReadUnicodeString();

            // Get the service package name
            string servicePackageName = reader.ReadUnicodeString();

            Utility.TraceSource.WriteInfo(
                TraceType,
                "ETW event received. Task {0}, event {1}, node name: {2}, application instance ID: {3}, service package name: {4}.",
                taskName,
                eventName,
                nodeName,
                appInstanceId,
                servicePackageName);

            // Delete record from service package table
            this.RemoveServicePackageHandler(nodeName, appInstanceId, servicePackageName, eventTimestamp, true, implicitlyInactive);
        }
    }
}