// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.JsonSerializerImpl
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.Linq;
    using System.Runtime.Serialization;

    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    // This custom convert is needed because native has a SafetyCheckWrapper
    // which managed does not have. As a result, when native serializes a SafetyCheck,
    // it produces "{"SafetyCheck": {"Kind": ...}}" while the managed service model
    // would expect just the inner "{"Kind": ...}". This converter handles the extra
    // wrapper when converting to/from native's serialization.
    internal sealed class SafetyCheckJsonConverter : JsonConverter
    {
        private const string TraceSource = "SafetyCheckJsonConverter";

        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof(SafetyCheck) || objectType.IsSubclassOf(typeof(SafetyCheck));
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

            SafetyCheck result = null;

            var wrapperJObject = JObject.Load(reader);
            JToken safetyCheckJToken = wrapperJObject[JsonSerializerImplConstants.SafetyCheck];

            if (safetyCheckJToken != null)
            {
                JToken safetyCheckKindJToken = safetyCheckJToken[JsonSerializerImplConstants.Kind];
                SafetyCheckKind kind = (SafetyCheckKind)safetyCheckKindJToken.ToObject(typeof(SafetyCheckKind));

                switch (kind)
                {
                    case SafetyCheckKind.EnsureSeedNodeQuorum:
                        result = new SeedNodeSafetyCheck(kind);
                        break;

                    case SafetyCheckKind.EnsurePartitionQuorum:
                    case SafetyCheckKind.WaitForInBuildReplica:
                    case SafetyCheckKind.WaitForPrimaryPlacement:
                    case SafetyCheckKind.WaitForPrimarySwap:
                    case SafetyCheckKind.WaitForReconfiguration:
                    case SafetyCheckKind.EnsureAvailability:
                        JToken partitionIdJToken = safetyCheckJToken[JsonSerializerImplConstants.PartitionId];
                        Guid partitionId = partitionIdJToken.ToObject<Guid>(serializer);
                        result = new PartitionSafetyCheck(kind, partitionId);
                        break;

                    default:
                        result = new UnknownSafetyCheck(kind);
                        break;
                }
            }

            return result;
        }

        public override void WriteJson(
            JsonWriter writer,
            object value,
            JsonSerializer serializer)
        {
            ThrowIf.Null(writer, "writer");

            JObject wrapperJObject = new JObject();

            JObject safetyCheckJObject = JObject.FromObject(value);

            // Enforce order of properties
            JProperty kindJProperty = safetyCheckJObject.Property(JsonSerializerImplConstants.Kind);
            safetyCheckJObject.Remove(JsonSerializerImplConstants.Kind);
            safetyCheckJObject.AddFirst(kindJProperty);

            wrapperJObject.Add(JsonSerializerImplConstants.SafetyCheck, safetyCheckJObject);

            wrapperJObject.WriteTo(writer);
        }
    }
}