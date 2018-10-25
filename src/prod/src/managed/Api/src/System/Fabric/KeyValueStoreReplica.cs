// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Globalization;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Threading;
    using System.Threading.Tasks;
    using BOOLEAN = System.SByte;
    using System.Reflection;
    using System.Fabric.BackupRestore;
    using System.IO;

    /// <summary>
    /// <para>Provides a transactional, replicated, associative data storage component to service writers - ready for integration into any Service Fabric service.</para>
    /// This is used by legacy Service Fabric services. All new services should use the <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-reliable-collections">Reliable Collections</see>.
    /// </summary>
    public class KeyValueStoreReplica : IStatefulServiceReplica, IBackupRestoreReplica
    {
        /// <summary>
        /// <para>Specifies the behavior of <see cref="System.Fabric.KeyValueStoreReplica.OnCopyComplete(System.Fabric.KeyValueStoreEnumerator)" /> and <see cref="System.Fabric.KeyValueStoreReplica.OnReplicationOperation(IEnumerator{KeyValueStoreNotification})" /> for replicas in the secondary role.</para>
        /// </summary>
        public enum SecondaryNotificationMode
        {
            /// <summary>
            /// <para>An invalid secondary notification mode. Reserved for future use.</para>
            /// </summary>
            Invalid = NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_INVALID,

            /// <summary>
            /// <para>Secondary notifications are disabled. Neither <see cref="System.Fabric.KeyValueStoreReplica.OnCopyComplete(System.Fabric.KeyValueStoreEnumerator)" /> nor <see cref="System.Fabric.KeyValueStoreReplica.OnReplicationOperation(IEnumerator{KeyValueStoreNotification})" /> will be invoked.</para>
            /// </summary>
            None = NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_NONE,

            /// <summary>
            /// <para>The secondary replica may have already applied and acknowledged replication operations when the <see cref="System.Fabric.KeyValueStoreReplica.OnReplicationOperation(IEnumerator{KeyValueStoreNotification})" /> callback method is invoked. Operations are guaranteed to have been acked by a quorum of replicas by the time the callback is invoked.</para>
            /// </summary>
            NonBlockingQuorumAcked = NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_NON_BLOCKING_QUORUM_ACKED,

            /// <summary>
            /// <para>The secondary replica will not apply or acknowledge replication operations until the <see cref="System.Fabric.KeyValueStoreReplica.OnReplicationOperation(IEnumerator{KeyValueStoreNotification})" /> callback method returns. Operations are not guaranteed to have been acked by a quorum of replicas at the time the callback is invoked.</para>
            /// </summary>
            BlockSecondaryAck = NativeTypes.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE.FABRIC_KEY_VALUE_STORE_NOTIFICATION_MODE_BLOCK_SECONDARY_ACK,
        }

        /// <summary>
        /// <para>Specifies the behavior to use when building new secondary replicas (full copy).</para>
        /// </summary>
        public enum FullCopyMode
        {
            /// <summary>
            /// <para>The full copy mode specified in the cluster manifest will be used.</para>
            /// </summary>
            Default = NativeTypes.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_DEFAULT,

            /// <summary>
            /// <para>Full copies will be performed by taking a backup of the primary replica database and sending the physical database files to secondary replicas for restoration. This is the recommended and default mode.</para>
            /// </summary>
            Physical = NativeTypes.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_PHYSICAL,

            /// <summary>
            /// <para>Full copies will be performed by reading all database contents and sending them to secondary replicas for replay against their own databases. Since this mode requires opening a long-running transaction on the primary for the duration of the build, it's only recommended for small databases or services with low write activity. This mode enables changing database parameters that are normally fixed after initialization such as <see cref="System.Fabric.LocalEseStoreSettings.DatabasePageSizeInKB" /> and <see cref="System.Fabric.LocalEseStoreSettings.LogFileSizeInKB" />.</para>
            /// </summary>
            Logical = NativeTypes.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_LOGICAL,
            
            /// <summary>
            /// <para>Full copies will be performed as a physical copy, but with an additional step of replaying all database contents in primary index order to a new database on the secondary. This mode also enables changing database parameters that are normally fixed after initialization, but will take longer than either physical or logical build due to the extra replay step. After replay, the final data layout is optimal since insertion occurred in primary index order. Currently not supported in Linux.</para>
            /// </summary>
            Rebuild = NativeTypes.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE.FABRIC_KEY_VALUE_STORE_FULL_COPY_MODE_REBUILD,
        }

        /// <summary>
        /// <para>Indicates that sequence number checking should not occur.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        /// Can be used in APIs accepting a check sequence number parameter to indicate that sequence number checking
        /// should not occur:
        /// <list type="bullet">
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Update(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Remove(TransactionBase, string, long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryUpdate(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryRemove(TransactionBase, string, long)" /></description></item>
        /// </list>
        ///
        /// This is equivalent to calling API overloads that do not have a check sequence number parameter:
        /// <list type="bullet">
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Update(TransactionBase, string, byte[])" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Remove(TransactionBase, string)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryUpdate(TransactionBase, string, byte[])" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryRemove(TransactionBase, string)" /></description></item>
        /// </list>
        /// </para>
        /// </remarks>
        public const long IgnoreSequenceNumberCheck = NativeTypes.FABRIC_IGNORE_SEQUENCE_NUMBER_CHECK;

        private NativeRuntime.IFabricKeyValueStoreReplica6 nativeStore;

        private readonly IBackupRestoreManager backupRestoreManager;

        private const string BackupMetadataKeyName = "urn:c0e554a9-5936-4655-b175-46b6f969549f:BackupRestoreStore";
        private const string LocalBackupFolderName = "B";
        private const string BackupRootFolderPrefix = "kvsbrs_";
        private StatefulServiceInitializationParameters initializationParams;

        private State state;

        private ReplicaRole storeRole;

        private long instanceTimestamp;

        #region Constructors

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.KeyValueStoreReplica" /> class with the specified key/value store name.</para>
        /// </summary>
        /// <param name="storeName">
        /// <para>The name of the key/value store.</para>
        /// </param>
        /// <remarks>
        /// <para>The store name should conform to valid filename characters.</para>
        /// </remarks>
        public KeyValueStoreReplica(string storeName)
            : this(storeName, new LocalEseStoreSettings(), new ReplicatorSettings(), SecondaryNotificationMode.None)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.KeyValueStoreReplica" /> class with the specified key/value store name and local store settings.</para>
        /// </summary>
        /// <param name="storeName">
        /// <para>The name of the key/value store.</para>
        /// </param>
        /// <param name="localStoreSettings">
        /// <para>The optional settings for the local store.</para>
        /// </param>
        public KeyValueStoreReplica(
            string storeName,
            LocalStoreSettings localStoreSettings)
            : this(storeName, localStoreSettings, new ReplicatorSettings(), SecondaryNotificationMode.None)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.KeyValueStoreReplica" /> class with the specified key/value store name and store replicator settings.</para>
        /// </summary>
        /// <param name="storeName">
        /// <para>The name of the key/value store.</para>
        /// </param>
        /// <param name="replicatorSettings">
        /// <para>The option settings for the key/value store replicator.</para>
        /// </param>
        /// <remarks>
        /// <para>The store name should conform to valid filename characters.</para>
        /// </remarks>
        public KeyValueStoreReplica(
            string storeName,
            ReplicatorSettings replicatorSettings)
            : this(storeName, new LocalEseStoreSettings(), replicatorSettings, SecondaryNotificationMode.None)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.KeyValueStoreReplica" /> class with the specified key/value store name, local store settings, and replicator settings.</para>
        /// </summary>
        /// <param name="storeName">
        /// <para>The name of the key/value store.</para>
        /// </param>
        /// <param name="localStoreSettings">
        /// <para>The optional settings for the local store.</para>
        /// </param>
        /// <param name="replicatorSettings">
        /// <para>The option settings for the key/value store replicator.</para>
        /// </param>
        public KeyValueStoreReplica(
            string storeName,
            LocalStoreSettings localStoreSettings,
            ReplicatorSettings replicatorSettings)
            : this(storeName, localStoreSettings, replicatorSettings, SecondaryNotificationMode.None)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.KeyValueStoreReplica" /> class with the specified key/value store name, local store settings, and replicator settings. Secondary replica notifications are enabled via the notification mode.</para>
        /// </summary>
        /// <param name="storeName">
        /// <para>The name of the key/value store.</para>
        /// </param>
        /// <param name="localStoreSettings">
        /// <para>The optional settings for the local store.</para>
        /// </param>
        /// <param name="replicatorSettings">
        /// <para>The option settings for the key/value store replicator.</para>
        /// </param>
        /// <param name="notificationMode">
        /// <para>The secondary notification mode to enable <see cref="System.Fabric.KeyValueStoreReplica.OnCopyComplete(System.Fabric.KeyValueStoreEnumerator)" /> 
        /// and <see cref="System.Fabric.KeyValueStoreReplica.OnReplicationOperation(System.Collections.Generic.IEnumerator{System.Fabric.KeyValueStoreNotification})" /> 
        /// callbacks on this replica.</para>
        /// </param>
        public KeyValueStoreReplica(
            string storeName,
            LocalStoreSettings localStoreSettings,
            ReplicatorSettings replicatorSettings,
            SecondaryNotificationMode notificationMode)
            : this(
                storeName, 
                localStoreSettings, 
                replicatorSettings, 
                new KeyValueStoreReplicaSettings() { SecondaryNotificationMode = notificationMode })
        {
        }

        /// <summary>
        /// Initializes a new instance of the KeyValueStoreReplica class with the specified key/value store name, local store settings, replicator settings, and replica settings.
        /// </summary>
        /// <param name="storeName">The name of the key/value store.</param>
        /// <param name="localStoreSettings">The optional settings for the local store.</param>
        /// <param name="replicatorSettings">The optional settings for the key/value store replicator.</param>
        /// <param name="kvsSettings">The optional settings for the key/value store replica.</param>
        public KeyValueStoreReplica(
            string storeName,
            LocalStoreSettings localStoreSettings,
            ReplicatorSettings replicatorSettings,
            KeyValueStoreReplicaSettings kvsSettings)
        {
            Requires.Argument<string>("storeName", storeName).NotNullOrEmpty();
            Requires.Argument<LocalStoreSettings>("localStoreSettings", localStoreSettings).NotNull();
            Requires.Argument<ReplicatorSettings>("replicatorSettings", replicatorSettings).NotNull();
            Requires.Argument<KeyValueStoreReplicaSettings>("kvsSettings", kvsSettings).NotNull();

            if (kvsSettings.SecondaryNotificationMode == SecondaryNotificationMode.Invalid)
            {
                throw new ArgumentException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Invalid_Notification_Mode), "notificationMode");
            }

            this.state = State.Created;
            this.StoreName = storeName;
            this.LocalStoreSettings = localStoreSettings;
            this.ReplicatorSettings = replicatorSettings;
            this.KeyValueStoreReplicaSettings = kvsSettings;
            this.storeRole = ReplicaRole.Unknown;
            this.instanceTimestamp = DateTime.UtcNow.Ticks;
#if !DotNetCoreClrLinux
            this.backupRestoreManager = BackupRestoreManagerFactory.GetBackupRestoreManager(this);
#else
            this.backupRestoreManager = null;
#endif
        }

        #endregion

        private enum State
        {
            Created,
            Initialized,
            Opened,
            ClosedOrAborted,
        }

        /// <summary>
        /// Handler for data loss events.
        /// </summary>
        public event EventHandler DataLossReported;

        /// <summary>
        /// <para>Signals that the replica set may have experienced data loss. The application can either override this method or listen for the
        /// <see cref="System.Fabric.KeyValueStoreReplica.DataLossReported" /> Event. Both represent the same event</para>
        /// </summary>
        /// <param name="args">
        /// <para>Currently contains no data. Reserved for future use.</para>
        /// </param>
        protected virtual void OnDatalossReported(EventArgs args)
        {
            EventHandler handler = DataLossReported;
            if (handler != null)
            {
                handler(this, args);
            }
        }

        /// <summary>
        /// Signals that the replica set may have experienced data loss. The application can either override this method to process the event asynchronously 
        /// or use the <see cref="System.Fabric.KeyValueStoreReplica.DataLossReported" /> event to process synchronously. Both represent the same event.
        /// </summary>
        /// <param name="cancellationToken">The token used to check for cancellation of the operation.</param>
        /// <returns>True to indicate that data was modified during recovery and the replica set needs to be resynchronized. Otherwise, false to indicate that data has not been modified.</returns>
        protected virtual Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(false);
        }

        /// <summary>
        /// Signals that the replica's state was successfully restored by the system.
        /// This is invoked only when system internally triggers a restore via the Backup Restore service.
        /// </summary>
        /// <param name="cancellationToken">The token used to check for cancellation of the operation.</param>
        protected virtual Task OnRestoreCompletedAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        /// <summary>
        /// <para>Gets or sets the name of the key/value store.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the key/value store.</para>
        /// </value>
        /// <remarks>
        /// <para>The store name should conform to valid filename characters.</para>
        /// </remarks>
        public string StoreName { get; private set; }

        /// <summary>
        /// <para>Gets or sets the option settings for the local key/value store.</para>
        /// </summary>
        /// <value>
        /// <para>The local store option settings.</para>
        /// </value>
        public LocalStoreSettings LocalStoreSettings { get; private set; }

        /// <summary>
        /// <para>Gets or sets the option settings for the key/value store replicator.</para>
        /// </summary>
        /// <value>
        /// <para>The store replicator option settings.</para>
        /// </value>
        public ReplicatorSettings ReplicatorSettings { get; private set; }

        /// <summary>
        /// <para>Gets or sets the option settings for the <see cref="System.Fabric.KeyValueStoreReplica"/>.</para>
        /// </summary>
        /// /// <value>
        /// <para>The <see cref="System.Fabric.KeyValueStoreReplica"/> option settings.</para>
        /// </value>
        public KeyValueStoreReplicaSettings KeyValueStoreReplicaSettings { get; private set; }

        /// <summary>
        /// <para>Gets the secondary notification mode specified during construction of this replica.</para>
        /// </summary>
        /// <value>
        /// <para>The current secondary notification mode</para>
        /// </value>
        public virtual SecondaryNotificationMode NotificationMode 
        { 
            get 
            { 
                return this.KeyValueStoreReplicaSettings.SecondaryNotificationMode;
            } 
        }

        internal Guid PartitionId { get; private set; }

        internal long ReplicaId { get; private set; }
        
        internal NativeRuntimeInternal.IFabricInternalStatefulServiceReplica InternalStore { get { return this.nativeStore as NativeRuntimeInternal.IFabricInternalStatefulServiceReplica; } }

        private string GetTraceId()
        {
            return string.Format(
                CultureInfo.InvariantCulture, 
                "{0}:{1}:{2}", 
                this.PartitionId, 
                this.ReplicaId, 
                this.instanceTimestamp);
        }

        private string GetOperationId(string key)
        {
            return string.Format(
                CultureInfo.InvariantCulture, 
                "{0}:key=\"{1}\"", 
                this.GetTraceId(),
                key);
        }

        #region IStatefulServiceReplica Methods

        /// <summary>
        /// <para>Initializes the replica in preparation for opening.</para>
        /// </summary>
        /// <param name="initializationParameters">
        /// <para>The initialization information for the replica.</para>
        /// </param>
        /// <remarks>
        /// <para>This method does not need to be called explicitly if the application replica derives from <see cref="System.Fabric.KeyValueStoreReplica" />,
        /// which is the recommended pattern. In this case, the application replica should override
        /// <see cref="System.Fabric.KeyValueStoreReplica.OnInitialize(System.Fabric.StatefulServiceInitializationParameters)" /> instead.</para>
        /// </remarks>
        public void Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            if (this.state == State.Initialized) { return; }

            this.ThrowIfNotCreated();
            this.initializationParams = initializationParameters;
            this.ReplicaId = initializationParameters.ReplicaId;
            
            this.Service_Initialize(initializationParameters);
            this.Store_Initialize(initializationParameters);
            this.BackupRestoreManager_Initialize(initializationParameters);
            this.state = State.Initialized;
        }

        /// <summary>
        /// <para>Opens the replica and its replicator in preparation for coming online in a replica set.</para>
        /// </summary>
        /// <param name="openMode">
        /// <para>Specifies the context under which this replica is being opened.</para>
        /// </param>
        /// <param name="partition">
        /// <para>Contains information describing the replica set to which this replica belongs.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>Currently unused. Reserved for future use.</para>
        /// </param>
        /// <returns>
        /// <para>A Task to indicate completion of the open <see cref="System.Threading.Tasks.Task{T}" />.</para>
        /// </returns>
        /// <remarks>
        /// <para>This method does not need to be called explicitly if the application replica derives from <see cref="System.Fabric.KeyValueStoreReplica" />, 
        /// which is the recommended pattern. In this case, the application replica should override OnOpenAsync instead.</para>
        /// </remarks>
        public async Task<IReplicator> OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            this.ThrowIfNotInitialized();

            this.PartitionId = partition.PartitionInfo.Id;

            Exception openException = null;
            IReplicator replicator = null;

            try
            {
                replicator = await this.Store_OpenAsync(openMode, partition, cancellationToken);
            }
            catch (OperationCanceledException e)
            {
                // This occurs when RA aborts a replica open, which propagates to ComAsyncOperationContext
                // and completes it with E_ABORT. However, the store may have already finished opening,
                // so we must make sure to cleanup.
                //
                openException = e;
            }

            if (openException == null)
            {
                try
                {
                    await this.Service_OpenAsync(openMode, partition, cancellationToken);

                    this.storeRole = ReplicaRole.Unknown;
                }
                catch (Exception e)
                {
                    openException = e;
                }
            }

            if (openException != null)
            {
                // Must cleanup store even if task is cancelled or a subsequent re-open
                // will fail at the store level
                //
                await this.Store_CloseAsync(CancellationToken.None);

                throw openException;
            }

            await this.BackupRestoreManager_OpenAsync(openMode, partition, cancellationToken);      // This doesnt throw exception

            this.state = State.Opened;
            return replicator;
        }

        /// <summary>
        /// <para>Changes the replica role of the replica and its replicator.</para>
        /// </summary>
        /// <param name="newRole">
        /// <para>The target replica role.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>Currently unused. Reserved for future use.</para>
        /// </param>
        /// <returns>
        /// <para>A task whose result is the address of this replica.</para>
        /// </returns>
        /// <remarks>
        /// <para>This method does not need to be called explicitly if the application replica derives from <see cref="System.Fabric.KeyValueStoreReplica" />, 
        /// which is the recommended pattern. In this case, the application replica should override <see cref="System.Fabric.KeyValueStoreReplica.OnChangeRoleAsync(System.Fabric.ReplicaRole,System.Threading.CancellationToken)" /> instead.</para>
        /// </remarks>
        public async Task<string> ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            this.ThrowIfNotOpened();

            if (newRole != this.storeRole)
            {
                await this.Store_ChangeRoleAsync(newRole, cancellationToken);

                this.storeRole = newRole;
            }

            var result = await this.Service_ChangeRoleAsync(newRole, cancellationToken);
            await this.BackupRestoreManager_ChangeRoleAsync(newRole, cancellationToken);
            return result;
        }

        /// <summary>
        /// <para>Closes the replica and its replicator in preparation for going offline from a replica set.</para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>Currently unused. Reserved for future use.</para>
        /// </param>
        /// <returns>
        /// <para>A task that represents the asynchronous operation.</para>
        /// </returns>
        /// <remarks>
        /// <para>The replica has not necessarily been removed permanently from the replica set and may be re-opened at a later time. The most common causes for closing a replica is graceful shutdown in preparation for upgrade or load balancing. This method does not need to be called explicitly if the application replica derives from <see cref="System.Fabric.KeyValueStoreReplica" />, which is the recommended pattern. In this case, the application replica should override <see cref="System.Fabric.KeyValueStoreReplica.OnCloseAsync(System.Threading.CancellationToken)" /> instead.</para>
        /// </remarks>
        public async Task CloseAsync(CancellationToken cancellationToken)
        {
            this.ThrowIfNotOpened();

            Exception serviceException = null;

            try
            {
                await this.BackupRestoreManager_CloseAsync(cancellationToken);
            }
            catch (Exception)
            {
                // Ignore exception if any
            }

            try
            {
                await this.Service_CloseAsync(cancellationToken);

                this.storeRole = ReplicaRole.Unknown;
            }
            catch (Exception e)
            {
                serviceException = e;
            }

            await this.Store_CloseAsync(cancellationToken);

            if (serviceException != null)
            {
                throw serviceException;
            }

            this.state = State.ClosedOrAborted;
        }

        /// <summary>
        /// <para>Aborts this instance of the <see cref="System.Fabric.KeyValueStoreReplica" /> class.</para>
        /// </summary>
        public void Abort()
        {
            this.BackupRestoreManager_Abort();
            this.Service_Abort();
            this.Store_Abort();

            this.state = State.ClosedOrAborted;
        }

        #endregion

        #region KeyValueStoreReplica Methods

        /// <summary>
        /// <para>Gets the current epoch for the key/value store.</para>
        /// </summary>
        /// <returns>
        /// <para>The current epoch for the key/value store.</para>
        /// </returns>
        public Epoch GetCurrentEpoch()
        {
            this.ThrowIfNotOpened();

            return Utility.WrapNativeSyncInvoke(
                 () => this.GetCurrentEpochHelper(),
                 "KeyValueStoreReplica.GetEpoch");
        }

        /// <summary>
        /// <para>Updates the key/value store replicator with the settings in the specified <see cref="System.Fabric.ReplicatorSettings" /> object.</para>
        /// </summary>
        /// <param name="settings">
        /// <para>The settings used to update the key/value store replicator.</para>
        /// </param>
        public void UpdateReplicatorSettings(ReplicatorSettings settings)
        {
            Requires.Argument<ReplicatorSettings>("settings", settings).NotNull();

            if (this.nativeStore == null)
            {
                this.ReplicatorSettings = settings;
            }
            else
            {
                Utility.WrapNativeSyncInvoke(
                  () => this.UpdateReplicatorSettingsHelper(settings),
                  "KeyValueStoreReplica.UpdateReplicatorSettings");
                this.ReplicatorSettings = settings;
            }
        }

        /// <summary>
        /// <para>Creates a unique <see cref="System.Fabric.Transaction" /> instance, which is used to commit or rollback groups of key/value store operations.</para>
        /// </summary>
        /// <returns>
        /// <para>A <see cref="System.Fabric.Transaction" /> object representing a transaction.</para>
        /// </returns>
        public Transaction CreateTransaction()
        {
            return this.CreateTransaction(null);
        }

        /// <summary>
        /// <para>Creates a unique <see cref="System.Fabric.Transaction" /> instance, which is used to commit or rollback groups of key/value store operations.</para>
        /// </summary>
        /// <param name="settings">
        /// <para>The transaction settings.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Fabric.Transaction" /> object representing a transaction.</para>
        /// </returns>
        public Transaction CreateTransaction(KeyValueStoreTransactionSettings settings)
        {
            this.ThrowIfNotOpened();

            return Utility.WrapNativeSyncInvoke(
                  () => this.CreateTransactionHelper(settings),
                  "KeyValueStoreReplica.CreateTransaction");
        }

        #endregion

        #region KeyValueStoreReplica CRUD methods

        /// <summary>
        /// <para>Adds a value indexed by the specified key to the key/value store.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to be added (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value (as a byte array) to be stored, limited to 2GB in length.</para>
        /// </param>
        public void Add(TransactionBase transactionBase, string key, byte[] value)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();
            Requires.Argument<byte[]>("value", value).NotNullOrEmptyList();

            Utility.WrapNativeSyncInvoke(
                () => this.AddHelper(transactionBase, key, value),
                "KeyValueStoreReplica.Add",
                this.GetOperationId(key));
        }

        /// <summary>
        /// Attempts to add a value indexed by the specified key to the key/value store.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be added (as a string). Limited to 800 characters in length.</param>
        /// <param name="value">The value (as a byte array) to be stored, limited to 2GB in length.</param>
        /// <returns>True if the specified key was not already found and added. False if the specified key already exists.</returns>
        public bool TryAdd(TransactionBase transactionBase, string key, byte[] value)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();
            Requires.Argument<byte[]>("value", value).NotNullOrEmptyList();

            return Utility.WrapNativeSyncInvoke(
                () => this.TryAddHelper(transactionBase, key, value),
                "KeyValueStoreReplica.TryAdd",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Removes the value indexed by the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key, or index, of the value to be removed (as a string). Limited to 800 characters in length.</para>
        /// </param>
        public void Remove(TransactionBase transactionBase, string key)
        {
            this.Remove(transactionBase, key, KeyValueStoreReplica.IgnoreSequenceNumberCheck);
        }

        /// <summary>
        /// <para>Removes the value indexed by the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key, or index, of the value to be removed (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <param name="checkSequenceNumber">
        ///   <para>The expected sequence number of the key to be removed.</para>
        /// </param>
        public void Remove(TransactionBase transactionBase, string key, long checkSequenceNumber)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            Utility.WrapNativeSyncInvoke(
                () => this.RemoveHelper(transactionBase, key, checkSequenceNumber),
                "KeyValueStoreReplica.Remove",
                this.GetOperationId(key));
        }

        /// <summary>
        /// Attempts to remove the value indexed by the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be removed (as a string). Limited to 800 characters in length.</param>
        /// <returns>True if the specified key was found and removed. False if the specified key does not exist.</returns>
        public bool TryRemove(TransactionBase transactionBase, string key)
        {
            return this.TryRemove(transactionBase, key, KeyValueStoreReplica.IgnoreSequenceNumberCheck);
        }

        /// <summary>
        /// Attempts to remove the value indexed by the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be removed (as a string). Limited to 800 characters in length.</param>
        /// <param name="checkSequenceNumber">The expected sequence number of the key to be removed.</param>
        /// <returns>True if the specified key was found and removed. False if the specified key does not exist.</returns>
        public bool TryRemove(TransactionBase transactionBase, string key, long checkSequenceNumber)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.TryRemoveHelper(transactionBase, key, checkSequenceNumber),
                "KeyValueStoreReplica.TryRemove",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Updates the stored value associated with the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to be updated (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value (as a byte array) to be stored, limited to 2GB in length.</para>
        /// </param>
        public void Update(TransactionBase transactionBase, string key, byte[] value)
        {
            this.Update(transactionBase, key, value, IgnoreSequenceNumberCheck);
        }

        /// <summary>
        /// <para>Updates the value indexed by the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to be updated (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <param name="value">
        /// <para>The value (as a byte array) to be stored, limited to 2GB in length.</para>
        /// </param>
        /// <param name="checkSequenceNumber">
        ///   <para>The expected sequence number of the key to be updated.</para>
        /// </param>
        public void Update(TransactionBase transactionBase, string key, byte[] value, long checkSequenceNumber)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();
            Requires.Argument<byte[]>("value", value).NotNullOrEmptyList();

            Utility.WrapNativeSyncInvoke(
                () => this.UpdateHelper(transactionBase, key, value, checkSequenceNumber),
                "KeyValueStoreReplica.Update",
                this.GetOperationId(key));
        }

        /// <summary>
        /// Attempts to update the value indexed by the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be updated (as a string). Limited to 800 characters in length.</param>
        /// <param name="value">The value (as a byte array) to be stored, limited to 2GB in length.</param>
        /// <returns>True if the specified key was found and updated. False if the specified key does not exist.</returns>
        public bool TryUpdate(TransactionBase transactionBase, string key, byte[] value)
        {
            return this.TryUpdate(transactionBase, key, value, IgnoreSequenceNumberCheck);
        }

        /// <summary>
        /// Attempts to update the value indexed by the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be updated (as a string). Limited to 800 characters in length.</param>
        /// <param name="value">The value (as a byte array) to be stored, limited to 2GB in length.</param>
        /// <param name="checkSequenceNumber">The expected sequence number of the key to be updated.</param>
        /// <returns>True if the specified key was found and updated. False if the specified key does not exist.</returns>
        public bool TryUpdate(TransactionBase transactionBase, string key, byte[] value, long checkSequenceNumber)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();
            Requires.Argument<byte[]>("value", value).NotNullOrEmptyList();

            return Utility.WrapNativeSyncInvoke(
                () => this.TryUpdateHelper(transactionBase, key, value, checkSequenceNumber),
                "KeyValueStoreReplica.TryUpdate",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Determines whether a value is contained in the key/value store.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to look up (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value is contained in the key/value store; <languageKeyword>false</languageKeyword>, otherwise.</para>
        /// </returns>
        public bool Contains(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.ContainsHelper(transactionBase, key),
                "KeyValueStoreReplica.Contains",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Gets the stored value, as a <see cref="System.Fabric.KeyValueStoreItem" /> object, associated with the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to be retrieved (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItem" /> object representing the stored value.</para>
        /// </returns>
        public KeyValueStoreItem Get(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.GetHelper(transactionBase, key),
                "KeyValueStoreReplica.Get",
                this.GetOperationId(key));
        }

        /// <summary>
        /// Attempts to get the stored value, as a <see cref="System.Fabric.KeyValueStoreItem" /> object, associated with the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be retrieved (as a string). Limited to 800 characters in length.</param>
        /// <returns>A <see cref="System.Fabric.KeyValueStoreItem" /> object representing the stored value or null if the specified key does not exist.</returns>
        public KeyValueStoreItem TryGet(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.TryGetHelper(transactionBase, key),
                "KeyValueStoreReplica.TryGet",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Gets the stored value as a byte array, associated with the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to be retrieved (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <returns>
        /// <para>A byte array representing the stored value.</para>
        /// </returns>
        public byte[] GetValue(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.GetValueHelper(transactionBase, key),
                "KeyValueStoreReplica.GetValue",
                this.GetOperationId(key));
        }

        /// <summary>
        /// Attempts to get the stored value as a byte array, associated with the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be retrieved (as a string). Limited to 800 characters in length.</param>
        /// <returns>A byte array representing the stored value or null if the specified key does not exist.</returns>
        public byte[] TryGetValue(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.TryGetValueHelper(transactionBase, key),
                "KeyValueStoreReplica.TryGetValue",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Gets the metadata, as a <see cref="System.Fabric.KeyValueStoreItemMetadata" /> object, for the value associated with the specified key.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="key">
        /// <para>The key or index of the value to be retrieved (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItemMetadata" /> object representing the metadata associated with the specified value.</para>
        /// </returns>
        public KeyValueStoreItemMetadata GetMetadata(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.GetMetadataHelper(transactionBase, key),
                "KeyValueStoreReplica.GetMetadata",
                this.GetOperationId(key));
        }

        /// <summary>
        /// Attempts to get the metadata as a <see cref="System.Fabric.KeyValueStoreItemMetadata" /> object, for the value associated with the specified key.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="key">The key, or index, of the value to be retrieved (as a string). Limited to 800 characters in length.</param>
        /// <returns>A <see cref="System.Fabric.KeyValueStoreItemMetadata" /> object representing the metadata associated with the specified value or null if the specified key does not exist.</returns>
        public KeyValueStoreItemMetadata TryGetMetadata(TransactionBase transactionBase, string key)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("key", key).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.TryGetMetadataHelper(transactionBase, key),
                "KeyValueStoreReplica.TryGetMetadata",
                this.GetOperationId(key));
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through the <see cref="System.Fabric.KeyValueStoreItem" /> values in the key/value store.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <remarks>
        /// <para>
        /// The items are enumerated in lexicographically increasing order by key.
        /// </para>
        /// </remarks>
        /// <returns>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItem" /> enumerator.</para>
        /// </returns>
        /// <remarks>
        /// <para>
        /// The items are enumerated in lexicographically increasing order by key.
        /// </para>
        /// </remarks>
        public IEnumerator<KeyValueStoreItem> Enumerate(TransactionBase transactionBase)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();

            return Utility.WrapNativeSyncInvoke(
                () => this.EnumerateHelper(transactionBase),
                "KeyValueStoreReplica.Enumerate",
                this.GetTraceId());
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through the <see cref="System.Fabric.KeyValueStoreItem" /> values in the key/value store, where the value keys match the specified key prefix.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="keyPrefix">
        /// <para>The key, or index, prefix to match (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <remarks>
        /// <para>
        /// Equivalent to calling <see cref="System.Fabric.KeyValueStoreReplica.Enumerate(TransactionBase, string, bool)" /> with <b>strictPrefix</b> set to <languageKeyword>true</languageKeyword>.
        /// </para>
        /// <para>
        /// The items are enumerated in lexicographically increasing order by key.
        /// </para>
        /// </remarks>
        /// <returns>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItem" /> enumerator.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItem> Enumerate(TransactionBase transactionBase, string keyPrefix)
        {
            return this.Enumerate(transactionBase, keyPrefix, true); // strictPrefix
        }

        /// <summary>
        /// Returns an enumerator that iterates through the <see cref="System.Fabric.KeyValueStoreItem" /> values in the key/value store.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="keyPrefix">The key, or index, prefix to match (as a string). Limited to 800 characters in length.</param>
        /// <param name="strictPrefix">When true, only keys prefixed by the value specified for <b>keyPrefix</b> are returned. Otherwise, enumeration starts at the first key matching or lexicographically greater than <b>keyPrefix</b> and continues until there are no more keys. The default is <b>true</b>.</param>
        /// <remarks>
        /// <para>
        /// The items are enumerated in lexicographically increasing order by key.
        /// </para>
        /// </remarks>
        /// <returns>A <see cref="System.Fabric.KeyValueStoreItem" /> enumerator.</returns>
        /// <remarks>
        /// <para>
        /// The items are enumerated in lexicographically increasing order by key.
        /// </para>
        /// </remarks>
        public IEnumerator<KeyValueStoreItem> Enumerate(TransactionBase transactionBase, string keyPrefix, bool strictPrefix)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("keyPrefix", keyPrefix).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.EnumerateHelper(transactionBase, keyPrefix, strictPrefix),
                "KeyValueStoreReplica.Enumerate",
                this.GetOperationId(keyPrefix));
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through the <see cref="System.Fabric.KeyValueStoreItemMetadata" /> values in the key/value store.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItemMetadata" /> enumerator.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadata(TransactionBase transactionBase)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();

            return Utility.WrapNativeSyncInvoke(
                () => this.EnumerateMetadataHelper(transactionBase),
                "KeyValueStoreReplica.EnumerateMetadata",
                this.GetTraceId());
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through the <see cref="System.Fabric.KeyValueStoreItemMetadata" /> values in the key/value store, where the value keys match the specified key prefix.</para>
        /// </summary>
        /// <param name="transactionBase">
        /// <para>The transaction instance.</para>
        /// </param>
        /// <param name="keyPrefix">
        /// <para>The key, or index, prefix to match (as a string). Limited to 800 characters in length.</para>
        /// </param>
        /// <remarks>
        /// Equivalent to calling <see cref="System.Fabric.KeyValueStoreReplica.EnumerateMetadata(TransactionBase, string, bool)" /> with <b>strictPrefix</b> set to <languageKeyword>true</languageKeyword>.
        /// </remarks>
        /// <returns>
        /// <para>A <see cref="System.Fabric.KeyValueStoreItemMetadata" /> enumerator.</para>
        /// </returns>
        public IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadata(TransactionBase transactionBase, string keyPrefix)
        {
            return this.EnumerateMetadata(transactionBase, keyPrefix, true); // strictPrefix
        }

        /// <summary>
        /// Returns an enumerator that iterates through the <see cref="System.Fabric.KeyValueStoreItemMetadata" /> values in the key/value store.
        /// </summary>
        /// <param name="transactionBase">The transaction instance.</param>
        /// <param name="keyPrefix">The key, or index, prefix to match (as a string). Limited to 800 characters in length.</param>
        /// <param name="strictPrefix">When true, only keys prefixed by the value specified for <b>keyPrefix</b> are returned. Otherwise, enumeration starts at the first key matching or lexicographically greater than <b>keyPrefix</b> and continues until there are no more keys. The default is <b>true</b>.</param>
        /// <returns>A <see cref="System.Fabric.KeyValueStoreItemMetadata" /> enumerator.</returns>
        public IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadata(TransactionBase transactionBase, string keyPrefix, bool strictPrefix)
        {
            this.ThrowIfNotOpened();

            Requires.Argument<TransactionBase>("transactionBase", transactionBase).NotNull();
            Requires.Argument<string>("keyPrefix", keyPrefix).NotNullOrEmpty();

            return Utility.WrapNativeSyncInvoke(
                () => this.EnumerateMetadataHelper(transactionBase, keyPrefix, strictPrefix),
                "KeyValueStoreReplica.EnumerateMetadata",
                this.GetOperationId(keyPrefix));
        }

        /// <summary>
        /// <para>DEPRECATED. Performs a full backup of the replica's local store to the specified destination directory. </para>
        /// </summary>
        /// <param name="backupDirectory">
        /// <para>The full path of the backup destination directory.</para>
        /// </param>
        /// <remarks>
        /// <para>
        /// This method is obsolete. Use <see cref="System.Fabric.KeyValueStoreReplica.BackupAsync(string, System.Fabric.StoreBackupOption, Func{System.Fabric.StoreBackupInfo, System.Threading.Tasks.Task{bool}})"/> instead.</para>
        /// <para>
        /// Incremental backups are not supported after creating a full backup using this method. 
        /// Use <see cref="System.Fabric.KeyValueStoreReplica.BackupAsync(string, System.Fabric.StoreBackupOption, Func{System.Fabric.StoreBackupInfo, System.Threading.Tasks.Task{bool}})"/> to create
        /// a full backup if subsequent incremental backups are to be created.
        /// </para>
        /// </remarks>
        [Obsolete("Use BackupAsync instead")]
        public void Backup(string backupDirectory)
        {
            BackupAsync(backupDirectory, StoreBackupOption.Full, null, CancellationToken.None).Wait();
        }

        /// <summary>
        /// <para>Asynchronously creates a backup of the key/value store.</para>
        /// </summary>
        /// <param name="backupDirectory">
        /// The directory where the backup is to be stored. 
        /// If <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/>, then this parameter should be <b>null</b>.
        /// Otherwise, this parameter cannot be <b>null</b>, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// If the directory doesn't exist, it is created. If it exists and isn't empty, then incremental backup fails with
        /// <see cref="System.Fabric.FabricBackupDirectoryNotEmptyException"/>.
        /// </param>
        /// <param name="backupOption">
        /// <para>The options for the backup.</para>
        /// </param>
        /// <param name="postBackupAsyncFunc">
        /// The post backup asynchronous method that is invoked by Service Fabric to allow the user to complete
        /// any post backup activity before returning control to the system.
        /// If <b>null</b> is passed in for this, incremental backups are disallowed.
        /// If the post-backup method returns false, then again, incremental backups are disallowed.
        /// </param>
        /// <returns>A task that represents the asynchronous backup operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <b>backupDirectory</b> is <b>null</b> when <b>backupOption</b> is not <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <b>backupDirectory</b> is empty or contains just whitespaces when <b>backupOption</b> is not <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/> or
        /// <b>backupDirectory</b> is not <b>null</b> when <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/>.
        /// </exception>        
        /// <exception cref="FabricBackupDirectoryNotEmptyException">
        /// When <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.Incremental"/> and the backup directory already contains files or sub-directories.        
        /// </exception>
        /// <exception cref="FabricBackupInProgressException">
        /// When a previously initiated backup is currently in progress.
        /// </exception>
        /// <remarks>
        /// The <b>postBackupAsyncFunc</b> is not invoked if there is an error during backup. Also, it is not invoked when 
        /// <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/> since there is no further action needed
        /// from the user in this case to complete a single backup cycle.
        /// </remarks>
        /// <example>
        /// Below is an example of a simple implementation of <b>postBackupAsyncFunc</b>
        /// <code>
        /// private async Task&lt;bool&gt; SimplePostBackupHandler(StoreBackupInfo info)
        /// {
        ///     return await CopyBackupToAzureBlobStorage(info);
        /// }
        /// </code>
        /// </example>
        public Task BackupAsync(
            string backupDirectory,
            StoreBackupOption backupOption,
            Func<StoreBackupInfo, Task<bool>> postBackupAsyncFunc)
        {
            return BackupAsync(backupDirectory, backupOption, postBackupAsyncFunc, CancellationToken.None);
        }

        /// <summary>
        /// Asynchronously creates a backup of the key/value store.
        /// </summary>
        /// <param name="backupDirectory">
        /// The directory where the backup is to be stored. 
        /// If <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/>, then this parameter should be <b>null</b>.
        /// Otherwise, this parameter cannot be <b>null</b>, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// If the directory doesn't exist, it is created. If it exists and isn't empty, then incremental backup fails with
        /// <see cref="System.Fabric.FabricBackupDirectoryNotEmptyException"/>.
        /// </param>
        /// <param name="backupOption">
        /// <para>The options for the backup.</para>
        /// </param>
        /// <param name="postBackupAsyncFunc">
        /// The post backup asynchronous method that is invoked by Service Fabric to allow the user to complete
        /// any post backup activity before returning control to the system.
        /// If <b>null</b> is passed in for this, incremental backups are disallowed.
        /// If the post-backup method returns false, then again, incremental backups are disallowed.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task that represents the asynchronous backup operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <b>backupDirectory</b> is <b>null</b> when <b>backupOption</b> is not <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <b>backupDirectory</b> is empty or contains just whitespaces when <b>backupOption</b> is not <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/> or
        /// <b>backupDirectory</b> is not <b>null</b> when <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/>.
        /// </exception>        
        /// <exception cref="FabricBackupDirectoryNotEmptyException">
        /// When <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.Incremental"/> and the backup directory already contains files or sub-directories.        
        /// </exception>
        /// <exception cref="FabricBackupInProgressException">
        /// When a previously initiated backup is currently in progress.
        /// </exception>
        /// <remarks>
        /// The <b>postBackupAsyncFunc</b> is not invoked if there is an error during backup. Also, it is not invoked when 
        /// <b>backupOption</b> is <see cref="System.Fabric.StoreBackupOption.TruncateLogsOnly"/> since there is no further action needed
        /// from the user in this case to complete a single backup cycle.
        /// </remarks>
        /// <example>
        /// Below is an example of a simple implementation of <b>postBackupAsyncFunc</b>
        /// <code>
        /// private async Task&lt;bool&gt; SimplePostBackupHandler(StoreBackupInfo info)
        /// {
        ///     return await CopyBackupToAzureBlobStorage(info);
        /// }
        /// </code>
        /// </example>
        public Task BackupAsync(
            string backupDirectory, 
            StoreBackupOption backupOption, 
            Func<StoreBackupInfo, Task<bool>> postBackupAsyncFunc,
            CancellationToken cancellationToken)
        {
            if (backupOption != StoreBackupOption.TruncateLogsOnly)
            {
                backupDirectory.ThrowIfNullOrWhiteSpace("backupDirectory");
            }
            else
            {
                if (backupDirectory != null)
                {
                    throw new ArgumentException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Truncate_Logs_Only), "backupDirectory");
                }
            }
            
            var postBackupHandler = new KeyValueStorePostBackupHandlerBroker(postBackupAsyncFunc);

            Task task = Utility.WrapNativeAsyncInvoke(
                callback => BackupBeginWrapper(backupDirectory, backupOption, postBackupHandler, callback),
                BackupEndWrapper,
                InteropExceptionTracePolicy.Info,
                cancellationToken,
                "KeyValueStoreReplica.BackupAsync");

            return task;
        }

        /// <summary>
        /// <para>Restores this replica's local store database from a backup that was previously created by calling <see cref="System.Fabric.KeyValueStoreReplica.BackupAsync(string, StoreBackupOption, Func{StoreBackupInfo, Task{bool}})"/>.</para>
        /// </summary>
        /// <param name="backupDirectory">
        /// <para>The full path to a directory containing a backup.</para>
        /// </param>
        /// <remarks>
        /// <para>This is only a local replica restore and the replica set is not automatically restored. The entire replica set must be restored by taking additional steps to cause a natural build of other replicas via reconfiguration. The recommended approach is to restore to an empty service with only a single replica and increase the target replica set size afterwards with a call to <see cref="System.Fabric.FabricClient.ServiceManagementClient.UpdateServiceAsync(Uri, System.Fabric.Description.ServiceUpdateDescription)" /> if needed.</para>
        /// <para>If the restore is successful, then the replica will restart itself and start using the restored local data after coming back online given that the recommendation to restore to a replica set containing only a single replica was followed.</para>
        /// </remarks>
        [Obsolete("Use RestoreAsync instead")]
        public void Restore(string backupDirectory)
        {
            RestoreAsync(backupDirectory, CancellationToken.None).Wait();
        }

        /// <summary>
        /// <para>Restores this replica's local store database from a backup that was previously created by calling <see cref="System.Fabric.KeyValueStoreReplica.BackupAsync(string, StoreBackupOption, Func{StoreBackupInfo, Task{bool}})"/>.</para>
        /// </summary>
        /// <param name="backupDirectory">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <returns>A task that represents the asynchronous restore operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <b>backupDirectory</b> is <b>null</b>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <b>backupDirectory</b> is empty or contains just whitespaces.
        /// </exception>        
        /// <exception cref="DirectoryNotFoundException">
        /// <b>backupDirectory</b> does not exist.
        /// </exception>
        /// <remarks>
        /// <para>It is recommended to not perform any write operations to the key/value store while 
        /// restore is underway since the updated data would be lost when the store is restored from
        /// the files in <b>backupDirectory</b>. </para>
        /// <para>This is only a local replica restore and the replica set is not automatically restored. The entire replica set must be restored by taking additional steps 
        /// to cause a natural build of other replicas via reconfiguration. The recommended approach is to restore to an empty service with only a single replica and increase the target 
        /// replica set size afterwards with a call to <see cref="System.Fabric.FabricClient.ServiceManagementClient.UpdateServiceAsync(Uri, System.Fabric.Description.ServiceUpdateDescription)" /> if needed.</para>
        /// <para>If the restore is successful, then the replica will restart itself and start using the restored local data after coming back online 
        /// given that the recommendation to restore to a replica set containing only a single replica was followed.</para>
        /// </remarks>
        public Task RestoreAsync(
            string backupDirectory)
        {
            return RestoreAsync(backupDirectory, CancellationToken.None);
        }

        /// <summary>
        /// <para>Restores this replica's local store database from a backup that was previously created by calling <see cref="System.Fabric.KeyValueStoreReplica.BackupAsync(string, StoreBackupOption, Func{StoreBackupInfo, Task{bool}})"/>.</para>
        /// </summary>
        /// <param name="backupDirectory">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The cancellation token</para>
        /// </param>
        /// <returns>A task that represents the asynchronous restore operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <b>backupDirectory</b> is <b>null</b>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <b>backupDirectory</b> is empty or contains just whitespaces.
        /// </exception>        
        /// <exception cref="DirectoryNotFoundException">
        /// <b>backupDirectory</b> does not exist.
        /// </exception>
        /// <remarks>
        /// <para>It is recommended to not perform any write operations to the key/value store while 
        /// restore is underway since the updated data would be lost when the store is restored from
        /// the files in <b>backupDirectory</b>. </para>
        /// <para>This is only a local replica restore and the replica set is not automatically restored. The entire replica set must be restored by taking additional steps 
        /// to cause a natural build of other replicas via reconfiguration. The recommended approach is to restore to an empty service with only a single replica and increase the target 
        /// replica set size afterwards with a call to <see cref="System.Fabric.FabricClient.ServiceManagementClient.UpdateServiceAsync(Uri, System.Fabric.Description.ServiceUpdateDescription)" /> if needed.</para>
        /// <para>If the restore is successful, then the replica will restart itself and start using the restored local data after coming back online 
        /// given that the recommendation to restore to a replica set containing only a single replica was followed.</para>
        /// </remarks>
        public Task RestoreAsync(
            string backupDirectory,
            CancellationToken cancellationToken)
        {
            return RestoreAsync(backupDirectory, null, cancellationToken);
        }

        /// <summary>
        /// Asynchronously restores the key/value store replica.
        /// </summary>
        /// <param name="backupDirectory">
        /// The directory where the replica is to be restored from.
        /// This parameter cannot be null, empty or contain just whitespace. 
        /// UNC paths may also be provided.
        /// </param>
        /// <param name="settings">
        /// Settings to modify restore behavior.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>A task that represents the asynchronous restore operation.</returns>
        /// <exception cref="ArgumentNullException">
        /// <b>backupDirectory</b> is <b>null</b>.
        /// </exception>
        /// <exception cref="ArgumentException">
        /// <b>backupDirectory</b> is empty or contains just whitespaces.
        /// </exception>        
        /// <exception cref="DirectoryNotFoundException">
        /// <b>backupDirectory</b> does not exist.
        /// </exception>
        /// <remarks>
        /// <para>It is recommended to not perform any write operations to the key/value store while 
        /// restore is underway since the updated data would be lost when the store is restored from
        /// the files in <b>backupDirectory</b>. </para>
        /// <para>This is only a local replica restore and the replica set is not automatically restored. The entire replica set must be restored by taking additional steps 
        /// to cause a natural build of other replicas via reconfiguration. The recommended approach is to restore to an empty service with only a single replica and increase the target 
        /// replica set size afterwards with a call to <see cref="System.Fabric.FabricClient.ServiceManagementClient.UpdateServiceAsync(Uri, System.Fabric.Description.ServiceUpdateDescription)" /> if needed.</para>
        /// <para>If the restore is successful, then the replica will restart itself and start using the restored local data after coming back online 
        /// given that the recommendation to restore to a replica set containing only a single replica was followed.</para>
        /// </remarks>
        public Task RestoreAsync(
            string backupDirectory,
            RestoreSettings settings,
            CancellationToken cancellationToken)
        {
            Task task = Utility.WrapNativeAsyncInvoke(
                callback => RestoreBeginWrapper(backupDirectory, settings, callback),
                RestoreEndWrapper,
                cancellationToken,
                "KeyValueStoreReplica.RestoreAsync");

            return task;
        }

        #endregion

        #region Protected Overrides

        /// <summary>
        /// <para>Initializes a newly created service replica.</para>
        /// </summary>
        /// <param name="initializationParameters">
        /// <para>The initialization parameters for the service replica, represented as a <see cref="System.Fabric.StatefulServiceInitializationParameters" /> object.</para>
        /// </param>
        protected virtual void OnInitialize(StatefulServiceInitializationParameters initializationParameters)
        {
        }

        /// <summary>
        /// <para>Called on an initialized service replica to open it so that additional actions can be taken.</para>
        /// </summary>
        /// <param name="openMode">
        /// <para>A <see cref="System.Fabric.ReplicaOpenMode" /> object specifying for this replica whether it is new or recovered.</para>
        /// </param>
        /// <param name="partition">
        /// <para>A <see cref="System.Fabric.IStatefulServicePartition" /> object representing the stateful service partition information for this replica. </para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>A <see cref="System.Threading.CancellationToken" /> object that the operation is monitoring, which can be used to notify the task of cancellation.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> object representing the asynchronous operation.</para>
        /// </returns>
        protected virtual Task OnOpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// <para>Indicates that this replica is changing roles.</para>
        /// </summary>
        /// <param name="newRole">
        /// <para>The target role.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>Currently unused. Reserved for future use.</para>
        /// </param>
        /// <returns>
        /// <para>A task whose result is the resolvable address of this replica.</para>
        /// </returns>
        /// <remarks>
        /// <para>The application replica should override this method if deriving from <see cref="System.Fabric.KeyValueStoreReplica" />, which is the recommended pattern. The application replica should return a <see cref="System.Threading.Tasks.Task" /> whose result is the address of this replica. This replica address is stored by the system as is and can be retrieved (unmodified) using <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(Uri)" />. The application must take care to complete the role change in a timely manner since reconfiguration of the replica set will be blocked behind the completion of all outstanding change role calls.</para>
        /// </remarks>
        protected virtual Task<string> OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<string>();
            tcs.SetResult(string.Empty);
            return tcs.Task;
        }

        /// <summary>
        /// <para>Called when this service replica is being shut down and needs to close.</para>
        /// </summary>
        /// <param name="cancellationToken">
        /// <para>A <see cref="System.Threading.CancellationToken" /> object that the operation is monitoring, which can be used to notify the task of cancellation.</para>
        /// </param>
        /// <returns>
        /// <para>The asynchronous operation.</para>
        /// </returns>
        protected virtual Task OnCloseAsync(CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<bool>();
            tcs.SetResult(true);
            return tcs.Task;
        }

        /// <summary>
        /// <para>Called to shut down this instance.</para>
        /// </summary>
        protected virtual void OnAbort()
        {
        }

        /// <summary>
        /// <para>Called by the system on secondary replicas when they have finished building from the primary and are ready to start applying replication operations.</para>
        /// <para>This method will only be called on secondary replicas if the <see cref="System.Fabric.KeyValueStoreReplica" /> object was constructed with a valid <see cref="System.Fabric.KeyValueStoreReplica.SecondaryNotificationMode" /> parameter.</para>
        /// </summary>
        /// <param name="enumerator">
        /// <para>The enumerator used to read data on the secondary.</para>
        /// </param>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.KeyValueStoreEnumerator" /> object can be used to read data on the secondary within the context of this method before any replication operations are applied. The <see cref="System.Fabric.KeyValueStoreEnumerator" /> object is no longer valid after this method returns and cannot be used outside the context of this method. The application must take care to complete this callback in a timely manner since replication operations are being queued on the secondary replica and will not start getting applied until this method returns. The <see cref="System.Fabric.KeyValueStoreEnumerator" /> object is backed by a single underlying local transaction and is not thread-safe.</para>
        /// </remarks>
        protected virtual void OnCopyComplete(KeyValueStoreEnumerator enumerator)
        {
        }

        /// <summary>
        /// <para>Called by the system on secondary replicas for incoming replication operations. Each <see cref="System.Fabric.KeyValueStoreNotification" /> object contains all the data for a single atomic replication operation.</para>
        /// </summary>
        /// <param name="enumerator">
        /// <para>The enumerator used to read the data in this replication operation.</para>
        /// </param>
        /// <remarks>
        /// <para>This method will only be called on secondary replicas if the <see cref="System.Fabric.KeyValueStoreReplica" /> object was constructed with a valid <see cref="System.Fabric.KeyValueStoreReplica.SecondaryNotificationMode" />.</para>
        /// <para>If the <see cref="System.Fabric.KeyValueStoreReplica.SecondaryNotificationMode.BlockSecondaryAck" /> mode was specified, then the incoming replication operation is not applied locally on the secondary replica and acknowledged to the primary until the method returns. This implies that the application must take care to return from this method in a timely manner to avoiding blocking the replication stream. Since the acknowledgment is not sent to the primary until this method returns, it cannot be assumed that the observed replication operation has already been (or is guaranteed to be in the future) applied by a quorum of replicas in the replica set.</para>
        /// <para>If the <see cref="System.Fabric.KeyValueStoreReplica.SecondaryNotificationMode.NonBlockingQuorumAcked" /> mode was specified, then the observed replication operation is guaranteed to have already been applied by a quorum of replicas in the replica set. Furthermore, the observed replication operation may have already been applied locally by this secondary and acknowledged to the primary at the time the method is invoked by the system. The method callback will not block the replication stream in this mode, but it will still block the replication operation notification stream. That is, there will only be one outstanding OnReplicationOperation method callback at any given time.</para>
        /// </remarks>
        protected virtual void OnReplicationOperation(IEnumerator<KeyValueStoreNotification> enumerator)
        {
        }

        #endregion

        #region Native Wrappers and Helper methods

        internal virtual NativeRuntime.IFabricKeyValueStoreReplica6 CreateNativeKeyValueStoreReplica(
            StatefulServiceInitializationParameters initParams,
            NativeRuntime.IFabricStoreEventHandler storeEventHandler,
            NativeRuntime.IFabricSecondaryEventHandler secondaryEventHandler)
        {
            if (this.nativeKvsSettings_V2 == IntPtr.Zero)
            {
                return Utility.WrapNativeSyncInvoke(
                    () => CreateNativeKeyValueStoreReplicaHelper(
                        this.StoreName,
                        initParams.PartitionId,
                        initParams.ReplicaId,
                        initParams.ServiceName,
                        this.ReplicatorSettings,
                        this.LocalStoreSettings,
                        this.KeyValueStoreReplicaSettings,
                        storeEventHandler,
                        secondaryEventHandler),
                    "KeyValueStoreReplica.CreateNativeKeyValueStoreReplica");
            }
            else
            {
                return Utility.WrapNativeSyncInvoke(
                    () => CreateNativeKeyValueStoreReplicaHelper_V2(
                        initParams.PartitionId,
                        initParams.ReplicaId,
                        this.nativeKvsSettings_V2,
                        this.ReplicatorSettings,
                        storeEventHandler,
                        secondaryEventHandler),
                    "KeyValueStoreReplica_V2.CreateNativeKeyValueStoreReplica");
            }
        }

        private static NativeRuntime.IFabricKeyValueStoreReplica6 CreateNativeKeyValueStoreReplicaHelper(
            string storeName,
            Guid partitionId,
            long replicaId,
            Uri serviceName,
            ReplicatorSettings replicatorSettings,
            LocalStoreSettings localStoreSettings,
            KeyValueStoreReplicaSettings kvsSettings,
            NativeRuntime.IFabricStoreEventHandler storeEventHandler,
            NativeRuntime.IFabricSecondaryEventHandler secondaryEventHandler)
        {
            using (var pin = new PinCollection())
            {
                var keyValueStoreGuid = typeof(NativeRuntime.IFabricKeyValueStoreReplica6).GetTypeInfo().GUID;
                var nativeStoreName = pin.AddBlittable(storeName);
                var nativeServiceName = pin.AddObject(serviceName);
                var nativeReplicatorSettings = replicatorSettings.ToNative(pin);
                var nativeKvsSettings = kvsSettings.ToNative(pin);

                NativeTypes.FABRIC_LOCAL_STORE_KIND nativeLocalStoreKind;
                var nativeLocalStoreSettings = localStoreSettings.ToNative(pin, out nativeLocalStoreKind);

                return NativeRuntime.FabricCreateKeyValueStoreReplica5(
                    ref keyValueStoreGuid,
                    nativeStoreName,
                    partitionId,
                    replicaId,
                    nativeServiceName,
                    nativeReplicatorSettings,
                    nativeKvsSettings,
                    nativeLocalStoreKind,
                    nativeLocalStoreSettings,
                    storeEventHandler,
                    secondaryEventHandler);
            }
        }

        private static NativeRuntime.IFabricKeyValueStoreReplica6 CreateNativeKeyValueStoreReplicaHelper_V2(
            Guid partitionId,
            long replicaId,
            IntPtr nativeKvsSettings,
            ReplicatorSettings replicatorSettings,
            NativeRuntime.IFabricStoreEventHandler storeEventHandler,
            NativeRuntime.IFabricSecondaryEventHandler secondaryEventHandler)
        {
            using (var pin = new PinCollection())
            {
                var keyValueStoreGuid = typeof(NativeRuntime.IFabricKeyValueStoreReplica6).GetTypeInfo().GUID;
                var nativeReplicatorSettings = replicatorSettings.ToNative(pin);

                return NativeRuntime.FabricCreateKeyValueStoreReplica_V2(
                    ref keyValueStoreGuid,
                    partitionId,
                    replicaId,
                    nativeKvsSettings,
                    nativeReplicatorSettings,
                    storeEventHandler,
                    secondaryEventHandler);
            }
        }

        private NativeCommon.IFabricAsyncOperationContext BackupBeginWrapper(
            string backupDir, 
            StoreBackupOption backupOption, 
            NativeRuntime.IFabricStorePostBackupHandler postBackupHandler, 
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.BackupBeginWrapper", "Calling BeginBackup on NativeStore");

            using (var pin = new PinCollection())
            {
                var dir = pin.AddBlittable(backupDir);
                
                var operation = this.nativeStore.BeginBackup(
                    dir,
                    (NativeTypes.FABRIC_STORE_BACKUP_OPTION)backupOption,
                    postBackupHandler,
                    callback);

                AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.BackupBeginWrapper", "Calling BeginBackup on NativeStore completed");

                return operation;
            }            
        }

        private void BackupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.BackupEndWrapper", "Calling EndBackup on NativeStore");
            this.nativeStore.EndBackup(context);
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.BackupEndWrapper", "Calling EndBackup on NativeStore completed");
        }

        private NativeCommon.IFabricAsyncOperationContext RestoreBeginWrapper(
            string backupDir,
            RestoreSettings settings,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.RestoreBeginWrapper", "Calling BeginRestore on NativeStore");

            using (var pin = new PinCollection())
            {
                var dir = pin.AddBlittable(backupDir);

                var operation = this.nativeStore.BeginRestore2(
                    dir,
                    (settings == null ? IntPtr.Zero : settings.ToNative(pin)),
                    callback);

                AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.RestoreBeginWrapper", "Calling BeginRestore on NativeStore completed");

                return operation;
            }
        }

        private void RestoreEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.RestoreEndWrapper", "Calling EndRestore on NativeStore");
            this.nativeStore.EndRestore(context);
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.RestoreEndWrapper", "Calling EndRestore on NativeStore completed");
        }

        private NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(ReplicaOpenMode openMode, StatefulServicePartition partition, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.OpenBeginWrapper", "Calling BeginOpen on NativeStore");

            return this.nativeStore.BeginOpen(
                (NativeTypes.FABRIC_REPLICA_OPEN_MODE)openMode,
                partition.NativePartition,
                callback);
        }

        private IReplicator OpenEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.OpenEndWrapper", "Calling EndOpen on NativeStore");

            NativeRuntime.IFabricReplicator nativeReplicator = this.nativeStore.EndOpen(context);

            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.OpenEndWrapper", "Creating and returning FabricReplicator");

            if (this.nativeKvsSettings_V2 == IntPtr.Zero)
            {
#if !DotNetCoreClrLinux
                return CreateReplicator_V1(nativeReplicator);
#else
                // V1 RocksDB stack is no longer supported in Linux.
                // It will be forcefully overridden to native V2 stack underneath.
                //
                return CreateReplicator_V2(nativeReplicator);
#endif
            }
            else
            {
                return CreateReplicator_V2(nativeReplicator);
            }
        }

        private IReplicator CreateReplicator_V1(NativeRuntime.IFabricReplicator nativeReplicator)
        {
            // Handling InvalidCastException is needed to support the ReplicatedStore/Test_EnableTStore 
            // fabric setting that forces user KVS services to use TStore instead of ESE
            //
            try
            {
                NativeRuntime.IFabricStateReplicator nativeStateReplicator = (NativeRuntime.IFabricStateReplicator)nativeReplicator;
                NativeRuntime.IOperationDataFactory nativeOperationDataFactory = (NativeRuntime.IOperationDataFactory)nativeStateReplicator;

                return new FabricReplicator(nativeReplicator, nativeStateReplicator, nativeOperationDataFactory);
            }
            catch (InvalidCastException)
            {
                AppTrace.TraceSource.WriteWarning("KeyValueStoreReplica.CreateReplicator_V1", "Falling back to V2 replicator creation");

                return CreateReplicator_V2(nativeReplicator);
            }
        }

        private IReplicator CreateReplicator_V2(NativeRuntime.IFabricReplicator nativeReplicator)
        {
            return new FabricReplicator(nativeReplicator);
        }

        private NativeCommon.IFabricAsyncOperationContext CloseBeginWrapper(NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.CloseBeginWrapper", "Calling BeginClose on NativeStore");
            return this.nativeStore.BeginClose(callback);
        }

        private void CloseEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.CloseEndWrapper", "Calling EndClose on NativeStore");
            this.nativeStore.EndClose(context);
        }

        private NativeCommon.IFabricAsyncOperationContext ChangeRoleBeginWrapper(ReplicaRole newRole, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.ChangeRoleBeginWrapper", "Calling BeginChangeRole on NativeStore, NewRole={0}", newRole);
            return this.nativeStore.BeginChangeRole((NativeTypes.FABRIC_REPLICA_ROLE)newRole, callback);
        }

        private string ChangeRoleEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.ChangeRoleEndWrapper", "Calling EndChangeRole on NativeStore");
            NativeCommon.IFabricStringResult nativeAddress = this.nativeStore.EndChangeRole(context);

            return NativeTypes.FromNativeString(nativeAddress.get_String());
        }

        private void AddHelper(TransactionBase transactionBase, string key, byte[] value)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                var nativeValue = NativeTypes.ToNativeBytes(pin, value);
                this.nativeStore.Add(transactionBase.NativeTransactionBase, nativeKey, (int)nativeValue.Item1, nativeValue.Item2);
            }
        }

        private bool TryAddHelper(TransactionBase transactionBase, string key, byte[] value)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                var nativeValue = NativeTypes.ToNativeBytes(pin, value);
                var nativeResult = this.nativeStore.TryAdd(transactionBase.NativeTransactionBase, nativeKey, (int)nativeValue.Item1, nativeValue.Item2);
                return NativeTypes.FromBOOLEAN(nativeResult);
            }
        }

        private void RemoveHelper(TransactionBase transactionBase, string key, long checkSequenceNumber)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                this.nativeStore.Remove(transactionBase.NativeTransactionBase, nativeKey, checkSequenceNumber);
            }
        }

        private bool TryRemoveHelper(TransactionBase transactionBase, string key, long checkSequenceNumber)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                var nativeResult = this.nativeStore.TryRemove(transactionBase.NativeTransactionBase, nativeKey, checkSequenceNumber);
                return NativeTypes.FromBOOLEAN(nativeResult);
            }
        }

        private void UpdateHelper(TransactionBase transactionBase, string key, byte[] value, long checkSequenceNumber)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                var nativeValue = NativeTypes.ToNativeBytes(pin, value);
                this.nativeStore.Update(transactionBase.NativeTransactionBase, nativeKey, (int)nativeValue.Item1, nativeValue.Item2, checkSequenceNumber);
            }
        }

        private bool TryUpdateHelper(TransactionBase transactionBase, string key, byte[] value, long checkSequenceNumber)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                var nativeValue = NativeTypes.ToNativeBytes(pin, value);
                var nativeResult = this.nativeStore.TryUpdate(transactionBase.NativeTransactionBase, nativeKey, (int)nativeValue.Item1, nativeValue.Item2, checkSequenceNumber);
                return NativeTypes.FromBOOLEAN(nativeResult);
            }
        }

        private bool ContainsHelper(TransactionBase transactionBase, string key)
        {
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                return NativeTypes.FromBOOLEAN(this.nativeStore.Contains(transactionBase.NativeTransactionBase, nativeKey));
            }
        }

        private KeyValueStoreItem GetHelper(TransactionBase transactionBase, string key)
        {
            NativeRuntime.IFabricKeyValueStoreItemResult nativeItemResult;
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                nativeItemResult = this.nativeStore.Get(transactionBase.NativeTransactionBase, nativeKey);
            }

            return KeyValueStoreItem.CreateFromNative(nativeItemResult);
        }

        private KeyValueStoreItem TryGetHelper(TransactionBase transactionBase, string key)
        {
            NativeRuntime.IFabricKeyValueStoreItemResult nativeItemResult;
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                nativeItemResult = this.nativeStore.TryGet(transactionBase.NativeTransactionBase, nativeKey);
            }

            return KeyValueStoreItem.CreateFromNative(nativeItemResult);
        }

        private byte[] GetValueHelper(TransactionBase transactionBase, string key)
        {
            NativeRuntime.IFabricKeyValueStoreItemResult nativeItemResult;
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                nativeItemResult = this.nativeStore.Get(transactionBase.NativeTransactionBase, nativeKey);
            }

            var item = KeyValueStoreItem.CreateFromNative(nativeItemResult);
            return item.Value;
        }

        private byte[] TryGetValueHelper(TransactionBase transactionBase, string key)
        {
            NativeRuntime.IFabricKeyValueStoreItemResult nativeItemResult;
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                nativeItemResult = this.nativeStore.TryGet(transactionBase.NativeTransactionBase, nativeKey);
            }

            var item = KeyValueStoreItem.CreateFromNative(nativeItemResult);
            return (item != null ? item.Value : null);
        }

        private KeyValueStoreItemMetadata GetMetadataHelper(TransactionBase transactionBase, string key)
        {
            NativeRuntime.IFabricKeyValueStoreItemMetadataResult nativeItemMetadataResult;
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                nativeItemMetadataResult = this.nativeStore.GetMetadata(transactionBase.NativeTransactionBase, nativeKey);
            }

            return KeyValueStoreItemMetadata.CreateFromNative(nativeItemMetadataResult);
        }

        private KeyValueStoreItemMetadata TryGetMetadataHelper(TransactionBase transactionBase, string key)
        {
            NativeRuntime.IFabricKeyValueStoreItemMetadataResult nativeItemMetadataResult;
            using (var pin = new PinCollection())
            {
                var nativeKey = pin.AddBlittable(key);
                nativeItemMetadataResult = this.nativeStore.TryGetMetadata(transactionBase.NativeTransactionBase, nativeKey);
            }

            return KeyValueStoreItemMetadata.CreateFromNative(nativeItemMetadataResult);
        }

        private IEnumerator<KeyValueStoreItem> EnumerateHelper(TransactionBase transactionBase)
        {
            return new KeyValueStoreItemEnumerator(
                transactionBase,
                (tx) => this.CreateNativeEnumerator(tx));
        }

        private NativeRuntime.IFabricKeyValueStoreItemEnumerator CreateNativeEnumerator(TransactionBase transactionBase)
        {
            return this.nativeStore.Enumerate(transactionBase.NativeTransactionBase);
        }

        private IEnumerator<KeyValueStoreItem> EnumerateHelper(TransactionBase transactionBase, string keyPrefix, bool strictPrefix)
        {
            return new KeyValueStoreItemEnumerator(
                transactionBase,
                (tx) => this.CreateNativeEnumeratorByKey(tx, keyPrefix, strictPrefix));
        }

        private NativeRuntime.IFabricKeyValueStoreItemEnumerator CreateNativeEnumeratorByKey(TransactionBase transactionBase, string keyPrefix, bool strictPrefix)
        {
            using (var pin = new PinCollection())
            {
                return this.nativeStore.EnumerateByKey2(
                    transactionBase.NativeTransactionBase,
                    pin.AddBlittable(keyPrefix),
                    NativeTypes.ToBOOLEAN(strictPrefix));
            }
        }

        private IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadataHelper(TransactionBase transactionBase)
        {
            return new KeyValueStoreItemMetadataEnumerator(
                transactionBase,
                (tx) => this.CreateNativeMetadataEnumerator(tx));
        }

        private NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator CreateNativeMetadataEnumerator(TransactionBase transactionBase)
        {
            return this.nativeStore.EnumerateMetadata(transactionBase.NativeTransactionBase);
        }

        private IEnumerator<KeyValueStoreItemMetadata> EnumerateMetadataHelper(TransactionBase transactionBase, string keyPrefix, bool strictPrefix)
        {
            return new KeyValueStoreItemMetadataEnumerator(
                transactionBase,
                (tx) => this.CreateNativeMetadataEnumeratorByKey(tx, keyPrefix, strictPrefix));
        }

        private NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator CreateNativeMetadataEnumeratorByKey(TransactionBase transactionBase, string keyPrefix, bool strictPrefix)
        {
            using (var pin = new PinCollection())
            {
                return this.nativeStore.EnumerateMetadataByKey2(
                    transactionBase.NativeTransactionBase,
                    pin.AddBlittable(keyPrefix),
                    NativeTypes.ToBOOLEAN(strictPrefix));
            }
        }

        private Epoch GetCurrentEpochHelper()
        {
            NativeTypes.FABRIC_EPOCH nativeEpoch;
            this.nativeStore.GetCurrentEpoch(out nativeEpoch);

            return new Epoch(nativeEpoch.DataLossNumber, nativeEpoch.ConfigurationNumber);
        }

        private void UpdateReplicatorSettingsHelper(ReplicatorSettings replicatorSettings)
        {
            AppTrace.TraceSource.WriteNoise(
                "KeyValueStoreReplica.UpdateReplicatorSettings",
                "Replicator Settings = {0}",
                replicatorSettings.ToString());

            using (var pin = new PinCollection())
            {
                IntPtr nativeReplicatorSettings = replicatorSettings.ToNative(pin);
                this.nativeStore.UpdateReplicatorSettings(nativeReplicatorSettings);
            }
        }

        private Transaction CreateTransactionHelper(KeyValueStoreTransactionSettings settings)
        {
            if (settings == null)
            {
                var nativeTransaction = this.nativeStore.CreateTransaction();

                return new Transaction(nativeTransaction);
            }
            else
            {
                using (var pin = new PinCollection())
                {
                    var nativeTransaction = this.nativeStore.CreateTransaction2(settings.ToNative(pin));

                    return new Transaction(nativeTransaction);
                }
            }
        }

        private void RestoreHelper(string backupDirectory)
        {
            using (var pin = new PinCollection())
            {
                this.nativeStore.Restore(pin.AddObject(backupDirectory));
            }
        }

        #endregion

        #region State Checking Helpers
        
        private void ThrowIfNotCreated()
        {
            if (this.state == State.Created)
            {
                return;
            }

            if (this.state < State.ClosedOrAborted)
            {
                throw new FabricObjectClosedException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_ClosedOrAborted));
            }
            else if ((this.state == State.Opened) || (this.state == State.Initialized))
            {
                throw new InvalidOperationException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Already_Initialized));
            }
        }

        private void ThrowIfNotOpened()
        {
            if (this.state == State.Opened)
            {
                return;
            }

            if (this.state == State.ClosedOrAborted)
            {
                throw new FabricObjectClosedException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_ClosedOrAborted));
            }
            else if (this.state == State.Initialized)
            {
                throw new InvalidOperationException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Not_Opened));
            }
            else if (this.state == State.Created)
            {
                throw new InvalidOperationException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Not_InitializedAndOpened));
            }
        }

        private void ThrowIfNotInitialized()
        {
            if (this.state == State.Initialized)
            {
                return;
            }

            if (this.state == State.ClosedOrAborted)
            {
                throw new FabricObjectClosedException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_ClosedOrAborted));
            }
            else if (this.state == State.Opened)
            {
                throw new InvalidOperationException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Already_Opened));
            }
            else if (this.state == State.Created)
            {
                throw new InvalidOperationException(this.GetExceptionMessage(StringResources.Error_KeyValueStoreReplica_Not_Initialized));
            }
        }

        private string GetExceptionMessage(string message)
        {
            return string.Format("{0}:{1}", this.GetTraceId(), message);
        }

        #endregion

        #region IStatefulServiceReplica implementation for Store

        private IntPtr nativeKvsSettings_V2 = IntPtr.Zero;

        internal void OverrideNativeKeyValueStore_V2(IntPtr nativeKvsSettings)
        {
            AppTrace.TraceSource.WriteInfo(
                "KeyValueStoreReplica.OverrideNativeKeyValueStore_V2", 
                "{0} overriding native KVS",
                this.GetTraceId());
            
            this.nativeKvsSettings_V2 = nativeKvsSettings;
        }

        private void Store_Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            var storeEventHandler = new KeyValueStoreEventHandlerBroker(this);

            KeyValueSecondaryEventHandlerBroker secondaryEventHandler = null;
            if (this.NotificationMode != SecondaryNotificationMode.None && this.NotificationMode != SecondaryNotificationMode.Invalid)
            {
                secondaryEventHandler = new KeyValueSecondaryEventHandlerBroker(this);
            }

            this.nativeStore = this.CreateNativeKeyValueStoreReplica(
                initializationParameters, 
                storeEventHandler,
                secondaryEventHandler);
        }

        private Task<IReplicator> Store_OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            StatefulServicePartition castedPartition = partition as StatefulServicePartition;
            Requires.Argument<StatefulServicePartition>("partition", castedPartition).NotNull();

            return Utility.WrapNativeAsyncInvoke<IReplicator>(
                (callback) => this.OpenBeginWrapper(openMode, castedPartition, callback),
                this.OpenEndWrapper,
                cancellationToken,
                "KeyValueStoreReplica.Open");
        }

        private Task<string> Store_ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke<string>(
               (callback) => this.ChangeRoleBeginWrapper(newRole, callback),
               this.ChangeRoleEndWrapper,
               cancellationToken,
               "KeyValueStoreReplica.ChangeRole");
        }

        private Task Store_CloseAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvoke(
               (callback) => this.CloseBeginWrapper(callback),
               this.CloseEndWrapper,
               cancellationToken,
               "KeyValueStoreReplica.Close");
        }

        private void Store_Abort()
        {
            Utility.WrapNativeSyncInvoke(
                () => this.nativeStore.Abort(),
                "KeyValueStoreReplica.Abort");
        }

        #endregion

        #region IStatefulServiceReplica implementation for Derived Service

        private void Service_Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            AppTrace.TraceSource.WriteNoise("KeyValueStoreReplica.Initialize", "Calling OnInitialize on the derived Service Replica");
            this.OnInitialize(initializationParameters);
        }

        private Task Service_OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(
                "KeyValueStoreReplica.Open", 
                "{0} Calling OnOpenAsync on the derived Service Replica",
                this.GetTraceId());
            return this.OnOpenAsync(openMode, partition, cancellationToken);
        }

        private Task<string> Service_ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(
                "KeyValueStoreReplica.ChangeRole", 
                "{0} Calling OnChangeRoleAsync({1}) on the derived Service Replica",
                this.GetTraceId(),
                newRole);
            return this.OnChangeRoleAsync(newRole, cancellationToken);
        }

        private Task Service_CloseAsync(CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(
                "KeyValueStoreReplica.Close", 
                "{0} Calling OnCloseAsync on the derived Service Replica",
                this.GetTraceId());
            return this.OnCloseAsync(cancellationToken);
        }

        private void Service_Abort()
        {
            AppTrace.TraceSource.WriteInfo(
                "KeyValueStoreReplica.Abort", 
                "{0} Calling OnAbort on the derived Service Replica",
                this.GetTraceId());
            this.OnAbort();
        }

        #endregion

        #region IStatefulServiceReplica implementation for BackupRestoreManager

        private void BackupRestoreManager_Initialize(StatefulServiceInitializationParameters initializationParameters)
        {
            this.backupRestoreManager?.Initialize(initializationParameters);
        }

        private Task BackupRestoreManager_OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partition, CancellationToken cancellationToken)
        {
            return this.backupRestoreManager?.OpenAsync(openMode, partition, cancellationToken) ?? Task.FromResult(0);
        }

        private Task BackupRestoreManager_ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return this.backupRestoreManager?.ChangeRoleAsync(newRole, cancellationToken) ?? Task.FromResult(0);
        }

        private Task BackupRestoreManager_CloseAsync(CancellationToken cancellationToken)
        {
            if (this.backupRestoreManager != null)
            {
                CleanupBackupFolder();
                return this.backupRestoreManager.CloseAsync(cancellationToken);
            }

            return Task.FromResult(0);
        }

        private void BackupRestoreManager_Abort()
        {
            if (this.backupRestoreManager != null)
            {
                CleanupBackupFolder();
                this.backupRestoreManager.Abort();
            }
        }

        #endregion

        #region native helpers for secondary notifications

        private void OnCopyComplete(NativeRuntime.IFabricKeyValueStoreEnumerator nativeEnumerator)
        {
            this.OnCopyComplete(new KeyValueStoreEnumerator(nativeEnumerator));
        }

        private void OnReplicationOperation(NativeRuntime.IFabricKeyValueStoreNotificationEnumerator nativeEnumerator)
        {
            this.OnReplicationOperation(new KeyValueStoreNotificationEnumerator(nativeEnumerator));
        }

        #endregion

        private class KeyValueStoreEventHandlerBroker : NativeRuntime.IFabricStoreEventHandler2
        {
            private KeyValueStoreReplica owner;

            public KeyValueStoreEventHandlerBroker(KeyValueStoreReplica owner)
            {
                this.owner = owner;
            }

            public void OnDataLoss()
            {
                owner.OnDatalossReported(new EventArgs());
            }

            public NativeCommon.IFabricAsyncOperationContext BeginOnDataLoss(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.owner.OnDataLossInternalAsync(cancellationToken), callback, "KeyValueStoreReplica.BeginOnDataLoss");
            }

            public BOOLEAN EndOnDataLoss(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NativeTypes.ToBOOLEAN(AsyncTaskCallInAdapter.End<bool>(context));
            }
        }

        private async Task<bool> OnDataLossInternalAsync(CancellationToken cancellationToken)
        {
            
            if (this.backupRestoreManager != null && await this.backupRestoreManager.OnDataLossAsync(cancellationToken))
            {
                await this.OnRestoreCompletedAsync(cancellationToken);
                return true;
            }
            else
            {
                return await this.OnDataLossAsync(cancellationToken);
            }
        }

        private class KeyValueSecondaryEventHandlerBroker : NativeRuntime.IFabricSecondaryEventHandler
        {
            private static readonly InteropApi CallbackApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};

            private KeyValueStoreReplica owner;

            public KeyValueSecondaryEventHandlerBroker(KeyValueStoreReplica owner)
            {
                this.owner = owner;
            }

            void NativeRuntime.IFabricSecondaryEventHandler.OnCopyComplete(
                NativeRuntime.IFabricKeyValueStoreEnumerator nativeEnumerator)
            {
                try
                {
                    owner.OnCopyComplete(nativeEnumerator);
                }
                catch (Exception ex)
                {
                    CallbackApi.HandleException(ex);
                    throw;
                }
            }

            void NativeRuntime.IFabricSecondaryEventHandler.OnReplicationOperation(
                NativeRuntime.IFabricKeyValueStoreNotificationEnumerator nativeEnumerator)
            {
                try
                {
                    owner.OnReplicationOperation(nativeEnumerator);
                }
                catch (Exception ex)
                {
                    CallbackApi.HandleException(ex);
                    throw;
                }
            }
        }

        private class KeyValueStorePostBackupHandlerBroker : NativeRuntime.IFabricStorePostBackupHandler
        {            
            private readonly Func<StoreBackupInfo, Task<bool>> postBackupAsyncFunc;            

            public KeyValueStorePostBackupHandlerBroker(
                Func<StoreBackupInfo, Task<bool>> postBackupAsyncFunc)
            {
                this.postBackupAsyncFunc = postBackupAsyncFunc;

                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.ctor",
                    postBackupAsyncFunc == null
                        ? "postBackupAsyncFunc is null, using default handler"
                        : "postBackupAsyncFunc provided, using it instead of default handler");
            }

            public NativeCommon.IFabricAsyncOperationContext BeginPostBackup(
                NativeTypes.FABRIC_STORE_BACKUP_INFO backupInfo, 
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.BeginPostBackup",
                    "Invoking Utility.WrapNativeAsyncMethodImplementation");

                var info = StoreBackupInfo.FromNative(backupInfo);

                NativeCommon.IFabricAsyncOperationContext operation = Utility.WrapNativeAsyncMethodImplementation(
                    cancellationToken => 
                        OuterPostBackupHandler(info), 
                        callback, 
                        "KeyValueStorePostBackupHandlerBroker.postBackupAsyncFunc");

                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.BeginPostBackup",
                    "Invoke Utility.WrapNativeAsyncMethodImplementation done");

                return operation;
            }

            public BOOLEAN EndPostBackup(NativeCommon.IFabricAsyncOperationContext context)
            {
                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.EndPostBackup",
                    "Invoking AsyncTaskCallInAdapter.End<bool>");

                var result = AsyncTaskCallInAdapter.End<bool>(context);

                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.EndPostBackup",
                    "'{0}' returned from AsyncTaskCallInAdapter.End<bool>", result);

                return NativeTypes.ToBOOLEAN(result);
            }

            private async Task<bool> OuterPostBackupHandler(StoreBackupInfo info)
            {
                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.OuterPostBackupHandler",
                    "Starting execution of outer post backup handler. StoreBackupInfo.BackupFolder: {0}, StoreBackupInfo.BackupOption: {1}", info.BackupFolder, info.BackupOption);

                Task<bool> task = postBackupAsyncFunc != null 
                    ? postBackupAsyncFunc(info) 
                    : Task.Factory.StartNew(() => false);

                bool result = await task;

                AppTrace.TraceSource.WriteNoise(
                    "KeyValueStorePostBackupHandlerBroker.OuterPostBackupHandler",
                    "Returning result '{0}' after execution of outer post backup handler", result);

                return result;
            }
        }

        private string GetLocalBackupFolderPath()
        {
            return Path.Combine(
                this.initializationParams.CodePackageActivationContext.WorkDirectory,
                BackupRootFolderPrefix + this.initializationParams.PartitionId,
                this.initializationParams.ReplicaId.ToString(),
                LocalBackupFolderName);
        }

        private static void PrepareBackupFolder(string backupFolder)
        {
            try
            {
                FabricDirectory.Delete(backupFolder, true);
            }
            catch (DirectoryNotFoundException)
            {
                // Already empty
            }

            FabricDirectory.CreateDirectory(backupFolder);
        }

        private void CleanupBackupFolder()
        {
            try
            {
                FabricDirectory.Delete(this.GetLocalBackupFolderPath(), true);
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteInfo(
                    "KeyValueStorePostBackupHandlerBroker.CleanupBackupFolder",
                    "Cleanup Backup Folder failed with exception: {0}", ex);
            }
        }

        #region IBackableReplica

        Task IBackupRestoreReplica.BackupAsync(BackupOption backupOption, Func<BackupInfo, CancellationToken, Task<bool>> backupCallback, CancellationToken cancellationToken)
        {
            StoreBackupOption option;
            if (backupOption == BackupOption.Full)
            {
                option = StoreBackupOption.Full;
            }
            else
            {
                option = StoreBackupOption.Incremental;
            }

            var backupPath = GetLocalBackupFolderPath();
            PrepareBackupFolder(backupPath);

            return this.BackupAsync(GetLocalBackupFolderPath(), option, info => PostBackupFunc(info, backupCallback), cancellationToken);
        }

        private Task<bool> PostBackupFunc(StoreBackupInfo storeBackupInfo, Func<BackupInfo, CancellationToken, Task<bool>> backupCallback)
        {
            var backupInfo = new BackupInfo
            {
                Directory = storeBackupInfo.BackupFolder,
                Option = storeBackupInfo.BackupOption == StoreBackupOption.Full
                    ? BackupOption.Full
                    : BackupOption.Incremental,
                LastBackupVersion = BackupVersion.InvalidBackupVersion,
                IndexBackupVersion = BackupVersion.InvalidBackupVersion,
                ParentBackupId = Guid.Empty,
                BackupId =
                    storeBackupInfo.BackupOption == StoreBackupOption.Full
                        ? storeBackupInfo.BackupChainId
                        : Guid.NewGuid(),
                BackupChainId = storeBackupInfo.BackupChainId,
                BackupIndex = storeBackupInfo.BackupIndex,
            };

            return backupCallback.Invoke(backupInfo, CancellationToken.None);          // TODO: Fix the cancellation token
        }

        Task IBackupRestoreReplica.RestoreAsync(string backupFolderPath, bool forceRestore, CancellationToken cancellationToken)
        {
            var restoreSettings = new RestoreSettings(true, !forceRestore);
            return this.RestoreAsync(backupFolderPath, restoreSettings, cancellationToken);
        }

        Task<byte[]> IBackupRestoreReplica.GetBackupMetadataAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            using (var tx = this.CreateTransaction())
            {
                var value = this.GetValue(tx, BackupMetadataKeyName);
                return Task.FromResult(value);
            }
        }

        async Task IBackupRestoreReplica.SaveBackupMetadataAsync(byte[] metadata, TimeSpan timeout, CancellationToken cancellationToken)
        {
            using (var tx = this.CreateTransaction())
            {
                if (!this.TryAdd(tx, BackupMetadataKeyName, metadata))
                {
                    this.Update(tx, BackupMetadataKeyName, metadata);
                }

                await tx.CommitAsync(timeout);
            }
        }

        async Task IBackupRestoreReplica.ClearBackupMetadataAsync(TimeSpan timeout)
        {
            using (var tx = this.CreateTransaction())
            {
                this.TryRemove(tx, BackupMetadataKeyName);
                await tx.CommitAsync(timeout);
            }
        }

        #endregion
    }
}
