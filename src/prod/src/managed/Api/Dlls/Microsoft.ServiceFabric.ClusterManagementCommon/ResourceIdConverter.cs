// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Fabric.Common;
    using Newtonsoft.Json;
    
    internal class ResourceIdConverter : JsonConverter
    {
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(ResourceId);
        }

        public override object ReadJson(
            JsonReader reader,
            Type objectType,
            object existingValue,
            JsonSerializer serializer)
        {
            var value = (string)reader.Value;

            if (value == null)
            {
                return null;
            }

            ResourceId resourceId;
            if (!string.IsNullOrEmpty(value) && ResourceId.TryParse(value, out resourceId))
            {
                return resourceId;
            }

            throw new ArgumentException("{0} is not a valid ResourceId", value);
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            ResourceId resourceId = value as ResourceId;
            ReleaseAssert.AssertIf(resourceId == null, "This converter should be called only for ResourceId");

            writer.WriteValue(resourceId.ToStringOriginal());
        }
    }
}