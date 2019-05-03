// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    [JsonObject("NodeDeactivateCompleted")]
    public sealed class NodeDeactivateCompleteEvent : NodeEvent
    {
        public NodeDeactivateCompleteEvent(NodeDeactivateCompleteTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeInstance = traceRecord.NodeInstance;
            this.EffectiveDeactivateIntent = traceRecord.EffectiveDeactivateIntent.ToString();
            this.BatchIdsWithDeactivateIntent = traceRecord.BatchIdsWithDeactivateIntent;
            this.StartTime = traceRecord.StartTime;
        }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "EffectiveDeactivateIntent")]
        public string EffectiveDeactivateIntent { get; set; }

        [JsonProperty(PropertyName = "BatchIdsWithDeactivateIntent")]
        public string BatchIdsWithDeactivateIntent { get; set; }

        [JsonProperty(PropertyName = "StartTime")]
        public DateTime StartTime { get; set; }
    }
}
