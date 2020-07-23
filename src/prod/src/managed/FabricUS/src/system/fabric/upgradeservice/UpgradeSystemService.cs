// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    extern alias ServiceFabricInternal;
    using KeyValueStoreReplica_V2 = ServiceFabricInternal::System.Fabric.KeyValueStoreReplica_V2;

    using System.Fabric;
    using System.Fabric.Common;
    using System.Linq;
    using System.Globalization;
    using System.IO;
    using System.Text;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class UpgradeSystemService : KeyValueStoreReplica_V2
    {
        private static readonly TraceType TraceType = new TraceType("UpgradeSystemService");
        private readonly ClusterEndpointSecuritySettingsChangeNotifier clusterEndpointSecuritySettingsChangeNotifier;
        private readonly string replicatorAddress;
        private readonly bool enableEndpointV2;

        // DatabaseFileName is the ESE database file name for KVS V1
        // and the shared log file name for KVS V2
        //
        private const string DatabaseFileName = "UpgradeServiceStore";

        // The original Upgrade Service unfortunately did not put its ESE database in 
        // a sub-directory, so the ESE database is located directly under the system
        // services application working directory.
        //
        // The sub-directory is only used by the Upgrade Service running on KVS V2
        // (shared log sub-directory)
        //
        private const string DatabaseSubDirectory = "US";

        private string configSectionName;
        private NativeConfigStore configStore;

        private string serviceId;
        private CancellationTokenSource cancellationTokenSource;
        private IStatefulServicePartition partition;
        private List<Task> coordinatorTasks;

        private UpgradeSystemService(
            string workingDirectory,
            Guid partitionId)
            : base(
                workingDirectory, // (V2 only)
                DatabaseSubDirectory, // (V2 only)
                DatabaseFileName, // (both V1 and V2)
                partitionId) // shared log ID (V2 only)
        {
            this.cancellationTokenSource = null;

            this.configStore = NativeConfigStore.FabricGetConfigStore();
            this.enableEndpointV2 = EnableEndpointV2Utility.GetValue(configStore);

            this.replicatorAddress = GetReplicatorAddress();
            Trace.WriteInfo(TraceType, "Read UpgradeServiceReplicatorAddress={0}", this.replicatorAddress);

            if (this.enableEndpointV2)
            {
                this.clusterEndpointSecuritySettingsChangeNotifier = new ClusterEndpointSecuritySettingsChangeNotifier(
                    this.replicatorAddress, 
                    this.UpdateReplicatorSettingsOnConfigChange);
            }
        }

        public static UpgradeSystemService CreateAndInitialize(StatefulServiceInitializationParameters initParams)
        {
            var replica = new UpgradeSystemService(
                initParams.CodePackageActivationContext.WorkDirectory,
                initParams.PartitionId);

            replica.InitializeKvs(initParams);

            return replica;
        } 

        private void InitializeKvs(StatefulServiceInitializationParameters initParams)
        {
            this.configSectionName = Encoding.Unicode.GetString(initParams.InitializationData);

            string enableTStoreString = this.configStore.ReadUnencryptedString(
                this.configSectionName, 
                Constants.ConfigurationSection.EnableTStore);

            bool enableTStore = (string.Compare(enableTStoreString, "true", StringComparison.CurrentCultureIgnoreCase) == 0);

            string globalEnableTStoreString = this.configStore.ReadUnencryptedString(
                Constants.ConfigurationSection.ReplicatedStore,
                Constants.ConfigurationSection.EnableTStore);

            bool globalEnableTStore = (string.Compare(globalEnableTStoreString, "true", StringComparison.CurrentCultureIgnoreCase) == 0);

            if (enableTStore || globalEnableTStore)
            {
                base.Initialize_OverrideNativeKeyValueStore(initParams);
            }
            else
            {
                base.Initialize(initParams);
            }
        }

        #region Override properties
        protected override void OnInitialize(StatefulServiceInitializationParameters initializationParameters)
        {
            initializationParameters.ThrowIfNull("initializationParameters");
            base.OnInitialize(initializationParameters);
            this.UpdateReplicatorSettings(this.GetReplicatorSettings());
            this.serviceId = string.Format(
                CultureInfo.InvariantCulture,
                "[{0}+{1}+{2}]",
                initializationParameters.ServiceName,
                initializationParameters.PartitionId,
                initializationParameters.ReplicaId);
        }

        protected override Task OnOpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            partition.ThrowIfNull("partition");
            this.partition = partition;

            Trace.WriteInfo(
                    TraceType,
                    "OpenAsync: serviceId = {0}, configSectionName = {1}, OpenMode = {2}",
                    this.serviceId,
                    this.configSectionName,
                    openMode);

            return base.OnOpenAsync(openMode, partition, cancellationToken);
        }

        protected override Task OnCloseAsync(CancellationToken cancellationToken)
        {
            Trace.WriteInfo(
                TraceType,
                "CloseAsync: serviceId = {0}",
                this.serviceId);

            this.CloseCoordinator();

            return base.OnCloseAsync(cancellationToken);
        }

        protected override Task<string> OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            Trace.WriteInfo(TraceType, "ChangeRole: {0}, {1}", this.serviceId, newRole);

            if (newRole == ReplicaRole.Primary)
            {
                var coordinator = this.CreateCoordinator(this.configStore, this.configSectionName);

                this.cancellationTokenSource = new CancellationTokenSource();

                // The primary will be grated RW access only if ChangeRoleAsync completes
                // Hence complete OnChangeRoleAsync synchronously without awaiting on the task
                var backgroundTask = Task.Run(
                    () => this.ScheduleProcessingAfterWriteAccess(coordinator, this.cancellationTokenSource.Token),
                    cancellationToken);

                return Task.FromResult(coordinator.ListeningAddress);
            }
            else
            {
                this.CloseCoordinator();
                return Task.FromResult(string.Empty);
            }            
        }

        protected override void OnAbort()
        {
            Trace.WriteInfo(
                TraceType,
                "Abort: serviceId = {0}",
                this.serviceId);
            this.CloseCoordinator();
        }

        #endregion

        private async Task ScheduleProcessingAfterWriteAccess(IUpgradeCoordinator coordinator, CancellationToken cancellationToken)
        {
            while (this.partition != null &&
                       this.partition.WriteStatus != PartitionAccessStatus.Granted)
            {
                await Task.Delay(100, cancellationToken);
            }

            var localCoordinatorTasks = coordinator.StartProcessing(cancellationToken);
            this.coordinatorTasks = localCoordinatorTasks.Select(task => task.ContinueWith(t =>
            {
                if (this.cancellationTokenSource.IsCancellationRequested)
                {
                    return;
                }

                Trace.WriteError(TraceType, "Coordinator task stopped. Exception: {0}", t.Exception);

                try
                {
                    this.partition.ReportFault(FaultType.Transient);
                }
                catch (Exception ex)
                {
                    Trace.WriteError(TraceType, "Error reporting failure. Exception: {0}", ex.ToString());
                    ReleaseAssert.Failfast("A required background worker stopped and can't ReportFault");
                }
            })).ToList();
        }

        private static string GetReplicatorAddress()
        {
            NativeConfigStore configStore = System.Fabric.Common.NativeConfigStore.FabricGetConfigStore();
            string replicatorAddress = configStore.ReadUnencryptedString("FabricNode", "UpgradeServiceReplicatorAddress");
            return replicatorAddress;
        }

        private IUpgradeCoordinator CreateCoordinator(IConfigStore configStore, string configSectionName)
        {
            string coordinatorType = configStore.ReadUnencryptedString(configSectionName, Constants.ConfigurationSection.CoordinatorType);
            Trace.WriteNoise(TraceType, "Coordinator type: {0}", coordinatorType ?? "<null>");

            if (string.Equals(coordinatorType, Constants.ConfigurationSection.PaasCoordinator, StringComparison.OrdinalIgnoreCase))
            {
                return new PaasCoordinator(configStore, configSectionName, this, this.partition);
            }

#if !DotNetCoreClr
            if (string.Equals(coordinatorType, Constants.ConfigurationSection.WindowsUpdateServiceCoordinator, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(coordinatorType, Constants.ConfigurationSection.WUTestCoordinator, StringComparison.OrdinalIgnoreCase))
            {
                return new WindowsUpdateServiceCoordinator(configStore, configSectionName, this, this.partition, new ExceptionHandlingPolicy(Constants.WUSCoordinator.HealthProperty, ResourceCoordinator.TaskName, configStore, configSectionName, partition));
            }
#endif

            // Else return dummy coordinator
            Trace.WriteInfo(TraceType, "Returning dummy coordinator");
            return new DummyCoordinator();
        }

        private ReplicatorSettings GetReplicatorSettings()
        {
            Trace.WriteInfo(TraceType, "Replicator address = {0}", this.replicatorAddress);
            SecurityCredentials securityCredentials = null;
            if (this.enableEndpointV2)
            {
                securityCredentials = SecurityCredentials.LoadClusterSettings();
            }

            return new ReplicatorSettings
            {
                ReplicatorAddress = this.replicatorAddress,
                SecurityCredentials = securityCredentials
            };
        }

        private void CloseCoordinator()
        {
            if (this.cancellationTokenSource != null && !this.cancellationTokenSource.IsCancellationRequested)
            {
                Trace.WriteInfo(TraceType, "Close Coordinator");
                this.cancellationTokenSource.Cancel();
            }
        }

        private void UpdateReplicatorSettingsOnConfigChange(string endpoint)
        {
            ReplicatorSettings replicatorSettings = this.GetReplicatorSettings();
            this.UpdateReplicatorSettings(replicatorSettings);
        }
    }
}