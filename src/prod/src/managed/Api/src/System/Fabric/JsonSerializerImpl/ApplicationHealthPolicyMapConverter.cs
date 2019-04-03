// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Collections.Generic;
    using System.Fabric.Health;

    internal class ApplicationHealthPolicyMapConverter : CustomCreationJsonConverterBase<ApplicationHealthPolicyMap>
    {
        public ApplicationHealthPolicyMapConverter()
            : base(true, true)
        {
        }

        protected override ApplicationHealthPolicyMap PopulateInstanceUsingJObject(JObject jObject, ApplicationHealthPolicyMap targetObject, JsonSerializer serializer)
        {
            var jApplicationHealthPolicyMap = jObject["ApplicationHealthPolicyMap"];
            foreach (var jToken in jApplicationHealthPolicyMap)
            {
                KeyValuePair<Uri, ApplicationHealthPolicy> kv = jToken.ToObject<KeyValuePair<Uri, ApplicationHealthPolicy>>(serializer);
                targetObject.Add(kv.Key, kv.Value);
            }

            return targetObject;
        }

        protected override ApplicationHealthPolicyMap Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new ApplicationHealthPolicyMap();
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            ApplicationHealthPolicyMap appHealthPolicyMap = (ApplicationHealthPolicyMap)value;
            JObject result = new JObject();
            result.Add("ApplicationHealthPolicyMap", JToken.FromObject(appHealthPolicyMap.ApplicationHealthPolicyMapWrapper, serializer));
            return result;
        }
    }
}