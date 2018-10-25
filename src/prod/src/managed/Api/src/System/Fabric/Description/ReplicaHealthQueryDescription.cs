// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Describes the query input for getting <see cref="System.Fabric.Health.ReplicaHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetReplicaHealthAsync(System.Fabric.Description.ReplicaHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class ReplicaHealthQueryDescription
    {
        /// <summary>
        /// <para>Instantiates a new instance of the <see cref="System.Fabric.Description.ReplicaHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="partitionId">
        /// <para>The partition identifier.</para>
        /// </param>
        /// <param name="replicaOrInstanceId">
        /// <para>The stateful service replica id or the stateless service instance.</para>
        /// </param>
        public ReplicaHealthQueryDescription(Guid partitionId, long replicaOrInstanceId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();
            this.PartitionId = partitionId;
            this.ReplicaOrInstanceId = replicaOrInstanceId;
        }

        /// <summary>
        /// <para>Gets the partition identifier.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Guid" /> representing the partition identifier.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>The stateful service replica id or stateless service instance.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Int64" /> representing the stateful service replica id or the stateless service instance.</para>
        /// </value>
        public long ReplicaOrInstanceId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate health.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate health.</para>
        /// </value>
        /// <remarks>If not specified, the health store uses the application health policy of the parent application
        /// to evaluate the replica health.</remarks>
        public ApplicationHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the replica. Only 
        /// events that match the filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthEventsFilter" /> used to filter the events returned by query.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the replica aggregated health state.
        /// If the filter is not specified, all events are returned.</para></remarks>
        public HealthEventsFilter EventsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets a string representation of the health query description.
        /// </summary>
        /// <returns>A string representation of the health query description.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Replica {0}+{1}", this.PartitionId, this.ReplicaOrInstanceId));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.HealthPolicy != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthPolicy));
            }

            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeReplicaHealthQueryDescription = new NativeTypes.FABRIC_REPLICA_HEALTH_QUERY_DESCRIPTION();

            nativeReplicaHealthQueryDescription.PartitionId = this.PartitionId;
            nativeReplicaHealthQueryDescription.ReplicaOrInstanceId = this.ReplicaOrInstanceId;
            if (this.HealthPolicy != null)
            {
                nativeReplicaHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }
            if (this.EventsFilter != null)
            {
                nativeReplicaHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            return pinCollection.AddBlittable(nativeReplicaHealthQueryDescription);
        }
    }
}
