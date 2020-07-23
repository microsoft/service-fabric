// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.ApiEndpoints.Operations
{
    using EventStore.Service.DataReader;
    using EventStore.Service.Exceptions;
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal abstract class BaseOperation<TResult>
    {
        private const string PreviewSuffix = "-preview";
        private const string EventSuffix = "Event";
        private string queryEventsTypesFilter;

        /// <summary>
        /// Minimum Preview API version
        /// </summary>
        public double MinPreviewApiVersion { get; private set; }

        /// <summary>
        /// Minimum API version
        /// </summary>
        public double MinApiVersion { get; private set; }

        /// <summary>
        /// The API version passed by user in this request
        /// </summary>
        public string ApiVersion { get; private set; }

        /// <summary>
        /// Query start time
        /// </summary>
        public DateTime? QueryStartTime { get; private set; }

        /// <summary>
        /// Query end time
        /// </summary>
        public DateTime? QueryEndTime { get; private set; }

        protected EventStoreRuntime CurrentRuntime { get; private set; }

        public IList<Type> QueryTypeFilter
        {
            get { return this.ConvertUserEventTypesToUnderlyingTypes(); }
        }

        internal BaseOperation(string apiVersion, EventStoreRuntime currentRuntime, string eventsTypesFilter = null)
        {
            this.queryEventsTypesFilter = eventsTypesFilter;
            this.ApiVersion = apiVersion;
            this.CurrentRuntime = currentRuntime;
            this.MinPreviewApiVersion = 6.2;
            this.MinApiVersion = 6.4;
        }

        public abstract EntityType GetSupportedEntityType();

        public async Task<TResult> RunAsyncWrappper(TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ValidateApiVersion();

            var startTime = DateTimeOffset.UtcNow;

            var results = await this.RunAsync(timeout, cancellationToken).ConfigureAwait(false);

            var castedAsList = (IList)results;

            var count = castedAsList != null ? castedAsList.Count : -1;
            DumpStatistics(startTime, DateTimeOffset.UtcNow, count);

            return results;
        }

        protected abstract Task<TResult> RunAsync(TimeSpan timeout, CancellationToken cancellationToken);

        protected void ValidateAndExtractStartAndEndTime(string startTimeUtc, string endTimeUtc)
        {
            if (string.IsNullOrEmpty(startTimeUtc) || string.IsNullOrEmpty(endTimeUtc))
            {
                throw new ArgumentException("Invalid Arguments");
            }

            DateTime parsedStartTime;
            DateTime parsedEndTime;
            if (!TryParseDateTime(startTimeUtc, out parsedStartTime) ||
                !TryParseDateTime(endTimeUtc, out parsedEndTime))
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "Couldn't parse query Dates. Specified Start: {0}, End: {1}", startTimeUtc, endTimeUtc));
            }

            this.QueryStartTime = parsedStartTime;
            this.QueryEndTime = parsedEndTime;
        }

        /// <summary>
        /// Validates the API version passed in the request
        /// </summary>
        private void ValidateApiVersion()
        {
            var apiVersion = this.ApiVersion;
            var isPreview = false;

            if (this.ApiVersion.EndsWith(PreviewSuffix))
            {
                var index = this.ApiVersion.LastIndexOf(PreviewSuffix);
                apiVersion = this.ApiVersion.Substring(0, index);
                isPreview = true;
            }

            // Convert the api version from string to double now
            if (Double.TryParse(apiVersion, out double doubleApiVersion))
            {
                if ((isPreview && doubleApiVersion >= MinPreviewApiVersion && doubleApiVersion < MinApiVersion) ||
                    (!isPreview && doubleApiVersion >= MinApiVersion && doubleApiVersion <= this.GetRuntimeVersion()))
                {
                    // Valid API version
                    return;
                }
            }

            throw new ArgumentException("Invalid API version");
        }

        private IList<Type> ConvertUserEventTypesToUnderlyingTypes()
        {
            var entityType = this.GetSupportedEntityType();

            // If user doesn't specify any filter, it implies the user wants all Types for this entity.
            if (string.IsNullOrWhiteSpace(this.queryEventsTypesFilter))
            {
                if (!Mapping.EntityToEventsMap.ContainsKey(entityType))
                {
                    return new List<Type>();
                }

                return Mapping.EntityToEventsMap[entityType].Select(item => item.UnderlyingType).ToList();
            }

            var userSuppliedFilters = this.queryEventsTypesFilter.Split(',');
            var supportedTypes = Mapping.EntityToEventsMap[entityType];
            var requestedTypes = new List<Type>();
            foreach (var oneUserSuppliedFilter in userSuppliedFilters)
            {
                var matchingTypes = supportedTypes.Where(item =>
                    item.ModelType.Name.Equals(oneUserSuppliedFilter, StringComparison.InvariantCultureIgnoreCase) ||
                    item.ModelType.Name.Equals(oneUserSuppliedFilter + EventSuffix, StringComparison.InvariantCultureIgnoreCase));
                if (!matchingTypes.Any())
                {
                    throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "EventType {0} Not Supported for Entity {1}", oneUserSuppliedFilter, entityType));
                }

                requestedTypes.AddRange(matchingTypes.Select(item => item.UnderlyingType));
            }

            return requestedTypes;
        }

        private double GetRuntimeVersion()
        {
            if (this.CurrentRuntime.TestMode)
            {
                return MinApiVersion;
            }

            var currentVersion = FabricRuntime.FabricRuntimeFactory.GetCurrentRuntimeVersion();
            var splitVersions = currentVersion.Split('.');
            var majorMinorVersion = splitVersions[0] + "." + splitVersions[1];
            return Double.Parse(majorMinorVersion);
        }

        private void DumpStatistics(DateTimeOffset startTime, DateTimeOffset endTime, int resultCount)
        {
            FabricEvents.Events.EventStoreQueryStat(
                this.GetSupportedEntityType().ToString(),
                this.ApiVersion,
                this.QueryStartTime.HasValue ? this.QueryStartTime.Value : DateTime.UtcNow,
                this.QueryEndTime.HasValue ? this.QueryEndTime.Value : DateTime.UtcNow,
                false, // TODO
                string.Join(",", this.QueryTypeFilter.Select(item => item.Name)),
                false, // TODO
                (endTime - startTime).TotalMilliseconds,
                resultCount);
        }

        private static bool TryParseDateTime(string time, out DateTime parsedTime)
        {
            // If time is a long, try converting to long (Treating it as ticks).
            long timeTicks;
            if (long.TryParse(time, out timeTicks))
            {
                parsedTime = new DateTime(timeTicks, DateTimeKind.Utc);
                return true;
            }

            // Note: We keep date time in UTC zone.
            return DateTime.TryParseExact(
                time,
                new[] { "yyyy-MM-ddTHH:mm:ssZ", "yyyy-MM-ddTHH:mm:ssz" },
                CultureInfo.InvariantCulture,
                DateTimeStyles.AdjustToUniversal,
                out parsedTime);
        }
    }
}