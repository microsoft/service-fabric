// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    [JsonObject("NodeDown")]
    public sealed class NodeDownEvent : NodeEvent
    {
        public NodeDownEvent(NodeDownTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeInstance = traceRecord.NodeInstance;
            this.LastNodeUpAt = traceRecord.LastNodeUpAt;
        }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "LastNodeUpAt")]
        public DateTime LastNodeUpAt { get; set; }
    }
}
