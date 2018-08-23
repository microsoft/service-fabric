// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a health report to be applied on a stateful service replica health entity.
    /// The report is sent to health store with 
    /// <see cref="System.Fabric.FabricClient.HealthClient.ReportHealth(System.Fabric.Health.HealthReport)" />.</para>
    /// </summary>
    public class StatefulServiceReplicaHealthReport : HealthReport
    {
        /// <summary>
        /// <para>Initializes a new instance of <see cref="System.Fabric.Health.StatefulServiceReplicaHealthReport" />.</para>
        /// </summary>
        /// <param name="partitionId">
        /// <para>The partition ID.</para>
        /// </param>
        /// <param name="replicaId">
        /// <para>The replica ID.</para>
        /// </param>
        /// <param name="healthInformation">
        /// <para>The <see cref="System.Fabric.Health.HealthInformation" /> which describes the report fields, like sourceId, 
        /// property, health state. Required.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>A null value was passed in for a required parameter.</para>
        /// </exception>
        public StatefulServiceReplicaHealthReport(Guid partitionId, long replicaId, HealthInformation healthInformation)
            : base(HealthReportKind.StatefulServiceReplica, healthInformation)
        {
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        /// <summary>
        /// <para>Gets the partition ID.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId { get; private set; }

        /// <summary>
        /// <para>Gets the replica ID.</para>
        /// </summary>
        /// <value>
        /// <para>The replica ID.</para>
        /// </value>
        public long ReplicaId { get; private set; }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeReplicaHealthReport = new NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT();

            nativeReplicaHealthReport.ReplicaId = this.ReplicaId;
            nativeReplicaHealthReport.PartitionId = this.PartitionId;
            nativeReplicaHealthReport.HealthInformation = this.HealthInformation.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeReplicaHealthReport);
        }
    }
}