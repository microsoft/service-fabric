// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// Trace Record level filters
    /// </summary>
    /// <remarks>
    /// This abstraction is used to create Trace Record Level Filters. For instance, while reading from a session, you can create a filters
    /// for a specific type of Record which have certain property values.
    /// </remarks>
    public class TraceRecordFilter
    {
        public TraceRecordFilter(Type type)
        {
            this.RecordType = type;
            this.Filters = new List<PropertyFilter>(2);
        }

        public TraceRecordFilter(Type type, PropertyFilter propertyFilter)
        {
            Assert.IsNotNull(propertyFilter, "propertyFilter != null");
            this.RecordType = type;
            this.Filters = new List<PropertyFilter>(2) { propertyFilter };
        }

        public TraceRecordFilter(Type type, IList<PropertyFilter> propertyFilters)
        {
            this.RecordType = type;
            this.Filters = new List<PropertyFilter>(propertyFilters);
        }

        /// <summary>
        /// Gets the type of the Record which this filter is looking for.
        /// </summary>
        public Type RecordType { get; }

        /// <summary>
        /// Gets the collection of Property level filters this Record filters is looking for.
        /// </summary>
        public List<PropertyFilter> Filters { get; }

        public TraceRecordFilter AddFilter(PropertyFilter filter)
        {
            Assert.IsNotNull(filter, "filter != null");
            this.Filters.Add(filter);

            return this;
        }

        public TraceRecordFilter AddFilterForInt(string propertyName, int value)
        {
            this.Filters.Add(new PropertyFilter(propertyName, value));

            return this;
        }

        public TraceRecordFilter AddFilterForGuid(string propertyName, Guid value)
        {
            this.Filters.Add(new PropertyFilter(propertyName, value));

            return this;
        }

        public TraceRecordFilter AddFilterForString(string propertyName, string value)
        {
            this.Filters.Add(new PropertyFilter(propertyName, value));

            return this;
        }
    }
}