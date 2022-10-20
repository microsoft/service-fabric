// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosCodePackageRestartScheduled")]
    public sealed class ChaosRestartCodePackageFaultScheduledEvent : ApplicationEvent
    {
        public ChaosRestartCodePackageFaultScheduledEvent(ChaosRestartCodePackageFaultScheduledTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.FaultGroupId = traceRecord.FaultGroupId;
            this.FaultId = traceRecord.FaultId;
            this.NodeName = traceRecord.NodeName;
            this.ServiceManifestName = traceRecord.ServiceManifestName;
            this.CodePackageName = traceRecord.CodePackageName;
            this.ServicePackageActivationId = traceRecord.ServicePackageActivationId;
        }

        [JsonProperty(PropertyName = "FaultGroupId")]
        public Guid FaultGroupId { get; set; }

        [JsonProperty(PropertyName = "FaultId")]
        public Guid FaultId { get; set; }

        [JsonProperty(PropertyName = "NodeName")]
        public string NodeName { get; set; }

        [JsonProperty(PropertyName = "ServiceManifestName")]
        public string ServiceManifestName { get; set; }

        [JsonProperty(PropertyName = "CodePackageName")]
        public string CodePackageName { get; set; }

        [JsonProperty(PropertyName = "ServicePackageActivationId")]
        public string ServicePackageActivationId { get; set; }
    }
}
