// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Partition
{
    using System;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;
    using Newtonsoft.Json;

    [JsonObject("PartitionPrimaryMoveAnalysis")]
    public sealed class PartitionPrimaryMoveAnalysisEvent : PartitionAnalysisEvent
    {
        public PartitionPrimaryMoveAnalysisEvent(PrimaryMoveAnalysisTraceRecord traceRecord) :
            base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.PartitionId,
                new AnalysisEventMetadata(TimeSpan.FromSeconds(traceRecord.AnalysisDelayInSeconds), TimeSpan.FromSeconds(traceRecord.AnalysisDurationInSeconds)), false)
        {
            this.WhenMoveCompleted = traceRecord.WhenMoveCompleted;
            this.PreviousNode = traceRecord.PreviousNode;
            this.CurrentNode = traceRecord.CurrentNode;
            this.MoveReason = traceRecord.Reason;
            this.RelevantTraces = traceRecord.CorrelatedTraceRecords;
        }

        [JsonProperty(PropertyName = "WhenMoveCompleted")]
        public DateTime WhenMoveCompleted { get; set; }

        [JsonProperty(PropertyName = "PreviousNode")]
        public string PreviousNode { get; set; }

        [JsonProperty(PropertyName = "CurrentNode")]
        public string CurrentNode { get; set; }

        [JsonProperty(PropertyName = "MoveReason")]
        public string MoveReason { get; set; }

        [JsonProperty(PropertyName = "RelevantTraces")]
        public string RelevantTraces { get; set; }
    }
}