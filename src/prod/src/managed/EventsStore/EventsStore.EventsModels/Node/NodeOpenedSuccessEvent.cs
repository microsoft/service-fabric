// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FabricNode;

    [JsonObject("NodeOpenSucceeded")]
    public sealed class NodeOpenedSuccessEvent : NodeEvent
    {
        public NodeOpenedSuccessEvent(NodeOpenedSuccessTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeInstance = traceRecord.NodeInstance;
            this.NodeId = traceRecord.NodeId;
            this.UpgradeDomain = traceRecord.UpgradeDomain;
            this.FaultDomain = traceRecord.FaultDomain;
            this.IpAddressOrFQDN = traceRecord.IpAddressOrFQDN;
            this.Hostname = traceRecord.Hostname;
            this.IsSeedNode = traceRecord.IsSeedNode;
            this.NodeVersion = traceRecord.NodeVersion;
        }

        [JsonProperty(PropertyName = "NodeInstance")]
        public long NodeInstance { get; set; }

        [JsonProperty(PropertyName = "NodeId")]
        public string NodeId { get; set; }

        [JsonProperty(PropertyName = "UpgradeDomain")]
        public string UpgradeDomain { get; set; }

        [JsonProperty(PropertyName = "FaultDomain")]
        public string FaultDomain { get; set; }

        [JsonProperty(PropertyName = "IpAddressOrFQDN")]
        public string IpAddressOrFQDN { get; set; }

        [JsonProperty(PropertyName = "Hostname")]
        public string Hostname { get; set; }

        [JsonProperty(PropertyName = "IsSeedNode")]
        public bool IsSeedNode { get; set; }

        [JsonProperty(PropertyName = "NodeVersion")]
        public string NodeVersion { get; set; }
    }
}
