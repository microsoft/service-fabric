// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.IO;
    using TelemetryAggregation;
    using Tools.EtlReader;

#if !DotNetCoreClrLinux && !DotNetCoreClrIOT
    internal class TelemetryEtwSink : IEtlFileSink, IEtlInMemorySink, IDisposable
    {
#else
    internal class TelemetryEtwSink : IEtlFileSink, IDisposable
    {
#endif
        private const string LogSourceId = "TelemetryEtwSink";
        private const string TelemetryPersistFileName = "tfile.json";
        private const string TelemetryWorkSubDirectoryKey = "Telemetry";
        private const string AggregationPersistenceFolder = "AggregationPersistence";

        private readonly TelemetryFilter telemetryFilter;

        private readonly TelemetryPerformanceInstrumentation telemetryPerformanceInstrumentation;
        private readonly ITelemetryWriter telemetryWriter;
        private readonly object telemetryLock;

        // collection of the traces and aggregations to send as telemetry
        private readonly TelemetryCollection telemetryCollection;

        // contains the information to uniquely identify a cluster, node
        private readonly TelemetryIdentifiers telemetryIdentifier;

        // contains information about the environment such as clusterType, memory, cpus, etc.
        private readonly EnvironmentTelemetry environmentTelemetry;

        // contains the file path to where telemetry is persisted
        private string telemetryPersistFilePath;

        private System.Threading.Timer telemetrySendTimer;

        // ETW manifest cache. Used for decoding ETW events.
        private ManifestCache manifestCache;

        public TelemetryEtwSink(
            ITelemetryWriter telemetryWriter,
            string clusterId,
            string tenantId,
            string clusterType,
            string serviceFabricVersion, 
            string nodeName, 
            bool isScaleMin, 
            TelemetryScheduler telemetryScheduler)
        {
            string clusterDevType = isScaleMin.ToString();

            this.telemetryWriter = telemetryWriter;

            // creating filter to only aggregate traces which are white-listed
            this.telemetryFilter = new TelemetryFilter();

            // creating the unique identifier of the cluster/node
            this.telemetryIdentifier = new TelemetryIdentifiers(clusterId, tenantId, nodeName);

            // getting path to where telemetry will be persisted
            Utility.CreateWorkSubDirectory(
                Utility.TraceSource,
                LogSourceId,
                TelemetryWorkSubDirectoryKey,
                AggregationPersistenceFolder,
                Utility.DcaWorkFolder,
                out this.telemetryPersistFilePath);

            this.telemetryPersistFilePath = Path.Combine(this.telemetryPersistFilePath, TelemetryEtwSink.TelemetryPersistFileName);

            // initializing the collection of telemetry (first it tries to load it from file, if fails an empty collection is created)
            bool telCollectionloadedFromDisk;
            this.telemetryCollection = TelemetryCollection.LoadOrCreateTelemetryCollection(this.telemetryPersistFilePath, this.telemetryIdentifier, telemetryScheduler.DailyPushes, out telCollectionloadedFromDisk, Utility.TraceSource.WriteError);

            // updating the scheduling for sending telemetry based on the persisted (or not persisted) telemetryCollection.
            telemetryScheduler.UpdateScheduler(telCollectionloadedFromDisk, this.telemetryCollection.TotalPushes > 0, this.telemetryCollection.PushTime, DateTime.Now.ToLocalTime().TimeOfDay);
            this.telemetryCollection.PushTime = telemetryScheduler.PushTime;

            // creating environment telemetry and including it in the collection
            this.environmentTelemetry = new EnvironmentTelemetry(
                clusterType,
                serviceFabricVersion,
                clusterDevType);

            this.telemetryCollection.Aggregate(this.environmentTelemetry);

            this.telemetryPerformanceInstrumentation = new TelemetryPerformanceInstrumentation();
            this.telemetryCollection.Aggregate(this.telemetryPerformanceInstrumentation);

            this.telemetryLock = new object();

            Utility.TraceSource.WriteInfo(LogSourceId, "{0}, Initial send telemetry delay of {1} at a frequency of every {2} minutes.", this.telemetryIdentifier, telemetryScheduler.SendDelay, telemetryScheduler.SendInterval.TotalMinutes);
            this.telemetrySendTimer = new System.Threading.Timer(this.SendTelemetry, null, telemetryScheduler.SendDelay, telemetryScheduler.SendInterval);
        }

        public bool NeedsDecodedEvent
        {
            get
            {
                return false;
            }
        }

        public void SetEtwManifestCache(Tools.EtlReader.ManifestCache manifestCache)
        {
            this.manifestCache = manifestCache;
        }

        public void OnEtwEventsAvailable(IEnumerable<EventRecord> etwEvents)
        {
            lock (this.telemetryLock)
            {
                this.telemetryPerformanceInstrumentation.ProcessingEventsMeasuredTimeStart();

                foreach (EventRecord eventRecord in etwEvents)
                {
                    this.AggregateTelemetry(eventRecord);
                }

                this.telemetryPerformanceInstrumentation.ProcessingEventsMeasuredTimeStop();
            }
        }

        public void OnEtwEventsAvailable(IEnumerable<DecodedEtwEvent> etwEvents)
        {
            // no work
        }

        // The telemetry client needs to be flushed explicitly otherwise the buffered events will be lost
        public void Dispose()
        {
            this.telemetryWriter.Dispose();
        }

        public void OnEtwEventProcessingPeriodStart()
        {
            // throw new NotImplementedException();
        }

        public void OnEtwEventProcessingPeriodStop()
        {
            lock (this.telemetryLock)
            {
                // tracking the peformance metrics for telemetry before persisting colletion to file
                this.telemetryCollection.Aggregate(this.telemetryPerformanceInstrumentation);

                // Finished aggregating data, lets persist it
                this.telemetryCollection.SaveToFile(this.telemetryPersistFilePath, Utility.TraceSource.WriteError);
            }
        }

        private void SendTelemetry(object stateInfo)
        {
            lock (this.telemetryLock)
            {
                Utility.TraceSource.WriteInfo(
                    LogSourceId, 
                    "Sending {0} telemetry items. Batch {1} of {2}.",
                    this.telemetryCollection.Count + 1,
                    this.telemetryCollection.TelemetryBatchNumber,
                    this.telemetryCollection.DailyNumberOfTelemetryPushes);

                // tracking the peformance metrics for telemetry
                this.telemetryCollection.Aggregate(this.telemetryPerformanceInstrumentation);

                this.telemetryWriter.PushTelemetry(this.telemetryCollection);
                this.telemetryCollection.IncrementTelemetryBatchNumber();

                this.telemetryPerformanceInstrumentation.ProcessingEventsMeasuredTimeReset();

                // saving telemetry to make sure it is not loaded with old data in case DCA restarts before the new persistence.
                this.telemetryCollection.SaveToFile(this.telemetryPersistFilePath, Utility.TraceSource.WriteError);

                // adding telemetry about the environment since it was removed after sending
                this.telemetryCollection.Aggregate(this.environmentTelemetry);
            }
        }

        private void AggregateTelemetry(EventRecord eventRecord)
        {
            EventDefinition eventDefinition = this.manifestCache.GetEventDefinition(eventRecord);
            if (null == eventDefinition)
            {
                return;
            }

            TraceAggregationConfig traceAggregationConfig;

            // see if taskname and eventname are whitelisted and get aggregation configuration
            if (!this.telemetryFilter.TryGetWhitelist(eventDefinition.TaskName, eventDefinition.EventName, out traceAggregationConfig))
            {
                return;
            }

            this.telemetryCollection.AggregateEventRecord(eventRecord, eventDefinition, traceAggregationConfig, LogSourceId);
        }
    }
}
