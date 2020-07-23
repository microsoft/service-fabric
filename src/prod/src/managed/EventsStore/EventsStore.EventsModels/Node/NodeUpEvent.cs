// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    [JsonObject("NodeUp")]
    public sealed class NodeUpEvent : NodeEvent
    {
        public NodeUpEvent(NodeUpTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeInstance = traceRecord.NodeInstance;
            this.LastNodeDownAt = traceRecord.LastNodeDownAt;
        }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "LastNodeDownAt")]
        public DateTime LastNodeDownAt { get; set; }
    }
}
