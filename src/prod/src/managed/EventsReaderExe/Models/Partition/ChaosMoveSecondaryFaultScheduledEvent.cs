// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Partition
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosMoveSecondaryFaultScheduled")]
    internal sealed class ChaosMoveSecondaryFaultScheduledEvent : PartitionEvent
    {
        public ChaosMoveSecondaryFaultScheduledEvent(Guid eventInstanceId, DateTime timeStamp, Guid partitionId, Guid faultGroupId, Guid faultId, string serviceName, string sourceNode, string destinationNode, bool forcedMove) : base(eventInstanceId, timeStamp, partitionId)
        {
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
            this.ServiceName = serviceName;
            this.SourceNode = sourceNode;
            this.DestinationNode = destinationNode;
            this.ForcedMove = forcedMove;
        }

        public ChaosMoveSecondaryFaultScheduledEvent(ChaosMoveSecondaryFaultScheduledTraceRecord chaosMoveSecondaryFaultScheduledTraceRecord) : base(chaosMoveSecondaryFaultScheduledTraceRecord.EventInstanceId, chaosMoveSecondaryFaultScheduledTraceRecord.TimeStamp, chaosMoveSecondaryFaultScheduledTraceRecord.PartitionId)
        {
            this.FaultGroupId = chaosMoveSecondaryFaultScheduledTraceRecord.FaultGroupId;
            this.FaultId = chaosMoveSecondaryFaultScheduledTraceRecord.FaultId;
            this.ServiceName = chaosMoveSecondaryFaultScheduledTraceRecord.ServiceName;
            this.SourceNode = chaosMoveSecondaryFaultScheduledTraceRecord.SourceNode;
            this.DestinationNode = chaosMoveSecondaryFaultScheduledTraceRecord.DestinationNode;
            this.ForcedMove = chaosMoveSecondaryFaultScheduledTraceRecord.ForcedMove;
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
