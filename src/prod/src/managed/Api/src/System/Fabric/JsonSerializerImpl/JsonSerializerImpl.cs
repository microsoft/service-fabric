// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using System.Collections.Generic;
    using System.Fabric.JsonSerializerWrapper;
    using System.IO;
    using System.Text;
    using Newtonsoft.Json.Converters;

    public class JsonSerializerImpl : IJsonSerializer
    {
        public static readonly Encoding DefaultEncoding = null;
        private readonly Encoding localEncoding;
        private JsonSerializerSettings settings;
        private JsonSerializer jsonSerializer;

        static JsonSerializerImpl()
        {
            DefaultEncoding = Encoding.UTF8;
        }

        /// <summary>
        /// Constructor with customized formatting and encoding.
        /// </summary>
        /// <param name="indentedFormatting">if true, output string will be formatted.</param>
        /// <param name="encoding">if null class static variable DefaultEncoding will be used.</param>
        public JsonSerializerImpl(bool indentedFormatting, Encoding encoding)
        {
            // If null use default encoding.
            this.localEncoding = encoding ?? DefaultEncoding;

            // Create setting for serializer.
            this.settings = new JsonSerializerSettings
            {
                Converters = new List<JsonConverter> {
                    new DateTimeJsonConverter(),
                    new TimeSpanToMilliSecondsJsonConverter(),
                    new Int64JsonConverter(),
                    new HealthEventJsonConverter(),
                    new NodeIdJsonConverter(),
                    new HealthEvaluationJsonReaderImpl(),
                    new HealthEvaluationJsonWriterImpl(),
                    new ApplicationHealthPolicyJsonConverter(),
                    new ApplicationHealthPolicyMapConverter(),
                    new ApplicationTypeHealthPolicyMapConverter(),
                    new ApplicationUpgradeDescriptionJsonConverter(),
                    new ApplicationUpgradeUpdateDescriptionJsonConverter(),
                    new ClusterHealthPolicyJsonConverter(),
                    new FabricUpgradeDescriptionJsonConverter(),
                    new FabricUpgradeUpdateDescriptionJsonConverter(),
                    new UpgradePolicyDescriptionJsonConverter(),
                    new RollingUpgradePolicyDescriptionJsonConverter(),
                    new ChaosEventJsonConverter(),
                    new ChaosParametersJsonConverter(),
                    new SafetyCheckJsonConverter(),
                    new KnownTypeJsonConverter(),
                },

                ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor,
                ContractResolver = CustomContractResolver.Instance,
                DateParseHandling = DateParseHandling.None  // We parse DateTime/TimeSpan using our custom converters
            };

            if (indentedFormatting)
            {
                this.settings.Formatting = Formatting.Indented;
            }

            // Create serializer using above settings.
            this.jsonSerializer = JsonSerializer.Create(this.settings);
        }

        public JsonSerializerImpl()
            :this(false, null)
        {
        }

        public void Serialize(object item, Stream stream)
        {
            try
            {
                using (var sr = new StreamWriter(stream))
                using (var jsonTextWriter = new JsonTextWriter(sr))
                {
                    this.jsonSerializer.Serialize(jsonTextWriter, item);

                    // Flush before returning.
                    jsonTextWriter.Flush();
                }
            }
            catch (Exception e)
            {
                throw new FabricSerializationException(string.Format("JsonSerializerImpl.Serialize(): Exception occured during serialization of type: {0}", item.GetType().Name), e);
            }
        }

        public string Serialize(object item)
        {
            MemoryStream mStream = new MemoryStream();
            this.Serialize(item, mStream);

            string jsonString = GetString(mStream);
            return jsonString;
        }

        public object Deserialize(Stream stream, Type type)
        {
            try
            {
                using (var sr = new StreamReader(stream))
                using (var jsonTextReader = new JsonTextReader(sr))
                {
                    return this.jsonSerializer.Deserialize(jsonTextReader, type);
                }
            }
            catch (Exception e)
            {
                throw new FabricSerializationException(string.Format("JsonSerializerImpl.Deserialize(): Exception occured during deserialization of type: {0}", type.Name), e);
            }
        }

        public object Deserialize(string jsonString, Type type)
        {
            MemoryStream mStream = GetMemoryStream(jsonString);
            return this.Deserialize(mStream, type);
        }

        // returns MemoryStream which supports GetBuffer() method call.
        private static MemoryStream GetMemoryStream(string str)
        {
            byte[] bytes = DefaultEncoding.GetBytes(str);
            var mStream = new MemoryStream(bytes, 0, bytes.Length, false, true);
            return mStream;
        }

        private static string GetString(MemoryStream mStream)
        {
            return DefaultEncoding.GetString(mStream.GetBuffer());
        }
    }
}