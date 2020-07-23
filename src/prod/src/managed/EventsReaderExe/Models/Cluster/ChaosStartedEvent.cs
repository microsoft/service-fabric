// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Cluster
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosStarted")]
    internal sealed class ChaosStartedEvent : ClusterEvent
    {
        public ChaosStartedEvent(Guid eventInstanceId, DateTime timeStamp, long maxConcurrentFaults, double timeToRunInSeconds, double maxClusterStabilizationTimeoutInSeconds, double waitTimeBetweenIterationsInSeconds, double waitTimeBetweenFautlsInSeconds, bool moveReplicaFaultEnabled, string includedNodeTypeList, string includedApplicationList, string clusterHealthPolicy, string chaosContext) : base(eventInstanceId, timeStamp)
        {
            this.MaxConcurrentFaults = maxConcurrentFaults;
            this.TimeToRunInSeconds = timeToRunInSeconds;
            this.MaxClusterStabilizationTimeoutInSeconds = maxClusterStabilizationTimeoutInSeconds;
            this.WaitTimeBetweenIterationsInSeconds = waitTimeBetweenIterationsInSeconds;
            this.WaitTimeBetweenFautlsInSeconds = waitTimeBetweenFautlsInSeconds;
            this.MoveReplicaFaultEnabled = moveReplicaFaultEnabled;
            this.IncludedNodeTypeList = includedNodeTypeList;
            this.IncludedApplicationList = includedApplicationList;
            this.ClusterHealthPolicy = clusterHealthPolicy;
            this.ChaosContext = chaosContext;
        }

        public ChaosStartedEvent(ChaosStartedTraceRecord chaosStartedTraceRecord) : base(chaosStartedTraceRecord.EventInstanceId, chaosStartedTraceRecord.TimeStamp)
        {
            this.MaxConcurrentFaults = chaosStartedTraceRecord.MaxConcurrentFaults;
            this.TimeToRunInSeconds = chaosStartedTraceRecord.TimeToRunInSeconds;
            this.MaxClusterStabilizationTimeoutInSeconds = chaosStartedTraceRecord.MaxClusterStabilizationTimeoutInSeconds;
            this.WaitTimeBetweenIterationsInSeconds = chaosStartedTraceRecord.WaitTimeBetweenIterationsInSeconds;
            this.WaitTimeBetweenFautlsInSeconds = chaosStartedTraceRecord.WaitTimeBetweenFautlsInSeconds;
            this.MoveReplicaFaultEnabled = chaosStartedTraceRecord.MoveReplicaFaultEnabled;
            this.IncludedNodeTypeList = chaosStartedTraceRecord.IncludedNodeTypeList;
            this.IncludedApplicationList = chaosStartedTraceRecord.IncludedApplicationList;
            this.ClusterHealthPolicy = chaosStartedTraceRecord.ClusterHealthPolicy;
            this.ChaosContext = chaosStartedTraceRecord.ChaosContext;
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
