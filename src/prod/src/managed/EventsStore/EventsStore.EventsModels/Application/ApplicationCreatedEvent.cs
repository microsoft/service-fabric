// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using Newtonsoft.Json;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    [JsonObject("ApplicationCreated")]
    public sealed class ApplicationCreatedEvent : ApplicationEvent
    {
        public ApplicationCreatedEvent(ApplicationCreatedTraceRecord traceRecord) : base(traceRecord.EventInstanceId, traceRecord.TimeStamp, traceRecord.Category, traceRecord.ApplicationName)
        {
            this.ApplicationTypeName = traceRecord.ApplicationTypeName;
            this.ApplicationTypeVersion = traceRecord.ApplicationTypeVersion;
            this.ApplicationDefinitionKind = traceRecord.ApplicationDefinitionKind.ToString();
        }

        [JsonProperty(PropertyName = "ApplicationTypeName")]
        public string ApplicationTypeName { get; set; }

        [JsonProperty(PropertyName = "ApplicationTypeVersion")]
        public string ApplicationTypeVersion { get; set; }

        [JsonProperty(PropertyName = "ApplicationDefinitionKind")]
        public string ApplicationDefinitionKind { get; set; }
    }
}
