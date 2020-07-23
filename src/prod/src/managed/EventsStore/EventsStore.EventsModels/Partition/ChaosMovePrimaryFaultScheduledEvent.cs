// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Partition
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosPartitionPrimaryMoveScheduled")]
    public sealed class ChaosMovePrimaryFaultScheduledEvent : PartitionEvent
    {
        public ChaosMovePrimaryFaultScheduledEvent(ChaosMovePrimaryFaultScheduledTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.PartitionId)
        {
            this.FaultGroupId = traceRecord.FaultGroupId;
            this.FaultId = traceRecord.FaultId;
            this.ServiceName = traceRecord.ServiceName;
            this.NodeTo = traceRecord.NodeTo;
            this.ForcedMove = traceRecord.ForcedMove;
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
