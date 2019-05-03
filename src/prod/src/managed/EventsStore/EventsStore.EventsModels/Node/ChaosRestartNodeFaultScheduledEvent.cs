// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Node
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosNodeRestartScheduled")]
    public sealed class ChaosRestartNodeFaultScheduledEvent : NodeEvent
    {
        public ChaosRestartNodeFaultScheduledEvent(ChaosRestartNodeFaultScheduledTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.NodeName)
        {
            this.NodeInstanceId = traceRecord.NodeInstanceId;
            this.FaultGroupId = traceRecord.FaultGroupId;
            this.FaultId = traceRecord.FaultId;
        }

        [JsonProperty(PropertyName = "NodeInstanceId")]
        public long NodeInstanceId { get; set; }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }
    }
}
