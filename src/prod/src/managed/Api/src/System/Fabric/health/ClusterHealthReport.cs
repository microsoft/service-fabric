// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on the cluster health entity.</para>
    /// </summary>
    public class ClusterHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Creates a cluster health report.</para>
        /// </summary>
        /// <param name="healthInformation">
        /// <para>The health information which describes the report parameters. Cannot be null.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public ClusterHealthReport(HealthInformation healthInformation)
            : base(HealthReportKind.Cluster, healthInformation)
        {
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeClusterHealthReport = new NativeTypes.FABRIC_CLUSTER_HEALTH_REPORT();
            nativeClusterHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeClusterHealthReport);
        }
    }
}