// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Newtonsoft.Json;

    // this class implements a data structure which stores the rules for aggregating telemetry
    // Each instance represents one trace which is to be white-listed and which fields and their
    // respective types of aggregations should be used
    [JsonObject(MemberSerialization.Fields)]
    public class TraceAggregationConfig
    {
        // constants
        private const string LogSourceId = "TraceAggregationConfig";

        public TraceAggregationConfig(string taskName, string eventName, string differentiatorFieldName)
        {
            this.TaskName = taskName;
            this.EventName = eventName;
            this.DifferentiatorFieldName = differentiatorFieldName;

            // Constructing the list of fields to be aggregated
            this.FieldsAndAggregationConfigs = new Dictionary<string, FieldAggregationConfig>();
        }

        [JsonConstructor]
        private TraceAggregationConfig()
        {
        }

        // List of of fields of the trace to be aggregate, and their respective set of aggregations
        // Using dictionary to query the existence of the field faster
        public Dictionary<string, FieldAggregationConfig> FieldsAndAggregationConfigs { get; }

        // Properties - Trace unique identifier (EventName, TaskName, differentiatorField)
        public string TaskName { get; }

        public string EventName { get; }

        public string DifferentiatorFieldName { get; }

        // this method is used to simplify the creation of a TraceAggregationConfig containing only snapshots type of aggregation
        public static TraceAggregationConfig CreateSnapshotTraceAggregatorConfig(
            string taskName,
            string eventName,
            string differentiatorFieldName,
            IEnumerable<string> properties,
            IEnumerable<string> metrics)
        {
            // all fields have only the snapshot
            return TraceAggregationConfig.CreateTraceAggregatorConfig(
                taskName,
                eventName,
                differentiatorFieldName,
                properties,
                new List<Aggregation.Kinds>() { Aggregation.Kinds.Snapshot },
                metrics,
                new List<Aggregation.Kinds>() { Aggregation.Kinds.Snapshot });
        }

        // this method is used to simplify the creation of a TraceAggregationConfig containing only snapshots type of aggregation
        public static TraceAggregationConfig CreateTraceAggregatorConfig(
            string taskName,
            string eventName,
            string differentiatorFieldName,
            IEnumerable<string> properties,
            IEnumerable<Aggregation.Kinds> aggrKindsProperties,
            IEnumerable<string> metrics,
            IEnumerable<Aggregation.Kinds> aggrKindsMetrics)
        {
            // all fields have only the snapshot

            // creating the list of all fields of the TraceAggregator
            List<FieldAggregationConfig> aggrFieldConfigs = new List<FieldAggregationConfig>();

            foreach (var p in properties)
            {
                aggrFieldConfigs.Add(new FieldAggregationConfig(new Field(p, Field.Kind.Property), aggrKindsProperties));
            }

            foreach (var m in metrics)
            {
                aggrFieldConfigs.Add(new FieldAggregationConfig(new Field(m, Field.Kind.Metric), aggrKindsMetrics));
            }

            // constructing the configuration of the aggregationConfig
            TraceAggregationConfig aggrConfig = new TraceAggregationConfig(taskName, eventName, differentiatorFieldName);
            aggrConfig.AddAggregationEntries(aggrFieldConfigs);

            return aggrConfig;
        }

        // Allows appending new field aggregation configuration
        public void AddAggregationEntries(IEnumerable<FieldAggregationConfig> fieldsAndAggregationConfigs)
        {
            try
            {
                foreach (var fieldAggr in fieldsAndAggregationConfigs)
                {
                    this.FieldsAndAggregationConfigs.Add(fieldAggr.Field.Name, fieldAggr);
                }
            }
            catch (ArgumentNullException)
            {
                ////Checkthis check this Utility.TraceSource.WriteError(LogSourceId, "Unexpected null field - {0}.", e.Message);
            }
            catch (ArgumentException)
            {
                ////Checkthis check this Utility.TraceSource.WriteWarning(LogSourceId, "Conflicting field names (trace already white-listed) - {0}.", e.Message);
            }
        }

        public void AddAggregationEntries(IEnumerable<Field> fields, IEnumerable<Aggregation.Kinds> aggregationTypes)
        {
            List<FieldAggregationConfig> fieldAggregations = new List<FieldAggregationConfig>();
            foreach (var field in fields)
            {
                FieldAggregationConfig aggrEntry = new FieldAggregationConfig(field, aggregationTypes);
                fieldAggregations.Add(aggrEntry);
            }

            this.AddAggregationEntries(fieldAggregations);
        }

        // Returns a human readable representation of the TraceAggregationConfig
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format("TaskName: \"{0}\", EventName: \"{1}\",DifferentiatorField: \"{2}\"; (", this.TaskName, this.EventName, this.DifferentiatorFieldName, Environment.NewLine));
            foreach (var field in this.FieldsAndAggregationConfigs)
            {
                sb.Append(string.Format("{0}", field.Value.ToString()));
            }

            sb.Append(")");
            return sb.ToString();
        }

        public override int GetHashCode()
        {
            return Tuple.Create(this.TaskName, this.EventName, this.DifferentiatorFieldName).GetHashCode();
        }
    }
}
