// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Replica
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;

    [JsonObject("StatefulReplicaHealthReportExpired")]
    public sealed class StatefulReplicaHealthReportExpiredEvent : ReplicaEvent
    {
        public StatefulReplicaHealthReportExpiredEvent(StatefulReplicaHealthReportExpiredTraceRecord traceRecord)
            : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.PartitionId, traceRecord.ReplicaId)
        {
            this.ReplicaInstanceId = traceRecord.ReplicaInstanceId;
            this.SourceId = traceRecord.SourceId;
            this.Property = traceRecord.Property;
            this.HealthState = traceRecord.HealthState.ToString();
            this.TimeToLiveMs = traceRecord.TTLTimespan;
            this.SequenceNumber = traceRecord.SequenceNumber;
            this.Description = traceRecord.Description;
            this.RemoveWhenExpired = traceRecord.RemoveWhenExpired;
            this.SourceUtcTimestamp = traceRecord.SourceUtcTimestamp;
        }

        [JsonProperty(PropertyName = "ReplicaInstanceId")]
        public long ReplicaInstanceId { get; set; }

        [JsonProperty(PropertyName = "SourceId")]
        public string SourceId { get; set; }

        [JsonProperty(PropertyName = "Property")]
        public string Property { get; set; }

        [JsonProperty(PropertyName = "HealthState")]
        public string HealthState { get; set; }

        [JsonProperty(PropertyName = "TimeToLiveMs")]
        public long TimeToLiveMs { get; set; }

        [JsonProperty(PropertyName = "SequenceNumber")]
        public long SequenceNumber { get; set; }

        [JsonProperty(PropertyName = "Description")]
        public string Description { get; set; }

        [JsonProperty(PropertyName = "RemoveWhenExpired")]
        public bool RemoveWhenExpired { get; set; }

        [JsonProperty(PropertyName = "SourceUtcTimestamp")]
        public DateTime SourceUtcTimestamp { get; set; }
    }
}
