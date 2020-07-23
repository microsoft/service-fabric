// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;

    [JsonObject("NodeClosed")]
    public sealed class NodeClosedEvent : NodeEvent
    {
        public NodeClosedEvent(NodeClosedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            /// This is by Design
            /// This is to account for the bug where we accidently write node ID in place of node instance and vice versa.
            this.NodeId = traceRecord.NodeInstance;
            this.NodeInstance = traceRecord.NodeId;
            this.Error = traceRecord.Error.ToString();
        }

        [JsonProperty(PropertyName = "NodeId")]
        public string NodeId { get; set; }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "Error")]
        public string Error { get; set; }
    }
}
