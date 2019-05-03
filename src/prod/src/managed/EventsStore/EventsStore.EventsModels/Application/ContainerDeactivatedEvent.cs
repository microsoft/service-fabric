// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;

    [JsonObject("ApplicationContainerInstanseExited")]
    public sealed class ContainerDeactivatedEvent : ApplicationEvent
    {
        public ContainerDeactivatedEvent(ContainerExitedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.ServiceName = traceRecord.ServiceName;
            this.ServicePackageName = traceRecord.ServicePackageName;
            this.ServicePackageActivationId = traceRecord.ServicePackageActivationId;
            this.IsExclusive = traceRecord.IsExclusive;
            this.CodePackageName = traceRecord.CodePackageName;
            this.EntryPointType = traceRecord.EntryPointType.ToString();
            this.ImageName = traceRecord.ImageName;
            this.ContainerName = traceRecord.ContainerName;
            this.HostId = traceRecord.HostId;
            this.ExitCode = traceRecord.ExitCode;
            this.UnexpectedTermination = traceRecord.UnexpectedTermination;
            this.StartTime = traceRecord.StartTime;
        }

        [JsonProperty(PropertyName = "ServiceName")]
        public string ServiceName { get; set; }

        [JsonProperty(PropertyName = "ServicePackageName")]
        public string ServicePackageName { get; set; }

        [JsonProperty(PropertyName = "ServicePackageActivationId")]
        public string ServicePackageActivationId { get; set; }

        [JsonProperty(PropertyName = "IsExclusive")]
        public bool IsExclusive { get; set; }

        [JsonProperty(PropertyName = "CodePackageName")]
        public string CodePackageName { get; set; }

        [JsonProperty(PropertyName = "EntryPointType")]
        public string EntryPointType { get; set; }

        [JsonProperty(PropertyName = "ImageName")]
        public string ImageName { get; set; }

        [JsonProperty(PropertyName = "ContainerName")]
        public string ContainerName { get; set; }

        [JsonProperty(PropertyName = "HostId")]
        public string HostId { get; set; }

        [JsonProperty(PropertyName = "ExitCode")]
        public long ExitCode { get; set; }

        [JsonProperty(PropertyName = "UnexpectedTermination")]
        public bool UnexpectedTermination { get; set; }

        [JsonProperty(PropertyName = "StartTime")]
        public DateTime StartTime { get; set; }
    }
}
