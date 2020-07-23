// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Partition
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;

    [JsonObject("PartitionReconfigured")]
    public sealed class PartitionReconfigurationCompletedEvent : PartitionEvent
    {
        public PartitionReconfigurationCompletedEvent(ReconfigurationCompletedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.PartitionId)
        {
            this.NodeName = traceRecord.NodeName;
            this.NodeInstanceId = traceRecord.NodeInstanceId;
            this.ServiceType = traceRecord.ServiceType;
            this.CcEpochDataLossVersion = traceRecord.CcEpochDataLossVersion;
            this.CcEpochConfigVersion = traceRecord.CcEpochConfigVersion;
            this.ReconfigType = traceRecord.ReconfigType.ToString();
            this.Result = traceRecord.Result.ToString();
            this.Phase0DurationMs = traceRecord.Phase0DurationMs;
            this.Phase1DurationMs = traceRecord.Phase1DurationMs;
            this.Phase2DurationMs = traceRecord.Phase2DurationMs;
            this.Phase3DurationMs = traceRecord.Phase3DurationMs;
            this.Phase4DurationMs = traceRecord.Phase4DurationMs;
            this.TotalDurationMs = traceRecord.TotalDurationMs;
        }

        [JsonProperty(PropertyName = "NodeName")]
        public string NodeName { get; set; }

        [JsonProperty(PropertyName = "NodeInstanceId")]
        public string NodeInstanceId { get; set; }

        [JsonProperty(PropertyName = "ServiceType")]
        public string ServiceType { get; set; }

        [JsonProperty(PropertyName = "CcEpochDataLossVersion")]
        public long CcEpochDataLossVersion { get; set; }

        [JsonProperty(PropertyName = "CcEpochConfigVersion")]
        public long CcEpochConfigVersion { get; set; }

        [JsonProperty(PropertyName = "ReconfigType")]
        public string ReconfigType { get; set; }

        [JsonProperty(PropertyName = "Result")]
        public string Result { get; set; }

        [JsonProperty(PropertyName = "Phase0DurationMs")]
        public double Phase0DurationMs { get; set; }

        [JsonProperty(PropertyName = "Phase1DurationMs")]
        public double Phase1DurationMs { get; set; }

        [JsonProperty(PropertyName = "Phase2DurationMs")]
        public double Phase2DurationMs { get; set; }

        [JsonProperty(PropertyName = "Phase3DurationMs")]
        public double Phase3DurationMs { get; set; }

        [JsonProperty(PropertyName = "Phase4DurationMs")]
        public double Phase4DurationMs { get; set; }

        [JsonProperty(PropertyName = "TotalDurationMs")]
        public double TotalDurationMs { get; set; }
    }
}
