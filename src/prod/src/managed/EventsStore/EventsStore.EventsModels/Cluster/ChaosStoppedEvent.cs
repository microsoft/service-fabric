// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosStopped")]
    public sealed class ChaosStoppedEvent : ClusterEvent
    {
        public ChaosStoppedEvent(ChaosStoppedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category)
        {
            this.Reason = traceRecord.Reason;
        }

        [JsonProperty(PropertyName = "Reason")]
        public string Reason { get; set; }
    }
}
