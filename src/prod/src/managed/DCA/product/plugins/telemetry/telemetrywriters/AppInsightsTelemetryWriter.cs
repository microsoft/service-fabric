// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using Microsoft.ApplicationInsights;
    using Microsoft.ApplicationInsights.DataContracts;
    using Microsoft.ApplicationInsights.Extensibility;
    using TelemetryAggregation;

    public class AppInsightsTelemetryWriter : ITelemetryWriter
    {
        private const string TaskNameStr = "TaskName";
        private const string EventNameStr = "EventName";
        private const string TimestampStr = "Timestamp";
        private const string ClusterIdStr = "ClusterId";
        private const string TenantIdStr = "TenantId";
        private const string NodeNameHashStr = "NodeNameHash";
        private const string TelBatchesStr = "TelBatchesDaily";
        private const string TelBatchNumberStr = "TelBatchNumber";

        private readonly TelemetryClient telemetryClient;

        public AppInsightsTelemetryWriter()
        {
            // getting default telemetry configuration
            var appInsightsTelemetryConf = TelemetryConfiguration.Active;

            appInsightsTelemetryConf.InstrumentationKey = AppInsightsTelemetryKey.AppInsightsInstrumentationKey;
            appInsightsTelemetryConf.TelemetryChannel.EndpointAddress = AppInsightsTelemetryKey.AppInsightsEndPointAddress;

            this.telemetryClient = new TelemetryClient(appInsightsTelemetryConf);
        }

        public static EventTelemetry CreateBaseEventTelemetry(
            TelemetryIdentifiers telemetryIdentifiers,
            string taskName,
            string eventName,
            int telBatchesDaily,
            int telBatchNumber,
            DateTime timestamp)
        {
            EventTelemetry eventEntity = new EventTelemetry(string.Format("{0}.{1}", taskName, eventName));
            try
            {
                eventEntity.Properties[ClusterIdStr] = telemetryIdentifiers.ClusterId;
                eventEntity.Properties[TenantIdStr] = telemetryIdentifiers.TenantId;
                eventEntity.Properties[NodeNameHashStr] = telemetryIdentifiers.NodeNameHash;
                eventEntity.Properties[TaskNameStr] = taskName;
                eventEntity.Properties[EventNameStr] = eventName;
                eventEntity.Properties[TimestampStr] = timestamp.ToString();
                eventEntity.Metrics[TelBatchesStr] = telBatchesDaily;
                eventEntity.Metrics[TelBatchNumberStr] = telBatchNumber;
            }
            catch (ArgumentOutOfRangeException)
            {
                // add information about the exception to the event
                eventEntity.Properties["DateTimeToStringException"] = "true";
            }

            return eventEntity;
        }

        // extension to create AppInsights EventTelemetry list from  a TelemetryCollection
        public static List<EventTelemetry> GetEventTelemetryList(TelemetryCollection telemetryCollection)
        {
            List<EventTelemetry> eventTelemetryList = new List<EventTelemetry>();

            foreach (TraceAggregator traceAggregator in telemetryCollection)
            {
                eventTelemetryList.Add(AppInsightsTelemetryWriter.ConvertAggregationToTelemetry(traceAggregator.TraceAggrConfig, traceAggregator, telemetryCollection.TelemetryIds, telemetryCollection.DailyNumberOfTelemetryPushes, telemetryCollection.TelemetryBatchNumber));
            }

            return eventTelemetryList;
        }

        public void Dispose()
        {
            if (null != this.telemetryClient)
            {
                this.telemetryClient.Flush();

                // allow time for flushing (Is this really the only way?)
                System.Threading.Thread.Sleep(1000);
            }
        }

        void ITelemetryWriter.PushTelemetry(TelemetryCollection telemetryCollection)
        {
            // sending all other events from telemetryCollection
            foreach (var telEvent in AppInsightsTelemetryWriter.GetEventTelemetryList(telemetryCollection))
            {
                this.telemetryClient.TrackEvent(telEvent);
            }
        }

        private static EventTelemetry ConvertAggregationToTelemetry(TraceAggregationConfig traceAggregationConfig, TraceAggregator traceAggregator, TelemetryIdentifiers telemetryIds, int telBatchesDaily, int telBatchNumber)
        {
            EventTelemetry eventEntity = AppInsightsTelemetryWriter.CreateBaseEventTelemetry(telemetryIds, traceAggregationConfig.TaskName, traceAggregationConfig.EventName, telBatchesDaily, telBatchNumber, traceAggregator.TraceTimestamp);

            Dictionary<string, string> properties;
            Dictionary<string, double> metrics;

            traceAggregator.CurrentFormattedFieldAggregationsValues(out properties, out metrics);

            foreach (var property in properties)
            {
                eventEntity.Properties[property.Key] = property.Value;
            }

            foreach (var metric in metrics)
            {
                eventEntity.Metrics[metric.Key] = metric.Value;
            }

            return eventEntity;
        }
    }
}