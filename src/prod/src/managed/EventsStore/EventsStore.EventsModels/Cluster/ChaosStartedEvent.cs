// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosStarted")]
    public sealed class ChaosStartedEvent : ClusterEvent
    {
        public ChaosStartedEvent(ChaosStartedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category)
        {
            this.MaxConcurrentFaults = traceRecord.MaxConcurrentFaults;
            this.TimeToRunInSeconds = traceRecord.TimeToRunInSeconds;
            this.MaxClusterStabilizationTimeoutInSeconds = traceRecord.MaxClusterStabilizationTimeoutInSeconds;
            this.WaitTimeBetweenIterationsInSeconds = traceRecord.WaitTimeBetweenIterationsInSeconds;
            this.WaitTimeBetweenFautlsInSeconds = traceRecord.WaitTimeBetweenFautlsInSeconds;
            this.MoveReplicaFaultEnabled = traceRecord.MoveReplicaFaultEnabled;
            this.IncludedNodeTypeList = traceRecord.IncludedNodeTypeList;
            this.IncludedApplicationList = traceRecord.IncludedApplicationList;
            this.ClusterHealthPolicy = traceRecord.ClusterHealthPolicy;
            this.ChaosContext = traceRecord.ChaosContext;
        }

        [JsonProperty(PropertyName = "MaxConcurrentFaults")]
        public long MaxConcurrentFaults { get; set; }

        [JsonProperty(PropertyName = "TimeToRunInSeconds")]
        public double TimeToRunInSeconds { get; set; }

        [JsonProperty(PropertyName = "MaxClusterStabilizationTimeoutInSeconds")]
        public double MaxClusterStabilizationTimeoutInSeconds { get; set; }

        [JsonProperty(PropertyName = "WaitTimeBetweenIterationsInSeconds")]
        public double WaitTimeBetweenIterationsInSeconds { get; set; }

        [JsonProperty(PropertyName = "WaitTimeBetweenFautlsInSeconds")]
        public double WaitTimeBetweenFautlsInSeconds { get; set; }

        [JsonProperty(PropertyName = "MoveReplicaFaultEnabled")]
        public bool MoveReplicaFaultEnabled { get; set; }

        [JsonProperty(PropertyName = "IncludedNodeTypeList")]
        public string IncludedNodeTypeList { get; set; }

        [JsonProperty(PropertyName = "IncludedApplicationList")]
        public string IncludedApplicationList { get; set; }

        [JsonProperty(PropertyName = "ClusterHealthPolicy")]
        public string ClusterHealthPolicy { get; set; }

        [JsonProperty(PropertyName = "ChaosContext")]
        public string ChaosContext { get; set; }
    }
}
