// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on the deployed application health entity. </para>
    /// </summary>
    /// <remarks>The report can be sent to the health store using <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(HealthReport)" />.</remarks>
    public class DeployedApplicationHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health. DeployedApplicationHealthReport" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null.</para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name. Cannot be null or empty.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, property, 
        /// health state. Cannot be null.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>The <paramref name="applicationName" /> cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        /// <para>
        ///     <paramref name="nodeName" /> cannot be empty.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>
        ///     <paramref name="nodeName" /> cannot be null.</para>
        /// </exception>
        /// <exception cref="System.ArgumentNullException">
        /// <para>
        ///     <paramref name="healthInformation" /> cannot be null.</para>
        /// </exception>
        public DeployedApplicationHealthReport(Uri applicationName, string nodeName, HealthInformation healthInformation)
            : base(HealthReportKind.DeployedApplication, healthInformation)
        {
            Requires.Argument<Uri>("applicationName", applicationName).NotNull();
            Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();

            this.ApplicationName = applicationName;
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
        /// <para>Gets the node name where the deployed application is running.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.String" /> representing the node name.</para>
        /// </value>
        public string NodeName { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeDeployedApplicationHealthReport = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_REPORT();

            nativeDeployedApplicationHealthReport.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDeployedApplicationHealthReport.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDeployedApplicationHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeDeployedApplicationHealthReport);
        }
    }
}