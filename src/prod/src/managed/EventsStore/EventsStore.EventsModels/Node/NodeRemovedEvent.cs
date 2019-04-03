// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    [JsonObject("NodeRemovedFromCluster")]
    public sealed class NodeRemovedEvent : NodeEvent
    {
        public NodeRemovedEvent(NodeRemovedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeId = traceRecord.NodeId;
            this.NodeInstance = traceRecord.NodeInstance;
            this.NodeType = traceRecord.NodeType;
            this.FabricVersion = traceRecord.FabricVersion;
            this.IpAddressOrFQDN = traceRecord.IpAddressOrFQDN;
            this.NodeCapacities = traceRecord.NodeCapacities;
        }

        [JsonProperty(PropertyName = "NodeId")]
        public string NodeId { get; set; }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "NodeType")]
        public string NodeType { get; set; }

        [JsonProperty(PropertyName = "FabricVersion")]
        public string FabricVersion { get; set; }

        [JsonProperty(PropertyName = "IpAddressOrFQDN")]
        public string IpAddressOrFQDN { get; set; }

        [JsonProperty(PropertyName = "NodeCapacities")]
        public string NodeCapacities { get; set; }
    }
}
