// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing.Filter
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.Tracing;

    internal class TraceSinkFilter
    {
        private readonly Dictionary<string, FilterType> eventList;
        private EventLevel level;
        private EventKeywords keywords;

        internal TraceSinkFilter(EventLevel level, EventKeywords keywords)
        {
            this.level = level;
            this.keywords = keywords;
            this.eventList = new Dictionary<string, FilterType>();
        }

        internal enum FilterType
        {
            Exclude = 0,
            Include = 1
        }

        internal EventLevel Level
        {
            set { this.level = value; }
        }

        internal EventKeywords Keywords
        {
            set { this.keywords = value; }
        }

        internal void AddFilter(string eventName,  FilterType filterType)
        {
            FilterType filter;
            if (this.eventList.TryGetValue(eventName, out filter))
            {
                // Same event shouldn't appear in include and exclude list, in case it appears we give higher priority to Include
                if (filter == FilterType.Exclude &&
                    filterType == FilterType.Include)
                {
                    this.eventList[eventName] = filterType;
                }
            }
            else
            {
                // Dictionary entry doesn't exist, add this entry
                this.eventList.Add(eventName, filterType);
            }
        }

        internal bool CheckEnabledStatus(EventLevel level, EventKeywords keywords, string eventName)
        {
            FilterType filter;
            if (this.eventList.TryGetValue(eventName, out filter))
            {
                if (filter == FilterType.Include)
                {
                    return true;
                }

                return false;
            }
            
            return (level <= this.level) && (keywords == 0 || (keywords & this.keywords) != 0);
        }
    }
}