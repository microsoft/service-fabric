// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    [JsonObject("NodeDeactivateStarted")]
    public sealed class NodeDeactivateStartEvent : NodeEvent
    {
        public NodeDeactivateStartEvent(NodeDeactivateStartTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeInstance = traceRecord.NodeInstance;
            this.BatchId = traceRecord.BatchId;
            this.DeactivateIntent = traceRecord.DeactivateIntent.ToString();
        }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "BatchId")]
        public string BatchId { get; set; }

        [JsonProperty(PropertyName = "DeactivateIntent")]
        public string DeactivateIntent { get; set; }
    }
}
