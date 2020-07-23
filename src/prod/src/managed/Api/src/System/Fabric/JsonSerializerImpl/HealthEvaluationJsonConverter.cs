// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using Newtonsoft.Json.Serialization;
    using System.Fabric.Health;

    internal class HealthEvaluationJsonReaderImpl : CustomCreationJsonConverterBase<HealthEvaluation>
    {
        public HealthEvaluationJsonReaderImpl()
            : base(true, false) // Can-read = true, resolves derived type. Can-Write= false -- Should not be needed.
        {
        }

        protected override HealthEvaluation Create(Type objectType, JObject jObject)
        {
            return null;
        }
        protected override HealthEvaluation PopulateInstanceUsingJObject(JObject jObject, HealthEvaluation targetObject, JsonSerializer serializer)
        {
            JToken jToken;

            // Remove wrapper property HealthEvaluation (added from native serializer).
            if (jObject.TryGetValue("HealthEvaluation", out jToken))
            {
                return jToken.ToObject<HealthEvaluation>(serializer);
            }

            // Retrieve "Kind"
            if (!jObject.TryGetValue("Kind", out jToken))
            {
                return null;
            }

            HealthEvaluationKind kind = jToken.ToObject<HealthEvaluationKind>(serializer);
            Type targetType = HealthEvaluation.GetDerivedHealthEvaluationClassTypeFromKind(kind);
            return (HealthEvaluation)jObject.ToObject(targetType, serializer);
        }

        protected HealthEvaluation PopulateInstanceUsingJObjectCore(JObject jObject, HealthEvaluation targetObject, JsonSerializer serializer)
        {

            JToken jHealthEvaluationCore;

            jObject.TryGetValue("HealthEvaluation", out jHealthEvaluationCore);

            // Retrieve "Kind"
            if (!(jHealthEvaluationCore as JObject).TryGetValue("Kind", out jHealthEvaluationCore))
            {
                return null;
            }

            HealthEvaluationKind kind = jHealthEvaluationCore.ToObject<HealthEvaluationKind>(serializer);
            Type targetType = HealthEvaluation.GetDerivedHealthEvaluationClassTypeFromKind(kind);
            var result = (HealthEvaluation)Activator.CreateInstance(targetType, true);
            if (true)
            {

                foreach (var tok in jHealthEvaluationCore.Children())
                {
                }
                var contract = serializer.ContractResolver.ResolveContract(targetType) as JsonObjectContract;
                foreach (var p in contract.Properties)
                {
                    // value for this property in given healthEvaluation instance
                    var jtoken = jHealthEvaluationCore.SelectToken(p.PropertyName);
                    if (jtoken != null)
                    {
                        targetType.GetProperty(p.UnderlyingName).SetValue(result, jtoken.ToObject(p.PropertyType, serializer));
                    }
                }

                return result;
            }
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            // This method call is not expected.
            throw new FabricSerializationException("HealthEvaluationJsonConverterImpl(): Invalid operation 'WriteJson'. This JsonConverter does not implement WriteJson.");
        }
    }

    internal class HealthEvaluationJsonWriterImpl : CustomCreationJsonConverterBase<HealthEvaluation>
    {
        public HealthEvaluationJsonWriterImpl()
            : base(false, true)
        {
        }

        public override bool CanConvert(Type objectType)
        {
            /// All instance of HealthEvaluation and it's derived classes.
            return typeof(HealthEvaluation).IsAssignableFrom(objectType);
        }

        protected override JContainer GetJContainerForInstance(object value, JsonSerializer serializer)
        {
            // crate internal JSON from input HealthEvaluation
            JObject jHealthEvaluationCore = this.CreateJObjectFrom((HealthEvaluation)value, serializer);
            if (jHealthEvaluationCore == null)
            {
                return null;
            }

            // Insert one property in resultWithWrapper with name 'HealthEvaluation' which would wrap the core part
            JObject resultWithWrapper = new JObject();
            resultWithWrapper.Add("HealthEvaluation", jHealthEvaluationCore);
            return resultWithWrapper;
        }

        protected override HealthEvaluation Create(Type objectType, JObject jObject)
        {
            // This method call is not expected.
            throw new FabricSerializationException("HealthEvaluationJsonWriterImpl(): Create(): Invalid operation 'ReadJson'. This JsonConverter does not implement ReadJson.");
        }
    }
}
