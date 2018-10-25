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
    /// <para>Provides query input for getting <see cref="System.Fabric.Health.ServiceHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetServiceHealthAsync(System.Fabric.Description.ServiceHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class ServiceHealthQueryDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ServiceHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="serviceName">
        /// <para>The service name. Cannot be null.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public ServiceHealthQueryDescription(Uri serviceName)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            this.ServiceName = serviceName;
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Uri" /> of the service name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> of the service name.</para>
        /// </value>
        public Uri ServiceName
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
        /// to evaluate the service health.</remarks>
        public ApplicationHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the service. Only events 
        /// that match the filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthEventsFilter" /> used to filter returned events.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the service aggregated health state.
        /// If the filter is not specified, all events are returned.</para></remarks>
        public HealthEventsFilter EventsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.PartitionHealthState" /> children. Only children that match the filter 
        /// will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.PartitionHealthStatesFilter" /> used to filter returned partition health states.</para>
        /// </value>
        /// <remarks><para> Only children that match the filter will be returned. 
        /// All children will be used to evaluate the service aggregated health state.
        /// If the filter is not specified, all partition children are returned.</para></remarks>
        public PartitionHealthStatesFilter PartitionsFilter
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
        /// The health statistics filter controls whether the <see cref="System.Fabric.Health.ServiceHealth"/> returned by the query
        /// contains the service health statistics. If not specified, the statistics are included.
        /// </para>
        /// </remarks>
        public ServiceHealthStatisticsFilter HealthStatisticsFilter
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
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Service {0}", this.ServiceName));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.PartitionsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.PartitionsFilter));
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
            var nativeServiceHealthQueryDescription = new NativeTypes.FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION();

            nativeServiceHealthQueryDescription.ServiceName = pinCollection.AddObject(this.ServiceName);
            if (this.HealthPolicy != null)
            {
                nativeServiceHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }

            if (this.EventsFilter != null)
            {
                nativeServiceHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            if (this.PartitionsFilter != null)
            {
                nativeServiceHealthQueryDescription.PartitionsFilter = this.PartitionsFilter.ToNative(pinCollection);
            }

            if (this.HealthStatisticsFilter != null)
            {
                var ex1 = new NativeTypes.FABRIC_SERVICE_HEALTH_QUERY_DESCRIPTION_EX1();
                ex1.HealthStatisticsFilter = this.HealthStatisticsFilter.ToNative(pinCollection);
                nativeServiceHealthQueryDescription.Reserved = pinCollection.AddBlittable(ex1);
            }

            return pinCollection.AddBlittable(nativeServiceHealthQueryDescription);
        }
    }
}
