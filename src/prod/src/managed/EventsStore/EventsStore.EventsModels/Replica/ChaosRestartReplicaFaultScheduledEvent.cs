// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Replica
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosReplicaRestartScheduled")]
    public sealed class ChaosRestartReplicaFaultScheduledEvent : ReplicaEvent
    {
        public ChaosRestartReplicaFaultScheduledEvent(ChaosRestartReplicaFaultScheduledTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.PartitionId, traceRecord.ReplicaId)
        {
            this.FaultGroupId = traceRecord.FaultGroupId;
            this.FaultId = traceRecord.FaultId;
            this.ServiceUri = traceRecord.ServiceUri;
        }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }

        [JsonProperty(PropertyName = "ServiceUri")]
        public string ServiceUri { get; set; }
    }
}
