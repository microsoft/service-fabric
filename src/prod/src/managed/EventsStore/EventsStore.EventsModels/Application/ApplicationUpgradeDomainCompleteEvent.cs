// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ApplicationUpgradeDomainCompleted")]
    public sealed class ApplicationUpgradeDomainCompleteEvent : ApplicationEvent
    {
        public ApplicationUpgradeDomainCompleteEvent(ApplicationUpgradeDomainCompleteTraceRecord traceRecord)
            : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.ApplicationTypeName = traceRecord.ApplicationTypeName;
            this.CurrentApplicationTypeVersion = traceRecord.CurrentApplicationTypeVersion;
            this.ApplicationTypeVersion = traceRecord.ApplicationTypeVersion;
            this.UpgradeState = traceRecord.UpgradeState.ToString();
            this.UpgradeDomains = traceRecord.UpgradeDomains;
            this.UpgradeDomainElapsedTimeInMs = traceRecord.UpgradeDomainElapsedTimeInMs;
        }

        [JsonProperty(PropertyName = "ApplicationTypeName")]
        public string ApplicationTypeName { get; set; }

        [JsonProperty(PropertyName = "CurrentApplicationTypeVersion")]
        public string CurrentApplicationTypeVersion { get; set; }

        [JsonProperty(PropertyName = "ApplicationTypeVersion")]
        public string ApplicationTypeVersion { get; set; }

        [JsonProperty(PropertyName = "UpgradeState")]
        public string UpgradeState { get; set; }

        [JsonProperty(PropertyName = "UpgradeDomains")]
        public string UpgradeDomains { get; set; }

        [JsonProperty(PropertyName = "UpgradeDomainElapsedTimeInMs")]
        public double UpgradeDomainElapsedTimeInMs { get; set; }
    }
}
