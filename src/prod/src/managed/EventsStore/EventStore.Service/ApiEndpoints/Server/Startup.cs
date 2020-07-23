// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Server
{
    using System;
    using System.Web.Http;
    using System.Threading;
    using System.Net;
    using System.Fabric;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Linq;
    using System.Net.Http.Formatting;
    using Microsoft.ServiceFabric.Services.Runtime;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Serialization;
    using Owin;
    using EventStore.Service.ApiEndpoints.Controllers;
    using EventStore.Service.ApiEndpoints.Handlers;

    public class Startup
    {
        private ControllerSetting controllerSetting;

        private SecuritySetting fabricSecuritySetting;

        internal Startup(EventStoreRuntime currentRuntime, SecuritySetting securitySetting, CancellationToken cancellationToken)
        {
            this.controllerSetting = new ControllerSetting(currentRuntime, cancellationToken);
            this.fabricSecuritySetting = securitySetting;
        }

        public void Configuration(IAppBuilder appBuilder)
        {
            // Configure Web API for self-host. 
            HttpConfiguration config = new HttpConfiguration
            {
                DependencyResolver = new ControllerDependencyResolver<BaseController>(this.controllerSetting)
            };

            switch (this.fabricSecuritySetting.ServerAuthCredentialType)
            {
                case CredentialType.None:
                    break;

                case CredentialType.X509:
                    config.MessageHandlers.Add(new ClientCertAuthorizationHandler(this.fabricSecuritySetting));
                    break;

                case CredentialType.Windows:
                    HttpListener listener = (HttpListener)appBuilder.Properties["System.Net.HttpListener"];
                    listener.AuthenticationSchemes = AuthenticationSchemes.IntegratedWindowsAuthentication;
                    config.MessageHandlers.Add(new ClientIdentityAuthorizationHandler(this.fabricSecuritySetting));
                    break;

                default:
                    throw new Exception(string.Format("Unexpected Security Setting: {0}", this.fabricSecuritySetting.ServerAuthCredentialType));
            }

            config.MessageHandlers.Add(new XContentTypeHandler());
            config.MapHttpAttributeRoutes();

            SetupJSONSerializer(config);

            appBuilder.UseWebApi(config);
        }

        private static void SetupJSONSerializer(HttpConfiguration config)
        {
            var serializationSettings = new JsonSerializerSettings
            {
                Formatting = Formatting.Indented,
                DateFormatHandling = DateFormatHandling.IsoDateFormat,
                DateTimeZoneHandling = DateTimeZoneHandling.Utc,
                NullValueHandling = NullValueHandling.Ignore,
                ReferenceLoopHandling = ReferenceLoopHandling.Serialize,
                Converters = new List<JsonConverter>
                    {
                        new PolymorphicSerializeJsonConverter<EventsStore.EventsModels.FabricEvent>("Kind")
                    }
            };

            JsonConvert.DefaultSettings = () => { return serializationSettings; };
            config.Formatters.Clear();
            config.Formatters.Add(new JsonMediaTypeFormatter());
            config.Formatters.JsonFormatter.SerializerSettings = serializationSettings;
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