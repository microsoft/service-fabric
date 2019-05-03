// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using System.Fabric.Interop;
    using System.Globalization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal class DateTimeJsonConverter : DateTimeConverterBase
    {
        static readonly string[] datetimeFormatStrings =
        {
            "yyyy-MM-dd'T'HH:mm:ss.fffZ", // with ms UTC
            "yyyy-MM-dd'T'HH:mm:ssZ", // without ms UTC
            "yyyy-MM-dd'T'HH:mm:ss.fff", // with ms local
            "yyyy-MM-dd'T'HH:mm:ss" // without ms local
        };

    public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(DateTime);
        }

        /// <summary>
        /// Writes the JSON representation of the object.
        /// </summary>
        /// <param name="writer">The <see cref="Newtonsoft.Json.JsonWriter"/> to write to.
        /// </param><param name="value">The value.</param>
        /// <param name="serializer">The calling serializer.</param>
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            DateTime dateTimeValue = (DateTime)value;

            writer.WriteValue(dateTimeValue.ToUniversalTime().ToString("yyyy-MM-dd'T'HH:mm:ss.fffZ"));
        }

        /// <summary>
        /// Reads the JSON representation of the object.
        /// </summary>
        /// <param name = "reader">JsonReader to read from.</param>
        /// <param name = "objectType">Type of the object.</param>
        /// <param name = "existingValue">The existing value of object being read.</param>
        /// <param name = "serializer">The calling serializer.</param>
        /// <returns>The object value.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.String)
            {
                
                try
                {
                    return DateTime.ParseExact(
                        (string) reader.Value, 
                        datetimeFormatStrings, 
                        CultureInfo.InvariantCulture,
                        DateTimeStyles.AdjustToUniversal);
                }
                catch (Exception)
                {
                    // datetime could be given as ticks
                }
                try
                {
                    long ticks = long.Parse((string)reader.Value);
                    return NativeTypes.FromNativeTicks(ticks);
                }
                catch (Exception e)
                {
                    if (e is OverflowException || e is FormatException || e is ArgumentNullException)
                    {
                        throw new FabricSerializationException(
                            string.Format("DateTimeJsonConverter: Error during parsing 'FileTime' value from string : '{0}'", reader.Value ?? ""),
                            e);
                    }
                throw;
                }
            }
            else
            {
                throw new FabricSerializationException(string.Format("FileTimeDateTimeConverter(): Wrong Token Type: {0}. Expected 'JsonToken.String'", reader.TokenType));
            }

        }
    }
}