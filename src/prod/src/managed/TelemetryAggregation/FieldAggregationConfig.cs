// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TelemetryAggregation
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Newtonsoft.Json;

    // this class describes which aggregation types are enabled for a field in the trace
    // this set of aggregations is represented as bit flags in AggregationTypesFlag
    [JsonObject(MemberSerialization.Fields)]
    public class FieldAggregationConfig
    {
        public FieldAggregationConfig(Field field, IEnumerable<Aggregation.Kinds> aggrTypeList)
        {
            this.Field = field;
            this.AggregationTypesFlag = 0;
            foreach (var aggrType in aggrTypeList)
            {
                // setting the flags for aggregation types
                this.AggregationTypesFlag |= aggrType;
            }
        }

        [JsonConstructor]
        private FieldAggregationConfig()
        {
        }

        // bit flags representing enabled aggregation types
        public Aggregation.Kinds AggregationTypesFlag { get; }

        // name of the field
        public Field Field { get; }

        // used for creating a human readable representation of the information in the class
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat("({0}){{ ", this.Field);

            var aggregationTypes = this.AggregationTypesFlag.ToString();

            // prints the list of all aggregators
            sb.Append(string.Join(", ", aggregationTypes));

            sb.Append("} ");

            return sb.ToString();
        }
    }
}
