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
    /// <para>Describes query input for getting <see cref="System.Fabric.Health.DeployedApplicationHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetDeployedApplicationHealthAsync(System.Fabric.Description.DeployedApplicationHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class DeployedApplicationHealthQueryDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.DeployedApplicationHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null.</para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A required parameter can�t be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>A required parameter can't be empty.</para>
        /// </exception>
        public DeployedApplicationHealthQueryDescription(Uri applicationName, string nodeName)
        {
            Requires.Argument<Uri>("applicationName", applicationName).NotNull();
            Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();
            this.ApplicationName = applicationName;
            this.NodeName = nodeName;
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
        /// <para>Gets the node name.</para>
        /// </summary>
        /// <value>
        /// <para>The node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate the deployed application health.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate health.</para>
        /// </value>
        /// <remarks>If not specified, the health store uses the application health policy or the parent application to evaluate the deployed application.
        /// The application health policy is specified in the application manifest.
        /// </remarks>
        public ApplicationHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the deployed application. 
        /// Only events that match the filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthEventsFilter" /> used to filter returned events.</para>
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
        /// <para>Gets or sets the filter for <see cref="System.Fabric.Health.DeployedServicePackageHealthState" /> children. Only children that 
        /// match the filter will be returned.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.DeployedServicePackageHealthStatesFilter" /> used to filter returned deployed service packages.</para>
        /// </value>
        /// <remarks><para> Only children that match the filter will be returned. 
        /// All children will be used to evaluate the deployed application aggregated health state.
        /// If the filter is not specified, all deployed service package children of the deployed application are returned.</para></remarks>
        public DeployedServicePackageHealthStatesFilter DeployedServicePackagesFilter
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
        /// The health statistics filter controls whether the <see cref="System.Fabric.Health.DeployedApplicationHealth"/> returned by the query
        /// contains the deployed application health statistics. If not specified, the statistics are included.
        /// </para>
        /// </remarks>
        public DeployedApplicationHealthStatisticsFilter HealthStatisticsFilter
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
            sb.Append(string.Format(CultureInfo.CurrentCulture, "Application {0} on node {1}", this.ApplicationName, this.NodeName));
            if (this.EventsFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.EventsFilter));
            }

            if (this.DeployedServicePackagesFilter != null)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.DeployedServicePackagesFilter));
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
            var nativeDeployedApplicationHealthQueryDescription = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION();

            nativeDeployedApplicationHealthQueryDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDeployedApplicationHealthQueryDescription.NodeName = pinCollection.AddBlittable(this.NodeName);
            if (this.HealthPolicy != null)
            {
                nativeDeployedApplicationHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }

            if (this.EventsFilter != null)
            {
                nativeDeployedApplicationHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            if (this.DeployedServicePackagesFilter != null)
            {
                nativeDeployedApplicationHealthQueryDescription.DeployedServicePackagesFilter = this.DeployedServicePackagesFilter.ToNative(pinCollection);
            }

            if (this.HealthStatisticsFilter != null)
            {
                var ex1 = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_QUERY_DESCRIPTION_EX1();
                ex1.HealthStatisticsFilter = this.HealthStatisticsFilter.ToNative(pinCollection);
                nativeDeployedApplicationHealthQueryDescription.Reserved = pinCollection.AddBlittable(ex1);
            }

            return pinCollection.AddBlittable(nativeDeployedApplicationHealthQueryDescription);
        }

    }
}
