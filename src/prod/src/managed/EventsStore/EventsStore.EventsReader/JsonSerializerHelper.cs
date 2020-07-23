// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;

    /// <summary>
    /// Represents a single collection segment of an Uri.
    /// </summary>
    internal class JsonSerializerHelper
    {
        /// <summary>
        /// Settings for serialization.
        /// </summary>
        private JsonSerializerSettings serializationSettings;

        public JsonSerializerHelper()
        {
            serializationSettings = new JsonSerializerSettings
            {
                Formatting = Newtonsoft.Json.Formatting.Indented,
                DateFormatHandling = Newtonsoft.Json.DateFormatHandling.IsoDateFormat,
                DateTimeZoneHandling = Newtonsoft.Json.DateTimeZoneHandling.Utc,
                NullValueHandling = Newtonsoft.Json.NullValueHandling.Ignore,
                ReferenceLoopHandling = Newtonsoft.Json.ReferenceLoopHandling.Serialize,
                Converters = new List<JsonConverter>
                    {
                        new PolymorphicSerializeJsonConverter<EventsStore.EventsModels.FabricEvent>("Kind")
                    }
            };
        }

        /// <summary>
        /// Serializes an object.
        /// </summary>
        /// <param name="obj">The object to be serialized into Json</param>
        /// <returns></returns>
        public string SerializeObject(object obj)
        {
            var serializer = JsonSerializer.Create(serializationSettings);
            var stringWriter = new StringWriter(CultureInfo.InvariantCulture);
            using (var jsonWriter = new JsonTextWriter(stringWriter) { Formatting = serializer.Formatting })
            {
                serializer.Serialize(jsonWriter, obj);
            }

            return stringWriter.ToString();

        }

        private class PolymorphicSerializeJsonConverter<T> : JsonConverter where T : class
        {
            /// <summary>
            /// Initializes an instance of the PolymorphicSerializeJsonConverter.
            /// </summary>
            /// <param name="discriminatorField">The JSON field used as a discriminator</param>

            public PolymorphicSerializeJsonConverter(string discriminatorField)
            {
                if (discriminatorField == null)
                {
                    throw new ArgumentNullException("discriminatorField");
                }

                Discriminator = discriminatorField;
            }

            /// <summary>
            /// Discriminator property name.
            /// </summary>
            public string Discriminator { get; protected set; }

            /// <summary>
            /// Returns type that matches specified name.
            /// </summary>
            /// <param name="baseType">Base type.</param>
            /// <param name="name">Derived type name</param>
            /// <returns></returns>
            public static Type GetDerivedType(Type baseType, string name)
            {
                if (baseType == null)
                {
                    throw new ArgumentNullException("baseType");
                }

                foreach (TypeInfo type in baseType.GetTypeInfo().Assembly.DefinedTypes
                    .Where(t => t.Namespace == baseType.Namespace && t != baseType.GetTypeInfo()))
                {
                    string typeName = type.Name;
                    if (type.GetCustomAttributes<JsonObjectAttribute>().Any())
                    {
                        typeName = type.GetCustomAttribute<JsonObjectAttribute>().Id;
                    }

                    if (typeName != null && typeName.Equals(name, StringComparison.OrdinalIgnoreCase))
                    {
                        return type.AsType();
                    }
                }

                return null;
            }

            /// <summary>
            /// Returns true if the object being serialized is assignable from the base type. False otherwise.
            /// </summary>
            /// <param name="objectType">The type of the object to check.</param>
            /// <returns>True if the object being serialized is assignable from the base type. False otherwise.</returns>
            public override bool CanConvert(Type objectType)
            {
                return typeof(T).GetTypeInfo().IsAssignableFrom(objectType.GetTypeInfo());
            }

            /// <summary>
            /// Returns false.
            /// </summary>
            public override bool CanRead
            {
                get { return false; }
            }

            /// <summary>
            /// Throws NotSupportedException.
            /// </summary>
            /// <param name="reader">The JSON reader.</param>
            /// <param name="objectType">The type of the object.</param>
            /// <param name="existingValue">The existing value.</param>
            /// <param name="serializer">The JSON serializer.</param>
            /// <returns></returns>
            public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
            {
                throw new NotSupportedException();
            }

            /// <summary>
            /// Serializes an object into a JSON string based on discriminator
            /// field and object name. If JsonObject attribute is available, its value is used instead.
            /// </summary>
            /// <param name="writer">The JSON writer.</param>
            /// <param name="value">The value to serialize.</param>
            /// <param name="serializer">The JSON serializer.</param>
            public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
            {
                if (writer == null)
                {
                    throw new ArgumentNullException("writer");
                }

                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                if (serializer == null)
                {
                    throw new ArgumentNullException("serializer");
                }

                string typeName = value.GetType().Name;
                if (value.GetType().GetTypeInfo().GetCustomAttributes<JsonObjectAttribute>().Any())
                {
                    typeName = value.GetType().GetTypeInfo().GetCustomAttribute<JsonObjectAttribute>().Id;
                }

                // Add discriminator field
                writer.WriteStartObject();
                writer.WritePropertyName(Discriminator);
                writer.WriteValue(typeName);

                // Serialize properties
                var contract = (JsonObjectContract)serializer.ContractResolver.ResolveContract(value.GetType());

                foreach (JsonProperty property in contract.Properties)
                {
                    object memberValue = property.ValueProvider.GetValue(value);

                    // Skipping properties with null value if NullValueHandling is set to Ignore
                    if (serializer.NullValueHandling == NullValueHandling.Ignore && memberValue == null)
                    {
                        continue;
                    }

                    // Skipping properties with JsonIgnore attribute, non-readable, and 
                    // ShouldSerialize returning false when set
                    if (!property.Ignored && property.Readable &&
                        (property.ShouldSerialize == null || property.ShouldSerialize(memberValue)))
                    {
                        string propertyName = property.PropertyName;
                        if (property.PropertyName.StartsWith("properties.", StringComparison.OrdinalIgnoreCase))
                        {
                            propertyName = property.PropertyName.Substring("properties.".Length);
                        }

                        writer.WritePropertyName(propertyName);
                        serializer.Serialize(writer, memberValue);
                    }
                }
                writer.WriteEndObject();
            }
        }
    }
}