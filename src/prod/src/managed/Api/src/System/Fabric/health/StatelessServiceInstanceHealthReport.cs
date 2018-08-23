// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on a stateless service health entity.
    /// The report is sent to health store with 
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </summary>
    public class StatelessServiceInstanceHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.Health.StatelessServiceInstanceHealthReport" />.</para>
        /// </summary>
        /// <param name="partitionId">
        /// <para>The partition ID.</para>
        /// </param>
        /// <param name="instanceId">
        /// <para>The instance ID.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, property, 
        /// health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public StatelessServiceInstanceHealthReport(Guid partitionId, long instanceId, HealthInformation healthInformation)
            : base(HealthReportKind.StatelessServiceInstance, healthInformation)
        {
            this.PartitionId = partitionId;
            this.InstanceId = instanceId;
        }

        /// <summary>
        /// <para>Gets the partition ID.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId { get; private set; }

        /// <summary>
        /// <para>Gets the instance ID.</para>
        /// </summary>
        /// <value>
        /// <para>The instance ID.</para>
        /// </value>
        public long InstanceId { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeReplicaHealthReport = new NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_REPORT();

            nativeReplicaHealthReport.InstanceId = this.InstanceId;
            nativeReplicaHealthReport.PartitionId = this.PartitionId;
            nativeReplicaHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeReplicaHealthReport);
        }
    }
}