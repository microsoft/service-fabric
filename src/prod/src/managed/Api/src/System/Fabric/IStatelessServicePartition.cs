// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Health;

    /// <summary>
    /// <para>Represents a partition that is associated with a stateless service instance.</para>
    /// </summary>
    /// <remarks>
    /// <para>Provided to a stateless service as a parameter to the <see cref="System.Fabric.IServicePartition" />.</para>
    /// </remarks>
    public interface IStatelessServicePartition : IServicePartition
    {
        /// <summary>
        /// Reports health information on the current stateless service instance of the partition. 
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
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.IStatelessServicePartition.ReportInstanceHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        void ReportInstanceHealth(HealthInformation healthInfo);

        /// <summary>
        /// Reports health information on the current stateless service instance of the partition. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property and health state.</param>
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
        /// The partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        void ReportInstanceHealth(HealthInformation healthInfo, HealthReportSendOptions sendOptions);
    }
}