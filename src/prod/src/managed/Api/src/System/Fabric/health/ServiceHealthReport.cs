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
    /// <para>Represents a health report to be applied on a service health entity.
    /// The report is sent to health store with 
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </summary>
    public class ServiceHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.Health.ServiceHealthReport" />.</para>
        /// </summary>
        /// <param name="serviceName">
        /// <para>The service name. Required.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The HealthInformation which describes the report fields, like sourceId, property, health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public ServiceHealthReport(Uri serviceName, HealthInformation healthInformation)
            : base(HealthReportKind.Service, healthInformation)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            this.ServiceName = serviceName;
        }

        /// <summary>
        /// <para>Gets the name of the service.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service.</para>
        /// </value>
        public Uri ServiceName { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeServiceHealthReport = new NativeTypes.FABRIC_SERVICE_HEALTH_REPORT();
            nativeServiceHealthReport.ServiceName = pinCollection.AddObject(this.ServiceName);
            nativeServiceHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeServiceHealthReport);
        }
    }
}