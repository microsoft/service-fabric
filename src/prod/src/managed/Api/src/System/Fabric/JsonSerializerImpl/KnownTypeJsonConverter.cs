// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using Newtonsoft.Json.Serialization;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Common.Serialization;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.Serialization;

    /// <summary>
    /// This class will be useful for serializing abstract type which have KnownTypeAttribute(s).
    /// It uses KnownTypeAttribute to match appropriate derived types suggested by KnownTypeAttribute(s).
    /// Selects the first class which has all properties in the json object.
    /// </summary>
    internal class KnownTypeJsonConverter : JsonConverter
    {
        public override bool CanConvert(Type objectType)
        {
            var attr = objectType.GetCustomAttributes(typeof(KnownTypeAttribute), false);
            return attr != null && attr.Length > 0;
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            JObject jObject = JObject.Load(reader);

            Type actualImplType = this.ResolveImplType(objectType, jObject, serializer);
            return serializer.Deserialize(jObject.CreateReader(), actualImplType);
        }

        private Type ResolveImplType(Type objectType, JObject givenJObject, JsonSerializer serializer)
        {
            Type targetType = ResolveImplTypeUsingTypeResolverMethod(objectType, givenJObject, serializer);
            if(targetType != null)
            {
                return targetType;
            }

            IEnumerable<Type> knownImplTypes =
                System.Attribute.GetCustomAttributes(objectType)   // Select all custom attribute
                .Where(ca => ca is KnownTypeAttribute)             // filter which are KnownTypeAttribute
                .Select(kta => (kta as KnownTypeAttribute).Type);         // return type of Attribute as type of KnownTypeAttribute

            var jsonKeysInJsonString = givenJObject.Select<KeyValuePair<string, JToken>, string>(kv => kv.Key).ToList();
            var candidateJsonPropertiesUnion = new JsonPropertyCollection(objectType).Select(p => p.PropertyName);

            foreach (var candidateImplType in knownImplTypes)
            {
                var jsonContract = serializer.ContractResolver.ResolveContract(candidateImplType) as JsonObjectContract;
                candidateJsonPropertiesUnion = candidateJsonPropertiesUnion.Union(jsonContract.Properties.Select(p => p.PropertyName));
            }

            var relevantJsonKeysInJson = jsonKeysInJsonString.Intersect(candidateJsonPropertiesUnion);

            foreach (var candidateImplType in knownImplTypes)
            {
                var jsonContract = serializer.ContractResolver.ResolveContract(candidateImplType) as JsonObjectContract;
                var jsonPropsInType = jsonContract.Properties;
                var jsonPropNamesInType = jsonPropsInType.Select(p => p.PropertyName);
                var missingProperties = relevantJsonKeysInJson.Except(jsonPropNamesInType);
                if (missingProperties.FirstOrDefault() == null)
                {
                    return candidateImplType;
                }
            }

            throw new FabricSerializationException(
                string.Format("KnownTypeJsonConverter(): None of the known types: {0} has all the properties contained in given json string. Properties in json string: {1}.",
                string.Join(",", knownImplTypes.Select(t => t.Name)), // comma separated names of known types.
                string.Join(",", jsonKeysInJsonString)));             // comma separated keys in json object
        }

        private Type ResolveImplTypeUsingTypeResolverMethod(Type objectType, JObject givenJObject, JsonSerializer serializer)
        {
            // get all static methods.
            MethodInfo[] methods = objectType.GetMethods(BindingFlags.Static|BindingFlags.NonPublic);

            // get method with 'DerivedTypeResolverAttribute' if any
            var method = methods.FirstOrDefault(m => m.GetCustomAttribute<DerivedTypeResolverAttribute>() != null);
            if(method == null)
            {
                return null;
            }

            // Find property name of the resolver parameter in json.
            string jsonPropertyName = method.GetCustomAttribute<DerivedTypeResolverAttribute>().ResolverJsonPropertyName;
            Type parameterType = method.GetParameters()[0].ParameterType;

            // Get parameter for invoking resolver method.
            dynamic kind = givenJObject.GetValue(jsonPropertyName).ToObject(parameterType);
            var args = new object[] { kind };

            Type targetType = (Type) method.Invoke(null, args);
            return targetType;
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new FabricSerializationException("KnownTypeJsonConverter(): Invalid operation 'WriteJson'. This JsonConverter is only meant for reading json string.");
        }
    }
}
