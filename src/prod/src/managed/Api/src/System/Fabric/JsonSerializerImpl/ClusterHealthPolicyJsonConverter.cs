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

    internal class ClusterHealthPolicyJsonConverter : CustomCreationJsonConverterBase<ClusterHealthPolicy>
    {
        private const string AppTypePolicyMapName = "ApplicationTypeHealthPolicyMap";

        public ClusterHealthPolicyJsonConverter()
            : base(true, true)
        {
        }

        /// For reading json string. JObject has been created from json string already. Convert JObject
        protected override ClusterHealthPolicy PopulateInstanceUsingJObject(JObject jObject, ClusterHealthPolicy targetObject, JsonSerializer serializer)
        {
            serializer.Populate(jObject.CreateReader(), targetObject);
            JToken jtoken;

            if (jObject.TryGetValue(AppTypePolicyMapName, out jtoken))
            {
                var appTypeHealthPolicies = new List<KeyValuePair<string, byte>>();
                serializer.Populate(jtoken.CreateReader(), appTypeHealthPolicies);

                foreach (var item in appTypeHealthPolicies)
                {
                    targetObject.ApplicationTypeHealthPolicyMap.Add(item.Key, item.Value);
                }
            }

            return targetObject;
        }

        /// For writing json string
        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            if (value == null)
            {
                return null;
            }

            ClusterHealthPolicy policy = (ClusterHealthPolicy)value;
            var jobject = JObject.FromObject(policy);
            if (policy.ApplicationTypeHealthPolicyMap == null)
            {
                return jobject;
            }

            var appTypePolicyMap = new List<KeyValuePair<string, byte>>(policy.ApplicationTypeHealthPolicyMap);

            // Remove property AppTypePolicyMapName of type IDictionary and write it as List.
            jobject.Remove(AppTypePolicyMapName);

            // Add List of KeyValuePair
            jobject.Add(AppTypePolicyMapName, JArray.FromObject(appTypePolicyMap));

            return jobject;
        }

        protected override ClusterHealthPolicy Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new ClusterHealthPolicy();
        }
    }
}