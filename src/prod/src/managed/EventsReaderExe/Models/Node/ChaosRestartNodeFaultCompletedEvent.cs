// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosRestartNodeFaultCompleted")]
    internal sealed class ChaosRestartNodeFaultCompletedEvent : NodeEvent
    {
        public ChaosRestartNodeFaultCompletedEvent(Guid eventInstanceId, DateTime timeStamp, string nodeName, long nodeInstanceId, Guid faultGroupId, Guid faultId) : base(eventInstanceId, timeStamp, nodeName)
        {
            this.NodeInstanceId = nodeInstanceId;
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
        }

        public ChaosRestartNodeFaultCompletedEvent(ChaosRestartNodeFaultCompletedTraceRecord chaosRestartNodeFaultCompletedTraceRecord) : base(chaosRestartNodeFaultCompletedTraceRecord.EventInstanceId, chaosRestartNodeFaultCompletedTraceRecord.TimeStamp, chaosRestartNodeFaultCompletedTraceRecord.NodeName)
        {
            this.NodeInstanceId = chaosRestartNodeFaultCompletedTraceRecord.NodeInstanceId;
            this.FaultGroupId = chaosRestartNodeFaultCompletedTraceRecord.FaultGroupId;
            this.FaultId = chaosRestartNodeFaultCompletedTraceRecord.FaultId;
        }

        [JsonProperty(PropertyName = "NodeInstanceId")]
        public long NodeInstanceId { get; set; }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }
    }
}
