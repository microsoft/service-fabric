// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;

    /// <summary>
    /// A wrapper for EventsAPI related query parameters.
    /// </summary>
    internal class QueryParametersWrapper
    {
        // Wrapped dictionary has IgnoreCase set.
        private IReadOnlyDictionary<string, string> parameters;

        public QueryParametersWrapper(IReadOnlyDictionary<string, string> parameters)
        {
            this.parameters = parameters;
        }

        public string APIVersion
        {
            get
            {
                string apiVersion;
                if (!this.parameters.TryGetValue("api-version", out apiVersion))
                {
                    return null;
                }

                return apiVersion;
            }
        }
        
        public DateTime StartTimeUtc
        {
            get
            {
                string startTimeUtc;
                DateTime dtStartTimeUtc;
                if (!this.parameters.TryGetValue("StartTimeUtc", out startTimeUtc) ||
                    !TryParseDateTime(startTimeUtc, out dtStartTimeUtc))
                {
                    return DateTime.MinValue;
                }

                return dtStartTimeUtc;
            }
        }
        
        public DateTime EndTimeUtc
        {
            get
            {
                string endTimeUtc;
                DateTime dtEndTimeUtc;
                if (!this.parameters.TryGetValue("EndTimeUtc", out endTimeUtc) ||
                    !TryParseDateTime(endTimeUtc, out dtEndTimeUtc))
                {
                    return DateTime.MaxValue;
                }

                return dtEndTimeUtc;
            }
        }

        public IList<string> EventsTypesFilter
        {
            get
            {
                string eventsTypesFilter;
                if (!this.parameters.TryGetValue("EventsTypesFilter", out eventsTypesFilter))
                {
                    return null;
                }

                return eventsTypesFilter.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries)
                        .Select(type => type.Trim().ToLower()).ToList();
            }
        }

        public bool ExcludeAnalysisEvents
        {
            get
            {
                string excludeAnalysisEvents;
                bool isExcludeAnalysisEvents;
                if (!this.parameters.TryGetValue("ExcludeAnalysisEvents", out excludeAnalysisEvents) ||
                    !bool.TryParse(excludeAnalysisEvents, out isExcludeAnalysisEvents))
                {
                    return false;
                }

                return isExcludeAnalysisEvents;
            }
        }

        public bool SkipCorrelationLookup
        {
            get
            {
                string skipCorrelationLookup;
                bool isSkipCorrelationLookup;
                if (!this.parameters.TryGetValue("SkipCorrelationLookup", out skipCorrelationLookup) ||
                    !bool.TryParse(skipCorrelationLookup, out isSkipCorrelationLookup))
                {
                    return false;
                }

                return isSkipCorrelationLookup;
            }
        }

        private bool TryParseDateTime(string time, out DateTime parsedTime)
        {
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