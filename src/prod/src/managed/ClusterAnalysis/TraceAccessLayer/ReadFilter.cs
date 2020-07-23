// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using ClusterAnalysis.TraceAccessLayer.Exceptions;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;

    /// <summary>
    /// Define a set of filters for reading data from store.
    /// </summary>
    public class ReadFilter
    {
        private ReadFilter(TraceRecordFilter filter)
        {
            this.TraceRecordFilters = new List<TraceRecordFilter> { filter };
        }

        private ReadFilter(List<TraceRecordFilter> filters)
        {
            this.TraceRecordFilters = filters;
        }

        public List<TraceRecordFilter> TraceRecordFilters { get; }

        /// <summary>
        /// Gets the Trace Record lambda filter
        /// </summary>
        public Func<TraceRecord, bool> TraceFilter { get; private set; }

        /// <summary>
        /// Create an instance of <see cref="ReadFilter"/>
        /// </summary>
        /// <returns></returns>
        public static ReadFilter CreateReadFilter()
        {
            return new ReadFilter(new List<TraceRecordFilter>());
        }

        /// <summary>
        /// Create an instance of <see cref="ReadFilter"/>
        /// </summary>
        /// <param name="eventType"> Type of Event</param>
        /// <returns></returns>
        public static ReadFilter CreateReadFilter(Type eventType)
        {
            return new ReadFilter(new TraceRecordFilter(eventType));
        }

        /// <summary>
        /// Create an instance of <see cref="ReadFilter"/>
        /// </summary>
        /// <param name="eventType">A list of Types of Event we want to filter on.</param>
        /// <returns></returns>
        public static ReadFilter CreateReadFilter(IList<Type> eventType)
        {
            return new ReadFilter(eventType.Select(item => new TraceRecordFilter(item)).ToList());
        }

        /// <summary>
        /// Create an instance of <see cref="ReadFilter"/>
        /// </summary>
        /// <param name="eventType">Type of the Event</param>
        /// <param name="propertyName">Property name inside the event</param>
        /// <param name="propertyValue">Value of the property</param>        
        /// <returns></returns>
        public static ReadFilter CreateReadFilter<T>(Type eventType, string propertyName, T propertyValue)
        {
            return new ReadFilter(new TraceRecordFilter(eventType, CreatePropertyFilter(propertyName, propertyValue)));
        }

        /// <summary>
        /// Create an instance of <see cref="ReadFilter"/>
        /// </summary>
        /// <param name="eventType">Type of the Event we are filtering on</param>
        /// <param name="propertyFilters">For this type, a set of property name and their values to filter on.</param>
        /// <returns></returns>
        public static ReadFilter CreateReadFilter(Type eventType, IDictionary<string, string> propertyFilters)
        {
            return new ReadFilter(new TraceRecordFilter(eventType, propertyFilters.Select(item => new PropertyFilter(item.Key, item.Value)).ToList()));
        }

        /// <summary>
        /// Add a filter by Event Type
        /// </summary>
        /// <param name="filter"></param>
        /// <returns></returns>
        public ReadFilter CombineFilter(ReadFilter filter)
        {
            Assert.IsNotNull(filter, "filter != null");
            foreach (var oneTraceRecordFilter in filter.TraceRecordFilters)
            {
                this.TraceRecordFilters.Add(oneTraceRecordFilter);
            }

            return this;
        }

        /// <summary>
        /// Add a new Filter condition
        /// </summary>
        /// <param name="eventType"></param>
        /// <returns></returns>
        public ReadFilter AddFilter(Type eventType)
        {
            this.TraceRecordFilters.Add(new TraceRecordFilter(eventType));
            return this;
        }

        /// <summary>
        /// Add a new Filter condition
        /// </summary>
        /// <param name="eventTypes">A list of Types of Events</param>
        /// <returns></returns>
        public ReadFilter AddFilter(IList<Type> eventTypes)
        {
            this.TraceRecordFilters.AddRange(eventTypes.Select(oneType => new TraceRecordFilter(oneType)));
            return this;
        }

        /// <summary>
        /// Add a new Filter condition
        /// </summary>
        /// <param name="eventType">Type of the Event</param>
        /// <param name="propertyName">Property name inside the event</param>
        /// <param name="propertyValue">Value of the property</param>
        /// <returns></returns>
        public ReadFilter AddFilter<T>(Type eventType, string propertyName, T propertyValue)
        {
            var existingFilter = this.TraceRecordFilters.SingleOrDefault(item => item.RecordType == eventType);
            if (existingFilter != null)
            {
                existingFilter.AddFilter(CreatePropertyFilter(propertyName, propertyValue));
            }
            else
            {
                this.TraceRecordFilters.Add(new TraceRecordFilter(eventType, CreatePropertyFilter(propertyName, propertyValue)));
            }

            return this;
        }

        /// <summary>
        /// Add a new Filter condition
        /// </summary>
        /// <param name="eventType"></param>
        /// <param name="propertyFilters"></param>
        /// <returns></returns>
        public ReadFilter AddFilter(Type eventType, IDictionary<string, string> propertyFilters)
        {
            this.TraceRecordFilters.Add(new TraceRecordFilter(eventType, propertyFilters.Select(item => new PropertyFilter(item.Key, item.Value)).ToList()));
            return this;
        }

        /// <summary>
        /// Add a lambda filter for Trace record
        /// </summary>
        /// <param name="traceFilter"></param>
        /// <returns></returns>
        public ReadFilter AddTraceRecordFilter(Func<TraceRecord, bool> traceFilter)
        {
            if (this.TraceFilter != null)
            {
                throw new FilterConditionAlreadyExist(
                    string.Format(CultureInfo.InvariantCulture, "TraceRecord Filter Already Exists. Existing value: {0}", this.TraceFilter));
            }

            Assert.IsNotNull(traceFilter, "Trace filter can't be null");
            this.TraceFilter = traceFilter;

            return this;
        }

        private static PropertyFilter CreatePropertyFilter<T>(string propertyName, T propertyValue)
        {
            Assert.IsNotNull(propertyName, "propertyName != null");
            Assert.IsNotNull(propertyValue, "propertyValue != null");

            if (propertyValue.GetType() == typeof(Guid))
            {
                return new PropertyFilter(propertyName, (Guid)(object)propertyValue);
            }

            if (propertyValue.GetType() == typeof(string))
            {
                return new PropertyFilter(propertyName, (string)(object)propertyValue);
            }

            if (propertyValue.GetType() == typeof(int))
            {
                return new PropertyFilter(propertyName, (int)(object)propertyValue);
            }

            if (propertyValue.GetType() == typeof(long))
            {
                return new PropertyFilter(propertyName, (long)(object)propertyValue);
            }

            throw new NotSupportedException(string.Format("Type : {0} Not supported for Property filter", propertyValue.GetType()));
        }
    }
}