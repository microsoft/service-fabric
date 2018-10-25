// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using Newtonsoft.Json;
    using TelemetryAggregation;

    // this class implements a data structure which stores a dictionary key
    // to differentiate between traces with different differentiator field values
    // it differentiates traces by the tuple (TaskName, EventName, DifferentiatorFieldName, DifferentiatorFieldValue)
    [JsonObject(MemberSerialization.Fields)]
    public class TraceAggregationKey
    {
        public TraceAggregationKey(TraceAggregationConfig traceAggregationConfig, string differentiatorFieldValue)
        {
            this.TaskName = traceAggregationConfig.TaskName;
            this.EventName = traceAggregationConfig.EventName;
            this.DifferentiatorFieldName = traceAggregationConfig.DifferentiatorFieldName;
            this.DifferentiatorFieldValue = differentiatorFieldValue;
        }

        [JsonConstructor]
        private TraceAggregationKey()
        {
        }

        public string TaskName { get; }

        public string EventName { get; }

        public string DifferentiatorFieldName { get; }

        public string DifferentiatorFieldValue { get; }

        public override bool Equals(object obj)
        {
            if (obj == null || GetType() != obj.GetType())
            {
                return false;
            }

            TraceAggregationKey traceAggregationKey = (TraceAggregationKey)obj;

            return
                this.TaskName == traceAggregationKey.TaskName &&
                this.EventName == traceAggregationKey.EventName &&
                this.DifferentiatorFieldName == traceAggregationKey.DifferentiatorFieldName &&
                this.DifferentiatorFieldValue == traceAggregationKey.DifferentiatorFieldValue;
        }

        public override int GetHashCode()
        {
            return Tuple.Create(
                this.TaskName,
                this.EventName,
                this.DifferentiatorFieldName,
                this.DifferentiatorFieldValue).GetHashCode();
        }
    }
}
