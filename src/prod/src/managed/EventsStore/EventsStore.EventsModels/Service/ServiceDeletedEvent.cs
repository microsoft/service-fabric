// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Service
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.FM;

    [JsonObject("ServiceDeleted")]
    public sealed class ServiceDeletedEvent : ServiceEvent
    {
        public ServiceDeletedEvent(ServiceDeletedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ServiceName)
        {
            this.ServiceTypeName = traceRecord.ServiceTypeName;
            this.ApplicationName = traceRecord.ApplicationName;
            this.ApplicationTypeName = traceRecord.ApplicationTypeName;
            this.ServiceInstance = traceRecord.ServiceInstance;
            this.IsStateful = traceRecord.IsStateful;
            this.PartitionCount = traceRecord.PartitionCount;
            this.TargetReplicaSetSize = traceRecord.TargetReplicaSetSize;
            this.MinReplicaSetSize = traceRecord.MinReplicaSetSize;
            this.ServicePackageVersion = traceRecord.ServicePackageVersion;
        }

        [JsonProperty(PropertyName = "ServiceTypeName")]
        public string ServiceTypeName { get; set; }

        [JsonProperty(PropertyName = "ApplicationName")]
        public string ApplicationName { get; set; }

        [JsonProperty(PropertyName = "ApplicationTypeName")]
        public string ApplicationTypeName { get; set; }

        [JsonProperty(PropertyName = "ServiceInstance")]
        public long ServiceInstance { get; set; }

        [JsonProperty(PropertyName = "IsStateful")]
        public bool IsStateful { get; set; }

        [JsonProperty(PropertyName = "PartitionCount")]
        public int PartitionCount { get; set; }

        [JsonProperty(PropertyName = "TargetReplicaSetSize")]
        public int TargetReplicaSetSize { get; set; }

        [JsonProperty(PropertyName = "MinReplicaSetSize")]
        public int MinReplicaSetSize { get; set; }

        [JsonProperty(PropertyName = "ServicePackageVersion")]
        public string ServicePackageVersion { get; set; }
    }
}
