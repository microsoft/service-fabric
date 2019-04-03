// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosStopped")]
    internal sealed class ChaosStoppedEvent : ClusterEvent
    {
        public ChaosStoppedEvent(Guid eventInstanceId, DateTime timeStamp, string reason) : base(eventInstanceId, timeStamp)
        {
            this.Reason = reason;
        }

        public ChaosStoppedEvent(ChaosStoppedTraceRecord chaosStoppedTraceRecord) : base(chaosStoppedTraceRecord.EventInstanceId, chaosStoppedTraceRecord.TimeStamp)
        {
            this.Reason = chaosStoppedTraceRecord.Reason;
        }

        [JsonProperty(PropertyName = "Reason")]
        public string Reason { get; set; }
    }
}
