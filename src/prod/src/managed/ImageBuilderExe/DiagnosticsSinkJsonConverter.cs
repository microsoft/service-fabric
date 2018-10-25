// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageBuilderExe
{
    using System;
    using Newtonsoft.Json;
    using System.Fabric.Management.ImageBuilder.SingleInstance;
    using Newtonsoft.Json.Linq;

    internal class DiagnosticsSinkJsonConverter : JsonConverter
    {
        public override bool CanConvert(Type objectType)
        {
            return typeof(DiagnosticsSinkBase).IsAssignableFrom(objectType);
        }

        public override bool CanWrite { get { return false; } }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            JObject item = JObject.Load(reader);
            string kind = item["kind"].Value<string>();

            if (kind == "AzureInternalMonitoringPipeline")
            {
                return item.ToObject<AzureInternalMonitoringPipelineSinkDescription>();
            }

            throw new JsonReaderException(string.Format("Unknown type of DiagnosticsSink. {0}", kind));
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }
}