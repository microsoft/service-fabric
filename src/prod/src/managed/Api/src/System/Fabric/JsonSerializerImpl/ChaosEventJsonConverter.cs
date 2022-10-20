// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;

    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// There are several types of ChaosEvent's. This is a custom converter that looks into the value of "Kind" in the JSON response
    /// and converts into an appropriate derived ChaosEvent, like StartedEvent or ExecutingFaultsEvent.
    /// </summary>
    internal sealed class ChaosEventJsonConverter : JsonConverter
    {
        private const string TraceSource = "ChaosEventJsonConverter";

        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(List<ChaosEvent>);
        }

        /// <inheritdoc />
        public override object ReadJson(
            JsonReader reader,
            Type objectType,
            object existingValue,
            JsonSerializer serializer)
        {
            ThrowIf.Null(reader, "reader");

            if (reader.TokenType == JsonToken.Null)
            {
                return null;
            }

            var history = new List<ChaosEvent>();

            var jArray = JArray.Load(reader);

            ThrowIf.IsTrue(jArray == null, "The list of ChaosEvent's in History should be readable as a JArray");

            foreach (var jObject in jArray.Children<JObject>())
            {
                foreach (var property in jObject.Properties())
                {
                    var jChaosEvent = (JObject)property.Value;
                    string kind = jChaosEvent[JsonSerializerImplConstants.Kind].ToString();

                    if (kind.Equals(ChaosEventType.ExecutingFaults.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        history.Add(jChaosEvent.ToObject<ExecutingFaultsEvent>(serializer));
                    }
                    else if (kind.Equals(ChaosEventType.Started.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        history.Add(jChaosEvent.ToObject<StartedEvent>(serializer));
                    }
                    else if (kind.Equals(ChaosEventType.Stopped.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        history.Add(jChaosEvent.ToObject<StoppedEvent>(serializer));
                    }
                    else if (kind.Equals(ChaosEventType.TestError.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        history.Add(jChaosEvent.ToObject<TestErrorEvent>(serializer));
                    }
                    else if (kind.Equals(ChaosEventType.ValidationFailed.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        history.Add(jChaosEvent.ToObject<ValidationFailedEvent>(serializer));
                    }
                    else if (kind.Equals(ChaosEventType.Waiting.ToString(), StringComparison.OrdinalIgnoreCase))
                    {
                        history.Add(jChaosEvent.ToObject<WaitingEvent>(serializer));
                    }
                    else
                    {
                        throw new InvalidOperationException(string.Format("'{0}' field of the ChaosEvent does not contain a known value.", JsonSerializerImplConstants.Kind));
                    }
                }
            }

            return history;
        }

        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }
    }
}