// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// ReplicatedStoreReplica that uses TransactionalReplicator.
    /// </summary>
    internal class ReliableStateManagerReplica : StatefulServiceReplica, IStateProviderReplica
    {
        private const string TraceType = "ReliableStateManagerReplica";
        private const string DefaultReplicatorEndpointResourceName = "ReplicatorEndpoint";
        private readonly string _traceId;
        private readonly ReliableStateManagerConfiguration _configuration;
        private IStatefulServicePartition _partition;
        private Func<CancellationToken, Task<bool>> onDataLossAsyncFunction;

        public new TransactionalReplicator TransactionalReplicator
        {
            get { return base.TransactionalReplicator; }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ReliableStateManagerReplica"/> class.
        /// </summary>
        /// <param name="traceId">Id of the trace.</param>
        /// <param name="configuration">The reliable state manager's configuration.</param>
        public ReliableStateManagerReplica(string traceId, ReliableStateManagerConfiguration configuration)
        {
            Requires.ThrowIfNull(traceId, "traceId");
            Requires.ThrowIfNull(configuration, "configuration");
            Requires.ThrowIfNull(configuration.OnInitializeStateSerializersEvent, "configuration.OnInitializeStateSerializersEvent");

            this._traceId = traceId;
            this._configuration = configuration;
        }

        #region IStateProviderReplica

        Func<CancellationToken, Task<bool>> IStateProviderReplica.OnDataLossAsync
        {
            set { this.onDataLossAsyncFunction = value; }
        }

        /// <summary>
        /// Initialization of the store replica.
        /// Should only be called by Windows Fabric.
        /// </summary>
        /// <param name="initializationParameters">Initialization parameters.</param>
        void IStateProviderReplica.Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            ((IStatefulServiceReplica) this).Initialize(initializationParameters);
        }

        /// <summary>
        /// Open of the store replica.
        /// Should only be called by Windows Fabric.
        /// </summary>
        /// <param name="openMode">The open mode.</param>
        /// <param name="partition">Partition of the replica.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. </param>
        /// <returns> Task that represents the asynchronous operation.</returns>
        Task<IReplicator> IStateProviderReplica.OpenAsync(
            ReplicaOpenMode openMode,
            IStatefulServicePartition partition,
            CancellationToken cancellationToken)
        {
            this._partition = partition;
            return ((IStatefulServiceReplica) this).OpenAsync(openMode, partition, cancellationToken);
        }

        /// <summary>
        /// Change role of the store replica.
        /// Should only be called by Windows Fabric.
        /// </summary>
        /// <param name="newRole">The new role.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. </param>
        /// <returns> Task that represents the asynchronous operation.</returns>
        Task IStateProviderReplica.ChangeRoleAsync(
            ReplicaRole newRole,
            CancellationToken cancellationToken)
        {
            return ((IStatefulServiceReplica) this).ChangeRoleAsync(newRole, cancellationToken);
        }

        /// <summary>
        /// Close of the store replica.
        /// Should only be called by Windows Fabric.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. </param>
        /// <returns> Task that represents the asynchronous operation.</returns>
        Task IStateProviderReplica.CloseAsync(CancellationToken cancellationToken)
        {
            return ((IStatefulServiceReplica) this).CloseAsync(cancellationToken);
        }

        /// <summary>
        /// Abort of the store replica.
        /// </summary>
        void IStateProviderReplica.Abort()
        {
            ((IStatefulServiceReplica) this).Abort();
        }

        /// <summary>
        /// Performs a full backup of all reliable state managed by this <see cref="IReliableStateManager"/>.
        /// </summary>
        /// <param name="backupCallback">Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.</param>
        /// <returns>Task that represents the asynchronous backup operation.</returns>
        /// <remarks>
        /// A FULL backup will be performed with a one-hour timeout.
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        public Task BackupAsync(Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return this.TransactionalReplicator.BackupAsync(backupCallback);
        }

        /// <summary>
        /// Performs a backup of all reliable state managed by this <see cref="IReliableStateManager"/>.
        /// </summary>
        /// <param name="option">The type of backup to perform.</param>
        /// <param name="timeout">The timeout for this operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <param name="backupCallback">Callback to be called when the backup folder has been created locally and is ready to be moved out of the node.</param>
        /// <returns>Task that represents the asynchronous backup operation.</returns>
        /// <remarks>
        /// Boolean returned by the backupCallback indicate whether the service was able to successfully move the backup folder to an external location.
        /// If false is returned, BackupAsync throws InvalidOperationException with the relevant message indicating backupCallback returned false.
        /// Also, backup will be marked as unsuccessful.
        /// </remarks>
        public Task BackupAsync(BackupOption option, TimeSpan timeout, CancellationToken cancellationToken, Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return this.TransactionalReplicator.BackupAsync(backupCallback, option, timeout, cancellationToken);
        }

        /// <summary>
        /// Restore a backup taken by <see cref="IStateProviderReplica.BackupAsync(System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/> or 
        /// <see cref="IStateProviderReplica.BackupAsync(Microsoft.ServiceFabric.Data.BackupOption,System.TimeSpan,System.Threading.CancellationToken,System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/>.
        /// </summary>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <remarks>
        /// A safe backup will be performed, meaning the restore will only be completed if the data to restore is ahead of state of the current replica.
        /// </remarks>
        /// <returns>Task that represents the asynchronous restore operation.</returns>
        public Task RestoreAsync(string backupFolderPath)
        {
            return this.TransactionalReplicator.RestoreAsync(backupFolderPath);
        }

        /// <summary>
        /// Restore a backup taken by <see cref="IStateProviderReplica.BackupAsync(System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/> or 
        /// <see cref="IStateProviderReplica.BackupAsync(Microsoft.ServiceFabric.Data.BackupOption,System.TimeSpan,System.Threading.CancellationToken,System.Func{Microsoft.ServiceFabric.Data.BackupInfo,CancellationToken,System.Threading.Tasks.Task{bool}})"/>.
        /// </summary>
        /// <param name="restorePolicy">The restore policy.</param>
        /// <param name="backupFolderPath">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous restore operation.</returns>
        public Task RestoreAsync(string backupFolderPath, RestorePolicy restorePolicy, CancellationToken cancellationToken)
        {
            return this.TransactionalReplicator.RestoreAsync(backupFolderPath, restorePolicy, cancellationToken);
        }

        #endregion

        #region IEnumerable

        /// <summary>
        /// The get enumerator.
        /// </summary>
        /// <returns>
        /// The <see cref="IEnumerator"/>.
        /// </returns>
        public IAsyncEnumerator<IStateProvider2> GetAsyncEnumerator()
        {
            // Reliable state manager should always have true indicating that only the top-level state providers need to be enumerated.
            return this.TransactionalReplicator.CreateAsyncEnumerable(true, false).GetAsyncEnumerator();
        }

        #endregion

        protected override async Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            var res = await base.OnDataLossAsync(cancellationToken).ConfigureAwait(false);
            if (this.onDataLossAsyncFunction != null)
            {
                res = await this.onDataLossAsyncFunction.Invoke(cancellationToken).ConfigureAwait(false);
            }
            return res;
        }

        protected override async Task<ReliableStateManagerReplicatorSettings> OnOpenAsync(CancellationToken cancellationToken)
        {
            await this._configuration.OnInitializeStateSerializersEvent().ConfigureAwait(false);

            return this.GetReplicatorSettingsOnOpen();
        }

        private void OnConfigurationPackageModified(object sender, PackageModifiedEventArgs<ConfigurationPackage> e)
        {
            try
            {
                var settings = this.LoadReplicatorSettingsFromConfigPackage(false);
                this.UpdateReplicatorSettings(settings);
            }
            catch (FabricElementNotFoundException ex)
            {
                // Trace and Report fault when section is not found for ReplicatorSettings.
                DataImplTrace.Source.WriteErrorWithId(
                    TraceType,
                    this._traceId,
                    "FabricElementNotFoundException while loading replicator settings from configuation.",
                    ex);
                this._partition.ReportFault(FaultType.Transient);
            }
            catch (FabricException ex)
            {
                // Trace and Report fault if user intended to provide Replicator Security config but provided it incorrectly.
                DataImplTrace.Source.WriteErrorWithId(
                    TraceType,
                    this._traceId,
                    "FabricException while loading replicator security settings from configuation.",
                    ex);
                this._partition.ReportFault(FaultType.Transient);
            }
            catch (ArgumentException ex)
            {
                DataImplTrace.Source.WriteWarningWithId(TraceType, this._traceId, "ArgumentException while updating replicator settings from configuation.", ex);
                this._partition.ReportFault(FaultType.Transient);
            }
        }

        private ReliableStateManagerReplicatorSettings GetReplicatorSettingsOnOpen()
        {
            if (this._configuration.ReplicatorSettings != null)
            {
                return this._configuration.ReplicatorSettings;
            }

            return this.LoadReplicatorSettingsFromConfigPackage(true);
        }

        private ReliableStateManagerReplicatorSettings LoadReplicatorSettingsFromConfigPackage(bool addChangeListener)
        {
            ReliableStateManagerReplicatorSettings settings = null;

            var configPackage = this.GetConfigurationPackage();
            if (configPackage == null)
            {
                settings = new ReliableStateManagerReplicatorSettings();
            }
            else
            {
                if (string.IsNullOrEmpty(this._configuration.ReplicatorSettingsSectionName)
                    || !configPackage.Settings.Sections.Contains(this._configuration.ReplicatorSettingsSectionName))
                {
                    DataImplTrace.Source.WriteWarningWithId(
                        TraceType,
                        this._traceId,
                        "Using default replicator settings, unable to load replicator settings config section named '{0}' from config package named '{1}'.",
                        this._configuration.ReplicatorSettingsSectionName,
                        this._configuration.ConfigPackageName);

                    settings = new ReliableStateManagerReplicatorSettings();
                }
                else
                {
                    DataImplTrace.Source.WriteInfoWithId(
                        TraceType,
                        this._traceId,
                        "Loading replicator settings using <config package name: '{0}', settings section name: '{1}'>",
                        this._configuration.ConfigPackageName,
                        this._configuration.ReplicatorSettingsSectionName);

                    settings = ReliableStateManagerReplicatorSettingsUtil.LoadFrom(
                        this.InitializationParameters.CodePackageActivationContext,
                        this._configuration.ConfigPackageName,
                        this._configuration.ReplicatorSettingsSectionName);
                }

                // SecurityCredentials.LoadFrom doesn't return the correct value when CredentialType is None in config.  todo: find bug#
                // To workaround this bug, only read replicator security settings if CredentialType is present and not explicitly None.
                const string replicatorCredentialTypeName = "CredentialType";
                if (string.IsNullOrEmpty(this._configuration.ReplicatorSecuritySectionName)
                    || !configPackage.Settings.Sections.Contains(this._configuration.ReplicatorSecuritySectionName)
                    || !configPackage.Settings.Sections[this._configuration.ReplicatorSecuritySectionName].Parameters.Contains(replicatorCredentialTypeName)
                    ||
                    configPackage.Settings.Sections[this._configuration.ReplicatorSecuritySectionName].Parameters[replicatorCredentialTypeName].Value.Equals(
                        "None",
                        StringComparison.OrdinalIgnoreCase))
                {
                    DataImplTrace.Source.WriteWarningWithId(
                        TraceType,
                        this._traceId,
                        "Using default replicator security settings, unable to load replicator security settings config section named '{0}' from config package named '{1}'.",
                        this._configuration.ReplicatorSecuritySectionName,
                        this._configuration.ConfigPackageName);
                }
                else
                {
                    if (addChangeListener)
                    {
                        this.InitializationParameters.CodePackageActivationContext.ConfigurationPackageModifiedEvent += this.OnConfigurationPackageModified;
                    }

                    settings.SecurityCredentials = SecurityCredentials.LoadFrom(
                        this.InitializationParameters.CodePackageActivationContext,
                        this._configuration.ConfigPackageName,
                        this._configuration.ReplicatorSecuritySectionName);
                }
            }

            if (string.IsNullOrWhiteSpace(settings.ReplicatorAddress))
            {
                settings.ReplicatorAddress = this.TryGetDefaultReplicatorAddress();
            }

            return settings;
        }

        internal ConfigurationPackage GetConfigurationPackage()
        {
            if (string.IsNullOrEmpty(this._configuration.ConfigPackageName))
            {
                DataImplTrace.Source.WriteWarningWithId(
                    TraceType,
                    this._traceId,
                    "Using default replicator settings as the config package name is null or empty.");
                return null;
            }

            if (this.InitializationParameters == null
                || this.InitializationParameters.CodePackageActivationContext == null)
            {
                DataImplTrace.Source.WriteErrorWithId(
                    TraceType,
                    this._traceId,
                    "Using default replicator settings as ServiceInitializationParameters or CodePackageActivationContext are null.");
                Debug.Assert(false, "ServiceInitializationParameters or CodePackageActivationContext are null");
                return null;
            }

            if (!this.InitializationParameters.CodePackageActivationContext.GetConfigurationPackageNames().Contains(this._configuration.ConfigPackageName))
            {
                DataImplTrace.Source.WriteWarningWithId(
                    TraceType,
                    this._traceId,
                    "Using default replicator settings, unable to find config package named: '{0}'.",
                    this._configuration.ConfigPackageName);
                return null;
            }

            var configPackage = this.InitializationParameters.CodePackageActivationContext.GetConfigurationPackageObject(this._configuration.ConfigPackageName);
            if (configPackage == null
                || configPackage.Settings == null
                || configPackage.Settings.Sections == null)
            {
                DataImplTrace.Source.WriteErrorWithId(
                    TraceType,
                    this._traceId,
                    "Using default replicator settings, unable to load config package named: '{0}'.",
                    this._configuration.ConfigPackageName);
                Debug.Assert(false, "GetConfigurationPackageObject returned null or the returned config package had null settings or sections");
                return null;
            }

            return configPackage;
        }

        private string TryGetDefaultReplicatorAddress()
        {
            var nodeContext = FabricRuntime.GetNodeContext();
            var replicationPort = 0;

            try
            {
                var endpoint = this.InitializationParameters.CodePackageActivationContext.GetEndpoints()[DefaultReplicatorEndpointResourceName];
                if (endpoint.Protocol != System.Fabric.Description.EndpointProtocol.Tcp)
                {
                    throw new NotSupportedException(
                        string.Format(CultureInfo.CurrentCulture, SR.Error_ReliableSMReplica_InvalidEndpointProtocol, endpoint.Protocol));
                }
                replicationPort = endpoint.Port;
            }
            catch (Exception e)
            {
                DataImplTrace.Source.WriteInfoWithId(
                    TraceType,
                    this._traceId,
                    "CodePackageActivationContext.GetEndpoints() for DefaultReplicatorEndpointResource failed. Using port number 0",
                    e);
            }

            return (string.Format(CultureInfo.InvariantCulture, "{0}:{1}", nodeContext.IPAddressOrFQDN, replicationPort));
        }
    }
}