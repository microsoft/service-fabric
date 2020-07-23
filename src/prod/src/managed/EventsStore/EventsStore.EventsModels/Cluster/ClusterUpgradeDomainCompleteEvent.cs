// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ClusterUpgradeDomainCompleted")]
    public sealed class ClusterUpgradeDomainCompleteEvent : ClusterEvent
    {
        public ClusterUpgradeDomainCompleteEvent(ClusterUpgradeDomainCompleteTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category)
        {
            this.TargetClusterVersion = traceRecord.TargetClusterVersion;
            this.UpgradeState = traceRecord.UpgradeState.ToString();
            this.UpgradeDomains = traceRecord.UpgradeDomains;
            this.UpgradeDomainElapsedTimeInMs = traceRecord.UpgradeDomainElapsedTimeInMs;
        }

        [JsonProperty(PropertyName = "TargetClusterVersion")]
        public string TargetClusterVersion { get; set; }

        [JsonProperty(PropertyName = "UpgradeState")]
        public string UpgradeState { get; set; }

        [JsonProperty(PropertyName = "UpgradeDomains")]
        public string UpgradeDomains { get; set; }

        [JsonProperty(PropertyName = "UpgradeDomainElapsedTimeInMs")]
        public double UpgradeDomainElapsedTimeInMs { get; set; }
    }
}
