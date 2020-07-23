// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Partition
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosPartitionSecondaryMoveScheduled")]
    public sealed class ChaosMoveSecondaryFaultScheduledEvent : PartitionEvent
    {
        public ChaosMoveSecondaryFaultScheduledEvent(ChaosMoveSecondaryFaultScheduledTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.PartitionId)
        {
            this.FaultGroupId = traceRecord.FaultGroupId;
            this.FaultId = traceRecord.FaultId;
            this.ServiceName = traceRecord.ServiceName;
            this.SourceNode = traceRecord.SourceNode;
            this.DestinationNode = traceRecord.DestinationNode;
            this.ForcedMove = traceRecord.ForcedMove;
        }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }

        [JsonProperty(PropertyName = "ServiceName")]
        public string ServiceName { get; set; }

        [JsonProperty(PropertyName = "SourceNode")]
        public string SourceNode { get; set; }

        [JsonProperty(PropertyName = "DestinationNode")]
        public string DestinationNode { get; set; }

        [JsonProperty(PropertyName = "ForcedMove")]
        public bool ForcedMove { get; set; }
    }
}
