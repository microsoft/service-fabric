// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ClusterUpgradeRollbackStarted")]
    public sealed class ClusterUpgradeRollbackStartEvent : ClusterEvent
    {
        public ClusterUpgradeRollbackStartEvent(ClusterUpgradeRollbackStartTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category)
        {
            this.TargetClusterVersion = traceRecord.TargetClusterVersion;
            this.FailureReason = traceRecord.FailureReason.ToString();
            this.OverallUpgradeElapsedTimeInMs = traceRecord.OverallUpgradeElapsedTimeInMs;
        }

        [JsonProperty(PropertyName = "TargetClusterVersion")]
        public string TargetClusterVersion { get; set; }

        [JsonProperty(PropertyName = "FailureReason")]
        public string FailureReason { get; set; }

        [JsonProperty(PropertyName = "OverallUpgradeElapsedTimeInMs")]
        public double OverallUpgradeElapsedTimeInMs { get; set; }
    }
}
