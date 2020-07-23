// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using Newtonsoft.Json.Serialization;
    using System.Linq;

    internal abstract class CustomCreationJsonConverterBase<T> : JsonConverter
    {
        private readonly Type targetType;
        private readonly bool canWrite;
        private readonly bool canRead;

        protected CustomCreationJsonConverterBase(bool canRead, bool canWrite)
        {
            this.targetType = typeof(T);
            this.canRead = canRead;
            this.canWrite = canWrite;
        }

        // Can't override it.
        public sealed override bool CanRead
        {
            get
            {
                return this.canRead;
            }
        }

        // Can't override it.
        public sealed override bool CanWrite
        {
            get
            {
                return this.canWrite;
            }
        }

        /// <summary>
        /// Create an instance of objectType, based properties in the JSON object
        /// </summary>
        /// <param name="objectType">type of object expected</param>
        /// <param name="jObject">
        /// contents of JSON object that will be deserialized
        /// </param>
        /// <returns>returns new initialized object of type T. Null is okay in case where derive class is overriding ReadJson or PopulateInstanceUsingJObject</returns>
        protected abstract T Create(Type objectType, JObject jObject);

        /// <summary>
        /// Reads the JSON representation of the object.
        /// </summary>
        /// <param name="reader">The <see cref="JsonReader"/> to read from.</param>
        /// <param name="objectType">Type of the object.</param>
        /// <param name="existingValue">The existing value of object being read.</param>
        /// <param name="serializer">The calling serializer.</param>
        /// <returns>The object value.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.Null)
            {
                return null;
            }

            JObject jObject = JObject.Load(reader);
            T target = Create(objectType, jObject);
            return this.PopulateInstanceUsingJObject(jObject, target, serializer);
        }

        // Derived class can override it to do any customization.
        protected virtual T PopulateInstanceUsingJObject(JObject jObject, T targetObject, JsonSerializer serializer)
        {
            serializer.Populate(jObject.CreateReader(), targetObject);
            return targetObject;
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            JContainer jcontainer = this.GetJContainerForInstance(value, serializer);
            jcontainer.WriteTo(writer);
        }

        /// Derived class must implement this. Expected return object is JArray or JObject.
        protected abstract JContainer GetJContainerForInstance(object value, JsonSerializer serializer);

        public JObject CreateJObjectFrom(object instance, JsonSerializer serializer)
        {
            if (instance == null)
            {
                return null;
            }

            JObject jobject = new JObject();
            Type targetType = instance.GetType();

            var contract = serializer.ContractResolver.ResolveContract(targetType) as JsonObjectContract;
            foreach (var p in contract.Properties.Where(x => !x.Ignored))
            {
                // value for this property in given instance
                var prop = targetType.GetProperty(p.UnderlyingName, Reflection.BindingFlags.Instance | Reflection.BindingFlags.Public | Reflection.BindingFlags.NonPublic);
                var pVal = prop.GetValue(instance);

                if (p.DefaultValueHandling == DefaultValueHandling.Ignore && pVal == null)
                {
                    // Ignore null
                    continue;
                }

                var pJtoken = pVal == null ? null : JToken.FromObject(pVal, serializer);
                                
                // Add jToken corresponding to this property
                jobject.Add(p.PropertyName, pJtoken);
            }

            return jobject;
        }

        public override bool CanConvert(Type objectType)
        {
            bool canConvert = targetType.IsAssignableFrom(objectType) && objectType.IsAssignableFrom(targetType);
            return canConvert;
        }
    }
}