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
    /// <para>Describes query input for getting <see cref="System.Fabric.Health.PartitionHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetPartitionHealthAsync(System.Fabric.Description.PartitionHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class PartitionHealthQueryDescription
    {
        /// <summary>
        /// <para>Instantiates a new instance of the <see cref="System.Fabric.Description.PartitionHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="partitionId">
        /// <para>The partition identifier.</para>
        /// </param>
        public PartitionHealthQueryDescription(Guid partitionId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();
            this.PartitionId = partitionId;
        }

        /// <summary>
        /// <para>Gets or sets the partition identifier.</para>
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
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate health.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate partition health.</para>
        /// </value>
        /// <remarks>If not specified, the health store uses the application health policy of the parent application
        /// to evaluate the partition health.</remarks>
        public ApplicationHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the partition. Only 
        /// events that match the filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthEventsFilter" /> used to filter returned events.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the partition aggregated health state.
        /// If the filter is not specified, all events are returned.</para></remarks>
        public HealthEventsFilter EventsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.ReplicaHealthState" /> children. Only children that match the filter 
        /// will be returned. All children will be used to evaluate the partition aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ReplicaHealthStatesFilter" /> used to filter returned replicas.</para>
        /// </value>
        /// <remarks><para> Only children that match the filter will be returned. 
        /// All children will be used to evaluate the partition aggregated health state.
        /// If the filter is not specified, all partition children are returned.</para></remarks>
        public ReplicaHealthStatesFilter ReplicasFilter
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the health statistics filter.
        /// </summary>
        /// <value>The health statistics filter.</value>
        /// <remarks>
        /// <para>
        /// The health statistics filter controls whether the <see cref="System.Fabric.Health.PartitionHealth"/> returned by the query
        /// contains the partition health statistics. If not specified, the statistics are included.
        /// </para>
        /// </remarks>
        public PartitionHealthStatisticsFilter HealthStatisticsFilter
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
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Partition {0}", this.PartitionId));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.HealthPolicy != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthPolicy));
            }

            if (this.HealthStatisticsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.HealthStatisticsFilter));
            }

            return sb.ToString();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativePartitionHealthQueryDescription = new NativeTypes.FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION();

            nativePartitionHealthQueryDescription.PartitionId = this.PartitionId;
            if (this.HealthPolicy != null)
            {
                nativePartitionHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }

            if (this.EventsFilter != null)
            {
                nativePartitionHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            if (this.ReplicasFilter != null)
            {
                nativePartitionHealthQueryDescription.ReplicasFilter = this.ReplicasFilter.ToNative(pinCollection);
            }

            if (this.HealthStatisticsFilter != null)
            {
                var ex1 = new NativeTypes.FABRIC_PARTITION_HEALTH_QUERY_DESCRIPTION_EX1();
                ex1.HealthStatisticsFilter = this.HealthStatisticsFilter.ToNative(pinCollection);
                nativePartitionHealthQueryDescription.Reserved = pinCollection.AddBlittable(ex1);
            }

            return pinCollection.AddBlittable(nativePartitionHealthQueryDescription);
        }
    }
}
