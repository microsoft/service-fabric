// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    internal class StatefulServicePartition : PartitionBase, IStatefulServicePartition
    {
        private readonly NativeRuntime.IFabricStatefulServicePartition3 nativePartition;
        
        // internal constructor - used by test code
        internal StatefulServicePartition(
            NativeRuntime.IFabricStatefulServicePartition nativeStatefulPartition,
            ServicePartitionInformation partitionInfo)
            : base()
        {
            Requires.Argument("nativeStatefulPartition", nativeStatefulPartition).NotNull();
            Requires.Argument("partitionInfo", partitionInfo).NotNull();

            this.nativePartition = (NativeRuntime.IFabricStatefulServicePartition3) nativeStatefulPartition;
            this.PartitionInfo = partitionInfo;
        }

        public PartitionAccessStatus ReadStatus
        {
            get
            {
                return Utility.WrapNativeSyncInvoke(() => (PartitionAccessStatus)this.nativePartition.GetReadStatus(), "StatefulPartition.ReadStatus");
            }
        }

        public PartitionAccessStatus WriteStatus
        {
            get
            {
                return Utility.WrapNativeSyncInvoke(() => (PartitionAccessStatus)this.nativePartition.GetWriteStatus(), "StatefulPartition.WriteStatus");
            }
        }

        internal NativeRuntime.IFabricStatefulServicePartition NativePartition
        {
            get
            {
                return this.nativePartition;
            }
        }

        public FabricReplicator CreateReplicator(IStateProvider stateProvider, ReplicatorSettings replicatorSettings)
        {
            return Utility.WrapNativeSyncInvoke(() => this.CreateFabricReplicatorHelper(stateProvider, replicatorSettings), "StatefulPartition.CreateFabricReplicator");
        }

        public override void ReportLoad(NativeTypes.FABRIC_LOAD_METRIC[] loadmetrics, PinCollection pin)
        {
            this.nativePartition.ReportLoad((uint)loadmetrics.Length, pin.AddBlittable(loadmetrics));
        }

        public void ReportFault(FaultType faultType)
        {
            Utility.WrapNativeSyncInvoke(() => this.ReportFaultHelper(faultType), "StatefulPartition.ReportFault");
        }

        public void ReportMoveCost(MoveCost moveCost)
        {
            this.nativePartition.ReportMoveCost((NativeTypes.FABRIC_MOVE_COST)moveCost);
        }

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
        /// <see cref="System.Fabric.StatefulServicePartition.ReportReplicaHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportReplicaHealth(Health.HealthInformation healthInfo)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportReplicaHealthHelper(healthInfo, null), "StatefulServicePartition.ReportReplicaHealth");
        }

        /// <summary>
        /// Reports health on the current replica.
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
        public void ReportReplicaHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportReplicaHealthHelper(healthInfo, sendOptions), "StatefulServicePartition.ReportReplicaHealth");
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
        /// <see cref="System.Fabric.StatefulServicePartition.ReportPartitionHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportPartitionHealth(Health.HealthInformation healthInfo)
        {
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportPartitionHealthHelper(healthInfo, null), "StatefulServicePartition.ReportPartitionHealth");
        }

        /// <summary>
        /// Reports health on the current partition.
        /// Specifies send options that control how the report is sent to the health store.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
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
            Utility.WrapNativeSyncInvoke(() => this.ReportPartitionHealthHelper(healthInfo, sendOptions), "StatefulServicePartition.ReportPartitionHealth");
        }

        private void ReportReplicaHealthHelper(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            using (var pin = new PinCollection())
            {
                var nativeSendOptions = IntPtr.Zero;
                if (sendOptions != null)
                {
                    nativeSendOptions = sendOptions.ToNative(pin);
                }

                this.nativePartition.ReportReplicaHealth2(healthInfo.ToNative(pin), nativeSendOptions);
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

        private static StateProviderBroker CreateStateProviderBroker(IStateProvider stateProvider)
        {
            AppTrace.TraceSource.WriteNoise("StatefulPartition.CreateStateProviderBroker", "Creating broker for {0}", stateProvider);

            StateProviderBroker stateProviderBroker;

            IAtomicGroupStateProvider atomicGroupStateProvider = stateProvider as IAtomicGroupStateProvider;
            if (atomicGroupStateProvider != null)
            {
                AppTrace.TraceSource.WriteNoise("StatefulPartition.CreateStateProviderBroker", "StateProvider is IFabricAtomicGroupStateProvider");
                stateProviderBroker = new AtomicGroupStateProviderBroker(stateProvider, atomicGroupStateProvider);
            }
            else
            {
                stateProviderBroker = new StateProviderBroker(stateProvider);
            }

            return stateProviderBroker;
        }

        private void ReportFaultHelper(FaultType faultType)
        {
            this.nativePartition.ReportFault((NativeTypes.FABRIC_FAULT_TYPE)faultType);            
        }

        private FabricReplicator CreateFabricReplicatorHelper(IStateProvider stateProvider, ReplicatorSettings replicatorSettings)
        {
            Requires.Argument<IStateProvider>("stateProvider", stateProvider).NotNull();

            // Initialize replicator
            if (replicatorSettings == null)
            {
                AppTrace.TraceSource.WriteNoise("StatefulPartition.CreateFabricReplicator", "Using default replicator settings");
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("StatefulPartition.CreateFabricReplicator", "{0}", replicatorSettings);
            }

            NativeRuntime.IFabricReplicator nativeReplicator;
            NativeRuntime.IFabricStateReplicator nativeStateReplicator;

            StateProviderBroker stateProviderBroker = StatefulServicePartition.CreateStateProviderBroker(stateProvider);

            using (var pin = new PinCollection())
            {
                unsafe
                {
                    if (replicatorSettings == null)
                    {
                        nativeStateReplicator = this.nativePartition.CreateReplicator(stateProviderBroker, IntPtr.Zero, out nativeReplicator);
                    }
                    else
                    {
                        IntPtr nativeReplicatorSettings = replicatorSettings.ToNative(pin);
                        nativeStateReplicator = this.nativePartition.CreateReplicator(stateProviderBroker, nativeReplicatorSettings, out nativeReplicator);
                    }
                }
            }

            NativeRuntime.IOperationDataFactory nativeOperationDataFactory = (NativeRuntime.IOperationDataFactory)nativeStateReplicator;
            if (nativeStateReplicator == null || nativeReplicator == null || nativeOperationDataFactory == null)
            {
                AppTrace.TraceSource.WriteError("StatefulPartition.Create", "Native replicators could not be initialized.");
                throw new InvalidOperationException(StringResources.Error_StatefulServicePartition_Native_Replicator_Init_Failed);
            }

            stateProviderBroker.OperationDataFactory = new OperationDataFactoryWrapper(nativeOperationDataFactory);

            return new FabricReplicator(nativeReplicator, nativeStateReplicator, nativeOperationDataFactory);
        }

    }
}