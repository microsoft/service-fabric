// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Notifications;
    using Microsoft.ServiceFabric.Replicator;
    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using Microsoft.ServiceFabric.Data.Collections;
    using System.Fabric.BackupRestore;
    using System.Fabric.Common;
    using System.IO;

    /// <summary>
    /// Manages the reliable state for a service.
    /// </summary>
    public class ReliableStateManagerImpl : IReliableStateManagerReplica2, IBackupRestoreReplica
    {
        /// <summary>
        /// Arbitrary Uri prefix to be prepended to non-Uri string names
        /// </summary>
        private const string ArbitraryUriPrefix = "urn:";

        private const string TraceType = "ReliableStateManagerImpl";
        private static readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(4);

        private readonly Uri BackupRestoreInternalStore = new Uri("urn:c0e554a9-5936-4655-b175-46b6f969549f:BackupRestoreStore");

        /// <summary>
        /// Handles replica life-cycle
        /// </summary>
        private readonly ReliableStateManagerReplica _replica;

        /// <summary>
        /// Mapping of reliable object types to concrete wrapper types.
        /// </summary>
        private readonly StateManagerTypeCache _typeCache = new StateManagerTypeCache();

        /// <summary>
        /// Component's trace id.
        /// </summary>
        private readonly string _traceId;

        private readonly IBackupRestoreManager _backupRestoreManager;

        private Func<CancellationToken, Task<bool>> userOnDataLossAsyncFunc;

        private Func<CancellationToken, Task> userOnRestoreCompletedAsyncFunc;

#if DotNetCoreClr && !DotNetCoreClrLinux
        // This dynamic loading of System.Fabric.BackupRestore.dll is only required for NetStandard dll on Windows. 
        // As corresponding System.Fabric.dll doesn't have dynamic loading code for NetStandard code path.
        // RD Bug:12295686

        private const string SystemFabricBackupRestoreAssemblyName = "System.Fabric.BackupRestore";

        static ReliableStateManagerImpl()
        {
            AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(LoadFromFabricCodePath);
        }

        static Assembly LoadFromFabricCodePath(object sender, ResolveEventArgs args)
        {
            try
            {
                var assemblyToLoad = new AssemblyName(args.Name);

                if (string.Compare(SystemFabricBackupRestoreAssemblyName, assemblyToLoad.Name, true) == 0)
                {
                    var folderPath = FabricEnvironment.GetCodePath();

                    // .NetStandard compatible dlls with its dependencies are located in "NS' subfolder under Fabric.Code folder.
                    folderPath = Path.Combine(folderPath, "NS");

                    var assemblyPath = Path.Combine(folderPath, assemblyToLoad.Name + ".dll");

                    if (File.Exists(assemblyPath))
                    {
                        return Assembly.LoadFrom(assemblyPath);
                    }
                }
            }
            catch (Exception)
            {
                // Supress any Exception so that we can continue to
                // load the assembly through other means
            }

            return null;
        }
#endif

        /// <summary>
        /// Create a new ReliableStateManager
        /// </summary>
        /// <param name="serviceContext"></param>
        /// <param name="configuration"></param>
        public ReliableStateManagerImpl(StatefulServiceContext serviceContext, ReliableStateManagerConfiguration configuration)
        {
            this._traceId = GetTraceIdForReplica(serviceContext);
            this._replica = new ReliableStateManagerReplica(this._traceId, configuration);
#if !DotNetCoreClrLinux
            this._backupRestoreManager = BackupRestoreManagerFactory.GetBackupRestoreManager(this);
#else
            this._backupRestoreManager = null;
#endif

            ((IStateProviderReplica)this._replica).OnDataLossAsync = OnDataLossAsyncInternal;
        }

        /// <summary>
        /// Occurs when a transaction's state changes.
        /// </summary>
        public event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged
        {
            add { this.Replicator.TransactionChanged += value; }
            remove { this.Replicator.TransactionChanged -= value; }
        }

        /// <summary>
        /// Occurs when State Manager's state changes.
        /// </summary>
        public event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged
        {
            add { this.Replicator.StateManagerChanged += value; }
            remove { this.Replicator.StateManagerChanged -= value; }
        }

        /// <summary>
        /// Return the current data loss number.
        /// </summary>
        /// <returns>The current data loss number.</returns>
        public long GetCurrentDataLossNumber()
        {
            return this.Replicator.GetCurrentDataLossNumber();
        }

        /// <summary>
        /// Return the last committed sequence number.
        /// </summary>
        /// <returns>The last committed sequence number.</returns>
        public long GetLastCommittedSequenceNumber()
        {
            return this.Replicator.GetLastCommittedSequenceNumber();
        }

        /// <summary>
        /// Underlying Transactional Replicator
        /// </summary>
        internal TransactionalReplicator Replicator
        {
            get { return this._replica.TransactionalReplicator; }
        }

        IAsyncEnumerator<IReliableState> IAsyncEnumerable<IReliableState>.GetAsyncEnumerator()
        {
            return this._replica.TransactionalReplicator.CreateAsyncEnumerable(true, false).GetAsyncEnumerator();
        }

        ITransaction IReliableStateManager.CreateTransaction()
        {
            return this.Replicator.CreateTransaction();
        }

        async Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, Uri name, TimeSpan timeout)
        {
            var result = await this.Replicator.GetOrAddStateProviderAsync(
                (Transaction) tx,
                name,
                (Uri n) =>
                {
                    var passedType = typeof(T);
                    var needToWrap = !passedType.GetTypeInfo().IsClass;
                    var concreteType = needToWrap
                        ? this._typeCache.GetConcreteType(passedType)
                        : passedType;

                    return (IStateProvider2) Activator.CreateInstance(concreteType);
                },
                timeout,
                CancellationToken.None).ConfigureAwait(false);

            // todo: assert result.Value != null

            try
            {
                return (T) result.Value;
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException(
                    String.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_ReliableObject_CannotCast,
                        result.Value.GetType(),
                        typeof(T)));
            }
        }

        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, Uri name)
        {
            return ((IReliableStateManager) this).GetOrAddAsync<T>(tx, name, DefaultTimeout);
        }

        async Task<T> IReliableStateManager.GetOrAddAsync<T>(Uri name, TimeSpan timeout)
        {
            try
            {
                using (var tx = ((IReliableStateManager) this).CreateTransaction())
                {
                    var result = await ((IReliableStateManager) this).GetOrAddAsync<T>(tx, name, timeout).ConfigureAwait(false);
                    await tx.CommitAsync().ConfigureAwait(false);
                    return result;
                }
            }
            catch (Exception e)
            {
                DataImplTrace.Source.WriteWarningWithId(TraceType, this._traceId, "GetOrAddAsync exception: {0}", e.ToString());
                throw;
            }
        }

        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, string name, TimeSpan timeout)
        {
            return ((IReliableStateManager) this).GetOrAddAsync<T>(tx, this.GetUriName(name), timeout);
        }

        Task<T> IReliableStateManager.GetOrAddAsync<T>(ITransaction tx, string name)
        {
            return ((IReliableStateManager) this).GetOrAddAsync<T>(tx, this.GetUriName(name));
        }

        Task<T> IReliableStateManager.GetOrAddAsync<T>(string name, TimeSpan timeout)
        {
            return ((IReliableStateManager) this).GetOrAddAsync<T>(this.GetUriName(name), timeout);
        }

        Task<T> IReliableStateManager.GetOrAddAsync<T>(Uri name)
        {
            return ((IReliableStateManager) this).GetOrAddAsync<T>(name, DefaultTimeout);
        }

        Task<T> IReliableStateManager.GetOrAddAsync<T>(string name)
        {
            return ((IReliableStateManager) this).GetOrAddAsync<T>(this.GetUriName(name));
        }

        Task IReliableStateManager.RemoveAsync(ITransaction tx, Uri name, TimeSpan timeout)
        {
            return this.Replicator.RemoveStateProviderAsync((Transaction) tx, name, timeout, CancellationToken.None);
        }

        async Task IReliableStateManager.RemoveAsync(Uri name, TimeSpan timeout)
        {
            using (var tx = ((IReliableStateManager) this).CreateTransaction())
            {
                await ((IReliableStateManager) this).RemoveAsync(tx, name, timeout).ConfigureAwait(false);
                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        Task IReliableStateManager.RemoveAsync(Uri name)
        {
            return ((IReliableStateManager) this).RemoveAsync(name, DefaultTimeout);
        }

        Task IReliableStateManager.RemoveAsync(ITransaction tx, string name, TimeSpan timeout)
        {
            return ((IReliableStateManager) this).RemoveAsync(tx, this.GetUriName(name), timeout);
        }

        Task IReliableStateManager.RemoveAsync(string name, TimeSpan timeout)
        {
            return ((IReliableStateManager) this).RemoveAsync(this.GetUriName(name), timeout);
        }

        Task IReliableStateManager.RemoveAsync(string name)
        {
            return ((IReliableStateManager) this).RemoveAsync(this.GetUriName(name));
        }

        Task IReliableStateManager.RemoveAsync(ITransaction tx, Uri name)
        {
            return ((IReliableStateManager) this).RemoveAsync(tx, name, DefaultTimeout);
        }

        Task IReliableStateManager.RemoveAsync(ITransaction tx, string name)
        {
            return ((IReliableStateManager) this).RemoveAsync(tx, this.GetUriName(name));
        }

        Task<ConditionalValue<T>> IReliableStateManager.TryGetAsync<T>(Uri name)
        {
            // Check if the replicator has the underlying state provider
            var result = this.Replicator.TryGetStateProvider(name);
            if (!result.HasValue)
            {
                return Task.FromResult(new ConditionalValue<T>());
            }

            try
            {
                return Task.FromResult(new ConditionalValue<T>(true, (T) result.Value));
            }
            catch (InvalidCastException)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture, 
                        SR.Error_ReliableObject_CannotCast,
                        result.Value.GetType(),
                        typeof(T)));
            }
        }

        Task<ConditionalValue<T>> IReliableStateManager.TryGetAsync<T>(string name)
        {
            return ((IReliableStateManager) this).TryGetAsync<T>(this.GetUriName(name));
        }

        /// <summary>
        /// Called when the system suspects that the partition lost data.
        /// <see cref="IStateProviderReplica.RestoreAsync(string)"/> or
        /// <see cref="IStateProviderReplica.RestoreAsync(string, RestorePolicy, CancellationToken)"/> may be used here to restore state from backup.
        /// </summary>
        /// <returns>
        /// Task that represents the asynchronous operation.
        /// The boolean task result indicates whether or not state was restored, e.g. from a backup.
        /// True indicates that state was restored from an external source and false indicates that the state has not been changed.
        /// </returns>
        /// <remarks>
        /// Base implementation returns false from OnDataLoss indicating that state of the replica has not been modified during OnDataLossAsync call.
        /// In other words, replica has not been restored.
        ///
        /// If true is returned, rest of the replicas in the partition will be rebuilt by the Primary replica.
        ///
        /// During OnDataLossAsync, replica will not have read or write status granted.
        /// </remarks>
        Func<CancellationToken, Task<bool>> IStateProviderReplica.OnDataLossAsync
        {
            set { this.userOnDataLossAsyncFunc = value; }
        }

        Func<CancellationToken, Task> IStateProviderReplica2.OnRestoreCompletedAsync
        {
            set { this.userOnRestoreCompletedAsyncFunc = value; }
        }

        void IStateProviderReplica.Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            if(this._backupRestoreManager!= null)
            {
                this._backupRestoreManager.Initialize(initializationParameters);
            }

            ((IStateProviderReplica) this._replica).Initialize(initializationParameters);
        }

        async Task<IReplicator> IStateProviderReplica.OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            var replicator = await ((IStateProviderReplica) this._replica).OpenAsync(openMode, partition, cancellationToken);

            if (this._backupRestoreManager != null)
            {
                await this._backupRestoreManager.OpenAsync(openMode, partition, cancellationToken);
            }

            return replicator;
        }

        async Task IStateProviderReplica.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            await ((IStateProviderReplica) this._replica).ChangeRoleAsync(newRole, cancellationToken);

            if(this._backupRestoreManager != null)
            {
                await this._backupRestoreManager.ChangeRoleAsync(newRole, cancellationToken);
            }
        }

        async Task IStateProviderReplica.CloseAsync(CancellationToken cancellationToken)
        {
            if (this._backupRestoreManager != null)
            {
                await this._backupRestoreManager.CloseAsync(cancellationToken);
            }

            await ((IStateProviderReplica) this._replica).CloseAsync(cancellationToken);
        }

        void IStateProviderReplica.Abort()
        {
            if(this._backupRestoreManager != null)
            {
                this._backupRestoreManager.Abort();
            }

            ((IStateProviderReplica) this._replica).Abort();
        }

        /// <summary>
        /// Convert a user-supplied string name to a URI suitable to be passed to the TransactionalReplicator as a state provider name
        /// </summary>
        /// <param name="name">arbitrary string name</param>
        /// <returns>Uri either composed of an arbitrary Uri prefix (urn:) and the user-supplied string name
        /// </returns>
        internal Uri GetUriName(string name)
        {
            if (name.StartsWith(ArbitraryUriPrefix))
            {
                throw new FormatException(SR.Error_Name_PrefixReserved);
            }

            string escaped;
            try
            {
                escaped = Uri.EscapeDataString(name);
            }
            catch (Exception e)
            {
                DataImplTrace.Source.WriteWarningWithId(TraceType, this._traceId, "Failed to escape string name: {0} | Exception: {1}", name, e.ToString());
                throw;
            }

            var combined = ArbitraryUriPrefix + escaped;
            try
            {
                return new Uri(combined);
            }
            catch (Exception e)
            {
                DataImplTrace.Source.WriteWarningWithId(
                    TraceType,
                    this._traceId,
                    "Combining the standard URI prefix and the escaped string name failed to yield a valid URI: {0} | Exception: {1}",
                    combined,
                    e.ToString());
                throw;
            }
        }

        // TODO: For now only exposing TryAdd. Get and Remove can be exposed later.

        bool IReliableStateManager.TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer)
        {
            return this.Replicator.TryAddStateSerializer(stateSerializer);
        }

        /// <summary>
        /// Adds a state serializer.
        /// Adds it for the specific reliable collection instance.
        /// </summary>
        /// <typeparam name="T">Type that will be serialized and deserialized.</typeparam>
        /// <param name="name">Reliable collection name.</param>
        /// <param name="stateSerializer">
        /// The state serializer to be added.
        /// </param>
        /// <returns>
        /// True if the serializer was added.
        /// False if a serializer is already registered.
        /// </returns>
        /// <remarks>
        /// This method can only be called in InitializeStateSerializers.
        /// Instance specific state serializers always take precedence.
        /// Temporarily deprecated in favor of stable public TryAddStateSerializer
        /// </remarks> 
        internal bool TryAddStateSerializer<T>(Uri name, IStateSerializer<T> stateSerializer)
        {
            return this.Replicator.TryAddStateSerializer(name, stateSerializer);
        }

        /// <summary>
        /// Adds a state serializer.
        /// Adds it for the specific reliable collection instance.
        /// </summary>
        /// <typeparam name="T">Type that will be serialized and deserialized.</typeparam>
        /// <param name="name">Reliable collection name.</param>
        /// <param name="stateSerializer">
        /// The state serializer to be added.
        /// </param>
        /// <returns>
        /// True if the serializer was added.
        /// False if a serializer is already registered.
        /// </returns>
        /// <remarks>
        /// This method can only be called in InitializeStateSerializers.
        /// Instance specific state serializers always take precedence.
        /// Temporarily deprecated in favor of stable public TryAddStateSerializer
        /// </remarks> 
        internal bool TryAddStateSerializer<T>(string name, IStateSerializer<T> stateSerializer)
        {
            var uriName = this.GetUriName(name);

            return this.Replicator.TryAddStateSerializer(uriName, stateSerializer);
        }

        /// <summary>
        /// Performs a full backup of all reliable state managed by this <see cref="IStateProviderReplica"/>.
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
            return this.Replicator.BackupAsync(backupCallback);
        }

        /// <summary>
        /// Performs a backup of all reliable state managed by this <see cref="IStateProviderReplica"/>.
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
        public Task BackupAsync(
            BackupOption option,
            TimeSpan timeout,
            CancellationToken cancellationToken,
            Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            return this.Replicator.BackupAsync(backupCallback, option, timeout, cancellationToken);
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
            return this.Replicator.RestoreAsync(backupFolderPath);
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
        public Task RestoreAsync(
            string backupFolderPath,
            RestorePolicy restorePolicy,
            CancellationToken cancellationToken)
        {
            return this.Replicator.RestoreAsync(backupFolderPath, restorePolicy, cancellationToken);
        }

        private static string GetTraceIdForReplica(ServiceContext serviceContext)
        {
            var partitionId = serviceContext.PartitionId;
            var replicaId = serviceContext.ReplicaOrInstanceId;
            return String.Concat(
                partitionId.ToString("B"),
                ":",
                replicaId.ToString(CultureInfo.InvariantCulture));
        }

        private async Task<bool> OnDataLossAsyncInternal(CancellationToken cancellationToken)
        {
            var result = false;

            if (this._backupRestoreManager != null)
            {
                result = await this._backupRestoreManager.OnDataLossAsync(cancellationToken);
            }

            if (result && this.userOnRestoreCompletedAsyncFunc != null)
            {
                await this.userOnRestoreCompletedAsyncFunc(cancellationToken);
            }

            if (!result && this.userOnDataLossAsyncFunc != null)
            {
                DataImplTrace.Source.WriteInfoWithId(TraceType, this._traceId, "OnDataLossAsyncInternal, falling back to invoking user override");
                return await this.userOnDataLossAsyncFunc.Invoke(cancellationToken).ConfigureAwait(false);
            }

            return result;
        }

        #region IBackupRestoreReplica

        Task IBackupRestoreReplica.BackupAsync(System.Fabric.BackupRestore.BackupOption backupOption, Func<System.Fabric.BackupRestore.BackupInfo, CancellationToken, Task<bool>> backupCallback, CancellationToken cancellationToken)
        {
            BackupOption option;

            if (backupOption == System.Fabric.BackupRestore.BackupOption.Full)
            {
                option = BackupOption.Full;
            }
            else
            {
                option = BackupOption.Incremental;
            }

            DataImplTrace.Source.WriteInfo(TraceType, "Triggering backup of type {0}", option);
            return this.BackupAsync(option, TimeSpan.FromHours(1), cancellationToken, (info, token) => BackupCallback(info, backupCallback, token));
        }

        private Task<bool> BackupCallback(BackupInfo backupInfo, Func<System.Fabric.BackupRestore.BackupInfo, CancellationToken, Task<bool>> backupCallback, CancellationToken cancellationToken)
        {
            var info = new System.Fabric.BackupRestore.BackupInfo
            {
                Directory = backupInfo.Directory,
                Option =
                    backupInfo.Option == BackupOption.Full
                        ? System.Fabric.BackupRestore.BackupOption.Full
                        : System.Fabric.BackupRestore.BackupOption.Incremental,
                BackupId = backupInfo.BackupId,
                ParentBackupId = backupInfo.ParentBackupId,
                LastBackupVersion = new BackupVersion(backupInfo.Version.Epoch, backupInfo.Version.Lsn),
                IndexBackupVersion = new BackupVersion(backupInfo.StartBackupVersion.Epoch, backupInfo.StartBackupVersion.Lsn),
                BackupIndex = -1,
                BackupChainId = Guid.Empty,
            };

            return backupCallback(info, cancellationToken);
        }

        Task IBackupRestoreReplica.RestoreAsync(string backupFolderPath, bool forceRestore, CancellationToken cancellationToken)
        {
            return this.RestoreAsync(backupFolderPath, forceRestore ? RestorePolicy.Force : RestorePolicy.Safe,
                cancellationToken);
        }

        async Task<byte[]> IBackupRestoreReplica.GetBackupMetadataAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            var dictionary = await ((IReliableStateManager) this).TryGetAsync<IReliableDictionary<string, byte[]>>(this.BackupRestoreInternalStore);
            if (!dictionary.HasValue) return null;

            using (var tx = ((IReliableStateManager) this).CreateTransaction())
            {
                var data = await dictionary.Value.TryGetValueAsync(tx, "metadata", timeout, cancellationToken);
                Debug.Assert(data.HasValue, "If the dictionary exists, expect the metadata to exist as well");
                return data.HasValue ? data.Value : null;
            }
        }

        async Task IBackupRestoreReplica.SaveBackupMetadataAsync(byte[] metadata, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var dictionary = await((IReliableStateManager)this).GetOrAddAsync<IReliableDictionary<string, byte[]>>(this.BackupRestoreInternalStore);
            using (var tx = ((IReliableStateManager)this).CreateTransaction())
            {
                await dictionary.AddOrUpdateAsync(tx, "metadata", metadata, (key, value) => metadata, timeout, cancellationToken);
                await tx.CommitAsync();
            }
        }

        async Task IBackupRestoreReplica.ClearBackupMetadataAsync(TimeSpan timeout)
        {
            try
            {
                await ((IReliableStateManager) this).RemoveAsync(this.BackupRestoreInternalStore, timeout);
            }
            catch (ArgumentException)
            {
                // The reliable state doesnt exist.. 
                // Ignore
            }
        }

        #endregion
    }
}