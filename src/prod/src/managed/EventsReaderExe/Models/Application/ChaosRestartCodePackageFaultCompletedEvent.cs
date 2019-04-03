// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosRestartCodePackageFaultCompleted")]
    internal sealed class ChaosRestartCodePackageFaultCompletedEvent : ApplicationEvent
    {
        public ChaosRestartCodePackageFaultCompletedEvent(Guid eventInstanceId, DateTime timeStamp, string applicationName, Guid faultGroupId, Guid faultId, string nodeName, string serviceManifestName, string codePackageName, string servicePackageActivationId) : base(eventInstanceId, timeStamp, applicationName)
        {
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
            this.NodeName = nodeName;
            this.ServiceManifestName = serviceManifestName;
            this.CodePackageName = codePackageName;
            this.ServicePackageActivationId = servicePackageActivationId;
        }

        public ChaosRestartCodePackageFaultCompletedEvent(ChaosRestartCodePackageFaultCompletedTraceRecord chaosRestartCodePackageFaultCompletedTraceRecord) : base(chaosRestartCodePackageFaultCompletedTraceRecord.EventInstanceId, chaosRestartCodePackageFaultCompletedTraceRecord.TimeStamp, chaosRestartCodePackageFaultCompletedTraceRecord.ApplicationName)
        {
            this.FaultGroupId = chaosRestartCodePackageFaultCompletedTraceRecord.FaultGroupId;
            this.FaultId = chaosRestartCodePackageFaultCompletedTraceRecord.FaultId;
            this.NodeName = chaosRestartCodePackageFaultCompletedTraceRecord.NodeName;
            this.ServiceManifestName = chaosRestartCodePackageFaultCompletedTraceRecord.ServiceManifestName;
            this.CodePackageName = chaosRestartCodePackageFaultCompletedTraceRecord.CodePackageName;
            this.ServicePackageActivationId = chaosRestartCodePackageFaultCompletedTraceRecord.ServicePackageActivationId;
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
