// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Partition
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("PartitionEvent")]
    public class PartitionEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the PartitionEvent class.
        /// </summary>
        public PartitionEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the PartitionEvent class.
        /// </summary>
        /// <param name="eventInstanceId">The identifier for the FabricEvent
        /// instance.</param>
        /// <param name="timeStamp">The time event was logged.</param>
        /// <param name="category">The Category of the Event.</param>
        /// <param name="partitionId">An internal ID used by Service Fabric to
        /// uniquely identify a partition. This is a randomly generated GUID
        /// when the service was created. The partition ID is unique and does
        /// not change for the lifetime of the service. If the same service was
        /// deleted and recreated the IDs of its partitions would be
        /// different.</param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public PartitionEvent(System.Guid eventInstanceId, DateTime timeStamp, string category, Guid partitionId, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
            PartitionId = partitionId;
        }

        /// <summary>
        /// Gets or sets an internal ID used by Service Fabric to uniquely
        /// identify a partition. This is a randomly generated GUID when the
        /// service was created. The partition ID is unique and does not change
        /// for the lifetime of the service. If the same service was deleted
        /// and recreated the IDs of its partitions would be different.
        /// </summary>
        [JsonProperty(PropertyName = "PartitionId")]
        public System.Guid PartitionId { get; set; }
    }
}