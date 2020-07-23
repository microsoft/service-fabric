// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Replica
{
    using System;
    using Newtonsoft.Json;

    [JsonObject("ReplicaEvent")]
    public class ReplicaEvent : FabricEvent
    {
        /// <summary>
        /// Initializes a new instance of the ReplicaEvent class.
        /// </summary>
        public ReplicaEvent()
        {
        }

        /// <summary>
        /// Initializes a new instance of the ReplicaEvent class.
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
        /// <param name="replicaId">Id of a stateful service replica. ReplicaId
        /// is used by Service Fabric to uniquely identify a replica of a
        /// partition. It is unique within a partition and does not change for
        /// the lifetime of the replica. If a replica gets dropped and another
        /// replica gets created on the same node for the same partition, it
        /// will get a different value for the id. Sometimes the id of a
        /// stateless service instance is also referred as a replica
        /// id.</param>
        /// <param name="hasCorrelatedEvents">Shows that there is existing
        /// related events available.</param>
        public ReplicaEvent(Guid eventInstanceId, DateTime timeStamp, string category, Guid partitionId, long replicaId, bool? hasCorrelatedEvents = false)
            : base(eventInstanceId, timeStamp, category, hasCorrelatedEvents)
        {
            PartitionId = partitionId;
            ReplicaId = replicaId;
        }

        /// <summary>
        /// Gets or sets an internal ID used by Service Fabric to uniquely
        /// identify a partition. This is a randomly generated GUID when the
        /// service was created. The partition ID is unique and does not change
        /// for the lifetime of the service. If the same service was deleted
        /// and recreated the IDs of its partitions would be different.
        /// </summary>
        [JsonProperty(PropertyName = "PartitionId")]
        public Guid PartitionId { get; set; }

        /// <summary>
        /// Gets or sets id of a stateful service replica. ReplicaId is used by
        /// Service Fabric to uniquely identify a replica of a partition. It is
        /// unique within a partition and does not change for the lifetime of
        /// the replica. If a replica gets dropped and another replica gets
        /// created on the same node for the same partition, it will get a
        /// different value for the id. Sometimes the id of a stateless service
        /// instance is also referred as a replica id.
        /// </summary>
        [JsonProperty(PropertyName = "ReplicaId")]
        public long ReplicaId { get; set; }
    }
}