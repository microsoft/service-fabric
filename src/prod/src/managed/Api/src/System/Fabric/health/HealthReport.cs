// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the base class for health report classes.</para>
    /// </summary>
    /// <remarks>
    /// <para>Supported types of health reports are: 
    ///�<list type="bullet">
    ///�<item><description><see cref="System.Fabric.Health.ApplicationHealthReport"/></description></item>
    ///�<item><description><see cref="System.Fabric.Health.ClusterHealthReport"/></description></item>
    ///�<item><description><see cref="System.Fabric.Health.NodeHealthReport"/></description></item>
    ///�<item><description><see cref="System.Fabric.Health.PartitionHealthReport"/></description></item>
    ///�<item><description><see cref="System.Fabric.Health.ServiceHealthReport"/></description></item>
    ///�<item><description><see cref="System.Fabric.Health.StatelessServiceInstanceHealthReport"/></description></item>
    ///�<item><description><see cref="System.Fabric.Health.StatefulServiceReplicaHealthReport"/></description></item>
    /// </list>
    /// </para>
    /// <para>The report can be sent to the health store using
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </remarks>
    [KnownType(typeof(ApplicationHealthReport))]
    [KnownType(typeof(ClusterHealthReport))]
    [KnownType(typeof(NodeHealthReport))]
    [KnownType(typeof(PartitionHealthReport))]
    [KnownType(typeof(ServiceHealthReport))]
    [KnownType(typeof(StatelessServiceInstanceHealthReport))]
    [KnownType(typeof(StatefulServiceReplicaHealthReport))]
    public abstract class HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.HealthReport" /> class.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>The kind of the health report. </para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation"/> which describes the report fields, like sourceId, property, health state. Required.</para>
        /// </param>
        protected HealthReport(HealthReportKind kind, HealthInformation healthInformation)
        {
            Requires.Argument<HealthInformation>("healthInformation", healthInformation).NotNull();

            this.Kind = kind;
            this.HealthInformation = healthInformation;
        }

        /// <summary>
        /// <para>Gets the kind of the health report.</para>
        /// </summary>
        /// <value>
        /// <para>The kind of the health report.</para>
        /// </value>
        public HealthReportKind Kind
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the health information that describes common health fields.</para>
        /// </summary>
        /// <value>
        /// <para>The health information that describes common health fields.</para>
        /// </value>
        /// <remarks>The health information is persisted in the health store inside the <see cref="System.Fabric.Health.HealthEvent"/>.</remarks>
        public HealthInformation HealthInformation
        {
            get;
            internal set;
        }

        internal abstract IntPtr ToNativeValue(PinCollection pinCollection);

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeHealthReport = new NativeTypes.FABRIC_HEALTH_REPORT();
            nativeHealthReport.Kind = (NativeTypes.FABRIC_HEALTH_REPORT_KIND)this.Kind;
            nativeHealthReport.Value = this.ToNativeValue(pinCollection);

            return pinCollection.AddBlittable(nativeHealthReport);
        }
    }
}