//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter
{
    using System.Fabric.Interop;
    using System.Globalization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Web.Http;

    internal class DateTimeJsonConverter : DateTimeConverterBase
    {
        static readonly string[] datetimeFormatStrings =
        {
            "yyyy-MM-dd'T'HH:mm:ssZ", // without ms UTC
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

            writer.WriteValue(dateTimeValue.ToUniversalTime().ToString("yyyy-MM-dd'T'HH:mm:ssZ"));
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
                    long ticks = long.Parse((string)reader.Value, CultureInfo.InvariantCulture);
                    return NativeTypes.FromNativeTicks(ticks);
                }
                catch (Exception)
                {
                    var fabricError = new FabricError
                    {
                        Message = string.Format(String.Format("Invalid DateTime format {0} ", reader.Value)),
                        Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                    };

                    throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
                }
            }
            else
            {
                var fabricError = new FabricError
                {
                    Message = string.Format(String.Format("Invalid DateTime format {0} ", reader.Value)),
                    Code = NativeTypes.FABRIC_ERROR_CODE.E_INVALIDARG,
                };

                throw new HttpResponseException(new FabricErrorError(fabricError).ToHttpResponseMessage());
            }
        }
    }
}
