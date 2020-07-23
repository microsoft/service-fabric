// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using System.CodeDom;
    using System.Xml;
    using Newtonsoft.Json;

    // Converts TimeSpan to long(number of milliseconds)
    internal class TimeSpanToMilliSecondsJsonConverter : JsonConverter
    {
        public readonly long TicksMultiplier;

        public TimeSpanToMilliSecondsJsonConverter()
            : this(TimeSpan.TicksPerMillisecond)  // TimeSpan to number of seconds
        {
        }

        protected TimeSpanToMilliSecondsJsonConverter(long ticksMultiplier)
        {
            this.TicksMultiplier = ticksMultiplier;
        }

        public long MaxLimit { get { return TimeSpan.MaxValue.Ticks / this.TicksMultiplier; } }

        public long MinLimit { get { return TimeSpan.MinValue.Ticks / this.TicksMultiplier; } }

        protected virtual long ConvertToLong(TimeSpan timespan)
        {
            long timespanAsNumber = timespan.Ticks / this.TicksMultiplier;
            return timespanAsNumber;
        }

        protected virtual TimeSpan ConvertToTimeSpan(long timespanAsNumber)
        {
            long ticks = timespanAsNumber * TicksMultiplier;
            return TimeSpan.FromTicks(ticks);
        }

        private TimeSpan TestLimitAndConvertToTimeSpan(long timespanAsNumber)
        {
            if (timespanAsNumber <= this.MinLimit)
            {
                return TimeSpan.MinValue;
            }

            if (timespanAsNumber >= this.MaxLimit)
            {
                return TimeSpan.MaxValue;
            }

            return ConvertToTimeSpan(timespanAsNumber);
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            // value could be "TimeSpan" or "TimeSpan?"
            if (value == null)
            {
                writer.WriteNull();
            }

            TimeSpan timespan;
            if (value.GetType() == typeof(TimeSpan?))
            {
                // value is not null.
                timespan = ((TimeSpan?)value).Value;
            }
            else
            {
                timespan = (TimeSpan)value;
            }

            // If the target type needs to be given as seconds, then serialize as number
            // if not, as ISO duration.
            if (this.TicksMultiplier == TimeSpan.TicksPerSecond)
            {
                long timespanAsNumber = ConvertToLong(timespan);
                writer.WriteValue(timespanAsNumber.ToString());
            }
            else
            {
                writer.WriteValue(XmlConvert.ToString(timespan));
            }
        }

        public override object ReadJson(JsonReader reader, Type type, object value, JsonSerializer serializer)
        {
            if (type == typeof(TimeSpan?) && reader.TokenType == JsonToken.Null)
            {
                // read "null" for this object.
                return null;
            }

            if (reader.Value is TimeSpan)
            {
                return value;
            }
           
            // If TimeSpan is given as ISO 8601
            try
            {
                return XmlConvert.ToTimeSpan((string)reader.Value);
            }
            catch (Exception)
            {
                // TimeSpan is prbly sent as elapsed milliseconds.
            }

            // JsonToken type is not null that means it represent value of TimeSpan          
            long timespanAsNumber = serializer.Deserialize<long>(reader);
            return TestLimitAndConvertToTimeSpan(timespanAsNumber);
        }

        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(TimeSpan) || objectType == typeof(TimeSpan?);
        }
    }

    // Converts TimeSpan to long(number of seconds)
    internal class TimeSpanToSecondsJsonConverter : TimeSpanToMilliSecondsJsonConverter
    {
        public TimeSpanToSecondsJsonConverter()
            : base(TimeSpan.TicksPerSecond) // TimeSpan to number of seconds
        {
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            base.WriteJson(writer, value, serializer);
        }

        public override object ReadJson(JsonReader reader, Type type, object value, JsonSerializer serializer)
        {
            return base.ReadJson(reader, type, value, serializer);
        }
    }
}