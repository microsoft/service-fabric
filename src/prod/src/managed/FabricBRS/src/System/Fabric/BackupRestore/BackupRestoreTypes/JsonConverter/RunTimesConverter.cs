// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.View.JsonConverter
{
    using Newtonsoft.Json;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Xml;

    public class RunTimesConverter : Newtonsoft.Json.JsonConverter
    {
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            if (value == null)
            {
                writer.WriteNull();
            }

            if (value.GetType() == typeof(TimeSpan))
            {
                // value is not null.
                TimeSpan timespan = (TimeSpan)value;
                DateTime dateTime = DateTime.MinValue;
                writer.WriteValue(DateTime.SpecifyKind(dateTime.AddTicks(timespan.Ticks), DateTimeKind.Utc));
            }

        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue,
            JsonSerializer serializer)
        {
            var stringTime = serializer.Deserialize<String>(reader);
            var dateTime = DateTime.Parse(stringTime, CultureInfo.InvariantCulture).ToUniversalTime();
            return dateTime.TimeOfDay;
        }

        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(List<TimeSpan>) || objectType == typeof(List<string>);
        }
    }
}
