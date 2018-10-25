// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Diagnostics.Tracing.Writer
{
    using System.Diagnostics.Tracing;
    using Config;

    /// <summary>
    /// TraceEvent stores the state of any event
    /// </summary>
    internal class TraceEvent
    {
        private readonly string eventName;
        private readonly EventLevel level;
        private readonly EventKeywords keywords;
        private readonly TraceConfig configMgr;
        private readonly string message;
        private bool filterState;
#if DotNetCoreClrLinux
        internal bool hasId;
        internal int typeFieldIndex;
#endif

        /// <summary>
        /// Constructor for TraceEvent
        /// </summary>
        /// <param name="eventName">Name of event</param>
        /// <param name="level">Level of event</param>
        /// <param name="keywords">Keywords of event</param>
        /// <param name="message">Message associated with the event</param>
        /// <param name="configMgr">Config manager for settings</param>
        /// <param name="hasId">Indicates if the current TraceEvent has a first parameter named "id", used in linux tracing</param>
        /// <param name="typeFieldIndex">Indicates if the current TraceEvent has any parameter named "type", used in linux tracing</param>
        internal TraceEvent(
            string eventName,
            EventLevel level,
            EventKeywords keywords,
            string message,
            TraceConfig configMgr,
            bool hasId,
            int typeFieldIndex)
        {
            this.eventName = eventName;
            this.level = level;
            this.keywords = keywords;
            this.configMgr = configMgr;
            this.message = message;
            this.UpdateSinkEnabledStatus();
            configMgr.OnFilterUpdate += this.UpdateSinkEnabledStatus;
#if DotNetCoreClrLinux
            this.hasId = hasId;
            this.typeFieldIndex = typeFieldIndex;
#endif
        }

        public string Message
        {
            get { return this.message; }
        }

        public string EventName
        {
            get { return this.eventName; }
        }

        public EventLevel Level
        {
            get { return this.level; }
        }

        /// <summary>
        /// Updates the status for the specified sink
        /// </summary>
        internal void UpdateSinkEnabledStatus()
        {
            this.filterState = this.configMgr.GetEventEnabledStatus(this.level, this.keywords, this.eventName);
        }

        /// <summary>
        /// Checks if the current event should be sent to EventSource, this is based on filters used
        /// </summary>
        /// <returns></returns>
        internal bool IsEventSinkEnabled()
        {
            return this.filterState;
        }
    }
}