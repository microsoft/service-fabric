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

    internal class ApplicationTypeHealthPolicyMapConverter : CustomCreationJsonConverterBase<ApplicationTypeHealthPolicyMap>
    {
        private const string AppTypePolicyMapName = "ApplicationTypeHealthPolicyMap";

        public ApplicationTypeHealthPolicyMapConverter()
            : base(true, true)
        {
        }

        protected override ApplicationTypeHealthPolicyMap PopulateInstanceUsingJObject(JObject jObject, ApplicationTypeHealthPolicyMap targetObject, JsonSerializer serializer)
        {
            var jApplicationTypeHealthPolicyMap = jObject[AppTypePolicyMapName];
            foreach (var jToken in jApplicationTypeHealthPolicyMap)
            {
                KeyValuePair<string, byte> kv = jToken.ToObject<KeyValuePair<string, byte>>(serializer);
                targetObject.Add(kv.Key, kv.Value);
            }

            return targetObject;
        }

        protected override ApplicationTypeHealthPolicyMap Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new ApplicationTypeHealthPolicyMap();
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            var appTypeHealthPolicyMap = (ApplicationTypeHealthPolicyMap)value;
            JObject result = new JObject();
            result.Add(AppTypePolicyMapName, JToken.FromObject(appTypeHealthPolicyMap.ApplicationTypeHealthPolicyMapWrapper, serializer));
            return result;
        }
    }
}