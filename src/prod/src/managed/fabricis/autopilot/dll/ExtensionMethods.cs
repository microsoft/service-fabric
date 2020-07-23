// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;

    internal static class ExtensionMethods
    {
        private static JsonSerializerSettings publicSerializationSettings = CreatePublicSerializationSettings();

        private static string ConvertToJson<T>(this T value)
        {
            string json = JsonConvert.SerializeObject(value, typeof(T), publicSerializationSettings);

            return json;
        }

        private static JsonSerializerSettings CreatePublicSerializationSettings()
        {
            var settings = new JsonSerializerSettings
            {
                Formatting = Formatting.Indented,
                NullValueHandling = NullValueHandling.Ignore,
                ReferenceLoopHandling = ReferenceLoopHandling.Ignore,
                ConstructorHandling = ConstructorHandling.Default,
                DateFormatHandling = DateFormatHandling.IsoDateFormat,
                DateFormatString = "o",
            };

            // for seeing enums as strings not as integer values
            settings.Converters.Add(new StringEnumConverter());

            return settings;
        }

        public static string ToJson(this IRepairTask task)
        {
            return ConvertToJson(task);
        }
    }
}