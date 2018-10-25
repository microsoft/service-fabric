// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;

    // This static class contains a enum Type which identifies different types of aggregation supported.
    // It also provide methods for mapping the name (string) of aggregation with their respectives Aggregation.Type
    public static class Aggregation
    {
        [Flags]
        public enum Kinds : long
        {
            Snapshot = 0x0001,
            Average = 0x0002,
            Variance = 0x0004,
            Minimum = 0x0008,
            Maximum = 0x0010,
            Sum = 0x0020,
            Count = 0x0040,
            Undefined = 0x1000   // no aggregation associated used for handling parsing errors
        }

        // Given a Aggregation.Type it returns the respective identifier (string)
        public static string AggregationKindToName(Kinds type)
        {
            return type.ToString();
        }

        // Given an identifier (string) return the respective Aggregation.Kinds
        // or Aggregation.Type.Undefined if no match is found
        public static Kinds CreateAggregationKindFromName(string aggregationName)
        {
            Kinds aggregationKind;
            if (!Enum.TryParse(aggregationName, true, out aggregationKind))
            {
                return Aggregation.Kinds.Undefined;
            }

            return aggregationKind;
        }
    }
}