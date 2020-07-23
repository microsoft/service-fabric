// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ClusterUpgradeStarted")]
    public sealed class ClusterUpgradeStartEvent : ClusterEvent
    {
        public ClusterUpgradeStartEvent(ClusterUpgradeStartTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category)
        {
            this.CurrentClusterVersion = traceRecord.CurrentClusterVersion;
            this.TargetClusterVersion = traceRecord.TargetClusterVersion;
            this.UpgradeType = traceRecord.UpgradeType.ToString();
            this.RollingUpgradeMode = traceRecord.RollingUpgradeMode.ToString();
            this.FailureAction = traceRecord.FailureAction.ToString();
        }

        [JsonProperty(PropertyName = "CurrentClusterVersion")]
        public string CurrentClusterVersion { get; set; }

        [JsonProperty(PropertyName = "TargetClusterVersion")]
        public string TargetClusterVersion { get; set; }

        [JsonProperty(PropertyName = "UpgradeType")]
        public string UpgradeType { get; set; }

        [JsonProperty(PropertyName = "RollingUpgradeMode")]
        public string RollingUpgradeMode { get; set; }

        [JsonProperty(PropertyName = "FailureAction")]
        public string FailureAction { get; set; }
    }
}
