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

    internal class ApplicationHealthPolicyJsonConverter : CustomCreationJsonConverterBase<ApplicationHealthPolicy>
    {
        private const string svcPolicyMapName = "ServiceTypeHealthPolicyMap";
        private const string defaultSvcTypePolicyName = "DefaultServiceTypeHealthPolicy";

        public ApplicationHealthPolicyJsonConverter()
            : base(true, true)
        {
        }

        /// For reading json string. JObject has been created from json string already. Convert JObject
        protected override ApplicationHealthPolicy PopulateInstanceUsingJObject(JObject jObject, ApplicationHealthPolicy targetObject, JsonSerializer serializer)
        {
            serializer.Populate(jObject.CreateReader(), targetObject);
            JToken jtoken;

            if (jObject.TryGetValue(svcPolicyMapName, out jtoken))
            {
                var svcHealthPolicies = new List<KeyValuePair<string, ServiceTypeHealthPolicy>>();
                serializer.Populate(jtoken.CreateReader(), svcHealthPolicies);

                foreach (var item in svcHealthPolicies)
                {
                    targetObject.ServiceTypeHealthPolicyMap.Add(item.Key, item.Value);
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

            ApplicationHealthPolicy policy = (ApplicationHealthPolicy)value;
            var jobject = JObject.FromObject(policy);

            if (policy.DefaultServiceTypeHealthPolicy == null)
            {
                // Remove the object if it has null value, so native part can create default value instead.
                jobject.Remove(defaultSvcTypePolicyName);
            }

            if (policy.ServiceTypeHealthPolicyMap == null)
            {
                return jobject;
            }

            var svcHealthPolicies = new List<KeyValuePair<string, ServiceTypeHealthPolicy>>(policy.ServiceTypeHealthPolicyMap);

            // Remove property svcPolicyMapName of type ServiceTypeHealthPolicy: IDictionary and write it as List.
            jobject.Remove(svcPolicyMapName);

            // Add List of KeyValuePair
            jobject.Add(svcPolicyMapName, JArray.FromObject(svcHealthPolicies));

            return jobject;
        }

        protected override ApplicationHealthPolicy Create(Type objectType, Newtonsoft.Json.Linq.JObject jObject)
        {
            return new ApplicationHealthPolicy();
        }
    }
}