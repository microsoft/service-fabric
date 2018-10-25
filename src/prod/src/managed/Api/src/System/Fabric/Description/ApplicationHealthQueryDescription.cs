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
    /// <para>Represents the query input for getting application health. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetApplicationHealthAsync(System.Fabric.Description.ApplicationHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class ApplicationHealthQueryDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be <languageKeyword>null</languageKeyword>.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public ApplicationHealthQueryDescription(Uri applicationName)
        {
            Requires.Argument<Uri>("applicationName", applicationName).NotNull();
            this.ApplicationName = applicationName;
        }

        /// <summary>
        /// <para>Gets the name of the application.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application.</para>
        /// </value>
        public Uri ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate the application health.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate application health.</para>
        /// </value>
        /// <remarks>If not specified, the health store uses the application health policy specified in the application manifest
        /// to evaluate the application.</remarks>
        public ApplicationHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the application. Only events 
        /// that match the filter will be returned. All events will be used to evaluate the application aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the application.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the application aggregated health state.
        /// If the filter is not specified, all events are returned.</para></remarks>
        public HealthEventsFilter EventsFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.ServiceHealthState" /> children. Only children that match the filter will 
        /// be returned. All children will be used to evaluate the application aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for <see cref="System.Fabric.Health.ServiceHealthState" /> children.</para>
        /// </value>
        /// <remarks><para> Only children that match the filter will be returned. 
        /// All children will be used to evaluate the application aggregated health state.
        /// If the filter is not specified, all service children of the application are returned.</para></remarks>
        public ServiceHealthStatesFilter ServicesFilter
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.DeployedApplicationHealthState" /> children. Only children that match the 
        /// filter will be returned. All children will be used to evaluate the application aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>The filter for <see cref="System.Fabric.Health.DeployedApplicationHealthStatesFilter" /> children.</para>
        /// </value>
        /// <remarks><para> Only children that match the filter will be returned. 
        /// All children will be used to evaluate the application aggregated health state.
        /// If the filter is not specified, all deployed application children of the application are returned.</para></remarks>
        public DeployedApplicationHealthStatesFilter DeployedApplicationsFilter
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
        /// The health statistics filter controls whether the <see cref="System.Fabric.Health.ApplicationHealth"/> returned by the query
        /// contains the application health statistics. If not specified, the statistics are included.
        /// </para>
        /// </remarks>
        public ApplicationHealthStatisticsFilter HealthStatisticsFilter
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
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Application {0}", this.ApplicationName));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.ServicesFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.ServicesFilter));
            }

            if (this.DeployedApplicationsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.DeployedApplicationsFilter));
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
            var nativeApplicationHealthQueryDescription = new NativeTypes.FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION();

            nativeApplicationHealthQueryDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            if (this.HealthPolicy != null)
            {
                nativeApplicationHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }

            if (this.EventsFilter != null)
            {
                nativeApplicationHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            if (this.ServicesFilter != null)
            {
                nativeApplicationHealthQueryDescription.ServicesFilter = this.ServicesFilter.ToNative(pinCollection);
            }

            if (this.DeployedApplicationsFilter != null)
            {
                nativeApplicationHealthQueryDescription.DeployedApplicationsFilter = this.DeployedApplicationsFilter.ToNative(pinCollection);
            }

            if (this.HealthStatisticsFilter != null)
            {
                var ex1 = new NativeTypes.FABRIC_APPLICATION_HEALTH_QUERY_DESCRIPTION_EX1();
                ex1.HealthStatisticsFilter = this.HealthStatisticsFilter.ToNative(pinCollection);
                nativeApplicationHealthQueryDescription.Reserved = pinCollection.AddBlittable(ex1);
            }

            return pinCollection.AddBlittable(nativeApplicationHealthQueryDescription);
        }
    }
}
