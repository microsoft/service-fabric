// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ApplicationDeleted")]
    public sealed class ApplicationDeletedEvent : ApplicationEvent
    {
        public ApplicationDeletedEvent(ApplicationDeletedTraceRecord traceRecord)
            : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.ApplicationTypeName = traceRecord.ApplicationTypeName;
            this.ApplicationTypeVersion = traceRecord.ApplicationTypeVersion;
        }

        [JsonProperty(PropertyName = "ApplicationTypeName")]
        public string ApplicationTypeName { get; set; }

        [JsonProperty(PropertyName = "ApplicationTypeVersion")]
        public string ApplicationTypeVersion { get; set; }
    }
}
