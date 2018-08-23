// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Health;

    /// <summary>
    /// <para>Provides information to the service about the partition to which it belongs and provides methods for the service to interact with the system during runtime.</para>
    /// </summary>
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public interface member from V1.")]
    public interface IServicePartition
    {
        /// <summary>
        /// <para>Reports the load for a set of load balancing metrics. The load can be reported at any time via the <see cref="System.Fabric.IServicePartition.ReportLoad(System.Collections.Generic.IEnumerable{System.Fabric.LoadMetric})" /> method and provides one or more properties of the <see cref="System.Fabric.LoadMetric" /> method.</para>
        /// </summary>
        /// <param name="metrics">
        /// <para>A collection of <see cref="System.Fabric.LoadMetric" /> to report the load for. </para>
        /// </param>
        /// <remarks>
        /// <para>The reported metrics should correspond to those that are provided in the <see cref="System.Fabric.Description.ServiceLoadMetricDescription" /> as a part of the <see cref="System.Fabric.Description.ServiceDescription" /> that is used to create the service. Load metrics that are not present in the description are ignored. Reporting custom metrics allows Service Fabric to balance services that are based on additional custom information.</para>
        /// </remarks>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        void ReportLoad(IEnumerable<LoadMetric> metrics);

        /// <summary>
        /// <para>Enables the replica to report a fault to the runtime and 
        /// indicates that it has encountered an error from which it cannot 
        /// recover and must either be restarted or removed.</para>
        /// </summary>
        /// <param name="faultType">
        /// <para>The <see cref="System.Fabric.FaultType" /> that the service has encountered.</para>
        /// </param>
        /// <remarks>
        /// <para>A fault is typically reported when the service code encounters an issue from which it cannot recover.</para>
        /// </remarks>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        void ReportFault(FaultType faultType);

        /// <summary>
        /// <para>Provides access to the <see cref="System.Fabric.ServicePartitionInformation" /> of the service, which contains the partition type and ID.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.ServicePartitionInformation" />.</para>
        /// </value>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        ServicePartitionInformation PartitionInfo { get; }

        /// <summary>
        /// <para>Reports the move cost for a replica.</para>
        /// </summary>
        /// <param name="moveCost">
        /// <para>The reported <see cref="System.Fabric.MoveCost" />.</para>
        /// </param>
        /// <remarks>
        /// <para>Services can report move cost of a replica using this method. While the Service Fabric Resource Balances searches 
        /// for the best balance in the cluster, it examines both load information and move cost of each replica. 
        /// Resource balances will prefer to move replicas with lower cost in order to achieve balance. </para>
        /// </remarks>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>
        ///     This indicates that the partition object is closed. The replica/replicator/instance has either been closed or is about to be closed.</para>
        /// </exception>
        void ReportMoveCost(MoveCost moveCost);

        /// <summary>
        /// Reports current partition health. 
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
        /// <see cref="System.Fabric.IServicePartition.ReportPartitionHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        void ReportPartitionHealth(HealthInformation healthInfo);

        /// <summary>
        /// Reports current partition health. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the partition report is sent.</para>
        /// </param>
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
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        void ReportPartitionHealth(HealthInformation healthInfo, HealthReportSendOptions sendOptions);
    }
}