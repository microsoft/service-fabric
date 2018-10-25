// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>Represents a partition that is associated with a stateful service replica. </para>
    /// </summary>
    /// <remarks>
    /// <para>Derived from <see cref="System.Fabric.IServicePartition" />.</para>
    /// </remarks>
    public interface IStatefulServicePartition : IServicePartition
    {
        /// <summary>
        /// <para>Used to check the readiness of the replica in regard to read operations. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.PartitionAccessStatus" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.IStatefulServicePartition.ReadStatus" /> should be checked before the replica is servicing a customer request that is a read operation.</para>
        /// </remarks>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        PartitionAccessStatus ReadStatus { get; }

        /// <summary>
        /// <para>Used to check the readiness of the partition in regard to write operations. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.PartitionAccessStatus" />.</para>
        /// </value>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.IStatefulServicePartition.WriteStatus" /> should be checked before the replica
        /// services a customer request that is a write operation.</para>
        /// </remarks>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        PartitionAccessStatus WriteStatus { get; }

        /// <summary>
        /// <para>Creates a <see cref="System.Fabric.FabricReplicator" /> with the specified settings and returns it to the replica. </para>
        /// </summary>
        /// <param name="stateProvider">
        /// <para>The <see cref="System.Fabric.IStateProvider" /> with which the returned <see cref="System.Fabric.FabricReplicator" /> should be associated. 
        /// This is often the same object that implements <see cref="System.Fabric.IStatefulServiceReplica" />, but certain services might be factored differently. </para>
        /// </param>
        /// <param name="replicatorSettings">
        /// <para>The <see cref="System.Fabric.ReplicatorSettings" /> with which the returned <see cref="System.Fabric.FabricReplicator" /> should be configured. </para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.FabricReplicator" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>This method should be used to create a <see cref="System.Fabric.FabricReplicator" /> to service as the <see cref="System.Fabric.IStateReplicator" /> for a stateful service that implements <see cref="System.Fabric.IStateProvider" />.</para>
        /// </remarks>
        FabricReplicator CreateReplicator(IStateProvider stateProvider, ReplicatorSettings replicatorSettings);

        /// <summary>
        /// Reports health on the current stateful service replica of the partition.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <returns></returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.IStatefulServicePartition.ReportReplicaHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        void ReportReplicaHealth(Health.HealthInformation healthInfo);

        /// <summary>
        /// Reports health on the current stateful service replica of the partition.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <returns></returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// Internally, the partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        void ReportReplicaHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions);
    }
}