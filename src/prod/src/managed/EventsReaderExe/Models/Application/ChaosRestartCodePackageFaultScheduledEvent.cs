// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader.Models.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    [JsonObject("ChaosRestartCodePackageFaultScheduled")]
    internal sealed class ChaosRestartCodePackageFaultScheduledEvent : ApplicationEvent
    {
        public ChaosRestartCodePackageFaultScheduledEvent(Guid eventInstanceId, DateTime timeStamp, string applicationName, Guid faultGroupId, Guid faultId, string nodeName, string serviceManifestName, string codePackageName, string servicePackageActivationId) : base(eventInstanceId, timeStamp, applicationName)
        {
            this.FaultGroupId = faultGroupId;
            this.FaultId = faultId;
            this.NodeName = nodeName;
            this.ServiceManifestName = serviceManifestName;
            this.CodePackageName = codePackageName;
            this.ServicePackageActivationId = servicePackageActivationId;
        }

        public ChaosRestartCodePackageFaultScheduledEvent(ChaosRestartCodePackageFaultScheduledTraceRecord chaosRestartCodePackageFaultScheduledTraceRecord) : base(chaosRestartCodePackageFaultScheduledTraceRecord.EventInstanceId, chaosRestartCodePackageFaultScheduledTraceRecord.TimeStamp, chaosRestartCodePackageFaultScheduledTraceRecord.ApplicationName)
        {
            this.FaultGroupId = chaosRestartCodePackageFaultScheduledTraceRecord.FaultGroupId;
            this.FaultId = chaosRestartCodePackageFaultScheduledTraceRecord.FaultId;
            this.NodeName = chaosRestartCodePackageFaultScheduledTraceRecord.NodeName;
            this.ServiceManifestName = chaosRestartCodePackageFaultScheduledTraceRecord.ServiceManifestName;
            this.CodePackageName = chaosRestartCodePackageFaultScheduledTraceRecord.CodePackageName;
            this.ServicePackageActivationId = chaosRestartCodePackageFaultScheduledTraceRecord.ServicePackageActivationId;
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
