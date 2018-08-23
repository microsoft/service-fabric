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
    /// <para>Represents a health report to be applied on an application health entity.</para>
    /// </summary>
    /// <remarks>The report can be sent to the health store using <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(HealthReport)" />.</remarks>
    public class ApplicationHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ApplicationHealthReport" /> class.</para>
        /// </summary>
        /// <param name="applicationName">
        /// <para>The application name. Cannot be null.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, property, health state. Cannot be null.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public ApplicationHealthReport(Uri applicationName, HealthInformation healthInformation)
            : base(HealthReportKind.Application, healthInformation)
        {
            Requires.Argument<Uri>("applicationName", applicationName).NotNull();
            this.ApplicationName = applicationName;
        }

        /// <summary>
        /// <para>Gets the application name.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Uri" /> representing the application name.</para>
        /// </value>
        public Uri ApplicationName { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeApplicationHealthReport = new NativeTypes.FABRIC_APPLICATION_HEALTH_REPORT();
            nativeApplicationHealthReport.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeApplicationHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeApplicationHealthReport);
        }
    }
}