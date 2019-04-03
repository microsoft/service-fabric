// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Container
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("ContainerEvent")]
    public class ContainerEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the ContainerEvent class.
        /// </summary>
        public ContainerEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the ContainerEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>
        /// <param name="timeStamp">The time event was logged.</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public ContainerEvent(System.Guid eventInstanceId, string category, DateTime timeStamp, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
        }

    }
}
