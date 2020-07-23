// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using System.Numerics;

    // Will convert Int64 <=> JSON string.
    internal class Int64JsonConverter : JsonConverter
    {
        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(long) || objectType == typeof(BigInteger);
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            // Could be Int64 or BigInteger
            writer.WriteValue(value.ToString());
        }

        public override object ReadJson(JsonReader reader, Type type, object value, JsonSerializer serializer)
        {
            if (reader.TokenType != JsonToken.String && reader.TokenType != JsonToken.Integer)
            {
                throw new FabricSerializationException(string.Format("Int64JsonConverter(): Wrong Token Type: {0}. Expected 'JsonToken.String' or 'JsonToken.Integer'", reader.TokenType));
            }

            try
            {
                var stringValue = reader.Value.ToString();
                object result;

                if (type == typeof(BigInteger))
                {
                    result = BigInteger.Parse(stringValue);
                }
                else
                {
                    result = long.Parse(stringValue);
                }

                return result;
            }
            catch (Exception e)
            {
                if (e is OverflowException || e is FormatException || e is ArgumentNullException)
                {
                    throw new FabricSerializationException(
                        string.Format("Int64JsonConverter(): Error during parsing value of type '{0}' from string : '{1}'", type.Name, reader.Value ?? ""),
                        e);
                }

                throw;
            }
        }
    }
}
