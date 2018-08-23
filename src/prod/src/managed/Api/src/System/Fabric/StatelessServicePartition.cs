// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// In Service Fabric we have stateless as well as stateful services which are partitioned. Stateless Services are partitioned for sticky routing.
    /// In programming model we need to decide if we want to expose StatelessPartition as a concept to end users
    /// </summary>
    internal class StatelessServicePartition : PartitionBase, IStatelessServicePartition
    {
        private readonly NativeRuntime.IFabricStatelessServicePartition3 nativePartition;

        internal StatelessServicePartition(StatelessServiceBroker statelessServiceBroker, NativeRuntime.IFabricStatelessServicePartition nativeStatelessPartition)
            : base()
        {
            //// Calls native code, requires UnmanagedCode permission

            Requires.Argument("statelessServiceBroker", statelessServiceBroker).NotNull();
            Requires.Argument("nativeStatelessPartition", nativeStatelessPartition).NotNull();

            this.nativePartition = (NativeRuntime.IFabricStatelessServicePartition3)nativeStatelessPartition;
            this.PartitionInfo = ServicePartitionInformation.FromNative(nativeStatelessPartition.GetPartitionInfo());
        }

        public override void ReportLoad(NativeTypes.FABRIC_LOAD_METRIC[] loadmetrics, PinCollection pin)
        {
            this.nativePartition.ReportLoad((uint)loadmetrics.Length, pin.AddBlittable(loadmetrics));
        }

        public void ReportFault(FaultType faultType)
        {
            Utility.WrapNativeSyncInvoke(() => this.ReportFaultHelper(faultType), "StatelessPartition.ReportFault");
        }

        public void ReportMoveCost(MoveCost moveCost)
        {
            this.nativePartition.ReportMoveCost((NativeTypes.FABRIC_MOVE_COST)moveCost);
        }

        /// <summary>
        /// Reports health on the current stateless service instance.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.StatelessServicePartition.ReportInstanceHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportInstanceHealth(Health.HealthInformation healthInfo)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportInstanceHealthHelper(healthInfo, null), "StatelessPartition.ReportInstanceHealth");
        }

        /// <summary>
        /// Reports health on the current stateless service instance.
        /// Specifies send options that control how the report is sent to the health store.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// Internally, the partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportInstanceHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportInstanceHealthHelper(healthInfo, sendOptions), "StatelessPartition.ReportInstanceHealth");
        }

        /// <summary>
        /// Reports health on the current partition.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.StatelessServicePartition.ReportPartitionHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportPartitionHealth(Health.HealthInformation healthInfo)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportPartitionHealthHelper(healthInfo, null), "StatelessPartition.ReportPartitionHealth");
        }

        /// <summary>
        /// Reports health on the current partition.
        /// Specifies send options that control how the report is sent to the health store.
        /// </summary>
        /// <param name="healthInfo">Health information that is to be reported.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the partition report is sent.</para>
        /// </param>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// Internally, the partition uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportPartitionHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportPartitionHealthHelper(healthInfo, sendOptions), "StatelessPartition.ReportPartitionHealth");
        }

        private void ReportFaultHelper(FaultType faultType)
        {
            this.nativePartition.ReportFault((NativeTypes.FABRIC_FAULT_TYPE)faultType);            
        }

        private void ReportInstanceHealthHelper(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            using (var pin = new PinCollection())
            {
                var nativeSendOptions = IntPtr.Zero;
                if (sendOptions != null)
                {
                    nativeSendOptions = sendOptions.ToNative(pin);
                }

                this.nativePartition.ReportInstanceHealth2(healthInfo.ToNative(pin), nativeSendOptions);
            }
        }

        private void ReportPartitionHealthHelper(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            using (var pin = new PinCollection())
            {
                var nativeSendOptions = IntPtr.Zero;
                if (sendOptions != null)
                {
                    nativeSendOptions = sendOptions.ToNative(pin);
                }

                this.nativePartition.ReportPartitionHealth2(healthInfo.ToNative(pin), nativeSendOptions);
            }
        }
    }
}