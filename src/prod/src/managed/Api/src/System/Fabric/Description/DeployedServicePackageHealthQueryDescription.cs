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
    using System.Fabric.Query;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// <para>Describes the query input for getting <see cref="System.Fabric.Health.DeployedServicePackageHealth" />. Used by 
    /// <see cref="System.Fabric.FabricClient.HealthClient.GetDeployedServicePackageHealthAsync(System.Fabric.Description.DeployedServicePackageHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class DeployedServicePackageHealthQueryDescription
    {
        /// <summary>
        /// <para>Instantiates a <see cref="System.Fabric.Description.DeployedServicePackageHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null.</para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="serviceManifestName">
        /// <para>The service manifest name. Cannot be null or empty.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A required parameter can't be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>A required parameter can't be empty.</para>
        /// </exception>
        public DeployedServicePackageHealthQueryDescription(Uri applicationName, string nodeName, string serviceManifestName)
            : this (applicationName, nodeName, serviceManifestName, string.Empty)
        {
        }


        /// <summary>
        /// <para>Instantiates a <see cref="System.Fabric.Description.DeployedServicePackageHealthQueryDescription" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null.</para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="serviceManifestName">
        /// <para>The service manifest name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="servicePackageActivationId">
        /// <para>
        /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package.
        /// ServicePackageActivationId of a deployed service package can obtained by using 
        /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A required parameter can't be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>A required parameter can't be empty.</para>
        /// </exception>
        public DeployedServicePackageHealthQueryDescription(
            Uri applicationName, 
            string nodeName, 
            string serviceManifestName,
            string servicePackageActivationId)
        {
            Requires.Argument("applicationName", applicationName).NotNull();
            Requires.Argument("nodeName", nodeName).NotNullOrEmpty();
            Requires.Argument("serviceManifestName", serviceManifestName).NotNullOrEmpty();
            Requires.Argument("servicePackageActivationId", servicePackageActivationId).NotNull();

            this.ApplicationName = applicationName;
            this.NodeName = nodeName;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId;
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
        /// <para>The <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.String" /> representing the service manifest name.</para>
        /// </value>
        public string ServiceManifestName
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets the ActivationId of the service package.
        /// </summary>
        /// <value>
        /// <para>
        /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package. 
        /// ServicePackageActivationId of a deployed service package can obtained by using 
        /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, Uri)"/> query. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in which case
        /// it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>. 
        /// </para>
        /// </value>
        public string ServicePackageActivationId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate the health.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.ApplicationHealthPolicy" /> used to evaluate health.</para>
        /// </value>
        /// <remarks>If not specified, the health store uses the application health policy of the parent application
        /// to evaluate the deployed service package health.</remarks>
        public ApplicationHealthPolicy HealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// <para>Gets or sets the filter for the collection of <see cref="System.Fabric.Health.HealthEvent" /> reported on the deployed service 
        /// package. Only events that match the filter will be returned. All events will be used to evaluate the aggregated health state.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Health.HealthEventsFilter" /> used to filter returned events.</para>
        /// </value>
        /// <remarks><para> Only events that match 
        /// the filter will be returned. All events will be used to evaluate the entity aggregated health state.
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
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture, "Application {0} on node {1} for service manifest {2}.",
                    this.ApplicationName,
                    this.NodeName,
                    this.ServiceManifestName));

            if (!string.IsNullOrWhiteSpace(this.ServicePackageActivationId))
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.ServicePackageActivationId));
            }

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
            var nativeDeployedServicePackageHealthQueryDescription = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION();

            nativeDeployedServicePackageHealthQueryDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDeployedServicePackageHealthQueryDescription.NodeName = pinCollection.AddBlittable(this.NodeName);
            nativeDeployedServicePackageHealthQueryDescription.ServiceManifestName = pinCollection.AddBlittable(this.ServiceManifestName);
            if (this.HealthPolicy != null)
            {
                nativeDeployedServicePackageHealthQueryDescription.HealthPolicy = this.HealthPolicy.ToNative(pinCollection);
            }
            if (this.EventsFilter != null)
            {
                nativeDeployedServicePackageHealthQueryDescription.EventsFilter = this.EventsFilter.ToNative(pinCollection);
            }

            if (!string.IsNullOrWhiteSpace(this.ServicePackageActivationId))
            {
                var ex1 = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION_EX1();
                ex1.ServicePackageActivationId = pinCollection.AddBlittable(this.ServicePackageActivationId);

                nativeDeployedServicePackageHealthQueryDescription.Reserved = pinCollection.AddBlittable(ex1);
            }
            
            return pinCollection.AddBlittable(nativeDeployedServicePackageHealthQueryDescription);
        }
    }
}
