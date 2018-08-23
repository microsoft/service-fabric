// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.Text;

    class JsonSerializationHelper
    {
        public static JsonSerializerSettings DefaultJsonSerializerSettings;

        static JsonSerializationHelper()
        {
            var serializationSettings = new JsonSerializerSettings();
            serializationSettings.NullValueHandling = NullValueHandling.Ignore;           
            serializationSettings.Converters.Add(new StringEnumConverter() { CamelCaseText = false });           
            serializationSettings.ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor;                        

            DefaultJsonSerializerSettings = serializationSettings;
        }

        public static T DeserializeObject<T>(byte[] value)
        {
            return DeserializeObject<T>(value, DefaultJsonSerializerSettings);
        }

        public static T DeserializeObject<T>(byte[] value, JsonSerializerSettings jsonSerializerSettings)
        {
            return (T)JsonConvert.DeserializeObject(ConvertBytesToString(value), typeof(T), jsonSerializerSettings);
        }

        public static byte[] SerializeObject<T>(T value)
        {
            return SerializeObject<T>(value, DefaultJsonSerializerSettings);
        }

        public static byte[] SerializeObject<T>(T value, JsonSerializerSettings jsonSerializerSettings)
        {
            return ConvertStringToBytes(JsonConvert.SerializeObject(value, typeof(T), jsonSerializerSettings));
        }

        public static byte[] ConvertStringToBytes(string s)
        {
            return Encoding.UTF8.GetBytes(s);
        }

        public static string ConvertBytesToString(byte[] bytes)
        {
            return Encoding.UTF8.GetString(bytes);
        }
    }
}