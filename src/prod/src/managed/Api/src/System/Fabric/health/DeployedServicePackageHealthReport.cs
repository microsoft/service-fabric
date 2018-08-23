// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Query;
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on the deployed service package health entity. </para>
    /// </summary>
    /// <remarks>The report can be sent to the health store using <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(HealthReport)" />.</remarks>
    public class DeployedServicePackageHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.DeployedServicePackageHealthReport" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="serviceManifestName">
        /// <para>Service manifest name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, property, health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>The application name cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>Node name is invalid.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Node name cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Health information cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Service manifest name cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>Service manifest name is invalid.</para>
        /// </exception>
        public DeployedServicePackageHealthReport(
            Uri applicationName, 
            string serviceManifestName,
            string nodeName, 
            HealthInformation healthInformation)
            : this(applicationName, serviceManifestName, string.Empty, nodeName, healthInformation)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.DeployedServicePackageHealthReport" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="serviceManifestName">
        /// <para>Service manifest name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="servicePackageActivationId">
        /// <para>
        /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package. 
        /// This can be obtained by using <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/>
        /// specified at the time of creating the service was <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it was not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, property, health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>The application name cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>Node name is invalid.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Node name cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Health information cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Service manifest name cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>Service manifest name is invalid.</para>
        /// </exception>
        public DeployedServicePackageHealthReport(
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string nodeName,
            HealthInformation healthInformation)
            : base(HealthReportKind.DeployedServicePackage, healthInformation)
        {
            Requires.Argument("applicationName", applicationName).NotNull();
            Requires.Argument("serviceManifestName", serviceManifestName).NotNullOrEmpty();
            Requires.Argument("nodeName", nodeName).NotNullOrEmpty();
            Requires.Argument("servicePackageActivationId", servicePackageActivationId).NotNull();

            this.ApplicationName = applicationName;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId;
            this.NodeName = nodeName;
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> representing the application name.</para>
        /// </value>
        public Uri ApplicationName { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> representing the service manifest name.</para>
        /// </value>
        public string ServiceManifestName { get; private set; }

        /// <summary>
        /// Gets the ActivationId of service package.
        /// </summary>
        /// <value>
        /// <para>
        /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package. 
        /// This can be obtained by using <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
        /// </para>
        /// <para>
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/>
        /// specified at the time of creating the service was <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it was not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId { get; private set; }

        /// <summary>
        /// <para>Gets the name of the node where the deployed service package is running.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeDeployedServicePackageHealthReport = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT();

            nativeDeployedServicePackageHealthReport.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDeployedServicePackageHealthReport.ServiceManifestName = pinCollection.AddObject(this.ServiceManifestName);
            nativeDeployedServicePackageHealthReport.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDeployedServicePackageHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            if (!string.IsNullOrWhiteSpace(this.ServicePackageActivationId))
            {
                var ex1 = new NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT_EX1();
                ex1.ServicePackageActivationId = pinCollection.AddObject(this.ServicePackageActivationId);

                nativeDeployedServicePackageHealthReport.Reserved = pinCollection.AddBlittable(ex1);
            }
            
            return pinCollection.AddBlittable(nativeDeployedServicePackageHealthReport);
        }
    }
}