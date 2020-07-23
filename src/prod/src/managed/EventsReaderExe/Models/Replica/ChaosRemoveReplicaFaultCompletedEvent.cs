// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Replica
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosRemoveReplicaFaultCompleted")]
    internal sealed class ChaosRemoveReplicaFaultCompletedEvent : ReplicaEvent
    {
        public ChaosRemoveReplicaFaultCompletedEvent(Guid eventInstanceId, DateTime timeStamp, Guid partitionId, long replicaId, Guid faultGroupId, Guid faultId, string serviceUri) : base(eventInstanceId, timeStamp, partitionId, replicaId)
        {
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
            this.ServiceUri = serviceUri;
        }

        public ChaosRemoveReplicaFaultCompletedEvent(ChaosRemoveReplicaFaultCompletedTraceRecord chaosRemoveReplicaFaultCompletedTraceRecord) : base(chaosRemoveReplicaFaultCompletedTraceRecord.EventInstanceId, chaosRemoveReplicaFaultCompletedTraceRecord.TimeStamp, chaosRemoveReplicaFaultCompletedTraceRecord.PartitionId, chaosRemoveReplicaFaultCompletedTraceRecord.ReplicaId)
        {
            this.FaultGroupId = chaosRemoveReplicaFaultCompletedTraceRecord.FaultGroupId;
            this.FaultId = chaosRemoveReplicaFaultCompletedTraceRecord.FaultId;
            this.ServiceUri = chaosRemoveReplicaFaultCompletedTraceRecord.ServiceUri;
        }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }

        [JsonProperty(PropertyName = "ServiceUri")]
        public string ServiceUri { get; set; }
    }
}
