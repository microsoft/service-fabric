// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on a partition health entity. 
    /// The report is sent to health store with 
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </summary>
    public class PartitionHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.PartitionHealthReport" /> class.</para>
        /// </summary>
        /// <param name="partitionId">
        /// <para>The partition ID. Required.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, property, 
        /// health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public PartitionHealthReport(Guid partitionId, HealthInformation healthInformation)
            : base(HealthReportKind.Partition, healthInformation)
        {
            this.PartitionId = partitionId;
        }

        /// <summary>
        /// <para>Gets the partition ID.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativePartitionHealthReport = new NativeTypes.FABRIC_PARTITION_HEALTH_REPORT();

            nativePartitionHealthReport.PartitionId = this.PartitionId;
            nativePartitionHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativePartitionHealthReport);
        }
    }
}