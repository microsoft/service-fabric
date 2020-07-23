// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ApplicationUpgradeCompleted")]
    public sealed class ApplicationUpgradeCompleteEvent : ApplicationEvent
    {
        public ApplicationUpgradeCompleteEvent(ApplicationUpgradeCompleteTraceRecord traceRecord)
            : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.ApplicationTypeName = traceRecord.ApplicationTypeName;
            this.ApplicationTypeVersion = traceRecord.ApplicationTypeVersion;
            this.OverallUpgradeElapsedTimeInMs = traceRecord.OverallUpgradeElapsedTimeInMs;
        }

        [JsonProperty(PropertyName = "ApplicationTypeName")]
        public string ApplicationTypeName { get; set; }

        [JsonProperty(PropertyName = "ApplicationTypeVersion")]
        public string ApplicationTypeVersion { get; set; }

        [JsonProperty(PropertyName = "OverallUpgradeElapsedTimeInMs")]
        public double OverallUpgradeElapsedTimeInMs { get; set; }
    }
}
