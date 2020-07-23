// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels
{
    using System;
    using Newtonsoft.Json;

    public class FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the FabricEvent class.
        /// </summary>
        public FabricEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the FabricEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>
        /// <param name="timeStamp">The time event was logged.</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public FabricEvent(Guid eventInstanceId, DateTime timeStamp, string category, bool? hasCorrelatedEvents = default(bool?))
        {
            EventInstanceId = eventInstanceId;
            TimeStamp = timeStamp;
            this.Category = category;
            HasCorrelatedEvents = hasCorrelatedEvents;
        }

        /// <summary>
        /// Gets or sets the identifier for the FabricEvent instance.
        /// </summary>
        [JsonProperty(PropertyName = "EventInstanceId")]
        public Guid EventInstanceId { get; set; }

        /// <summary>
        /// Gets or sets the time event was logged.
        /// </summary>
        [JsonProperty(PropertyName = "TimeStamp")]
        public DateTime TimeStamp { get; set; }

        /// <summary>
        /// Gets or sets the Event category.
        /// </summary>
        [JsonProperty(PropertyName = "Category")]
        public string Category { get; set; }

        /// <summary>
        /// Gets or sets shows that there is existing related events available.
        /// </summary>
        [JsonProperty(PropertyName = "HasCorrelatedEvents")]
        public bool? HasCorrelatedEvents { get; set; }
    }
}
