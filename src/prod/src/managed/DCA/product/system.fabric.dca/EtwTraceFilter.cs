// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Strings;
    using System.Globalization;
    
    // This class implements the ETW trace filtering logic
    internal class EtwTraceFilter
    {
        // Whether or not the filter applies to Windows Fabric events
        internal readonly bool IsWindowsFabricEventFilter;

        // Alias that represents the default filters that are hardcoded in the DCA.
        private const string DefaultFiltersAlias = "_default_";

        // Alias that represents the summary filters that are hardcoded in the DCA.
        private const string SummaryFiltersAlias = "_summary_";

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;
        
        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Array of ETW trace filters
        private FilterElement[] filters;
        
        internal EtwTraceFilter(
                    FabricEvents.ExtensionsEvents traceSource, 
                    string logSourceId, 
                    string filterListAsString, 
                    string defaultFilters,
                    string summaryFilters,
                    bool isWindowsFabricEventFilter)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.StringRepresentation = filterListAsString;
            this.IsWindowsFabricEventFilter = isWindowsFabricEventFilter;

            // Build the array of filters
            this.BuildFilters(filterListAsString, defaultFilters, summaryFilters);
        }

        // Enum for components in the string representation of a filter
        private enum FilterComponents
        {
            TaskName = 0,
            EventType,
            Level,

            // This is not a component. It is a value that represents the total
            // count of components
            ComponentCount
        }

        // Enum for how closely an event matches a particular filter
        // The values in this enum are arranged in increasing order of closeness,
        // i.e. higher the value, closer the match.
        private enum Closeness
        {
            // The event did not match the filter
            NoMatch = 0,

            // The event matched the filter because the task name and event name
            // in the filter were both wildcards
            TaskWildcardEventWildcardMatch,

            // The event matched the filter because their task names matched and
            // the filter's event name was a wildcard
            TaskNameEventWildcardMatch,

            // The event matched the filter because their task names and event 
            // names both matched
            TaskNameEventTypeMatch,

            // Alias for the maximum enum value
            MaxCloseness = TaskNameEventTypeMatch
        }

        // String representation of the list of filters
        private string StringRepresentation { get; set; }

        internal bool ShouldIncludeEvent(DecodedEventWrapper etwEvent)
        {
            // If there are no filters, then we should include the event
            if (null == this.filters)
            {
                return true;
            }

            // If we have filters, then don't include the event unless the 
            // filters effectively ask for it to be included.
            bool includeEvent = false;
            Closeness closeness = Closeness.NoMatch;

            // Go through each filter and compare ...
            foreach (FilterElement filterElement in this.filters)
            {
                // Check if the event matches the current filter. If it matches,
                // check whether the filter asks for the event to be included or
                // excluded.
                bool tempIncludeEvent;
                Closeness tempCloseness;
                this.MatchEventWithFilter(
                    filterElement,
                    etwEvent,
                    out tempCloseness,
                    out tempIncludeEvent);

                // If this filter is a closer match than previously encountered 
                // filters, then update our decision to include or exclude the
                // event.
                // If this filter is not a closer match than previously 
                // encountered filters, then we do not update our decision.
                if (tempCloseness > closeness)
                {
                    System.Fabric.Interop.Utility.ReleaseAssert(tempCloseness != Closeness.NoMatch, string.Empty);
                    includeEvent = tempIncludeEvent;
                    closeness = tempCloseness;

                    if (Closeness.MaxCloseness == closeness)
                    {
                        // We already found the closest possible match. We can't 
                        // find a closer match than this. So break out of the loop.
                        break;
                    }
                }
            }

            return includeEvent;
        }

        private static void ConvertStringToFilterElement(string filter, out FilterElement filterElement)
        {
            // The filter string has the following format -
            // <taskName>.<eventType>:<level>

            // First split the filter string into its components
            char[] filterSeparators = { '.', ':' };
            string[] filterParts = filter.Split(filterSeparators);

            // Verify that the expected number of components was found
            System.Fabric.Interop.Utility.ReleaseAssert(
                filterParts.Length == ((int)FilterComponents.ComponentCount),
                string.Format(CultureInfo.CurrentCulture, StringResources.DCAError_InvalidComponentNumber_Formatted, filter));

            // Trim whitespaces from the components
            filterParts[(int)FilterComponents.TaskName] = filterParts[(int)FilterComponents.TaskName].Trim();
            filterParts[(int)FilterComponents.EventType] = filterParts[(int)FilterComponents.EventType].Trim();
            filterParts[(int)FilterComponents.Level] = filterParts[(int)FilterComponents.Level].Trim();

            // Initialize the filter element using these components

            // Initialize event name
            if (filterParts[(int)FilterComponents.EventType].Equals("*"))
            {
                // This filter applies to all events
                filterElement.EventType = null;
            }
            else
            {
                // This filter applies to a specific event
                filterElement.EventType = filterParts[(int)FilterComponents.EventType];
            }

            // Initialize task name
            if (filterParts[(int)FilterComponents.TaskName].Equals("*"))
            {
                // This filter applies to all tasks
                filterElement.TaskName = null;

                // If the filter applies to all tasks, we require that it apply 
                // to all events too
                System.Fabric.Interop.Utility.ReleaseAssert(
                    null == filterElement.EventType,
                    string.Format(CultureInfo.CurrentCulture, StringResources.DCAError_FormatNotSupported_Formatted, filter));
            }
            else
            {
                // This filter applies to a specific task
                filterElement.TaskName = filterParts[(int)FilterComponents.TaskName];
            }

            // Initialize level
            filterElement.Level = int.Parse(
                                       filterParts[(int)FilterComponents.Level],
                                       CultureInfo.InvariantCulture);
        }

        private string ExpandFilterAliases(string filterListAsString, string defaultFilters, string summaryFilters)
        {
            bool aliasFound = false;

            // First split the string into individual filters
            string[] filtersAsStrings = filterListAsString.Split(',');

            // Search for a filter that uses an alias to represent the default filter list
            for (int i = 0; i < filtersAsStrings.Length; i++)
            {
                string trimmedFilters = filtersAsStrings[i].Trim();
                if (trimmedFilters.Equals(
                        DefaultFiltersAlias,
                        StringComparison.OrdinalIgnoreCase))
                {
                    // Found a filter that uses an alias to represent the default filter list.
                    // Replace the alias with the default filter list.
                    filtersAsStrings[i] = defaultFilters;
                    aliasFound = true;
                }
                else if (trimmedFilters.Equals(
                            SummaryFiltersAlias,
                            StringComparison.OrdinalIgnoreCase))
                {
                    // Found a filter that uses an alias to represent the summary filter list.
                    // Replace the alias with the summary filter list.
                    filtersAsStrings[i] = summaryFilters;
                    aliasFound = true;
                }
            }

            return aliasFound ? string.Join(",", filtersAsStrings) : filterListAsString;
        }

        private void BuildFilters(string filterListAsString, string defaultFilters, string summaryFilters)
        {
            // If the caller passed in a null or empty string, return immediately
            if (string.IsNullOrEmpty(filterListAsString))
            {
                return;
            }

            // If the string contains an alias to represent the default filter, replace the
            // alias with the default filter list
            filterListAsString = this.ExpandFilterAliases(filterListAsString, defaultFilters, summaryFilters);

            // First split the string into individual filters
            string[] filtersAsStrings = filterListAsString.Split(',');

            // Allocate array of filter elements to hold the filters
            this.filters = new FilterElement[filtersAsStrings.Length];

            // Convert each filter from its string representation to its filter
            // element representation
            int i = 0;
            foreach (string filterAsString in filtersAsStrings)
            {
                ConvertStringToFilterElement(filterAsString, out this.filters[i]);
                i++;
            }

            // Make sure that each task/event pair in the filter list is unique.
            // We do not support multiple task/event pairs in the filter list.
            // The level component of the filter list specifies that all events
            // at and below that level are to be included. Therefore, multiple
            // task/event pairs in the filter list should not be needed.
            for (int j = 0; j < i; j++)
            {
                for (int k = j + 1; k < i; k++)
                {
                    if (string.Equals(this.filters[j].TaskName, this.filters[k].TaskName) &&
                        string.Equals(this.filters[j].EventType, this.filters[k].EventType))
                    {
                        Debug.Fail(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                "The task/event pair {0}.{1} appears more than once in filter list '{2}'. This is not supported.",
                                string.IsNullOrEmpty(this.filters[j].TaskName) ? "*" : this.filters[j].TaskName,
                                string.IsNullOrEmpty(this.filters[j].EventType) ? "*" : this.filters[j].EventType,
                                filterListAsString));
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "The task/event pair {0}.{1} appears more than once in filter list '{2}'. This is not supported.",
                            string.IsNullOrEmpty(this.filters[j].TaskName) ? "*" : this.filters[j].TaskName,
                            string.IsNullOrEmpty(this.filters[j].EventType) ? "*" : this.filters[j].EventType,
                            filterListAsString);

                        if (this.filters[j].Level != this.filters[k].Level)
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "The filter {0}.{1}:{2} will be discarded from filter list '{3}' because it has the same task/event pair as another filter.",
                                filterListAsString,
                                string.IsNullOrEmpty(this.filters[k].TaskName) ? "*" : this.filters[k].TaskName,
                                string.IsNullOrEmpty(this.filters[k].EventType) ? "*" : this.filters[k].EventType,
                                this.filters[k].Level,
                                filterListAsString);
                        }

                        break;
                    }
                }
            }
        }

        private Closeness ComputeCloseness(FilterElement filterElement, DecodedEventWrapper etwEvent)
        {
            if (null == filterElement.TaskName)
            {
                // If the task name for this filter is a wildcard, the event name
                // should also be a wildcard because we don't support filters of
                // the format '*.<eventType>'. This means we have a wildcard match.
                System.Fabric.Interop.Utility.ReleaseAssert(null == filterElement.EventType, StringResources.DCAError_EventNameNotAWildcard);
                return Closeness.TaskWildcardEventWildcardMatch;
            }

            if (false == filterElement.TaskName.Equals(etwEvent.TaskName))
            {
                // If the task name for this filter does not match the event's 
                // task name, then we have no match.
                return Closeness.NoMatch;
            }

            if (null == filterElement.EventType)
            {
                // If the event name for this filter is a wildcard, we have a 
                // task name, event wildcard match.
                return Closeness.TaskNameEventWildcardMatch;
            }
            
            if (false == filterElement.EventType.Equals(etwEvent.EventType))
            {
                // If the event name for this filter does not match the event's 
                // event name, then we have no match.
                return Closeness.NoMatch;
            }

            // If we get here, it means that both task name and event name match
            return Closeness.TaskNameEventTypeMatch;
        }
        
        private void MatchEventWithFilter(FilterElement filterElement, DecodedEventWrapper etwEvent, out Closeness closeness, out bool includeEvent)
        {
            includeEvent = false;

            // Figure out if we have a match and if so how close is the match
            closeness = this.ComputeCloseness(filterElement, etwEvent);

            // If we have a match, figure out whether the event should be 
            // included or not
            if (closeness > Closeness.NoMatch)
            {
                includeEvent = filterElement.Level >= etwEvent.Level;
            }
        }

        // Represents an ETW trace filter
        private struct FilterElement
        {
            internal string TaskName;
            internal string EventType;
            internal int Level;
        }
    }
}