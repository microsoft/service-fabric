// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ApplicationUpgradeStarted")]
    public sealed class ApplicationUpgradeStartEvent : ApplicationEvent
    {
        public ApplicationUpgradeStartEvent(ApplicationUpgradeStartTraceRecord traceRecord)
            : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.ApplicationTypeName = traceRecord.ApplicationTypeName;
            this.CurrentApplicationTypeVersion = traceRecord.CurrentApplicationTypeVersion;
            this.ApplicationTypeVersion = traceRecord.ApplicationTypeVersion;
            this.UpgradeType = traceRecord.UpgradeType.ToString();
            this.RollingUpgradeMode = traceRecord.RollingUpgradeMode.ToString();
            this.FailureAction = traceRecord.FailureAction.ToString();
        }

        [JsonProperty(PropertyName = "ApplicationTypeName")]
        public string ApplicationTypeName { get; set; }

        [JsonProperty(PropertyName = "CurrentApplicationTypeVersion")]
        public string CurrentApplicationTypeVersion { get; set; }

        [JsonProperty(PropertyName = "ApplicationTypeVersion")]
        public string ApplicationTypeVersion { get; set; }

        [JsonProperty(PropertyName = "UpgradeType")]
        public string UpgradeType { get; set; }

        [JsonProperty(PropertyName = "RollingUpgradeMode")]
        public string RollingUpgradeMode { get; set; }

        [JsonProperty(PropertyName = "FailureAction")]
        public string FailureAction { get; set; }
    }
}
