// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using Newtonsoft.Json;

    [JsonObject(MemberSerialization.Fields)]
    public class Field
    {
        public Field(string name, Kind fieldKind)
        {
            this.Name = name;
            this.FieldKind = fieldKind;
        }

        [JsonConstructor]
        private Field()
        {
        }

        public enum Kind
        {
            Property,
            Metric,
            Undefined   // no type associated used for handling parsing errors
        }

        public string Name { get; set; }

        public Kind FieldKind { get; set; }

        // Given a Aggregation.Kinds it returns the respective identifier (string)
        public static string FieldKindToName(Kind kind)
        {
            return kind.ToString();
        }

        // Given an identifier (string) return the respective Aggregation.Type
        // or Aggregation.Type.Undefined if no match is found
        public static Kind CreateFieldKindFromName(string fieldKindName)
        {
            Kind fieldKind;
            if (!Enum.TryParse(fieldKindName, true, out fieldKind))
            {
                return Field.Kind.Undefined;
            }

            return fieldKind;
        }

        public static bool IsAggregationAvailable(Kind fieldType, Aggregation.Kinds aggrType)
        {
            switch (fieldType)
            {
                case Kind.Property:
                    {
                        switch (aggrType)
                        {
                            case Aggregation.Kinds.Snapshot:
                                return true;
                            case Aggregation.Kinds.Count:
                                return true;
                            default:
                                return false;
                        }
                    }

                case Kind.Metric:
                    {
                        switch (aggrType)
                        {
                            case Aggregation.Kinds.Undefined:
                                return false;
                            default:
                                return true;
                        }
                    }

                default:
                    return false;
            }
        }

        public override string ToString()
        {
            return string.Format("{0}:{1}", this.Name, Field.FieldKindToName(this.FieldKind));
        }

        // returns the hashcode as the hash code of the name only
        // this is useful when checking if the field from the trace is white-listed
        public override int GetHashCode()
        {
            return this.Name.GetHashCode();
        }
    }
}