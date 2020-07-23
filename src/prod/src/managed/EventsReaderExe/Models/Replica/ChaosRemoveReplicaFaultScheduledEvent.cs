// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Replica
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosRemoveReplicaFaultScheduled")]
    internal sealed class ChaosRemoveReplicaFaultScheduledEvent : ReplicaEvent
    {
        public ChaosRemoveReplicaFaultScheduledEvent(Guid eventInstanceId, DateTime timeStamp, Guid partitionId, long replicaId, Guid faultGroupId, Guid faultId, string serviceUri) : base(eventInstanceId, timeStamp, partitionId, replicaId)
        {
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
            this.ServiceUri = serviceUri;
        }

        public ChaosRemoveReplicaFaultScheduledEvent(ChaosRemoveReplicaFaultScheduledTraceRecord chaosRemoveReplicaFaultScheduledTraceRecord) : base(chaosRemoveReplicaFaultScheduledTraceRecord.EventInstanceId, chaosRemoveReplicaFaultScheduledTraceRecord.TimeStamp, chaosRemoveReplicaFaultScheduledTraceRecord.PartitionId, chaosRemoveReplicaFaultScheduledTraceRecord.ReplicaId)
        {
            this.FaultGroupId = chaosRemoveReplicaFaultScheduledTraceRecord.FaultGroupId;
            this.FaultId = chaosRemoveReplicaFaultScheduledTraceRecord.FaultId;
            this.ServiceUri = chaosRemoveReplicaFaultScheduledTraceRecord.ServiceUri;
        }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }

        [JsonProperty(PropertyName = "ServiceUri")]
        public string ServiceUri { get; set; }
    }
}
