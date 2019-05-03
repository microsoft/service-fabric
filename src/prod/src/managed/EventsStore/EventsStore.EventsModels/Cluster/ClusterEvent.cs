// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Cluster
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("ClusterEvent")]
    public class ClusterEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the ClusterEvent class.
        /// </summary>
        public ClusterEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the ClusterEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>       
        /// <param name="timeStamp">The time event was logged</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public ClusterEvent(Guid eventInstanceId, DateTime timeStamp, string category, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
        }
    }
}
