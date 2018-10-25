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

    // this class holds the aggregation information for all fields of a trace
    [JsonObject(MemberSerialization.Fields)]
    public class TraceAggregator
    {
        private const string LogSourceId = "TraceAggregator";
        private int exceptionCount;
        private Dictionary<string, FieldAggregator> fieldAggregators;

        public TraceAggregator(TraceAggregationConfig configEntry)
        {
            this.TraceAggrConfig = configEntry;
            this.exceptionCount = 0;
            this.fieldAggregators = new Dictionary<string, FieldAggregator>();
            foreach (var fieldAggregatorConfig in configEntry.FieldsAndAggregationConfigs)
            {
                this.fieldAggregators.Add(fieldAggregatorConfig.Key, new FieldAggregator(fieldAggregatorConfig.Value));
            }
        }

        [JsonConstructor]
        private TraceAggregator()
        {
        }

        public DateTime TraceTimestamp { get; set; }

        public TraceAggregationConfig TraceAggrConfig { get; }

        // this should be event record when integrated into code
        public void Aggregate(IEnumerable<KeyValuePair<string, double>> events)
        {
            try
            {
                foreach (var ev in events)
                {
                    FieldAggregator fieldAggr;

                    // aggregate value if whitelisted in the configuration file
                    if (this.fieldAggregators.TryGetValue(ev.Key, out fieldAggr))
                    {
                        // aggregate field on the types of aggregation defined in the configuration
                        fieldAggr.Aggregate(ev.Value);
                    }
                }
            }
            catch (ArgumentNullException)
            {
                // exception catches argumentnull in TryGetValue
                // swallowing exception and counting the number of exceptions
                this.exceptionCount++;
            }
        }

        public void Aggregate(IEnumerable<KeyValuePair<string, string>> events)
        {
            try
            {
                foreach (var ev in events)
                {
                    FieldAggregator fieldAggr;

                    // aggregate value if whitelisted in the configuration file
                    if (this.fieldAggregators.TryGetValue(ev.Key, out fieldAggr))
                    {
                        // aggregate field on the types of aggregation defined in the configuration
                        fieldAggr.Aggregate(ev.Value);
                    }
                }
            }
            catch (ArgumentNullException)
            {
                // exception catches argumentnull in TryGetValue
                // swallowing exception and counting the number of exceptions
                this.exceptionCount++;
            }
        }

        // returns a dictionary where each entry has the name of the field and the set of aggregations and their values
        public void CurrentFormattedFieldAggregationsValues(out Dictionary<string, string> properties, out Dictionary<string, double> metrics)
        {
            properties = new Dictionary<string, string>();
            metrics = new Dictionary<string, double>();

            // iterate over all fields and append their property and metric aggregations
            foreach (var field in this.fieldAggregators)
            {
                field.Value.AppendAggregationValuesFormated(properties, metrics);
            }

            // it should report any exceptions found in Aggregate method
            // in practice it should never report this field, unless there is a null field in EventRecord which is not expected
            if (this.exceptionCount > 0)
            {
                metrics.Add("AggregationException_", (double)this.exceptionCount);
            }
        }

        // human readable representation of the aggregation of the trace
        public override string ToString()
        {
            Dictionary<string, string> properties;
            Dictionary<string, double> metrics;

            this.CurrentFormattedFieldAggregationsValues(out properties, out metrics);
            StringBuilder sb = new StringBuilder();

            // writing property aggregations
            foreach (var propertyField in properties)
            {
                sb.AppendFormat("({0} = {1})", propertyField.Key, propertyField.Value);
            }

            // writing metric aggregations
            foreach (var metricField in metrics)
            {
                sb.AppendFormat("({0} = {1})", metricField.Key, metricField.Value);
            }

            return sb.ToString();
        }
    }
}
