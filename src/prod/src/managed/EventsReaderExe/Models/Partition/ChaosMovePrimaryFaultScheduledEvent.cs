// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Partition
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosMovePrimaryFaultScheduled")]
    internal sealed class ChaosMovePrimaryFaultScheduledEvent : PartitionEvent
    {
        public ChaosMovePrimaryFaultScheduledEvent(Guid eventInstanceId, DateTime timeStamp, Guid partitionId, Guid faultGroupId, Guid faultId, string serviceName, string nodeTo, bool forcedMove) : base(eventInstanceId, timeStamp, partitionId)
        {
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
            this.ServiceName = serviceName;
            this.NodeTo = nodeTo;
            this.ForcedMove = forcedMove;
        }

        public ChaosMovePrimaryFaultScheduledEvent(ChaosMovePrimaryFaultScheduledTraceRecord chaosMovePrimaryFaultScheduledTraceRecord) : base(chaosMovePrimaryFaultScheduledTraceRecord.EventInstanceId, chaosMovePrimaryFaultScheduledTraceRecord.TimeStamp, chaosMovePrimaryFaultScheduledTraceRecord.PartitionId)
        {
            this.FaultGroupId = chaosMovePrimaryFaultScheduledTraceRecord.FaultGroupId;
            this.FaultId = chaosMovePrimaryFaultScheduledTraceRecord.FaultId;
            this.ServiceName = chaosMovePrimaryFaultScheduledTraceRecord.ServiceName;
            this.NodeTo = chaosMovePrimaryFaultScheduledTraceRecord.NodeTo;
            this.ForcedMove = chaosMovePrimaryFaultScheduledTraceRecord.ForcedMove;
        }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }

        [JsonProperty(PropertyName = "ServiceName")]
        public string ServiceName { get; set; }

        [JsonProperty(PropertyName = "NodeTo")]
        public string NodeTo { get; set; }

        [JsonProperty(PropertyName = "ForcedMove")]
        public bool ForcedMove { get; set; }
    }
}
