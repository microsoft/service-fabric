// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Common;
    using Microsoft.ServiceFabric.Data.Notifications;
    using Microsoft.ServiceFabric.Data.ReplicatedStore.DifferentialStore;
    using Microsoft.ServiceFabric.ReplicatedStore.Diagnostics;
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    using ReplicatorTransaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Defines the store behavior.
    /// </summary>
    internal enum StoreBehavior : byte
    {
        /// <summary>
        /// Single version store.
        /// </summary>
        SingleVersion = 0,

        /// <summary>
        /// Multi-version store.
        /// </summary>
        MultiVersion = 1,

        /// <summary>
        /// All version store.
        /// </summary>
        Historical = 2,
    }

    /// <summary>
    /// Defines the store modification for store operations.
    /// </summary>
    internal enum StoreModificationType : byte
    {
        Add = 0,
        Remove = 1,
        Update = 2,
        PartialUpdate = 3,
        Get = 4,
        Clear = 5,
        Checkpoint = 6,
        Copy = 7,
        Pause = 8,
    }

    /// <summary>
    /// Store.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    /// <typeparam name="TKeyRangePartitioner">Key range partitioner type.</typeparam>
    internal class TStore<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
        IStateProvider2,
        IStore<TKey, TValue>,
        INotifyDictionaryChanged<TKey, TValue>,
        IStoreTransactionProvider,
        ILoadMetricProvider,
        IStoreCopyProvider,
        IStoreSweepProvider
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        #region Instance Members

        private const string currentDiskMetadataFileName = "current_metadata.sfm";
        private const string tempDiskMetadataFileName = "temp_metadata.sfm";
        private const string bkpDiskMetadataFileName = "backup_metadata.sfm";

        /// <summary>
        /// This value * 2 will be the first backoff.
        /// </summary>
        private const int ExponentialBackoffMin = 64 / 2;

        /// <summary>
        /// Max used for exponential backoff of replication operations.
        /// </summary>
        private const int ExponentialBackoffMax = 4 * 1024;

        /// <summary>
        /// Disk metric.
        /// </summary>
        private const string OnDiskSizeInMBMetricName = "DiskMB";

        /// <summary>
        /// Disk metric.
        /// </summary>
        private const string OnDiskSizeInBytesMetricName = "DiskBytes";

        /// <summary>
        /// Used to distinguish asof for datetime.
        /// </summary>
        private const double DefaultSystemTimerResolution = 1000.0/64.0;

        /// <summary>
        /// Timeout(ms) used for lock acquistion on checkpoint and copy to avoid indefinite checkpoint and copy delays during ClearAsync/PrepareForRemoveStateAsync operations progress.
        /// </summary>
        private const int CheckpointOrCopyLockTimeout = 256*1000;

        /// <summary>
        /// Max time a complete checkpoint should take.
        /// </summary>
        private const long MaxCompleteCheckpointTime = 1000 * 30;

        /// <summary>
        /// Helper class to build the metadata and checkpoint files.
        /// </summary>
        private CopyManager copyManager;

        /// <summary>
        /// Whether copy was aborted when copying to this store
        /// </summary>
        private bool wasCopyAborted = false;

        /// <summary>
        /// Key comparer.
        /// </summary>
        private TKeyComparer keyComparer = new TKeyComparer();

        /// <summary>
        /// Key equality comparer.
        /// </summary>
        private TKeyEqualityComparer keyEqualityComparer = new TKeyEqualityComparer();

        /// <summary>
        /// Key serialization.
        /// </summary>
        private IStateSerializer<TKey> keyConverter;

        /// <summary>
        /// Value serialization.
        /// </summary>
        private IStateSerializer<TValue> valueConverter;

        /// <summary>
        /// Range partition descriptions.
        /// </summary>
        private TKeyRangePartitioner keyRangePartitioner = new TKeyRangePartitioner();

        /// <summary>
        /// Lock managers used for concurrency control.
        /// </summary>
        private LockManager lockManager = new LockManager();

        /// <summary>
        /// Partition id hosting this store.
        /// </summary>
        private Guid partitionId = Guid.Empty;

        /// <summary>
        /// Replica id hosting this store.
        /// </summary>
        private long replicaId = long.MinValue;

        /// <summary>
        /// Service instance name hosting this store.
        /// </summary>
        private Uri serviceInstanceName;

        /// <summary>
        /// Working directory for this store.
        /// </summary>
        private string workingDirectory;

        /// <summary>
        /// Records in-flight read/write transactions.
        /// </summary>
        private ConcurrentDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>> inflightReadWriteStoreTransactions =
            new ConcurrentDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>>(1, 1);

        /// <summary>
        /// Specifies that locks should be taken if the store supports reading from secondary replicas.
        /// </summary>
        private bool isAlwaysReadable = true;

        /// <summary>
        /// Used for tracing. Computed once.
        /// </summary>
        private string traceType;

        /// <summary>
        /// Transactional replicator
        /// </summary>
        private ITransactionalReplicator transactionalReplicator;

        /// <summary>
        /// Store behavior.
        /// </summary>
        private StoreBehavior storeBehavior;

        /// <summary>
        /// Indicates whether checkpoint is currently being recovered.
        /// </summary>
        private bool isRecoverCheckpointInProgress = false;

        /// <summary>
        /// State provider id.
        /// </summary>
        private long stateProviderId;

        private bool IsClearApplied;

        private bool IsPrepareAfterClear;

        private SemaphoreSlim MetadataTableLock;

        private Func<IReliableState, NotifyDictionaryRebuildEventArgs<TKey, TValue>, Task> rebuildNotificationsAsyncCallback = null;

        /// <summary>
        /// Indicates if the value type for tstore is a reference type or value type.
        /// </summary>
        private bool isValueAReferenceType;

        /// <summary>
        /// Keeps the BEST EFFORT count of number of items in the TStore.
        /// </summary>
        private long count;

        /// <summary>
        /// Flag to synchronize sweep operation ensuring that only one task exceutes at a time.
        /// </summary>
        private int sweepInProgress = 0;

        /// <summary>
        /// Flag that indicates store is closing.
        /// </summary>
        private bool isClosing;

        /// <summary>
        /// Time stamp to track time when checkpoint files are written.
        /// </summary>
        private long lastCheckpointFileTimeStamp;

        /// <summary>
        /// Represents the task that tracks background consolidation.
        /// </summary>
        private Task<PostMergeMetadatableInformation> consolidationTask = null;

        /// <summary>
        /// Lock object to synchronize on file id increments.
        /// </summary>
        /// <devnote>Interlocked operations are not supported on uints</devnote>
        private object fileIdLock = new object();

        /// <summary>
        /// Sweep threshold that decides when to trigger sweep following load on reads.
        /// </summary>
        private int sweepThreshold = -1;

        /// <summary>
        /// Last checkpoint LSN 
        /// </summary>
        private long lastPrepareCheckpointLSN = DifferentialStoreConstants.InvalidLSN;

        /// <summary>
        /// Last checkpoint LSN that was performed.
        /// This is only used for Asserts.
        /// </summary>
        private long lastPerformCheckpointLSN = DifferentialStoreConstants.InvalidLSN;

        /// <summary>
        /// Store settings.
        /// </summary>
        private StoreSettings storeSettings;

        /// <summary>
        /// This settings configures the store to recover values as well as the keys during recovering state from disk.
        /// It will execute is any rebuild scenario: recovery, copy or restore.
        /// </summary>
        private bool shouldLoadValuesInRecovery = false;

        /// <summary>
        /// This setting indicates max number of parallel tasks that should be used to load values during recovery.
        /// This setting is only used  if "shouldLoadValuesInRecovery" is set to true.
        /// </summary>
        private int numberOfInflightValueRecoveryTasks = Environment.ProcessorCount;

        /// <summary>
        /// This setting indicates whether strict 2PL is enabled.
        /// </summary>
        private bool enableStrict2PL = false;

        #region Performance Counters

        /// <summary>
        /// Store's performance counter set instance
        /// </summary>
        private FabricPerformanceCounterSetInstance perfCounterSetInstance = null;

        /// <summary>
        /// Performance counter for total disk size of the store
        /// </summary>
        private TStoreDiskSizeCounterWriter diskSizePerfCounterWriter = null;

        /// <summary>
        /// Performance counter for total number of items in the store
        /// </summary>
        private TStoreItemCountCounterWriter itemCountPerfCounterWriter = null;

        #endregion

        #region Metrics

        private TStoreMetricProvider metrics;

        private long lastRecordedDiskSize = 0;

        private long lastRecordedItemCount = 0;

        #endregion

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="TStore{TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner}"/> class.
        /// </summary>
        public TStore()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TStore{TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner}"/> class.
        /// </summary>
        /// <param name="storeBehavior">store behavior.</param>
        public TStore(StoreBehavior storeBehavior)
        {
            this.storeBehavior = storeBehavior;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TStore{TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner}"/> class.
        /// </summary>
        /// <param name="transactionalReplicator">transactional replicator.</param>
        /// <param name="name">name of the state provider.</param>
        /// <param name="storeBehavior">store behavior.</param>
        /// <param name="allowReadableSecondary">flag indicating if secondaries should support reads.</param>
        public TStore(
            ITransactionalReplicator transactionalReplicator,
            Uri name,
            StoreBehavior storeBehavior,
            bool allowReadableSecondary)
        {
            this.transactionalReplicator = transactionalReplicator;
            this.Name = name;
            this.storeBehavior = storeBehavior;
            this.isAlwaysReadable = allowReadableSecondary;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TStore{TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner}"/> class for unit tests.
        /// </summary>
        internal TStore(
            ITransactionalReplicator transactionalReplicator,
            IStatefulServicePartition partition,
            Guid partitionId,
            long replicaId,
            Uri serviceName,
            Uri name,
            StoreBehavior storeBehavior,
            bool allowReadableSecondary,
            long stateProviderId,
            string workDirectory)
        {
            this.Initialize(
                transactionalReplicator,
                partition,
                partitionId,
                replicaId,
                serviceName,
                name,
                storeBehavior,
                allowReadableSecondary,
                stateProviderId,
                workDirectory);
        }

        /// <summary>
        /// Gets the number of key/value pairs contained in the store.
        /// </summary>
        /// <remarks>
        /// This property does not have transactional semantics. 
        /// It represents the BEST EFFORT number of items in the store at the moment when the property was accessed.
        /// </remarks>
        public long Count
        {
            get
            {
                long countSnap = Interlocked.Read(ref this.count);
                Diagnostics.Assert(countSnap >= 0, this.traceType, "Count {0} cannot be negative.", countSnap);

                return countSnap;
            }
        }

        /// <summary>
        /// Callback used to fire async events when the TStore is being rebuilt: e.g. after full copy, recovery, restore, etc.
        /// </summary>
        public Func<IReliableState, NotifyDictionaryRebuildEventArgs<TKey, TValue>, Task> RebuildNotificationsAsyncCallback
        {
            get
            {
                return this.rebuildNotificationsAsyncCallback;
            }

            set
            {
                Diagnostics.Assert(this.rebuildNotificationsAsyncCallback == null, this.TraceType, "must be null");
                this.rebuildNotificationsAsyncCallback = value;
            }
        }

        /// <summary>
        /// Gets or sets the flag that indicates if sweep is enabled or not.
        /// </summary>
        /// <devnote> This is exposed for testability, it should be exposed as a config soon.</devnote>
        public bool EnableSweep{get; set;}

        /// <summary>
        /// Gets or sets the flag that indicates whether strict 2PL is enabled.
        /// </summary>
        /// <devnote> This is exposed for testability, it should be exposed as a config soon.</devnote>
        public bool EnableStrict2PL
        {
            get { return this.enableStrict2PL; }
            set { this.enableStrict2PL = value; }
        }

        /// <summary>
        /// Tracks values being loaded from the disk on demand.
        /// </summary>
        public LoadValueCounter LoadValueCounter { get; private set; }

        /// <summary>
        /// Task that tracks sweep in the background.
        /// </summary>
        public Task SweepTask { get; private set; }

        /// <summary>
        /// Used to cancel the background sweep task.
        /// </summary>
        public CancellationTokenSource SweepTaskCancellationSource {get; private set;}

        /// <summary>
        /// Used to cancel background consalidate/merge task.
        /// </summary>
        public CancellationTokenSource ConsolidateTaskCancellationSource { get; private set; }

        /// <summary>
        /// Flag that indicates if background consolidation is enabled.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        public bool EnableBackgroundConsolidation { get; set; }

        /// <summary>
        /// Get ot sets the sweep threshold.
        /// </summary>
        public int SweepThreshold
        {
            get
            {
                if(this.sweepThreshold == 0)
                {
                    this.sweepThreshold = TStoreConstants.DefaultSweepThreshold;
                }

                return this.sweepThreshold;
            }

            set
            {
                Diagnostics.Assert(this.sweepThreshold == -1, this.TraceType, "Sweep threshold cannot be set more than once {0}", this.sweepThreshold);
                this.sweepThreshold = value;
            }
        }

        /// <summary>
        /// Gets the lock manager.
        /// </summary>
        public LockManager LockManager 
        { 
            get
            {
                return this.lockManager;
            }
        }

        /// <summary>
        /// Exposed for testability only.
        /// </summary>
        public Task ConsolidationTask
        {
            get
            {
                return this.consolidationTask;
            }
        }

        /// <summary>
        /// Exposed for testability only.
        /// </summary>
        public CopyManager CopyManager
        {
            get
            {
                return this.copyManager;
            }
        }

        /// <summary>
        /// Introduces delay during consolidation
        /// </summary>
        /// <devnote>Exposed for testability only</devnote>
        internal TaskCompletionSource<bool> TestDelayOnConsolidation
        {
            get;
            set;
        }

        #region INotifyDictionaryChanged<TKey, TValue> Members

        /// <summary>
        /// Notification handler for store changes.
        /// </summary>
        public event EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> DictionaryChanged;

        #endregion

        #region ILoadMetricProvider Members

        /// <summary>
        /// Provides built-in metrics for the store.
        /// </summary>
        public IReadOnlyDictionary<string, long> LoadMetrics
        {
            // TODO: Currently this method is not called by anyone in the production code.
            // Sizes should be a property in the metadatamanager or filemetadata to avoid calling FileSystem.
            get
            {
                var metrics = new Dictionary<string, long>();

                long onDiskSizeInBytes = 0;
                if (this.CurrentMetadataTable != null)
                {
                    onDiskSizeInBytes = this.ComputeOnDiskLoadMetric();
                }

                metrics.Add(OnDiskSizeInBytesMetricName, onDiskSizeInBytes);
                metrics.Add(OnDiskSizeInMBMetricName, onDiskSizeInBytes / (1024*1024));
                return metrics;
            }
        }

        #endregion

        #region IStoreCopyProvider Members

        /// <summary>
        /// Gets the current master table.
        /// </summary>
        async Task<MetadataTable> IStoreCopyProvider.GetMetadataTableAsync()
        {
            try
            {
                await this.MetadataTableLock.WaitAsync().ConfigureAwait(false);

                var snapshotMetadataTable = this.CurrentMetadataTable;
                snapshotMetadataTable.AddRef();

                return snapshotMetadataTable;
            }
            finally
            {
                this.MetadataTableLock.Release();
            }
        }

        /// <summary>
        /// Gets the working directory for the store.
        /// </summary>
        string IStoreCopyProvider.WorkingDirectory
        {
            get { return this.workingDirectory; }
        }

        #endregion

        /// <summary>
        /// Store name.
        /// </summary>
        public Uri Name { get; private set; }

        public MergeHelper MergeHelper { get; private set; }

        public string TraceType 
        {
            get
            {
                return this.traceType;
            }
        }

        /// <summary>
        /// Gets or sets initialization context for the store.
        /// </summary>
        public byte[] InitializationContext
        {
            get
            {
                var initializationParameters = new StoreInitializationParameters(this.storeBehavior, this.isAlwaysReadable);
                return initializationParameters.ToByteArray();
            }
        }

        /// <summary>
        /// Differential state.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        internal TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> DeltaDifferentialState { get; set; }

        /// <summary>
        /// File id initialized to zero, needs to be rehydrated following recovery.
        /// </summary>
        internal uint FileId { get; set; }

        /// <summary>
        /// Gets or sets time stamp to track time when checkpoint files are written.
        /// </summary>
        internal long LastCheckpointFileTimeStamp
        {
            get
            {
                return this.lastCheckpointFileTimeStamp;
            }
        }

        internal bool ShouldLoadValuesInRecovery
        {
            get { return this.shouldLoadValuesInRecovery; } 
            set { this.shouldLoadValuesInRecovery = value; }
        }

        internal int MaxNumberOfInflightValueRecoveryTasks
        {
            get { return this.numberOfInflightValueRecoveryTasks; }
            set { this.numberOfInflightValueRecoveryTasks = value; }
        }

        internal MetadataTable CurrentMetadataTable { get; set; }

        internal MetadataTable NextMetadataTable { get; set; }

        internal MetadataTable MergeMetadataTable { get; set; }

        private ConcurrentDictionary<FileMetadata, bool> filesToBeDeleted = new ConcurrentDictionary<FileMetadata, bool>();

        internal ConcurrentDictionary<FileMetadata, bool> FilesToBeDeleted
        {
            get
            {
                return this.filesToBeDeleted;
            }
        }

        /// <summary>
        /// Consolidating state that represents merging.
        /// </summary>
        internal ConsolidationManager<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> ConsolidationManager { get; set; }

        /// <summary>
        /// Contains versions maintained to support snapshot reads after they have been removed from differential and consolidated state.
        /// </summary>
        /// <devnote >Exposed for testability only.</devnote>
        internal SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> SnapshotContainer { get; set; }

        /// <summary>
        /// Gets or sets the current master table file name.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal string CurrentDiskMetadataFilePath { get; private set; }

        /// <summary>
        /// Gets or sets the next master table file name.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal string TmpDiskMetadataFilePath { get; private set; }

        /// <summary>
        /// Gets or sets the backup master table file name.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal string BkpDiskMetadataFilePath { get; private set; }

        /// <summary>
        /// Gets or sets the next master table file name.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal string TmpDiskMetadataFileShortName { get; private set; }

        /// <summary>
        /// Gets or sets the backup master table file name.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal string BkpDiskMetadataFileShortName { get; private set; }

        /// <summary>
        /// Gets or sets the key serializer.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal IStateSerializer<TKey> KeyConverter
        {
            get { return this.keyConverter; }

            set { this.keyConverter = value; }
        }

        /// <summary>
        /// Gets or sets the value serializer.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal IStateSerializer<TValue> ValueConverter
        {
            get { return this.valueConverter; }

            set { this.valueConverter = value; }
        }

        /// <summary>
        /// Gets or sets the replica id.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal long ReplicaId
        {
            get { return this.replicaId; }
            set { this.replicaId = value; }
        }

        /// <summary>
        /// Gets or sets the partition id.
        /// </summary>
        /// <devnote>This is exposed for testability only.</devnote>
        internal Guid PartitionId
        {
            get { return this.partitionId; }
            set { this.partitionId = value; }
        }

        internal string WorkingDirectory
        {
            get
            {
                return this.workingDirectory;
            }
        }

        /// <summary>
        /// Key comparer.
        /// </summary>
        internal TKeyComparer KeyComparer
        {
            get
            {
                return this.keyComparer;
            }
        }

        internal bool IsValueAReferenceType
        {
            get
            {
                return this.isValueAReferenceType;
            }
        }

        internal long StateProviderId
        {
            get
            {
                return this.stateProviderId;
            }
        }

        internal ITransactionalReplicator TransactionalReplicator
        {
            get
            {
                return this.transactionalReplicator;
            }
        }

        /// <summary>
        /// Gets differential state
        /// </summary>
        /// <devote>Exposed for testability.</devote>
        internal TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> DifferentialState { get; set; }

        #region IStoreTransactionProvider Members

        /// <summary>
        /// Creates a read/write store transaction.
        /// </summary>
        /// <param name="replicatorTransaction">Replicator transaction associated with the writes of this store transaction.</param>
        /// <returns></returns>
        public IStoreReadWriteTransaction CreateStoreReadWriteTransaction(ReplicatorTransaction replicatorTransaction)
        {
            // Check arguments.
            Requires.ThrowIfNull(replicatorTransaction, "replicatorTransaction");

            var rwtx = new StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(
                replicatorTransaction.Id,
                replicatorTransaction,
                this,
                this.traceType,
                this.inflightReadWriteStoreTransactions,
                false,
                this.enableStrict2PL);
            ((IDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>>) this.inflightReadWriteStoreTransactions).Add(
                rwtx.Id,
                rwtx);

            // Register for clear locks.
            replicatorTransaction.AddLockContext(rwtx);

            // Reset default isolation level, if needed.
            if (this.storeBehavior == StoreBehavior.SingleVersion)
            {
                rwtx.Isolation = StoreTransactionReadIsolationLevel.ReadCommittedSnapshot;
            }

            return rwtx;
        }

        /// <summary>
        /// Creates or retrieves a reader/writer transaction.
        /// </summary>
        /// <param name="replicatorTransaction">Associated replicator transaction.</param>
        /// <returns></returns>
        public ConditionalValue<IStoreReadWriteTransaction> CreateOrFindTransaction(ReplicatorTransaction replicatorTransaction)
        {
            Requires.ThrowIfNull(replicatorTransaction, "replicatorTransaction");

            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx = null;
            this.inflightReadWriteStoreTransactions.TryGetValue(replicatorTransaction.Id, out rwtx);
            IStoreReadWriteTransaction result = rwtx;
            if (null == result)
            {
                result = this.CreateStoreReadWriteTransaction(replicatorTransaction);
            }

            return new ConditionalValue<IStoreReadWriteTransaction>(null != rwtx, result);
        }

        #endregion

        #region IStore<TKey, TValue, TKeyComparer, TKeyEqualityComparer> Members

        /// <summary>
        /// Adds a new key and value into the store.
        /// </summary>
        /// <param name="transaction">Store transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>g4462
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>The version of the newly inserted item.</returns>
        public async Task AddAsync(IStoreWriteTransaction transaction, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (IsNull(key))
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key and value bytes.
            var keyBytes = this.GetKeyBytes(key);
            var valueBytes = this.GetValueBytes(default(TValue), value);
            var valueSize = this.GetValueSize(valueBytes);

            // Compute redo and undo information. This can be done outside the lock.
            MetadataOperationData<TKey, TValue> metadata = new MetadataOperationData<TKey, TValue>(key, value, TStoreConstants.SerializedVersion, StoreModificationType.Add, keyBytes, rwtx.Id, this.traceType);
            RedoUndoOperationData redo = new RedoUndoOperationData(valueBytes, null, this.traceType);
            IOperationData undo = null;

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
            {
                this.metrics.KeyValueSizeMetric.Add(keyBytes.Count, valueSize);
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await
                    this.AcquireKeyModificationLocksAsync(
                        "AddAsync",
                        this.lockManager,
                        StoreModificationType.Add,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.AddAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            // Check to see if this key was already added as part of this store transaction or by some previous store transaction.
            if (!this.CanKeyBeAdded(rwtx, key))
            {
                FabricEvents.Events.Error_AddAsync(this.traceType, keyLockResourceNameHash, rwtx.Id);

                // Key already exists.
                throw new ArgumentException(SR.Error_Key);
            }

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redo, undo, millisecondsRemaining, cancellationToken).ConfigureAwait(false);

                FabricEvents.Events.AcceptAddAsync(
                    this.traceType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    !IsNull(value) ? value.GetHashCode() : int.MinValue);

                // Add the change to the store transaction write-set.
                TVersionedItem<TValue> insertedVersion = new TInsertedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));
                rwtx.Component.Add(false, key, insertedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.AddAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }
        }

        /// <summary>
        /// Tries to add a new key and value into the store.
        /// </summary>
        /// <param name="transaction">Store transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>The version of the newly inserted item if insertion succeeds. Nothing if the key already exists.</returns>
        public async Task<bool> TryAddAsync(IStoreWriteTransaction transaction, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (IsNull(key))
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key and value bytes.
            var keyBytes = this.GetKeyBytes(key);
            var valueBytes = this.GetValueBytes(default(TValue), value);
            var valueSize = this.GetValueSize(valueBytes);

            // Compute redo and undo information. This can be done outside the lock.
            MetadataOperationData<TKey, TValue> metadata = new MetadataOperationData<TKey, TValue>(key, value, TStoreConstants.SerializedVersion, StoreModificationType.Add, keyBytes, rwtx.Id, this.traceType);
            RedoUndoOperationData redo = new RedoUndoOperationData(valueBytes, null, this.traceType);
            IOperationData undo = null;

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
            {
                this.metrics.KeyValueSizeMetric.Add(keyBytes.Count, valueSize);
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await
                    this.AcquireKeyModificationLocksAsync(
                        "AddAsync",
                        this.lockManager,
                        StoreModificationType.Add,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.TryAddAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            // Check to see if this key was already added as part of this store transaction or by some previous store transaction.
            if (!this.CanKeyBeAdded(rwtx, key))
            {
                FabricEvents.Events.Error_AddAsync(this.traceType, keyLockResourceNameHash, rwtx.Id);

                // Key already exists.
                return false;
            }

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redo, undo, millisecondsRemaining, cancellationToken).ConfigureAwait(false);

                FabricEvents.Events.AcceptAddAsync(
                    this.traceType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    !IsNull(value) ? value.GetHashCode() : int.MinValue);

                // Add the change to the store transaction write-set.
                TVersionedItem<TValue> insertedVersion = new TInsertedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));
                rwtx.Component.Add(false, key, insertedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("{0}@{1}", "TStore.TryAddAsync", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            // Return the appropriate version of the modification.
            return true;
        }

        /// <summary>
        /// Updates the value for a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>Version of the newly updated key/value pair.</returns>
        public async Task UpdateAsync(IStoreWriteTransaction transaction, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var updated = await this.ConditionalUpdateAsync(transaction, key, value, 0, false, timeout, cancellationToken).ConfigureAwait(false);
            if (!updated)
            {
                throw new ArgumentException(SR.Error_Key);
            }
        }

        /// <summary>
        /// Removes a given key and value from the store.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>True, existent version, and existent value of the item with that key, if it exists.</returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(IStoreWriteTransaction transaction, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ConditionalRemoveAsync(transaction, key, 0, false, timeout, cancellationToken);
        }

        /// <summary>
        /// Removes all data from the store.
        /// </summary>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public async Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Create atomic operation.
            var atomicOperation = this.transactionalReplicator.CreateAtomicRedoOperation();

            // Create write only store transaction associated with the atomic transaction.
            var rwtx = new StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(
                atomicOperation.Id,
                atomicOperation,
                this,
                this.traceType,
                this.inflightReadWriteStoreTransactions,
                false,
                this.enableStrict2PL);
            ((IDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>>) this.inflightReadWriteStoreTransactions).Add(
                rwtx.Id,
                rwtx);

            // Compute redo information. This can be done outside the lock. There is no undo for this operation.
            MetadataOperationData metaData = new MetadataOperationData(
                TStoreConstants.SerializedVersion,
                StoreModificationType.Clear,
                new ArraySegment<byte>(),
                rwtx.Id);
            RedoUndoOperationData redo = new RedoUndoOperationData(null, null, this.traceType);

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Compute locking timeout. For simplicity, given equal lock timeout for each store component.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var lockTimeout = milliseconds;
            var start = DateTime.UtcNow;
            var end = DateTime.UtcNow;
            var duration = TimeSpan.MaxValue;

            try
            {
                start = DateTime.UtcNow;

                var lockMode = LockMode.Exclusive;

                // Acquire exclusive lock on store component.
                await rwtx.AcquirePrimeLocksAsync(this.lockManager, lockMode, lockTimeout, true).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Compute left-over timeout, if needed.
                if (Timeout.Infinite != milliseconds && 0 != milliseconds)
                {
                    end = DateTime.UtcNow;
                    duration = end.Subtract(start);
                    if (duration.TotalMilliseconds < 0)
                    {
                        duration = TimeSpan.Zero;
                    }
                    if (milliseconds < duration.TotalMilliseconds)
                    {
                        // Locks could not be acquired in the given timeout.
                        throw new TimeoutException(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                SR.LockTimeout_TStore_TableLock,
                                lockMode,
                                this.traceType,
                                milliseconds,
                                rwtx.Id));
                    }
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.ClearAsync@{0}", this.traceType), e, "store={0} ", "acquiring prime locks");
                Diagnostics.TraceIfNot(e is TimeoutException || e is OperationCanceledException || e is TransactionFaultedException, traceType, "unexpected exception {0}", e.GetType());

                // Release all acquired store locks if any.
                rwtx.Dispose();

                // Rethrow inner exception.
                throw e;
            }

            // At this time, there are no other transactions (read or read/write) active in the store.
            try
            {
                // Fix-up store transaction for proper apply/unlock.
                rwtx.IncrementPrimaryOperationCount();

                // Replicate. There is no undo information and no lock context. This call is also not being retried.
                await atomicOperation.AddOperationAsync(metaData, redo, null, this.stateProviderId).ConfigureAwait(false);

                // In-memory state (differential, consolidated, master table, etc.) is reset in apply.
                FabricEvents.Events.AcceptClearAsync(this.traceType, rwtx.Id);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.ClearAsync@{0}", this.traceType), e, "replication");
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || this.IsRetryableReplicationException(e) || e is OperationCanceledException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }
            finally
            {
                // Release all acquired store locks if any.
                rwtx.Dispose();
            }
        }

        /// <summary>
        /// Determines whether the store contains the specified key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>1. Snapshot - last committed value according to snapshot isolation visibility rules as of when the store transaction started.</returns>
        /// <returns>2. ReadCommittedSnapshot - last committed value according to snapshot isolation visibility rules as of when this call is made.</returns>
        /// <returns>3. ReadCommitted - last committed value using short duration locks (locks kept for the duration of the call).</returns>
        /// <returns>4. ReadRepeatable - last committed value using long duration locks (locks kept until the end of transaction).</returns>
        public async Task<bool> ContainsKeyAsync(IStoreTransaction transaction, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var result = await this.ConditionalGetAsync(transaction, key, ReadMode.Off, timeout, cancellationToken).ConfigureAwait(false);
            return result.HasValue;
        }

        /// <summary>
        /// Gets the value for a given key. The read isolation level is observed.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public Task<ConditionalValue<TValue>> GetAsync(IStoreTransaction transaction, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Perform key seek.
            return this.GetAsync(transaction, key, ReadMode.CacheResult, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets the value for a given key. The read isolation level is observed.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns></returns>
        public Task<ConditionalValue<TValue>> GetAsync(IStoreTransaction transaction, TKey key, ReadMode readMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Perform key seek.
            return this.ConditionalGetAsync(transaction, key, readMode, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets the value for a given key. The read isolation level is observed.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public async Task<ConditionalValue<KeyValuePair<TKey, TValue>>> GetNextAsync(IStoreTransaction transaction, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            if (rwtx == null || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            var minKey = key;

            // Perform next key seek.
            while (true)
            {
                // Get next key from store transaction write-set.
                var nextKeyInStoreTransaction = new ConditionalValue<TKey>();
                if (!rwtx.IsWriteSetEmpty)
                {
                    nextKeyInStoreTransaction = rwtx.Component.ReadNext(minKey);
                }

                // Get next key from current state.
                var nextKeyInDifferentialState = this.DifferentialState.ReadNext(minKey);
                var nextKeyInConsolidatedState = this.ConsolidationManager.ReadNext(minKey);
                if (!nextKeyInStoreTransaction.HasValue && !nextKeyInDifferentialState.HasValue && !nextKeyInConsolidatedState.HasValue)
                {
                    // No more keys.
                    break;
                }

                // Compute min key.
                if (nextKeyInStoreTransaction.HasValue)
                {
                    minKey = nextKeyInStoreTransaction.Value;
                    if (nextKeyInDifferentialState.HasValue)
                    {
                        minKey = (0 < this.keyComparer.Compare(minKey, nextKeyInDifferentialState.Value)) ? nextKeyInDifferentialState.Value : minKey;
                        if (nextKeyInConsolidatedState.HasValue)
                        {
                            minKey = (0 < this.keyComparer.Compare(minKey, nextKeyInConsolidatedState.Value)) ? nextKeyInConsolidatedState.Value : minKey;
                        }
                    }
                    else
                    {
                        if (nextKeyInConsolidatedState.HasValue)
                        {
                            minKey = (0 < this.keyComparer.Compare(minKey, nextKeyInConsolidatedState.Value)) ? nextKeyInConsolidatedState.Value : minKey;
                        }
                    }
                }
                else
                {
                    if (nextKeyInDifferentialState.HasValue)
                    {
                        minKey = nextKeyInDifferentialState.Value;
                        if (nextKeyInConsolidatedState.HasValue)
                        {
                            minKey = (0 < this.keyComparer.Compare(minKey, nextKeyInConsolidatedState.Value)) ? nextKeyInConsolidatedState.Value : minKey;
                        }
                    }
                    else
                    {
                        minKey = nextKeyInConsolidatedState.Value;
                    }
                }

                // Get the next key.
                var resultMinKey = await this.GetAsync(transaction, minKey, timeout, cancellationToken).ConfigureAwait(false);
                if (resultMinKey.HasValue)
                {
                    return new ConditionalValue<KeyValuePair<TKey, TValue>>(true, new KeyValuePair<TKey, TValue>(minKey, resultMinKey.Value));
                }
            }

            // No more keys
            return new ConditionalValue<KeyValuePair<TKey, TValue>>();
        }

        /// <summary>
        /// Removes a given key and value from the store.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="conditionalCheckVersion">Specifies the desired version of the key to remove.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <returns>Existent value of the item with that key, if it exists.</returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(IStoreWriteTransaction transaction, TKey key, long conditionalCheckVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ConditionalRemoveAsync(transaction, key, conditionalCheckVersion, true, timeout, cancellationToken);
        }

        /// <summary>
        /// Updates the value for a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="conditionalCheckVersion">Desired version of the key to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>Version of the newly updated key/value pair.</returns>
        public Task<bool> TryUpdateAsync(
            IStoreWriteTransaction transaction, TKey key, TValue value, long conditionalCheckVersion, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ConditionalUpdateAsync(transaction, key, value, conditionalCheckVersion, true, timeout, cancellationToken);
        }

        /// <summary>
        /// Conditionally updates the value of a given key based on a previously obtained version.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="newValue">Value to update.</param>
        /// <param name="comparisonValue">Value to use for the conditional check.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>True and new version of the newly updated key/value pair, if conditional check succeeded.
        /// False and existent version, if the conditional check failed.</returns>
        public async Task<bool> TryUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            TValue newValue,
            TValue comparisonValue,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key and value bytes.
            var keyBytes = this.GetKeyBytes(key);
            var valueBytes = this.GetValueBytes(default(TValue), newValue);
            var valueSize = this.GetValueSize(valueBytes);

            // Compute redo/undo information. This part can be done outside the lock.
            MetadataOperationData<TKey, TValue> metadata = new MetadataOperationData<TKey, TValue>(key, newValue, TStoreConstants.SerializedVersion, StoreModificationType.Update, keyBytes, rwtx.Id, this.traceType);
            RedoUndoOperationData redoUpdate = new RedoUndoOperationData(valueBytes, null, this.traceType);

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
            {
                this.metrics.KeyValueSizeMetric.Update(valueSize);
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await this.AcquireKeyModificationLocksAsync(
                        "ConditionalUpdateAsync",
                        this.lockManager,
                        StoreModificationType.Update,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalUpdateAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            StoreComponentReadResult<TValue> readResult = await this.TryGetValueForReadWriteTransactionAsync(rwtx, key, ReadMode.ReadValue, cancellationToken).ConfigureAwait(false);
            TVersionedItem<TValue> versionedItem = readResult.VersionedItem;
            bool isVersionedItemFound = readResult.HasValue;

            // An item that does not exist, cannot be updated.
            if (isVersionedItemFound == false || versionedItem.Kind == RecordKind.DeletedVersion)
            {
                // Key does not exist.
                FabricEvents.Events.Error_ConditionalUpdateAsync(this.traceType, keyLockResourceNameHash, rwtx.Id, "keydoesnotexist");
                return false;
            }

            this.AssertIfUnexpectedVersionedItemValue(readResult, ReadMode.ReadValue);

            try
            {
                // Check to see if any update is required.
                if (!EqualityComparer<TValue>.Default.Equals(readResult.UserValue, comparisonValue))
                {
                    // Nothing to do.
                    FabricEvents.Events.Skip_ConditionalUpdateAsync(
                        this.traceType,
                        keyLockResourceNameHash,
                        null != readResult.UserValue ? readResult.UserValue.GetHashCode() : int.MinValue,
                        null != newValue ? newValue.GetHashCode() : int.MinValue,
                        rwtx.Id);

                    // Key did not change.
                    return false;
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalUpdateAsync@{0}", this.traceType), e, "equality comparer key={0} ", keyLockResourceNameHash);

                // Terminate store transaction.
                this.FixupDanglingStoreTransactionIfNeeded(rwtx);

                // Rethrow
                throw e;
            }

            // Compute redo/undo information. This cannot be done outside the lock because we need the previous value.
            IOperationData undoUpdate = null;

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redoUpdate, undoUpdate, millisecondsRemaining, CancellationToken.None).ConfigureAwait(false);

                FabricEvents.Events.AcceptConditionalUpdateAsync(
                    this.traceType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    null != newValue ? newValue.GetHashCode() : int.MinValue);

                // Add the change to the store transaction write-set.
                var updatedVersion = new TUpdatedItem<TValue>(newValue, valueSize, this.CanItemBeSweepedToDisk(newValue));
                rwtx.Component.Add(false, key, updatedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalUpdateAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            return true;
        }

        /// <summary>
        /// Gets the value for an existent key or adds the key and value if key does not exist.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="valueFactory">Delegate that produces the value to insert. This delegate will be called under lock.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>If the key does not exist, it returns the version and the value of the newly inserted key/value pair.
        /// If the key already exists, it returns the version and value of the existent key/value pair.</returns>
        /// <returns></returns>
        public async Task<TValue> GetOrAddAsync(
            IStoreWriteTransaction transaction, TKey key, Func<TKey, TValue> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            if (null == valueFactory)
            {
                throw new ArgumentNullException(SR.Error_ValueFactory);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key bytes.
            var keyBytes = this.GetKeyBytes(key);

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int timeoutLeftOver;

            try
            {
                FabricEvents.Events.GetOrAddAsync(this.traceType, (byte) LockMode.Update, keyLockResourceNameHash, rwtx.Id, milliseconds);

                // Initialize store transaction.
                var millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                var start = DateTime.UtcNow;

                // Acquire required locks for this operation.
                await rwtx.LockAsync(this.lockManager, keyLockResourceNameHash, LockMode.Update, millisecondsRemaining).ConfigureAwait(false);

                var end = DateTime.UtcNow;

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Compute left-over timeout, if needed.
                timeoutLeftOver = millisecondsRemaining;
                if (Timeout.Infinite != timeoutLeftOver && 0 != timeoutLeftOver)
                {
                    var duration = end.Subtract(start);
                    if (duration.TotalMilliseconds < 0)
                    {
                        duration = TimeSpan.Zero;
                    }

                    if (timeoutLeftOver < duration.TotalMilliseconds)
                    {
                        timeoutLeftOver = 0;
                    }
                    else
                    {
                        // Next use left-over timeout.
                        timeoutLeftOver = timeoutLeftOver - (int)duration.TotalMilliseconds;
                    }
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.GetOrAddAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            // Check to see if this key was already added.
            StoreComponentReadResult<TValue> readResult = await this.TryGetValueForReadWriteTransactionAsync(rwtx, key, ReadMode.CacheResult, cancellationToken).ConfigureAwait(false);
            TVersionedItem<TValue> versionedItem = readResult.VersionedItem;
            bool isVersionedItemFound = readResult.HasValue;

            if (isVersionedItemFound == true && (versionedItem is TInsertedItem<TValue> || versionedItem is TUpdatedItem<TValue>))
            {
                if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
                {
                    this.metrics.ReadWriteMetric.RegisterRead();
                }

                // Re-certify the read.
                var lockHints = rwtx.LockingHints;
                rwtx.LockingHints = LockingHints.UpdateLock;
                this.ThrowIfNotReadable(rwtx);
                rwtx.LockingHints = lockHints;

                // The key was already added.
                // Value has already been loaded above.
                this.AssertIfUnexpectedVersionedItemValue(readResult, ReadMode.CacheResult);
                return readResult.UserValue;
            }

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            try
            {
                // Acquire required locks for this operation.
                await this.AcquireKeyModificationLocksAsync(
                        "GetOrAddAsync",
                        this.lockManager,
                        StoreModificationType.Add,
                        keyLockResourceNameHash,
                        rwtx,
                        timeoutLeftOver).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.GetOrAddAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            // Execute delegate to obtain the value.
            var value = default(TValue);
            ArraySegment<byte>[] valueBytes;
            int valueSize;

            try
            {
                value = valueFactory(key);
                valueBytes = this.GetValueBytes(default(TValue), value);
                valueSize = this.GetValueSize(valueBytes);
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.GetOrAddAsync@{0}", this.traceType), e, "value factory key={0} ", keyLockResourceNameHash);

                // Terminate store transaction.
                this.FixupDanglingStoreTransactionIfNeeded(rwtx);

                // Rethrow
                throw e;
            }

            // The key can now be added.
            MetadataOperationData<TKey, TValue> metadata = new MetadataOperationData<TKey, TValue>(key, value, TStoreConstants.SerializedVersion, StoreModificationType.Add, keyBytes, rwtx.Id, this.traceType);
            RedoUndoOperationData redo = new RedoUndoOperationData(valueBytes, null, this.traceType);
            IOperationData undo = null;

            if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
            {
                this.metrics.KeyValueSizeMetric.Add(keyBytes.Count, valueSize);
            }

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redo, undo, timeoutLeftOver, cancellationToken).ConfigureAwait(false);

                FabricEvents.Events.AcceptGetOrAddAsync(
                    this.traceType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    null != value ? value.GetHashCode() : int.MinValue);

                // Add the change to the store transaction write-set.
                var insertedVersion = new TInsertedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));
                rwtx.Component.Add(false, key, insertedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.GetOrAddAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            return value;
        }

        /// <summary>
        /// Gets the value for an existent key or adds the key and value if key does not exist.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert.</param>
        /// <param name="value">Value to insert.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>If the key does not exist, it returns the version and the value of the newly inserted key/value pair.
        /// If the key already exists, it returns the version and value of the existent key/value pair.</returns>
        /// <returns></returns>
        public Task<TValue> GetOrAddAsync(IStoreWriteTransaction transaction, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetOrAddAsync(transaction, key, (keyArg) => value, timeout, cancellationToken);
        }

        /// <summary>
        /// Updates the value for a given key and returns its value before the update. Blind update.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="updateValueFactory">Delegate that provides the value to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>New version and old value.</returns>
        public async Task<TValue> UpdateWithOutputAsync(
            IStoreWriteTransaction transaction, TKey key, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;

            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key bytes.
            var keyBytes = this.GetKeyBytes(key);

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await
                    this.AcquireKeyModificationLocksAsync(
                        "UpdateWithOutputAsync",
                        this.lockManager,
                        StoreModificationType.Update,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.UpdateWithOutputAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            StoreComponentReadResult<TValue> readResult = await this.TryGetValueForReadWriteTransactionAsync(rwtx, key, ReadMode.ReadValue, cancellationToken).ConfigureAwait(false);
            TVersionedItem<TValue> versionedItem = readResult.VersionedItem;
            bool isVersionedItemFound = readResult.HasValue;

            // An item that does not exist, cannot be updated.
            if (isVersionedItemFound == false || versionedItem.Kind == RecordKind.DeletedVersion)
            {
                // Key does not exist.
                FabricEvents.Events.Error_UpdateWithOutputAsync(this.traceType, keyLockResourceNameHash, rwtx.Id, "keydoesnotexist");
                throw new ArgumentException(SR.Error_Key);
            }

            this.AssertIfUnexpectedVersionedItemValue(readResult, ReadMode.ReadValue);

            // Execute delegate to obtain the value. Serialize old and new value.
            var value = default(TValue);
            ArraySegment<byte>[] valueBytes;
            int valueSize;

            try
            {
                // Get new value.
                value = updateValueFactory(key, readResult.UserValue);
                valueBytes = this.GetValueBytes(default(TValue), value);
                valueSize = this.GetValueSize(valueBytes);
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(
                    string.Format("TStore.UpdateWithOutputAsync@{0}", this.traceType),
                    e,
                    "value factory or equality comparer key={0} ",
                    keyLockResourceNameHash);

                // Terminate store transaction.
                this.FixupDanglingStoreTransactionIfNeeded(rwtx);

                // Rethrow
                throw e;
            }

            // Compute redo/undo information. This cannot be done outside the lock because we need the previous value.
            MetadataOperationData metadata = new MetadataOperationData<TKey, TValue>(key, value, TStoreConstants.SerializedVersion, StoreModificationType.Update, keyBytes, rwtx.Id, this.traceType);
            IOperationData undoUpdate = null;
            RedoUndoOperationData redoUpdate = new RedoUndoOperationData(valueBytes, null, this.traceType);

            if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
            {
                this.metrics.KeyValueSizeMetric.Update(valueSize);
            }

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redoUpdate, undoUpdate, millisecondsRemaining, CancellationToken.None).ConfigureAwait(false);

                FabricEvents.Events.AcceptUpdateWithOutputAsync(
                    this.traceType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    null != value ? value.GetHashCode() : int.MinValue);

                // Add the changes to the store transaction write-set.
                var updatedVersion = new TUpdatedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));
                rwtx.Component.Add(false, key, updatedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.UpdateWithOutputAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            // Return the appropriate version of the modification.
            return readResult.UserValue;
        }

        /// <summary>
        /// Updates the value for a given key and returns its value before the update. Blind update.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>New version and old value.</returns>
        public Task<TValue> UpdateWithOutputAsync(
            IStoreWriteTransaction transaction, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.UpdateWithOutputAsync(transaction, key, (keyArg, valueArg) => value, timeout, cancellationToken);
        }

        /// <summary>
        /// Inserts or updates a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="addValue"></param>
        /// <param name="timeout"></param>
        /// <param name="updateValueFactory">Delegate that creates the value to update.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>If the key exists, it updates its value to the new value. If the key does not exist, it is inserted.
        /// Returns the version of the exitent or newly added key.</returns>
        public Task<TValue> AddOrUpdateAsync(
            IStoreWriteTransaction transaction, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.AddOrUpdateAsync(transaction, key, keyArg => addValue, updateValueFactory, timeout, cancellationToken);
        }

        /// <summary>
        /// Inserts or updates a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="addValue">Value to insert/update.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <param name="updateValue"></param>
        /// <param name="timeout"></param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>If the key exists, it updates its value to the new value. If the key does not exist, it is inserted.
        /// Returns the version of the exitent or newly added key.</returns>
        public Task AddOrUpdateAsync(
            IStoreWriteTransaction transaction, TKey key, TValue addValue, TValue updateValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddOrUpdateAsync(transaction, key, addValue, (keyArg, valueArg) => updateValue, timeout, cancellationToken);
        }

        /// <summary>
        /// Inserts or updates a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to insert/update.</param>
        /// <param name="addValueFactory">Delegate that creates the value to add.</param>
        /// <param name="updateValueFactory">Delegate that creates the value to update.</param>
        /// <param name="timeout"></param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>If the key exists, it updates its value to the new value. If the key does not exist, it is inserted.
        /// Returns the version of the exitent or newly added key.</returns>
        public async Task<TValue> AddOrUpdateAsync(
            IStoreWriteTransaction transaction,
            TKey key,
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            if (null == addValueFactory)
            {
                throw new ArgumentNullException(SR.Error_AddValueFactory);
            }

            if (null == updateValueFactory)
            {
                throw new ArgumentNullException(SR.Error_UpdateValueFactory);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key bytes.
            var keyBytes = this.GetKeyBytes(key);

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await this.AcquireKeyModificationLocksAsync(
                        "AddOrUpdateAsync",
                        this.lockManager,
                        StoreModificationType.Add,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.AddOrUpdateAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            // Check to see if the key can be added or it needs to be updated.
            string operationType;
            TValue value;
            TValue oldValue;
            StoreModificationType modification;
            bool isAdded;

            if (this.CanKeyBeAdded(rwtx, key))
            {
                // Key will be added.
                operationType = "add";
                oldValue = default(TValue);
                modification = StoreModificationType.Add;
                isAdded = true;
            }
            else
            {
                // Key will be updated.
                StoreComponentReadResult<TValue> readResult = await this.TryGetValueForReadWriteTransactionAsync(rwtx, key, ReadMode.ReadValue, cancellationToken).ConfigureAwait(false);
                TVersionedItem<TValue> versionedItem = readResult.VersionedItem;
                bool isVersionedItemFound = readResult.HasValue;

                Diagnostics.Assert(
                    isVersionedItemFound == true && versionedItem.Kind != RecordKind.DeletedVersion,
                    this.TraceType,
                    "key must either be added or updated");

                // Ensure that the value is in-memory
                this.AssertIfUnexpectedVersionedItemValue(readResult, ReadMode.ReadValue);

                operationType = "update";
                oldValue = readResult.UserValue;
                modification = StoreModificationType.Update;
                isAdded = false;
            }
            
            try
            {
                // Execute delegate to obtain the value.
                value = isAdded
                    ? addValueFactory(key)
                    : updateValueFactory(key, oldValue);
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(
                    string.Format("TStore.AddOrUpdateAsync@{0}", this.traceType),
                    e,
                    "{0} value factory key={1} ",
                    operationType,
                    keyLockResourceNameHash);

                // Terminate store transaction.
                this.FixupDanglingStoreTransactionIfNeeded(rwtx);

                // Rethrow
                throw e;
            }

            var valueBytes = this.GetValueBytes(default(TValue), value);
            var valueSize = this.GetValueSize(valueBytes);

            var metadataOperationData = new MetadataOperationData<TKey, TValue>(key, value, TStoreConstants.SerializedVersion, modification, keyBytes, rwtx.Id, this.traceType);
            var redo = new RedoUndoOperationData(valueBytes, null, this.traceType);
            IOperationData undo = null;

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadataOperationData, redo, undo, millisecondsRemaining, cancellationToken).ConfigureAwait(false);

                FabricEvents.Events.AcceptAddOrUpdateAsync(
                    this.traceType,
                    operationType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    null != value ? value.GetHashCode() : int.MinValue);

                // Add the change to the store transaction write-set.
                TVersionedItem<TValue> versionedItem;
                if (isAdded)
                {
                    versionedItem = new TInsertedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));

                    if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
                    {
                        this.metrics.KeyValueSizeMetric.Add(keyBytes.Count, valueSize);
                    }
                }
                else
                {
                    versionedItem = new TUpdatedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));

                    if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
                    {
                        this.metrics.KeyValueSizeMetric.Update(valueSize);
                    }
                }

                rwtx.Component.Add(false, key, versionedItem);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.AddOrUpdateAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            return value;
        }

        #endregion

        /// <summary>
        /// Creates an enumerable over the store.
        /// </summary>
        /// <param name="storeTransaction">Store transaction to use.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, bool isOrdered, ReadMode readMode)
        {
            return this.CreateEnumerableAsync(storeTransaction, key => true, isOrdered, readMode);
        }

        /// <summary>
        /// Creates an enumerable over the store.
        /// </summary>
        /// <param name="storeTransaction">Store transaction to use.</param>
        /// <param name="firstKey">First key in the range.</param>
        /// <param name="lastKey">Last key in the range.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, bool isOrdered, ReadMode readMode)
        {
            return this.CreateEnumerableAsync(storeTransaction, firstKey, lastKey, key => true, isOrdered, readMode);
        }

        /// <summary>
        /// Creates an ordered enumerable over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode)
        {
            return this.CreateEnumerableAsync(storeTransaction, default(TKey), false, default(TKey), false, filter, isOrdered, readMode);
        }

        /// <summary>
        /// Creates an ordered enumerable over the store items within a specified range.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode)
        {
            return this.CreateEnumerableAsync(storeTransaction, firstKey, true, lastKey, true, filter, isOrdered, readMode);
        }

        /// <summary>
        /// Creates an async enumerator over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, bool isOrdered, ReadMode readMode)
        {
            return this.CreateAsyncEnumeratorAsync(storeTransaction, key => true, isOrdered, readMode);
        }

        /// <summary>
        /// Creates an ordered enumerable over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public async Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode)
        {
            // Check arguments.
            if (null == filter)
            {
                throw new ArgumentException(SR.Error_Filter);
            }

            if (null == storeTransaction)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            // Concatenate enumerables from all components.
            var enumerable = await this.CreateComponentEnumerableAsync(storeTransaction, default(TKey), false, default(TKey), false, isOrdered, readMode, filter).ConfigureAwait(false);

            // Create async enumerator.
            return new TStoreAsyncEnumerator<TKey, TValue>(enumerable);
        }

        /// <summary>
        /// Creates an async enumerator over the store items.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(
            IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, bool isOrdered, ReadMode readMode)
        {
            return this.CreateAsyncEnumeratorAsync(storeTransaction, firstKey, lastKey, key => true, isOrdered, readMode);
        }

        /// <summary>
        /// Creates an ordered enumerable over the store items within a specified range.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies if values that are read from disk should be put into the in-memory cache.</param>
        /// <returns></returns>
        public async Task<IAsyncEnumerator<KeyValuePair<TKey, TValue>>> CreateAsyncEnumeratorAsync(IStoreTransaction storeTransaction, TKey firstKey, TKey lastKey, Func<TKey, bool> filter, bool isOrdered, ReadMode readMode)
        {
            // Check arguments.
            if (null == filter)
            {
                throw new ArgumentException(SR.Error_Filter);
            }

            if (null == storeTransaction)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (IsNull(firstKey))
            {
                throw new ArgumentNullException(SR.Error_FirstKey);
            }

            if (IsNull(lastKey))
            {
                throw new ArgumentNullException(SR.Error_LastKey);
            }

            if (0 < this.keyComparer.Compare(firstKey, lastKey))
            {
                throw new ArgumentException(SR.Error_FirstKey);
            }

            var enumerable =
                await this.CreateComponentEnumerableAsync(storeTransaction, firstKey, true, lastKey, true, isOrdered, readMode, filter).ConfigureAwait(false);

            // Create async enumerator.
            return new TStoreAsyncEnumerator<TKey, TValue>(enumerable);
        }

        /// <summary>
        /// Removes all the keys in the given store.
        /// </summary>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <returns>True,if keys are deleted, false otherwise.</returns>
        public async Task<bool> RemoveAllAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricEvents.Events.RemoveAllAsync(this.traceType, DifferentialStoreConstants.RemoveAll_Starting);

            var success = false;
            var startTime = DateTime.Now;
            var leftOverTime = timeout;
            using (var replicatorTransaction = this.transactionalReplicator.CreateTransaction())
            {
                var transaction = this.CreateStoreReadWriteTransaction(replicatorTransaction);
                transaction.LockingHints = LockingHints.UpdateLock;
                transaction.Isolation = StoreTransactionReadIsolationLevel.ReadRepeatable;
                var storeEnumerable = await this.CreateEnumerableAsync(transaction, false, ReadMode.Off).ConfigureAwait(false);
                var endTime = DateTime.Now;

                using (var storeEnumerator = storeEnumerable.GetAsyncEnumerator())
                {
                    while (await storeEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                    {
                        leftOverTime = this.ComputeTotalRemainingTimeForOperation(leftOverTime, startTime, endTime);
                        startTime = DateTime.Now;
                        await this.TryRemoveAsync(transaction, storeEnumerator.Current.Key, leftOverTime, cancellationToken).ConfigureAwait(false);
                        endTime = DateTime.Now;
                        success = true;
                    }
                }

                await replicatorTransaction.CommitAsync().ConfigureAwait(false);
            }

            FabricEvents.Events.RemoveAllAsync(this.traceType, DifferentialStoreConstants.RemoveAll_Completed);

            return success;
        }

        public void TryStartSweep()
        {
            if (this.EnableSweep)
            {
                // todo: Start only if the time since last sweep has been 10 minutes.
                if (Interlocked.CompareExchange(ref this.sweepInProgress, 1, 0) == 0)
                {
                    this.SweepTask = Task.Run(() => this.Sweep());
                }
            }
        }

        /// <summary>
        /// Test set sweep theshold.
        /// </summary>
        /// <devnote>Exposed for testability only</devnote>
        internal void TestSetSweepThreshold(int value)
        {
            this.sweepThreshold = value;
        }

        #region IStateProvider2 Members

        /// <summary>
        /// Initializes the state provider on the primary.
        /// </summary>
        /// <param name="transactionalReplicator">transactional replicator</param>
        /// <param name="name">state provider name</param>
        /// <param name="initializationContext">initialization context</param>
        /// <param name="stateProviderId">store provider name</param>
        /// <param name="workDirectory"></param>
        /// <param name="children"></param>
        void IStateProvider2.Initialize(
            TransactionalReplicator transactionalReplicator,
            Uri name,
            byte[] initializationContext,
            long stateProviderId,
            string workDirectory,
            IEnumerable<IStateProvider2> children)
        {
            Diagnostics.Assert(children == null, this.TraceType, "TStore does not have children.");

            this.transactionalReplicator = transactionalReplicator;
            var storeInitializationParameters = StoreInitializationParameters.FromByteArray(initializationContext);
            this.Initialize(
                transactionalReplicator,
                transactionalReplicator.StatefulPartition,
                transactionalReplicator.ServiceContext,
                name,
                storeInitializationParameters.StoreBehavior,
                storeInitializationParameters.AllowReadableSecondary,
                stateProviderId,
                workDirectory);
        }

        /// <summary>
        /// Applies operations given by the replicator.
        /// </summary>
        /// <param name="sequenceNumber"></param>
        /// <param name="replicatorTransaction"></param>
        /// <param name="metadata"></param>
        /// <param name="operationData"></param>
        /// <param name="applyContext"></param>
        /// <returns></returns>
        async Task<object> IStateProvider2.ApplyAsync(long sequenceNumber, ReplicatorTransactionBase replicatorTransaction, OperationData metadata, OperationData operationData, ApplyContext applyContext)
        {
            Diagnostics.Assert(false == this.wasCopyAborted, this.TraceType, "Copy was aborted. Cannot Apply");

            // Find role and operation.
            var roleContext = applyContext & ApplyContext.ROLE_MASK;
            var operationContext = applyContext & ApplyContext.OPERATION_MASK;

            Diagnostics.Assert(false == this.isRecoverCheckpointInProgress, this.TraceType, "ApplyAsync has been called before the RecoverCheckpointAsync was completed");

            // Check arguments.
            if (null == replicatorTransaction)
            {
                throw new ArgumentNullException(SR.Error_ReplicatorTransaction);
            }

            var commitSequenceNumber = replicatorTransaction.CommitSequenceNumber;

            // Undo can be null.
            Diagnostics.Assert(metadata != null, this.TraceType, "metadata cannot be null");
            Diagnostics.Assert(operationContext == ApplyContext.REDO || operationData == null, this.TraceType, "undo must be null");

            if (0 == metadata.Count)
            {
                throw new ArgumentException(SR.Error_Metadata);
            }

            FabricEvents.Events.ApplyAsync(this.traceType, sequenceNumber, commitSequenceNumber, replicatorTransaction.Id, (long) roleContext, (long) operationContext);

            // Idempotency check. In case of copy, state might not have recovered yet. Hence we might have to do this check again after copy recovery.
            var snapMetadataTable = this.CurrentMetadataTable;
            if (snapMetadataTable != null && this.IsDuplicateApply(commitSequenceNumber, snapMetadataTable, roleContext, operationContext))
            {
                return null;
            }

            MetadataOperationData metadataOperationData;
            if (roleContext == ApplyContext.PRIMARY)
            {
                metadataOperationData = metadata as MetadataOperationData;
                Diagnostics.Assert(metadataOperationData != null, this.TraceType, "metadata cannot be null");
            }
            else
            {
                metadataOperationData = MetadataOperationData.FromBytes(TStoreConstants.SerializedVersion, metadata);
            }

            // Apply context returned.
            IStoreReadWriteTransaction rwtx = null;

            // Choose how to apply.
            if (ApplyContext.PRIMARY == roleContext)
            {
                // Checks.
                Diagnostics.Assert(ApplyContext.REDO == operationContext, this.TraceType, "unexpected operation context");
                Diagnostics.Assert(0 < sequenceNumber, this.TraceType, "unexpected sequence number");
                Diagnostics.Assert(0 < commitSequenceNumber, traceType, "unexpected commitSequenceNumber number {0}", commitSequenceNumber);
                Diagnostics.Assert(operationData is RedoUndoOperationData, this.traceType, "operationData cannot be null");

                // Apply operation on the primary.
                rwtx = await this.OnPrimaryApplyAsync(commitSequenceNumber, replicatorTransaction, (MetadataOperationData)metadata).ConfigureAwait(false);
            }
            else if (ApplyContext.SECONDARY == roleContext)
            {
                // Idempotency check. Now the current metadata table cannot be null.
                if (this.IsDuplicateApply(commitSequenceNumber, this.CurrentMetadataTable, roleContext, operationContext))
                {
                    return null;
                }

                // MCoskun: Replicator currently only applys after commit. Hence UNDO is not expected.
                Diagnostics.Assert(ApplyContext.REDO == operationContext || ApplyContext.FALSE_PROGRESS == operationContext, traceType, "unexpected operation context {0}", operationContext);

                // MCoskun: Last performed checkpoint lsn is guaranteed to be stable.
                Diagnostics.Assert(
                    this.lastPerformCheckpointLSN < commitSequenceNumber,
                    traceType,
                    "Last performed checkpoint LSN must be stable: this.lastPerformCheckpointLSN {0} < sequenceNumber {1}",
                    this.lastPerformCheckpointLSN, sequenceNumber);

                // Apply operation on the secondary.
                if (ApplyContext.REDO == operationContext)
                {
                    // isIdempotent = could it be a duplicate.
                    // If CurrentMetadataTable.CheckpointLSN is invalid, than we must use the older weaker idempotency check.
                    // If replica is not readable (all operations since the copy have not been replayed) and recovered checkpoint is from old version, then it could be duplicate.
                    var isIdempotent = false;
                    if ((this.CurrentMetadataTable.CheckpointLSN == DifferentialStoreConstants.InvalidLSN) && (!this.transactionalReplicator.IsReadable))
                    {
                        isIdempotent = true;
                    }

                    var operationRedoUndo = RedoUndoOperationData.FromBytes(operationData, this.traceType);

                    rwtx = await this.OnSecondaryApplyAsync(commitSequenceNumber, replicatorTransaction, metadataOperationData, operationRedoUndo, isIdempotent).ConfigureAwait(false);
                }
                else
                {
                    rwtx = this.OnSecondaryUndoFalseProgress(commitSequenceNumber, replicatorTransaction, metadata, operationData);
                }
            }
            else if (ApplyContext.RECOVERY == roleContext)
            {
                // Checks.
                Diagnostics.Assert(ApplyContext.REDO == operationContext || ApplyContext.UNDO == operationContext, this.TraceType, "unexpected operation context");
                Diagnostics.Assert(0 < sequenceNumber, this.TraceType, "unexpected sequence number");

                // Apply operation as part of recovery.
                rwtx = await this.OnRecoveryApplyAsync(commitSequenceNumber, replicatorTransaction, metadata, operationData).ConfigureAwait(false);
            }

            // Return store transaction as context.
            return rwtx;
        }

        private bool IsDuplicateApply(long commitSequenceNumber, MetadataTable currentMetadataTable, ApplyContext roleContext, ApplyContext operationContext)
        {
            Diagnostics.Assert(currentMetadataTable != null, this.traceType, "Current metadata table cannot be null. Commit Sequence Number: {0}", commitSequenceNumber);
            Diagnostics.Assert(commitSequenceNumber != currentMetadataTable.CheckpointLSN, this.traceType, "CheckpointLSN {0} is on a barrier. It could not be seen by State Provider.", currentMetadataTable.CheckpointLSN);

            if (commitSequenceNumber < currentMetadataTable.CheckpointLSN)
            {
                Diagnostics.Assert(ApplyContext.PRIMARY != roleContext, this.traceType, "Invalid role context for duplicate Apply. Role: {0} CSN: {1}", roleContext, commitSequenceNumber);
                Diagnostics.Assert(ApplyContext.REDO == operationContext, this.traceType, "Checkpoint cannot contain false progress. Operation: {0} CSN: {1}", operationContext, commitSequenceNumber);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Called for each replicated transaction operation.
        /// </summary>
        /// <param name="operationContext"></param>
        void IStateProvider2.Unlock(object operationContext)
        {
            // Get the store transaction..
            var rwtx = operationContext as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            Diagnostics.Assert(null != rwtx, this.TraceType, "unexpected transaction");

            // Check if store transaction already completed. This can be called multiple times.
            if (rwtx.IsCompleted)
            {
                // Nothing to do.
                return;
            }

            FabricEvents.Events.Unlock(this.traceType, rwtx.Id);

            // Set the store transaction as completed.
            rwtx.IsCompleted = true;

            // Check if store transaction is being committed.
            if (rwtx.PrimaryOperationCount == 0)
            {
                FabricEvents.Events.Done_Unlock(this.traceType, rwtx.Id);
            }
            else
            {
                FabricEvents.Events.Abort_Unlock(this.traceType, rwtx.Id, rwtx.PrimaryOperationCount);
            }

            // Release all transaction locks.
            rwtx.Dispose();
        }

        /// <summary>
        /// Opens the store.
        /// </summary>
        /// <returns></returns>
        Task IStateProvider2.OpenAsync()
        {
            FabricEvents.Events.OpenAsync(this.traceType, DifferentialStoreConstants.Open_Starting);

            this.InitializeLockManager();
            this.InitializePerformanceCounters();
            this.InitializeMetrics();

            // Ensure the state provider's working sub-directory exists (no-op if it already exists).
            Diagnostics.Assert(FabricDirectory.Exists(this.workingDirectory), this.TraceType, "Work directory does not exist");

            FabricEvents.Events.OpenAsync(this.traceType, DifferentialStoreConstants.Open_Completed);

            // Return synchronously
            return Task.FromResult(true);
        }

        /// <summary>
        /// Closes the store.
        /// </summary>
        /// <returns></returns>
        async Task IStateProvider2.CloseAsync()
        {
            FabricEvents.Events.CloseAsync(this.traceType, DifferentialStoreConstants.Close_Starting);

            // Cleanup.
            await this.CleanupAsync();

            FabricEvents.Events.CloseAsync(this.traceType, DifferentialStoreConstants.Close_Completed);
        }

        /// <summary>
        /// Aborts the store.
        /// </summary>
        void IStateProvider2.Abort()
        {
            FabricEvents.Events.Abort_TStore(this.traceType, DifferentialStoreConstants.Abort_Starting);

            // Cleanup.
            Task.Run(async () => await this.CleanupAsync()).IgnoreExceptionVoid();

            FabricEvents.Events.Abort_TStore(this.traceType, DifferentialStoreConstants.Abort_Completed);
        }

        /// <summary>
        /// ChangeRole is called whenever the replica role is being changed.
        /// </summary>
        /// <param name="newRole">New role of this state provider.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        Task IStateProvider2.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ChangeRoleAsync(this.traceType, DifferentialStoreConstants.ChangeRole_Completed, newRole.ToString());
            return Task.FromResult(true);
        }

        /// <summary>
        /// Prepares for checkpoint by snapping the state to be checkpointed.
        /// </summary>
        /// <returns></returns>
        Task IStateProvider2.PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken)
        {
            Diagnostics.Assert(checkpointLSN >= DifferentialStoreConstants.ZeroLSN, traceType, "CheckpointLSN {0} must be >= 0", checkpointLSN);

            // MCoskun: We cannot assert that lastPrepareCheckpointLSN is InvalidLSN because of double prepare case. 
            this.lastPrepareCheckpointLSN = checkpointLSN;

            FabricEvents.Events.CheckpointAsync(this.traceType, DifferentialStoreConstants.Checkpoint_Prepare_Starting + checkpointLSN);

            try
            {
                // Take a snap of the differential state and then create a new empty differential state.
                // No lock is needed here because applies cannot happen in parallel when checkpoint is in progress.
                // Read will not have any correctness issues without taking locks. Consolidated state should be set before reinitializing differential state.

                if (this.DeltaDifferentialState != null)
                {
                    // This implies that the previous delta has not been checkpointed, so append to this one.
                    // Consolidating state says intact since this data has been appended to delta differantial, so reads will not be affected.
                    this.AppendDeltaDifferantialState();
                }
                else
                {
                    this.DeltaDifferentialState = this.DifferentialState;
                    this.ConsolidationManager.AppendToDeltaDifferentialState(this.DeltaDifferentialState);
                }

                // Differential state should be re-initialized only after consolidated state is made to point to consolidating state.
                this.DifferentialState = new TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(
                    this.transactionalReplicator,
                    this.stateProviderId,
                    this.SnapshotContainer,
                    this.LoadValueCounter,
                    this.isValueAReferenceType,
                    this.traceType);

                if (this.IsClearApplied)
                {
                    this.IsPrepareAfterClear = true;
                }
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(string.Format("TStore.PrepareCheckpointAsync@{0}", this.traceType), e, "failed ");

                // Rethrow inner exception.
                throw e;
            }

            FabricEvents.Events.PrepareCheckpointAsyncCompleted(this.traceType, this.Count);
            return Task.FromResult(true);
        }

        /// <summary>
        /// Called in order to perform a checkpoint.
        /// </summary>
        /// <param name="checkpointMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        async Task IStateProvider2.PerformCheckpointAsync(PerformCheckpointMode checkpointMode, CancellationToken cancellationToken)
        {
            string performCheckpointMessage = string.Format(DifferentialStoreConstants.Checkpoint_Perform_Starting, this.lastPrepareCheckpointLSN, this.GetCheckpointLSN());
            FabricEvents.Events.CheckpointAsync(this.traceType, performCheckpointMessage);

            // Update checkpoint time so that trim is not called immediately after checkpoint as checkpoint consolidates.
            DateTime startCheckpointTime = DateTime.UtcNow;

            try
            {
                // Run checkpoint.
                await this.CheckpointAsync(checkpointMode, CancellationToken.None).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(string.Format("TStore.PerformCheckpointAsync@{0}", this.traceType), e, "failed ");

                // Rethrow inner exception.
                throw e;
            }

            var endCheckpointTime = DateTime.UtcNow;
            var checkpointDuration = endCheckpointTime.Subtract(startCheckpointTime);

            this.lastPerformCheckpointLSN = this.lastPrepareCheckpointLSN;
            this.lastPrepareCheckpointLSN = DifferentialStoreConstants.InvalidLSN;

            UpdateInstantaneousStoreCounters(this.NextMetadataTable, out long itemCount, out long diskSize);

#if !DotNetCoreClr
            FabricEvents.Events.PerformCheckpointAsyncCompleted(this.traceType, this.FileId, checkpointDuration.TotalMilliseconds, itemCount, diskSize);
#endif
        }

        /// <summary>
        /// Called in order to complete checkpoint.
        /// </summary>
        /// <returns></returns>
        async Task IStateProvider2.CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            FabricEvents.Events.CompleteCheckpointAsync(this.traceType, DifferentialStoreConstants.Checkpoint_Complete_Starting);

            Stopwatch stopwatch = new Stopwatch();
            stopwatch.Start();

            // Replace the current metadata table file (if any) with the next metadata table file.
            if (FabricFile.Exists(this.TmpDiskMetadataFilePath))
            {
                await MetadataManager.SafeFileReplaceAsync(
                        this.CurrentDiskMetadataFilePath,
                        this.BkpDiskMetadataFilePath,
                        this.TmpDiskMetadataFilePath,
                        this.traceType).ConfigureAwait(false);
            }

            long snapTime = stopwatch.ElapsedMilliseconds;
            long timeToFileReplace = snapTime;

            if (this.NextMetadataTable != null)
            {
                try
                {
                    await this.MetadataTableLock.WaitAsync().ConfigureAwait(false);

                    var tempMetadataTable = this.CurrentMetadataTable;
                    this.CurrentMetadataTable = this.NextMetadataTable;
                    tempMetadataTable.ReleaseRef();

                    this.NextMetadataTable = null;
                }
                finally
                {
                    this.MetadataTableLock.Release();
                }
            }

            FabricEvents.Events.CompleteCheckpointAsync_ResetNextMetadataTableDone(this.traceType, this.CurrentMetadataTable.Table.Keys.Count);

            long timeToSwap = stopwatch.ElapsedMilliseconds - snapTime;
            snapTime = stopwatch.ElapsedMilliseconds;

            // Release files that need to be deleted.
            var filesToBeDeleted = new List<FileMetadata>();
            foreach (var item in this.filesToBeDeleted)
            {
                var fileMetadata = item.Key;
                Diagnostics.Assert(fileMetadata.ReferenceCount > 0, this.TraceType, "Invalid reference count");
                var immediate = item.Value;

                if (immediate)
                {
                    filesToBeDeleted.Add(fileMetadata);
                }
                else
                {
                    this.filesToBeDeleted[fileMetadata] = true;
                }
            }

            long timeToComputeToBeDeleted = stopwatch.ElapsedMilliseconds - snapTime;
            snapTime = stopwatch.ElapsedMilliseconds;

            foreach (var fileMetadata in filesToBeDeleted)
            {
                fileMetadata.ReleaseRef();

                bool immediate;
                bool fileRemoved = this.filesToBeDeleted.TryRemove(fileMetadata, out immediate);
                Diagnostics.Assert(fileRemoved, this.TraceType, "Failed to remove file");
            }

            stopwatch.Stop();
            long timeToDelete = stopwatch.ElapsedMilliseconds - snapTime;
            snapTime = stopwatch.ElapsedMilliseconds;

#if DEBUG
            Diagnostics.Assert(
                snapTime < MaxCompleteCheckpointTime,
                traceType,
                "Complete must take less than 30 seconds. Total: {0} Replace: {1} Swap: {2} ComputeToBeDeleted: {3} Delete: {4}",
                snapTime, timeToFileReplace, timeToSwap, timeToComputeToBeDeleted, timeToDelete);

            // Consistency checks.
            Diagnostics.Assert(FabricFile.Exists(this.CurrentDiskMetadataFilePath), this.traceType, "current metadata table does not exist.");
#else
            if (snapTime >= MaxCompleteCheckpointTime)
            {
                FabricEvents.Events.CompleteCheckpointAsyncWarning(this.traceType, snapTime, timeToFileReplace, timeToSwap, timeToComputeToBeDeleted, timeToDelete);
            }
#endif
            FabricEvents.Events.CompleteCheckpointAsyncCompleted(this.traceType, snapTime, timeToFileReplace, timeToSwap, timeToComputeToBeDeleted, timeToDelete);
        }

        /// <summary>
        /// Called in order to recover from a checkpoint.
        /// </summary>
        /// <returns></returns>
        async Task IStateProvider2.RecoverCheckpointAsync()
        {
            FabricEvents.Events.RecoverCheckpointAsync(this.traceType, "starting", -1, -1);

            Diagnostics.Assert(this.isRecoverCheckpointInProgress == false, this.TraceType, "RecoverCheckpointAsync was called concurrently on TStore");

            // Reset the last prepared and last performed checkpoint lsn.
            this.lastPrepareCheckpointLSN = DifferentialStoreConstants.InvalidLSN;
            this.lastPerformCheckpointLSN = DifferentialStoreConstants.InvalidLSN;

            try
            {
                this.isRecoverCheckpointInProgress = true;

                // Inspect checkpoint file.
                if (!FabricFile.Exists(this.CurrentDiskMetadataFilePath))
                {
                    FabricEvents.Events.OpenAsync(this.traceType, DifferentialStoreConstants.Open_Checkpoint_Create);

                    this.CurrentMetadataTable.CheckpointLSN = DifferentialStoreConstants.ZeroLSN;
                    // Create first checkpoint.
                    await MetadataManager.WriteAsync(this.CurrentMetadataTable, this.TmpDiskMetadataFilePath).ConfigureAwait(false);

                    // Only move the first checkpoint
                    FabricFile.Move(this.TmpDiskMetadataFilePath, this.CurrentDiskMetadataFilePath);
                }
                else
                {
                    await MetadataManager.OpenAsync(this.CurrentMetadataTable, this.CurrentDiskMetadataFilePath, this.traceType).ConfigureAwait(false);

                    try
                    {
                        await this.RecoverConsolidatedStateAsync(CancellationToken.None).ConfigureAwait(false);
                    }
                    catch (Exception e)
                    {
                        e = Diagnostics.ProcessException(string.Format("TStore.RecoverCheckpointAsync@{0}", this.traceType), e, "failed ");

                        // Rethrow inner exception.
                        throw e;
                    }
                }

                // StateManger calls RecoverCheckpointAsync when it can guarantee the current checkpoint is the correct one.  Remove any stale next checkpoints.
                if (FabricFile.Exists(this.TmpDiskMetadataFilePath))
                {
                    FabricEvents.Events.OpenAsync(this.traceType, DifferentialStoreConstants.Open_Checkpoint_DeleteStaleNext);
                    FabricFile.Delete(this.TmpDiskMetadataFilePath);
                }

                // Remove all files other than the current metadata file and files present in it.
                this.TrimFiles();
            }
            finally
            {
                this.isRecoverCheckpointInProgress = false;
            }

            await this.FireRebuildNotificationCallerHoldsLockAsync().ConfigureAwait(false);

            FabricEvents.Events.RecoverCheckpointAsync(this.traceType, DifferentialStoreConstants.Recover_Checkpoint_Completed, this.Count, this.CurrentMetadataTable.CheckpointLSN);
        }

        /// <summary>
        /// Remove any unreferences files in the given directory.
        /// </summary>
        /// <remarks>
        /// The metadata table is assumed to own the contents of the directory, so any checkpoint file
        /// that exists in the directory but not in the set of referenced files will be deleted.
        /// </remarks>
        public void TrimFiles()
        {
            FabricEvents.Events.MetadataTableTrimFilesStart(this.traceType, this.workingDirectory);

            // Construct a hash set from the currently referenced files.
            var referencedFiles = new HashSet<string>(this.GetCurrentMetadataFiles());
            var filesToBeDeleted = new List<string>();

            foreach (string file in this.GetCheckpointFiles())
            {
                // If this file does not exist in the hash set, it must not be referenced - so delete it.
                if (!referencedFiles.Remove(file))
                {
                    FabricEvents.Events.MetadataTableTrimFilesToDelete(this.traceType, file);
                    filesToBeDeleted.Add(file);
                }
            }

            // Safety verification - if we didn't find all the currently referenced files, something went
            // wrong and we should not delete any files (case mismatch? drive\network path mismatch?).
            Diagnostics.Assert(
                referencedFiles.Count == 0,
                this.TraceType,
                "failed to find all referenced files when trimming unreferenced files.  NOT deleting any files.");

            // We can now assume it's safe to delete this unreferenced files.
            foreach (var tableFile in filesToBeDeleted)
            {
                FabricEvents.Events.MetadataTableTrimFilesDeleting(this.traceType, tableFile);
                try
                {
                    FabricFile.Delete(tableFile);
                }
                catch (Exception e)
                {
                    Diagnostics.Assert(false, this.TraceType, "Unable to delete file {0} due to exception {1}", tableFile, e.Message);
                    throw;
                }
            }

            FabricEvents.Events.MetadataTableTrimFilesComplete(this.traceType, this.workingDirectory);
        }

        /// <summary>
        /// Gets an enumeration over all files in the current metadata table.
        /// </summary>
        /// <returns>Set of files references by the metadata table.</returns>
        public IEnumerable<string> GetCurrentMetadataFiles()
        {
            if (this.CurrentMetadataTable != null)
            {
                foreach (var item in this.CurrentMetadataTable.Table)
                {
                    var fileMetadata = item.Value;
                    yield return fileMetadata.CheckpointFile.KeyCheckpointFileName;
                    yield return fileMetadata.CheckpointFile.ValueCheckpointFileName;
                }
            }
        }

        /// <summary>
        /// Gets an enumeration over all files in the current metadata table.
        /// </summary>
        /// <returns>Set of files references by the metadata table.</returns>
        public IEnumerable<string> GetCheckpointFiles()
        {
            foreach (string keyFile in FabricDirectory.GetFiles(this.workingDirectory, "*" + KeyCheckpointFile.FileExtension, SearchOption.TopDirectoryOnly))
            {
                yield return keyFile;
            }

            foreach (string valueFile in FabricDirectory.GetFiles(this.workingDirectory, "*" + ValueCheckpointFile.FileExtension, SearchOption.TopDirectoryOnly))
            {
                yield return valueFile;
            }
        }

        private IAsyncEnumerable<KeyValuePair<TKey, TValue>> AsyncEnumerateCallerHoldsLock(ReadMode readMode)
        {
            Diagnostics.Assert(this.DifferentialState.Count() == 0, this.TraceType, "Differential State must be empty");
            Diagnostics.Assert(this.SnapshotContainer.GetCount() == 0, this.TraceType, "SnapshotContainer must be empty");

            var consolidatedStateKeys = this.ConsolidationManager.EnumerateKeys();

            return new AsyncEnumerable<TKey, TValue>(
                this.IsValueAReferenceType,
                readMode,
                consolidatedStateKeys, 
                this.DifferentialState, 
                this.ConsolidationManager, 
                this.CurrentMetadataTable, 
                this.LoadValueCounter,
                this.valueConverter,
                this.TraceType);
        }

        /// <summary>
        /// Called when recovery is completed.
        /// </summary>
        /// <returns></returns>
        Task IStateProvider2.OnRecoveryCompletedAsync()
        {
            FabricEvents.Events.OnRecoveryCompletedAsync(this.traceType, DifferentialStoreConstants.OnRecoveryCompleted_Starting);

            FabricEvents.Events.OnRecoveryCompletedAsync(this.traceType, DifferentialStoreConstants.OnRecoveryCompleted_Completed);

            // Return synchronously.
            return Task.FromResult(true);
        }

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        async Task IStateProvider2.BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            FabricEvents.Events.BackupCheckpointAsync(this.traceType, backupDirectory, DifferentialStoreConstants.BackupCheckpoint_Starting);

            // Argument checks.
            int numberOfItems = FabricDirectory.GetDirectories(backupDirectory).Length + FabricDirectory.GetFiles(backupDirectory).Length;
            if (numberOfItems != 0)
            {
                throw new ArgumentException(SR.Error_TStore_BackupDirectory);
            }

            // Consistency checks.
            Diagnostics.Assert(FabricFile.Exists(this.CurrentDiskMetadataFilePath), this.TraceType, "no metadata table exists on disk");

            // Read the current state on-disk state into memory.
            // Complete checkpoint never runs where there is a back-up call, so it is safe to do this here.
            using (var snapshotMetadataTable = new MetadataTable(this.TraceType))
            {
                await MetadataManager.OpenAsync(snapshotMetadataTable, this.CurrentDiskMetadataFilePath, this.traceType).ConfigureAwait(false);

                // Backup the current master table.
                var backupMetadataFileName = Path.Combine(backupDirectory, currentDiskMetadataFileName);

                FabricFile.Copy(this.CurrentDiskMetadataFilePath, backupMetadataFileName, overwrite: false);

                // Backup all currently referenced tables.
                foreach (var fileMetadata in snapshotMetadataTable.Table.Values)
                {
                    // Copy key file.
                    var keyFileName = fileMetadata.FileName + KeyCheckpointFile.FileExtension;
                    var fullKeyCheckpointFileName = Path.Combine(this.workingDirectory, keyFileName);
                    Diagnostics.Assert(
                        FabricFile.Exists(fullKeyCheckpointFileName),
                        traceType,
                        "unexpectedly missing key file: {0}",
                        fullKeyCheckpointFileName);
                    var backupKeyFileName = Path.Combine(backupDirectory, keyFileName);
                    FabricEvents.Events.BackupCheckpointAsync(
                        this.traceType,
                        backupDirectory,
                        string.Format(CultureInfo.InvariantCulture, "backing up file '{0}' to '{1}", fullKeyCheckpointFileName, backupKeyFileName));
                    FabricFile.Copy(fullKeyCheckpointFileName, backupKeyFileName, overwrite: false);

                    // Copy value file.
                    var valueFileName = fileMetadata.FileName + ValueCheckpointFile.FileExtension;
                    var fullValueCheckpointFileName = Path.Combine(this.workingDirectory, valueFileName);
                    Diagnostics.Assert(
                        FabricFile.Exists(fullValueCheckpointFileName),
                        traceType,
                        "unexpectedly missing value file: {0}",
                        fullValueCheckpointFileName);
                    var backupValueFileName = Path.Combine(backupDirectory, valueFileName);
                    FabricEvents.Events.BackupCheckpointAsync(
                        this.traceType,
                        backupDirectory,
                        string.Format(CultureInfo.InvariantCulture, "backing up file '{0}' to '{1}", fullValueCheckpointFileName, backupValueFileName));
                    FabricFile.Copy(fullValueCheckpointFileName, backupValueFileName, overwrite: false);
                }
            }

            FabricEvents.Events.BackupCheckpointAsync(this.traceType, backupDirectory, DifferentialStoreConstants.BackupCheckpoint_Completed);
        }

        /// <summary>
        /// Restore the checkpoint state from the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint restore.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <remarks>
        /// The previously backed up checkpoint state becomes the current checkpoint state.  The
        /// checkpoint is not recovered from.
        /// </remarks>
        async Task IStateProvider2.RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            FabricEvents.Events.RestoreCheckpointAsync(this.traceType, backupDirectory, DifferentialStoreConstants.RestoreCheckpoint_Starting);

            Exception exception = null;

            try
            {
                // Validate the backup.
                var backupMetadataTableFilePath = Path.Combine(backupDirectory, currentDiskMetadataFileName);
                if (!FabricFile.Exists(backupMetadataTableFilePath))
                {
                    throw new InvalidDataException(
                        string.Format(CultureInfo.CurrentCulture, SR.Error_TStore_InvalidBackup, backupMetadataTableFilePath));
                }

                var metadataTable = new MetadataTable(this.TraceType);

                // Validate the backup contains all tables referenced in the metadata table.
                await MetadataManager.OpenAsync(metadataTable, backupMetadataTableFilePath, this.traceType).ConfigureAwait(false);
                foreach (var fileMetadata in metadataTable.Table.Values)
                {
                    var keyFile = Path.Combine(backupDirectory, fileMetadata.FileName) + KeyCheckpointFile.FileExtension;
                    if (!FabricFile.Exists(keyFile))
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_TStore_InvalidBackup, keyFile));
                    }

                    var valueFile = Path.Combine(backupDirectory, fileMetadata.FileName) + ValueCheckpointFile.FileExtension;
                    if (!FabricFile.Exists(valueFile))
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_TStore_InvalidBackup, valueFile));
                    }
                }

                // Consistency checks.  preethas: Asserts temporarily disabled until disk leak is figured out.
                //Diagnostics.InternalAssert(!FabricFile.Exists(this.CurrentDiskMetadataFilePath), "state manager should already have cleared all state from all state providers via ChangeRole(None)");
                //Diagnostics.InternalAssert(!FabricFile.Exists(this.TmpDiskMetadataFilePath), "state manager should already have cleared all state from all state providers via ChangeRole(None)");

                // Restore the current master table from the backup, and all referenced tables.
                FabricEvents.Events.RestoreCheckpointAsync(
                    this.traceType,
                    backupDirectory,
                    string.Format(CultureInfo.InvariantCulture, "restoring file '{0}' to '{1}'", backupMetadataTableFilePath, this.CurrentDiskMetadataFilePath));
                FabricFile.Copy(backupMetadataTableFilePath, this.CurrentDiskMetadataFilePath, overwrite: true);

                // Restore all references tables.
                foreach (var fileMetadata in metadataTable.Table.Values)
                {
                    var keyFileName = Path.Combine(this.workingDirectory, fileMetadata.FileName) + KeyCheckpointFile.FileExtension;
                    var backupKeyFileName = Path.Combine(backupDirectory, fileMetadata.FileName) + KeyCheckpointFile.FileExtension;
                    FabricEvents.Events.RestoreCheckpointAsync(
                        this.traceType,
                        backupDirectory,
                        string.Format(CultureInfo.InvariantCulture, "restoring file '{0}' to '{1}'", backupKeyFileName, keyFileName));
                    FabricFile.Copy(backupKeyFileName, keyFileName, overwrite: true);

                    var valueFileName = Path.Combine(this.workingDirectory, fileMetadata.FileName) + ValueCheckpointFile.FileExtension;
                    var backupValueFileName = Path.Combine(backupDirectory, fileMetadata.FileName) + ValueCheckpointFile.FileExtension;
                    FabricEvents.Events.RestoreCheckpointAsync(
                        this.traceType,
                        backupDirectory,
                        string.Format(CultureInfo.InvariantCulture, "restoring file '{0}' to '{1}'", backupValueFileName, valueFileName));
                    FabricFile.Copy(backupValueFileName, valueFileName, overwrite: true);
                }
            }
            catch (AggregateException e)
            {
                exception = e.Flatten();

                throw;
            }
            catch (Exception e)
            {
                exception = e;

                throw;
            }
            finally
            {
                if (exception == null)
                {
                    FabricEvents.Events.RestoreCheckpointAsync(this.traceType, backupDirectory, DifferentialStoreConstants.RestoreCheckpoint_Completed);
                }
                else
                {
                    FabricEvents.Events.RestoreCheckpointAsync(this.traceType, backupDirectory, DifferentialStoreConstants.RestoreCheckpoint_FailedWith + exception.GetType() + " " + exception.Message);
                }
            }
        }

        /// <summary>
        /// Prepares for remove of the entire store.
        /// </summary>
        /// <param name="transaction"></param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        async Task IStateProvider2.PrepareForRemoveAsync(ReplicatorTransaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var storeTransaction = this.CreateOrFindTransaction(transaction).Value;
            var rwtx = storeTransaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;

            this.ThrowIfFaulted(rwtx);

            // It should be called on the primary only.
            this.ThrowIfNotWritable(rwtx.Id);

            // Compute locking timeout. For simplicity, given equal lock timeout for each store component.
            var lockTimeoutInMs = this.GetTimeoutInMilliseconds(timeout);
            var start = DateTime.UtcNow;
            var end = DateTime.UtcNow;
            var duration = TimeSpan.MaxValue;

            try
            {
                start = DateTime.UtcNow;
                var lockMode = LockMode.Exclusive;

                // Acquire exclusive lock on store component.
                await rwtx.AcquirePrimeLocksAsync(this.lockManager, lockMode, lockTimeoutInMs, true).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Compute left-over timeout, if needed.
                if (Timeout.Infinite != lockTimeoutInMs && 0 != lockTimeoutInMs)
                {
                    end = DateTime.UtcNow;
                    duration = end.Subtract(start);

                    if (duration.TotalMilliseconds < 0)
                    {
                        duration = TimeSpan.Zero;
                    }

                    if (lockTimeoutInMs < duration.TotalMilliseconds)
                    {
                        // Locks could not be acquired in the given timeout.
                        throw new TimeoutException(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                SR.LockTimeout_TStore_TableLock,
                                lockMode,
                                this.traceType,
                                lockTimeoutInMs,
                                storeTransaction.Id));
                    }
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.PrepareForRemoveAsync@{0}", this.traceType), e, "acquiring prime locks");
                Diagnostics.Assert(e is TimeoutException || e is OperationCanceledException || e is TransactionFaultedException, traceType, "unexpected exception {0}", e.GetType());

                // Unlock will be called, do not dispose the lock here.

                // Rethrow inner exception.
                throw e;
            }
        }

        /// <summary>
        /// Called when the state is not needed anymore.
        /// </summary>
        /// <param name="stateProviderId">Id that uniquely identifies a state provider.</param>
        /// <returns></returns>
        async Task IStateProvider2.RemoveStateAsync(long stateProviderId)
        {
            FabricEvents.Events.RemoveStateAsync(this.traceType, DifferentialStoreConstants.RemoveState_Starting, stateProviderId);

            // Prepare for remove has already taken a lock, so do not acquire again here, just set closing to true because 
            // we do not want to accept any new operation after remove state.

            await this.CancelBackgroundConnsolidationTaskAsync();
            await this.CancelSweepTaskAsync();

            this.isClosing = true;

            // Get rid of all persisted files.
            if (FabricDirectory.Exists(this.workingDirectory))
            {
                // Release files that need to be deleted.
                foreach (var item in this.filesToBeDeleted)
                {
                    // Instead of releasing ref, just dispose.
                    item.Key.Dispose();
                }

                this.filesToBeDeleted.Clear();

                // Remove current metadata table first so that if CR to none is retried following partially deleted files,
                // subsequent open will not fail. The leaked files will get cleaned as a prt of SM clean-up.

                if (this.CurrentMetadataTable != null)
                {

                    foreach (var fileMetadata in CurrentMetadataTable.Table.Values)
                    {
                        // Let the directory delete, remove these files.
                        fileMetadata.CanBeDeleted = false;
                    }

                    this.CurrentMetadataTable.Dispose();
                }

                if (this.NextMetadataTable != null)
                {
                    foreach (var fileMetadata in this.NextMetadataTable.Table.Values)
                    {
                        fileMetadata.CanBeDeleted = false;
                    }

                    this.NextMetadataTable.Dispose();
                }

                if (this.MergeMetadataTable != null)
                {
                    foreach (var fileMetadata in this.MergeMetadataTable.Table.Values)
                    {
                        fileMetadata.CanBeDeleted = false;
                    }

                    this.MergeMetadataTable.Dispose();
                }

                if (this.SnapshotContainer != null)
                {
                    this.SnapshotContainer.Dispose();
                }

                // Delete metadata files explicitly.
                if (FabricFile.Exists(this.CurrentDiskMetadataFilePath))
                {
                    FabricFile.Delete(this.CurrentDiskMetadataFilePath);
                }

                if (FabricFile.Exists(this.TmpDiskMetadataFilePath))
                {
                    FabricFile.Delete(this.TmpDiskMetadataFilePath);
                }
                
                if (this.SnapshotContainer != null)
                {
                    // Acquire snapshotcontainer write lock to avoid race between FabricDirectory.Delete & SnapshotContainer.Remove
                    // Otherwise FabricDirectory.Delete could race in removing the same file as being removed concurrently in SnapshotContainer.Remove
                    // and this causes FabricDirectory.Delete to throw FileNotFoundException or AccessDeniedException
                    // This happens because VersionManager callback for Remove can race with RemoveStateAsync call
                    this.SnapshotContainer.checkpointFileDeleteLock.EnterWriteLock();
                }

                try
                {
                    // State Manager will delete this folder, hence this is not required.
                    // Temporarily adding it back in since unit tests assume the folder will be deleted.
                    FabricDirectory.Delete(this.workingDirectory, recursive: true);
                }
                finally
                {
                    if (this.SnapshotContainer != null)
                    {
                        this.SnapshotContainer.checkpointFileDeleteLock.ExitWriteLock();
                    }
                }
            }

            this.itemCountPerfCounterWriter.ResetCount();
            this.metrics.ItemCountMetric.Add(-this.lastRecordedItemCount);
            this.lastRecordedItemCount = 0;

            this.diskSizePerfCounterWriter.SetSize(0);
            this.metrics.DiskSizeMetric.Add(-this.lastRecordedDiskSize);
            this.lastRecordedDiskSize = 0;

            FabricEvents.Events.RemoveStateAsync(this.traceType, DifferentialStoreConstants.RemoveState_Completed, stateProviderId);
        }

        /// <summary>
        ///
        /// </summary>
        /// <returns></returns>
        Task IStateProvider2.OnDataLossAsync()
        {
            FabricEvents.Events.OnDataLossAsync(this.traceType, DifferentialStoreConstants.OnDataLoss_Starting);
            FabricEvents.Events.OnDataLossAsync(this.traceType, DifferentialStoreConstants.OnDataLoss_Completed);

            // Return synchronously. State has not changed.
            return Task.FromResult(true);
        }

        /// <summary>
        /// Used for copy.
        /// </summary>
        /// <returns></returns>
        IOperationDataStream IStateProvider2.GetCurrentState()
        {
            // Create a copy stream from the state of the disk as of this logical snapshot.
            IOperationDataStream copyStream = new TStoreCopyStream(this, this.traceType, this.perfCounterSetInstance);
            FabricEvents.Events.GetCurrentState_TStore(this.traceType, DifferentialStoreConstants.GetCurrentState_Starting, copyStream.GetHashCode());
            return copyStream;
        }

        /// <summary>
        /// Used during replica copy.
        /// </summary>
        void IStateProvider2.BeginSettingCurrentState()
        {
            FabricEvents.Events.BeginSettingCurrentState(this.traceType, DifferentialStoreConstants.BeginSettingCurrent_Starting);

            // Drop all old in-memory state and re-create it.
            this.DifferentialState = new TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(
                this.transactionalReplicator,
                this.stateProviderId,
                this.SnapshotContainer,
                this.LoadValueCounter,
                this.isValueAReferenceType,
                this.traceType);
            this.ConsolidationManager = new ConsolidationManager<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(this);

            this.SnapshotContainer.Dispose();
            this.SnapshotContainer = new SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(this.valueConverter, this.TraceType);
            this.UninitializeLockManager();
            this.inflightReadWriteStoreTransactions = new ConcurrentDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>>(1, 1);

            // Create and initialize lock managers.
            this.lockManager = new LockManager();
            this.InitializeLockManager();

            // Prepare the new persisted state management.
            // Do not have to lock as there can be no readers yet and no checkpoint yet.
            // Should file metadata be derefrenced?
            if (this.CurrentMetadataTable != null)
            {
                foreach (var item in this.CurrentMetadataTable.Table)
                {
                    var fileMetadata = item.Value;
                    fileMetadata.CanBeDeleted = true;
                    fileMetadata.AddRef();

                    // Assert that the ref is one. There can be no readers during copy so it is safe to assert that count is one.
                    Diagnostics.Assert(fileMetadata.ReferenceCount == 2, this.TraceType, "Unexpected filemetadata count {0} for file file {1}", fileMetadata.ReferenceCount, fileMetadata.CheckpointFile.KeyCheckpointFileName);

                    bool add = this.filesToBeDeleted.TryAdd(item.Value, /*immediate:*/ true);
                    Diagnostics.Assert(add, traceType, "Failed to add file id {0}", item.Value.FileId);
                }

                this.CurrentMetadataTable.Dispose();
            }

            this.CurrentMetadataTable = null;
            this.copyManager = new CopyManager(this.traceType, this.perfCounterSetInstance);

            // Reset clear flags.
            this.IsClearApplied = false;
            this.IsPrepareAfterClear = false;

            FabricEvents.Events.BeginSettingCurrentState(this.traceType, DifferentialStoreConstants.BeginSettingCurrent_Completed);
        }

        /// <summary>
        /// Apply copy operation data.
        /// </summary>
        /// <param name="stateRecordNumber">Copy operation sequence number.</param>
        /// <param name="data">Copy data.</param>
        async Task IStateProvider2.SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            // Checks.
            Diagnostics.Assert(data != null, this.TraceType, "unexpected operation data");
            Diagnostics.Assert(this.copyManager != null, this.TraceType, "unexpectedly no longer building a new master table");

            await this.copyManager.AddCopyDataAsync(this.workingDirectory, data, this.traceType).ConfigureAwait(false);
        }

        /// <summary>
        /// Complete copy.
        /// </summary>
        async Task IStateProvider2.EndSettingCurrentStateAsync()
        {
            FabricEvents.Events.EndSettingCurrentState(this.traceType, DifferentialStoreConstants.EndSettingCurrent_Starting);
            this.wasCopyAborted = !this.copyManager.CopyCompleted;
            if (this.wasCopyAborted)
            {
#if !DotNetCoreClr
                FabricEvents.Events.EndSettingCurrentStateCopyAborted(this.traceType);
#endif
                this.copyManager.Dispose();
                this.copyManager = null;
            }
            else
            {
                await RecoverCopyStateAsync(CancellationToken.None);
            }

            FabricEvents.Events.EndSettingCurrentState(this.traceType, DifferentialStoreConstants.EndSettingCurrent_Completed);
        }

        /// <summary>
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <returns>Collection of state providers</returns>
        IEnumerable<IStateProvider2> IStateProvider2.GetChildren(Uri name)
        {
            return null;
        }

        #endregion

        /// <summary>
        /// Applies an operation on the secondary.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="replicatorTransaction">Replicator transaction.</param>
        /// <param name="metadataOperationData"></param>
        /// <param name="operationData">Data bytes to be applied.</param>
        /// <returns></returns>
        /// <devnote>Exposed for testability.</devnote>
        internal IStoreReadWriteTransaction OnSecondaryUndoFalseProgress(long sequenceNumber, ReplicatorTransactionBase replicatorTransaction, OperationData metadataOperationData, OperationData operationData)
        {
            Diagnostics.Assert(null == operationData, this.TraceType, "rundo operation data must be null");

            // Retrieve redo/undo operation.
            var operationMetadata = MetadataOperationData.FromBytes(TStoreConstants.SerializedVersion, metadataOperationData);
            Diagnostics.ThrowIfNotValid(
                StoreModificationType.Add == operationMetadata.Modification || StoreModificationType.Remove == operationMetadata.Modification ||
                StoreModificationType.Update == operationMetadata.Modification,
                "unexpected store operation type {0}",
                operationMetadata.Modification);
            Diagnostics.Assert(null != operationMetadata.KeyBytes.Array, this.TraceType, "unexpected key bytes");

            // Find or create the store transaction correspondent to this replicator transaction.
            var firstCreated = false;
            var rwtx = this.FindOrCreateTransaction(replicatorTransaction, operationMetadata.StoreTransaction, out firstCreated);
            FabricEvents.Events.OnSecondaryUndoFalseProgressAsync(this.traceType, firstCreated ? "starting" : "continuing", sequenceNumber, rwtx.Id);

            // Undo false progress.
            this.OnUndoFalseProgressAsync(sequenceNumber, rwtx, operationMetadata, operationMetadata.Modification);

            return firstCreated ? rwtx : null;
        }

        /// <summary>
        /// Converts key to byte array.
        /// </summary>
        /// <devnote>Exposed for testability.</devnote>
        internal ArraySegment<byte> GetKeyBytes(TKey key)
        {
            using (var memoryStream = new MemoryStream())
            {
                using (var binaryWriter = new BinaryWriter(memoryStream))
                {
                    this.keyConverter.Write(key, binaryWriter);

                    return new ArraySegment<byte>(memoryStream.GetBuffer(), 0, (int) memoryStream.Position);
                }
            }
        }

        /// <summary>
        /// Converts value to byte array.
        /// </summary>
        /// <devnote>Exposed for testability.</devnote>
        internal ArraySegment<byte>[] GetValueBytes(TValue currentValue, TValue newValue)
        {
            using (var memoryStream = new MemoryStream())
            {
                using (var binaryWriter = new BinaryWriter(memoryStream))
                {
                    this.valueConverter.Write(currentValue, newValue, binaryWriter);

                    return new[] {new ArraySegment<byte>(memoryStream.GetBuffer(), 0, (int) memoryStream.Position)};
                }
            }
        }

        /// <summary>
        /// Cancels existing sweep token and resets token source.
        /// </summary>
        private async Task CancelSweepAsync()
        {
            Diagnostics.Assert(this.SweepTaskCancellationSource != null, this.traceType, "this.SweepTaskCancellationSource != null");

            // Stop any ongoing sweep
            this.SweepTaskCancellationSource.Cancel();

            try
            {
                // Await the sweep task following cancellation
                if (this.SweepTask != null)
                {
                    await this.SweepTask.ConfigureAwait(false);
                }
            }
            catch (Exception e)
            {
                // trace and swallow the exception.
                Diagnostics.ProcessException(string.Format("TStore.CancelAndResetSweepTokenSourceAsync@{0}", this.traceType), e, "cancelling sweep task");
            }

            this.SweepTaskCancellationSource = new CancellationTokenSource();
        }

        private async Task ResetConsolidationTaskAsync()
        {
            if (this.consolidationTask != null)
            {
                Diagnostics.Assert(this.ConsolidateTaskCancellationSource != null, this.traceType, "this.ConsolidateTaskCancellationSource != null");

                // If consolidation task is in progress, then cancel it. 
                this.ConsolidateTaskCancellationSource.Cancel();
            }

            try
            {
                if (this.consolidationTask != null)
                {
                    await this.consolidationTask;
                }
            }
            catch (Exception e)
            {
                // trace and swallow the exception.
                Diagnostics.ProcessException(string.Format("TStore.ResetConsolidationTaskAsync@{0}", this.traceType), e, "cancelling consolidation task");
            }

            this.consolidationTask = null;
            this.ConsolidateTaskCancellationSource = new CancellationTokenSource();
        }

        private int GetValueSize(ArraySegment<byte>[] bytes)
        {
            var count = 0;
            if (bytes == null || bytes.Length == 0)
            {
                return 0;
            }

            for (var i = 0; i < bytes.Length; i++)
            {
                count += bytes[i].Count;
            }

            return count;
        }

        private TKey GetKeyFromBytes(ArraySegment<byte> bytes)
        {
            using (var memoryStream = new MemoryStream(bytes.Array, bytes.Offset, bytes.Count, writable: false))
            {
                using (var binaryReader = new BinaryReader(memoryStream))
                {
                    return this.keyConverter.Read(binaryReader);
                }
            }
        }

        private TValue GetValueFromBytes(ArraySegment<byte>[] byteSegments)
        {
            var bytes = byteSegments[0];
            using (var memoryStream = new MemoryStream(bytes.Array, bytes.Offset, bytes.Count, writable: false))
            {
                using (var binaryReader = new BinaryReader(memoryStream))
                {
                    return this.valueConverter.Read(default(TValue), binaryReader);
                }
            }
        }

        // check if current time stamp collides with lastCheckpointFileTimeStamp
        // If yes then increment it and return
        // else return current time stamp
        internal long NextCheckpointFileTimeStamp()
        {
            long lastUsedTimeStamp;
            long nextTimeStamp;

            do
            {
                lastUsedTimeStamp = lastCheckpointFileTimeStamp;
                nextTimeStamp = Math.Max(DateTime.UtcNow.Ticks, lastCheckpointFileTimeStamp + 1);
            }
            while (Interlocked.CompareExchange(ref lastCheckpointFileTimeStamp, nextTimeStamp, lastUsedTimeStamp) != lastUsedTimeStamp);

            return nextTimeStamp;
        }

        internal uint IncrementFileId()
        {
            lock (this.fileIdLock)
            {
                return ++this.FileId;
            }
        }

        internal void AssertIfUnexpectedVersionedItemValue(StoreComponentReadResult<TValue> readResult, ReadMode readMode)
        {
            if (readResult.HasValue == false)
            {
                return;
            }

            this.AssertIfUnexpectedVersionedItemValue(readResult.VersionedItem, readResult.UserValue, readMode);
        }

        internal void AssertIfUnexpectedVersionedItemValue(TVersionedItem<TValue> versionedItem, TValue value, ReadMode readMode)
        {
            if (this.isClosing)
            {
                throw new FabricObjectClosedException();
            }

            if (readMode == ReadMode.Off)
            {
                return;
            }

            if (versionedItem.Kind != RecordKind.DeletedVersion)
            {
                if (this.isValueAReferenceType)
                {
                    Diagnostics.Assert(!versionedItem.CanBeSweepedToDisk || value != null,
                           traceType,
                           "Value should be present in memory, is value sweepable: {0} and file id: {1}",
                           versionedItem.CanBeSweepedToDisk, versionedItem.FileId);
                }
                else
                {
                    Diagnostics.Assert(readMode != ReadMode.CacheResult || (versionedItem.InUse && !versionedItem.CanBeSweepedToDisk),
                        traceType,
                        "Value should be present in memory for value types :{0} and CanBeSweepedToDisk :{1} should be false",
                        versionedItem.InUse, versionedItem.CanBeSweepedToDisk);
                }
            }
        }

        internal void AssertIfUnexpectedVersionedItemValue(TVersionedItem<TValue> versionedItem, TValue value)
        {
            if (versionedItem.Kind != RecordKind.DeletedVersion)
            {
                if (this.isValueAReferenceType)
                {
                    Diagnostics.Assert(!versionedItem.CanBeSweepedToDisk || value != null,
                           traceType,
                           "Value should be present in memory, is value sweepable: {0} and file id: {1}",
                           versionedItem.CanBeSweepedToDisk, versionedItem.FileId);
                }
                else
                {
                    Diagnostics.Assert(versionedItem.InUse && !versionedItem.CanBeSweepedToDisk,
                        traceType,
                        "Value should be present in memory for value types :{0} and CanBeSweepedToDisk :{1} should be false",
                        versionedItem.InUse, versionedItem.CanBeSweepedToDisk);
                }
            }
        }

        private async Task RecoverConsolidatedStateAsync(CancellationToken cancellationToken)
        {
            Diagnostics.Assert(this.CurrentMetadataTable != null, this.TraceType, "CurrentMetadataTable cannot be null");
            Diagnostics.Assert(this.CurrentMetadataTable.Table != null, this.TraceType, "CurrentMetadataTable.Table cannot be null");
            FabricEvents.Events.RecoverConsolidatedAsync(this.traceType, this.CurrentMetadataTable.Table.Count);

            var recoveryStoreComponent = new RecoveryStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(
                    this.CurrentMetadataTable,
                    this.workingDirectory,
                    this.traceType,
                    this.keyConverter,
                    this.isValueAReferenceType);

            using (recoveryStoreComponent)
            {
                await recoveryStoreComponent.RecoveryAsync(cancellationToken).ConfigureAwait(false);

                this.FileId = recoveryStoreComponent.FileId;
                this.lastCheckpointFileTimeStamp = recoveryStoreComponent.LogicalCheckpointFileTimeStamp;

                Diagnostics.Assert(this.ConsolidationManager != null, this.TraceType, "ConsolidatedComponent must be a TConsolidatedStoreComponent");

                long oldCount = Interlocked.Exchange(ref this.count, 0);
                Diagnostics.Assert(oldCount >= 0, this.TraceType, "Old count {0} must not be negative.", oldCount);

                List<Task> taskList = null;
                bool snapEnableSweep = this.EnableSweep;
                Stopwatch stopwatch = new Stopwatch();
                if (this.shouldLoadValuesInRecovery)
                {
                    FabricEvents.Events.PreloadValues(this.traceType, this.numberOfInflightValueRecoveryTasks, true, 0);
                    this.EnableSweep = false;
                    taskList = new List<Task>(this.numberOfInflightValueRecoveryTasks);
                    stopwatch.Start();
                }

                foreach (var row in recoveryStoreComponent.GetEnumerable())
                {
                    if (row.Value.Kind == RecordKind.DeletedVersion)
                    {
                        continue;
                    }

                    this.ConsolidationManager.Add(row.Key, row.Value);

                    if (this.shouldLoadValuesInRecovery)
                    {
                        Task t = row.Value.GetValueAsync(
                            this.CurrentMetadataTable,
                            this.valueConverter,
                            this.isValueAReferenceType,
                            ReadMode.CacheResult,
                            this.LoadValueCounter,
                            cancellationToken,
                            this.traceType,
                            true /*duringRecovery*/);

                        taskList.Add(t);
                        Diagnostics.Assert(
                            taskList.Count <= this.numberOfInflightValueRecoveryTasks,
                            this.TraceType,
                            "Number of inflight value load tasks is limited to {0}. It can not be {1}",
                            this.numberOfInflightValueRecoveryTasks, taskList.Count);

                        // Check if the number of inflight value load tasks is at the max limit.
                        if (taskList.Count == this.numberOfInflightValueRecoveryTasks)
                        {
                            // Wait for at least one of them to complete.
                            await Task.WhenAny(taskList).ConfigureAwait(false);

                            // Remove all completed tasks from the list.
                            for (int i = 0; i < taskList.Count; i++)
                            {
                                if (taskList[i].IsCompleted)
                                {
                                    taskList.RemoveAt(i);
                                    i--;
                                }
                            }
                        }
                    }

                    // Using Invalid LSN in Recovery since we do not have the LSNs. 
                    this.IncrementCount(DifferentialStoreConstants.InvalidLSN, DifferentialStoreConstants.InvalidLSN);
                }

                if (this.shouldLoadValuesInRecovery)
                {
                    // Wait for the remaining 
                    await Task.WhenAll(taskList);

                    stopwatch.Stop();

                    // Introducing a memory barrier since order of the instructions is significant.
                    // We do not want to enable sweep before all load operations have completed.
                    Thread.MemoryBarrier();
                    this.EnableSweep = snapEnableSweep;

                    FabricEvents.Events.PreloadValues(this.traceType, this.numberOfInflightValueRecoveryTasks, true, stopwatch.ElapsedMilliseconds);
                }
                else
                {
                    Diagnostics.Assert(
                        taskList == null,
                        this.TraceType,
                        "If shouldLoadValuesInRecovery is disabled, inflight load value task list should be null.");
                }

                long consolidatedCount = this.ConsolidationManager.Count();
                Diagnostics.Assert(consolidatedCount == this.Count, traceType, "consolidatedComponent.Count {0} == this.Count {1}", consolidatedCount, this.Count);
            }

            UpdateInstantaneousStoreCounters(this.CurrentMetadataTable, out long itemCount, out long diskSize);

#if !DotNetCoreClr
            FabricEvents.Events.StoreSize(this.traceType, itemCount, diskSize);
#endif
        }

        /// <summary>
        /// Sets the current state of the store form the copy stream once.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        private async Task RecoverCopyStateAsync(CancellationToken cancellationToken)
        {
            // Check if copy is in progress.
            Diagnostics.Assert(this.copyManager != null, this.TraceType, "unexpectedly no longer building a new master table during copy");
            Diagnostics.Assert(this.copyManager.CopyCompleted == true, this.TraceType, "cannot recover copy state until copy is completed");

            this.CurrentMetadataTable = this.copyManager.MetadataTable;

            FabricEvents.Events.RecoverCopyStateAsync(this.traceType, "starting", -1);

            try
            {
                await this.RecoverConsolidatedStateAsync(cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                e = Diagnostics.ProcessException(string.Format("TStore.RecoverCopyStateAsync@{0}", this.traceType), e, "failed ");

                // Rethrow inner exception.
                throw e;
            }

            await this.FireRebuildNotificationCallerHoldsLockAsync().ConfigureAwait(false);

            this.copyManager = null;
            FabricEvents.Events.RecoverCopyStateAsync(this.traceType, DifferentialStoreConstants.RecoverCopyState_Completed, this.Count);
        }

        /// <summary>
        /// Computes the on-disk size in MB load metric.
        /// </summary>
        private long ComputeOnDiskLoadMetric()
        {
            Diagnostics.Assert(this.CurrentMetadataTable != null, this.TraceType, "cannot compute disk load metrics without a valid metadata table");

            var onDiskSizeInBytes = FabricFile.GetSize(this.CurrentDiskMetadataFilePath);
            foreach (var fileMetadata in this.CurrentMetadataTable.Table)
            {
                var checkpointFile = fileMetadata.Value.CheckpointFile;
                if (checkpointFile != null)
                {
                    var keyCheckpointPath = Path.Combine(this.workingDirectory, checkpointFile.KeyCheckpointFileName);
                    var valueCheckpointPath = Path.Combine(this.workingDirectory, checkpointFile.ValueCheckpointFileName);

                    onDiskSizeInBytes += FabricFile.GetSize(keyCheckpointPath);
                    onDiskSizeInBytes += FabricFile.GetSize(valueCheckpointPath);
                }
            }

            return onDiskSizeInBytes;
        }

        private void CopyMetadataTable(MetadataTable src, MetadataTable dest)
        {
            if (src.IsDoNotCopy)
            {
                return;
            }

            foreach (var keyValuePair in src.Table)
            {
                keyValuePair.Value.AddRef();
                dest.Table.Add(keyValuePair.Key, keyValuePair.Value);
            }
        }

        private void VerifyMetadataTable(MetadataTable metadataTable)
        {
            HashSet<long> checkpointFileTimeStampSet = new HashSet<long>();
            foreach (var item in metadataTable.Table)
            {
                var fileMetadata = item.Value;
                Diagnostics.Assert(!checkpointFileTimeStampSet.Contains(fileMetadata.TimeStamp), this.traceType, "Duplicate timestamps found");
                checkpointFileTimeStampSet.Add(fileMetadata.TimeStamp);
                fileMetadata.Validate();
            }
        }

        /// <summary>
        /// Performs a checkpoint. Assumes the start of the checkpoint is a full barrier.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <param name="checkpointMode">Represents different mode to perform checkpoint.</param>
        /// <returns></returns>
        private async Task CheckpointAsync(PerformCheckpointMode checkpointMode, CancellationToken cancellationToken)
        {
            // Create write only store transaction not associated with a replicator transaction.
            var rwtx = new StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(
                ReplicatorTransactionBase.CreateTransactionId(),
                null,
                this,
                this.traceType,
                null,
                false,
                this.enableStrict2PL);

            try
            {
                // Acquire shared lock on store component.
                await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, CheckpointOrCopyLockTimeout, true).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.CheckpointAsync@{0}", this.traceType), e, "store={0} ", "acquiring prime locks");
                Diagnostics.Assert(e is TimeoutException || e is OperationCanceledException, traceType, "unexpected exception {0}", e.GetType());

                // Release all acquired store locks if any.
                rwtx.Dispose();

                // Rethrow inner exception.
                throw e;
            }

            try
            {
                try
                {
                    FabricEvents.Events.CheckpointAsync(this.traceType, "dehydrating");

                    this.DeltaDifferentialState.Sort();

                    IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> items;
                    // Get the latest value from each key that has changed since the last checkpoint.
                    bool deltaDifferentialHasItems = this.DeltaDifferentialState.TryEnumerateKeyAndValue(out items, skipEphemeralKeys: true);

                    ConsolidationMode consolidationMode = ConsolidationMode.Default;
                    if (checkpointMode == PerformCheckpointMode.Periodic)
                    {
                        consolidationMode = ConsolidationMode.GDPR;
                    }

                    // TODO: Improve code sharing in the code below after sweep algorithm is done.
                    if (this.IsClearApplied && !this.IsPrepareAfterClear)
                    {
                        await this.PerformCheckpointAsyncWithPrepareThenClear(deltaDifferentialHasItems, items).ConfigureAwait(false);
                    }
                    else if (deltaDifferentialHasItems)
                    {
                        // Items present and no clear
                        if (!this.IsClearApplied)
                        {
                            var fileName = Guid.NewGuid().ToString();

                            // Write data files.
                            var fileId = this.IncrementFileId();
                            var timeStamp = this.NextCheckpointFileTimeStamp();
                            var filePath = Path.Combine(this.workingDirectory, fileName);
                            var checkpointFile =  await
                                CheckpointFile.CreateAsync(
                                        fileId,
                                        filePath,
                                        items,
                                        this.keyConverter,
                                        this.valueConverter,
                                        timeStamp,
                                        this.traceType,
                                        this.perfCounterSetInstance,
                                        this.isValueAReferenceType).ConfigureAwait(false);
                            FabricEvents.Events.CheckpointDataAsync(this.traceType, checkpointFile.KeyCount, checkpointFile.ValueCount, checkpointFile.DeletedKeyCount, fileId);

                            // Update  metadata to include the new id and checkpoint it.
                            var fileMetadata = new FileMetadata(
                                this.traceType,
                                fileId,
                                fileName,
                                checkpointFile.KeyCount,
                                checkpointFile.KeyCount,
                                timeStamp,
                                checkpointFile.DeletedKeyCount,
                                false,
                                checkpointFile.DeletedKeyCount == 0 ? FileMetadata.InvalidTimeStamp : timeStamp,
                                checkpointFile.DeletedKeyCount == 0 ? FileMetadata.InvalidTimeStamp : timeStamp,
                                this.MergeHelper.PercentageOfInvalidEntriesPerFile);

                            fileMetadata.Validate();

                            fileMetadata.CheckpointFile = checkpointFile;

                            // Populate next metadata table.
                            var tempMetadataTable = new MetadataTable(this.GetCheckpointLSN());
                            this.CopyMetadataTable(this.CurrentMetadataTable, tempMetadataTable);
                            MetadataManager.AddFile(tempMetadataTable.Table, fileId, fileMetadata);

#if DEBUG
                            this.VerifyMetadataTable(tempMetadataTable);
#endif
                            await this.ConsolidateAndSetNextMetadataTable(tempMetadataTable, true, consolidationMode);

                            // Checkpoint metadata table
                            await MetadataManager.WriteAsync(this.NextMetadataTable, this.TmpDiskMetadataFilePath).ConfigureAwait(false);
                        }
                        else if (this.IsPrepareAfterClear)
                        {
                            // Clear then Prepare.

                            // Set CanBeDeleted flag.
                            foreach (var item in this.CurrentMetadataTable.Table)
                            {
                                var fileMetadataToBeDeleted = item.Value;
                                fileMetadataToBeDeleted.CanBeDeleted = true;

                                // If the file is already present, do not add ref it again/
                                if (!this.filesToBeDeleted.ContainsKey(fileMetadataToBeDeleted))
                                {
                                    fileMetadataToBeDeleted.AddRef();
                                    bool add = this.filesToBeDeleted.TryAdd(item.Value, /*immediate:*/ true);
                                    Diagnostics.Assert(add, traceType, "Failed to add file id {0}", item.Value.FileId);
                                }
                            }

                            // Items present and clear before prep checkpoint

                            // Write data files.
                            var fileName = Guid.NewGuid().ToString();
                            var filePath = Path.Combine(this.workingDirectory, fileName);
                            var fileId = this.IncrementFileId();
                            var timeStamp = this.NextCheckpointFileTimeStamp();
                            var checkpointFile =
                                await
                                    CheckpointFile.CreateAsync(
                                        fileId,
                                        filePath,
                                        items,
                                        this.keyConverter,
                                        this.valueConverter,
                                        timeStamp,
                                        this.traceType,
                                        this.perfCounterSetInstance,
                                        this.isValueAReferenceType).ConfigureAwait(false);                           
                            FabricEvents.Events.CheckpointDataAsync(this.traceType, checkpointFile.KeyCount, checkpointFile.ValueCount, checkpointFile.DeletedKeyCount, fileId);

                            // Update  metadata to include the new id and checkpoint it.
                            var fileMetadata = new FileMetadata(
                                this.traceType,
                                fileId,
                                fileName,
                                checkpointFile.KeyCount,
                                checkpointFile.KeyCount,
                                timeStamp,
                                checkpointFile.DeletedKeyCount,
                                false,
                                checkpointFile.DeletedKeyCount == 0 ? FileMetadata.InvalidTimeStamp : timeStamp,
                                checkpointFile.DeletedKeyCount == 0 ? FileMetadata.InvalidTimeStamp : timeStamp,
                                this.MergeHelper.PercentageOfInvalidEntriesPerFile);
                            fileMetadata.CheckpointFile = checkpointFile;

                            // Populate next metadata table with new entry only
                            var tempMetadataTable = new MetadataTable(this.GetCheckpointLSN());
                            MetadataManager.AddFile(tempMetadataTable.Table, fileId, fileMetadata);

#if DEBUG
                            this.VerifyMetadataTable(tempMetadataTable);
#endif

                            await this.ConsolidateAndSetNextMetadataTable(tempMetadataTable, true, consolidationMode);

                            // Checkpoint next metadata table with one entry only since the data before prep checkpoint has been cleard.
                            await MetadataManager.WriteAsync(this.NextMetadataTable, this.TmpDiskMetadataFilePath).ConfigureAwait(false);
                        }
                    }
                    else
                    {
                        if (this.IsClearApplied && this.IsPrepareAfterClear)
                        {
                            // Clear then Prepare.

                            // Set CanBeDeleted flag.
                            foreach (var item in this.CurrentMetadataTable.Table)
                            {
                                var fileMetadataToBeDeleted = item.Value;
                                fileMetadataToBeDeleted.CanBeDeleted = true;

                                // If the file is already present, do not add ref it again/
                                if (!this.filesToBeDeleted.ContainsKey(fileMetadataToBeDeleted))
                                {
                                    fileMetadataToBeDeleted.AddRef();
                                    bool add = this.filesToBeDeleted.TryAdd(item.Value, /*immediate:*/ true);
                                    Diagnostics.Assert(add, traceType, "Failed to add file id {0}", item.Value.FileId);
                                }
                            }

                            var tempMetadataTable = new MetadataTable(this.GetCheckpointLSN());
                            await this.ConsolidateAndSetNextMetadataTable(tempMetadataTable, false, consolidationMode);

                            // Checkpoint next metadata table with no entries since the data before prep checkpoint has been cleard.
                            await MetadataManager.WriteAsync(this.NextMetadataTable, this.TmpDiskMetadataFilePath).ConfigureAwait(false);
                        }
                        else
                        {
                            // Following copy, updated checkpoint file needs to be written even there is no delta and no clear.

                            // Populate next metadata table so that current reference is not changed.
                            var tempMetadataTable = new MetadataTable(this.GetCheckpointLSN());
                            this.CopyMetadataTable(this.CurrentMetadataTable, tempMetadataTable);

                            await this.ConsolidateAndSetNextMetadataTable(tempMetadataTable, true, consolidationMode);

                            await MetadataManager.WriteAsync(this.NextMetadataTable, this.TmpDiskMetadataFilePath).ConfigureAwait(false);
                        }
                    }
                }
                catch (Exception e)
                {
                    FabricEvents.Events.Error_CheckpointAsync(this.traceType, this.workingDirectory, e.Message);
                    throw;
                }

                // Reset delta state.
                this.DeltaDifferentialState = null;

                // Reset clear flags.
                this.IsClearApplied = false;
                this.IsPrepareAfterClear = false;
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.CheckpointAsync@{0}", this.traceType), e, "failed ");
                Diagnostics.Assert(e is IOException || e is OperationCanceledException, traceType, "unexpected exception {0}", e.GetType());

                // Rethrow inner exception.
                throw e;
            }
            finally
            {
                // Release locks.
                rwtx.Dispose();
            }
        }

        private async Task ConsolidateAndSetNextMetadataTable(MetadataTable tempMetadataTable, bool consolidate, ConsolidationMode mode)
        {
            if (this.EnableBackgroundConsolidation)
            {
                this.ProcessConsolidationTask(tempMetadataTable);

                // Update the consolidated state only after assigning the next metadata table.
                // Set next metadata table only after adding and merging of items.
                // Merge metadata table gets updated as a part of processing consolidation task.
                this.NextMetadataTable = tempMetadataTable;

                if (consolidate)
                {
                    // Do not start consolidation task, if it is already in progress.
                    if (this.consolidationTask == null)
                    {
                        this.MergeMetadataTable = null;

                        // Consolidate after writing file so that the correct offset can be updated.
                        this.consolidationTask = this.ConsolidationManager.ConsolidateAsync(
                             string.Format("TStore.ConsolidateAsync@CheckpointAsync@{0}", this.traceType),
                             this.perfCounterSetInstance,
                             tempMetadataTable,
                             mode,
                             this.ConsolidateTaskCancellationSource.Token);
                    }
                }
            }
            else
            {
                Diagnostics.Assert(this.consolidationTask == null, this.TraceType, "ConsolidationTask should be null");
                this.MergeMetadataTable = null;

                if (consolidate)
                {
                    // Consolidate after writing file so that the correct offset can be updated.
                    this.consolidationTask = this.ConsolidationManager.ConsolidateAsync(
                         string.Format("TStore.ConsolidateAsync@CheckpointAsync@{0}", this.traceType),
                         this.perfCounterSetInstance,
                         tempMetadataTable,
                         mode,
                         this.ConsolidateTaskCancellationSource.Token);

                    await this.consolidationTask;
                    this.ProcessConsolidationTask(tempMetadataTable);
                    this.MergeMetadataTable = null;
                }

                // Update the consolidated state only after assigning the next metadata table.
                // Set next metadata table only after adding and merging of items.
                // Merge metadata table gets updated as a part of processing consolidation task.
                this.NextMetadataTable = tempMetadataTable;
                if(consolidate)
                {
                    // Note: This is needed only when bg consolidation is disbaled because next metadata table needs to be set 
                    // before switchin consolidating state to consolidtated state.
                    this.ConsolidationManager.ResetToNewAggregatedState();
                }

                Diagnostics.Assert(this.consolidationTask == null, this.TraceType, "ConsolidationTask should be null");
            }
        }

        private async Task PerformCheckpointAsyncWithPrepareThenClear(bool hasItems, IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> items)
        {
            Diagnostics.Assert(this.ConsolidationManager.Count() == 0, this.TraceType, "Consolidation Manager must be empty");

            // Set can be deleted but should not be dropped immediately, they cannot be removed in the next complete checkpoint, it needs to wait until the subsequent checkpoint.
            foreach (var item in this.CurrentMetadataTable.Table)
            {
                var fileMetadataToBeDeleted = item.Value;
                fileMetadataToBeDeleted.CanBeDeleted = true;

                // If the file is already present, do not add ref it again
                if (!this.filesToBeDeleted.ContainsKey(fileMetadataToBeDeleted))
                {
                    fileMetadataToBeDeleted.AddRef();
                    bool add = this.filesToBeDeleted.TryAdd(item.Value, /*immediate:*/ false);
                    Diagnostics.Assert(add, traceType, "Failed to add file id {0}", item.Value.FileId);
                }
            }

            this.consolidationTask = null;
            // Set merge metadata table to null because ApplyAsync for Clear resets this.ConsolidationManager
            this.MergeMetadataTable = null;

            var tempMetadataTable = new MetadataTable(this.GetCheckpointLSN(), true);
            this.CopyMetadataTable(this.CurrentMetadataTable, tempMetadataTable);

            if (hasItems)
            {
                // Write data files.
                var fileName = Guid.NewGuid().ToString();
                var fileId = this.IncrementFileId();
                var timeStamp = this.NextCheckpointFileTimeStamp();
                var filePath = Path.Combine(this.workingDirectory, fileName);
                var checkpointFile = await CheckpointFile.CreateAsync(
                            fileId,
                            filePath,
                            items,
                            this.keyConverter,
                            this.valueConverter,
                            timeStamp,
                            this.traceType,
                            this.perfCounterSetInstance,
                            this.isValueAReferenceType).ConfigureAwait(false);

                // Consolidation not needed, because ApplyAsync for Clear resets this.ConsolidationManager.
                // Update  metadata to include the new id and checkpoint it.
                var fileMetadata = new FileMetadata(
                    this.traceType,
                    fileId,
                    fileName,
                    checkpointFile.KeyCount,
                    checkpointFile.KeyCount,
                    timeStamp,
                    checkpointFile.DeletedKeyCount,
                    false,
                    checkpointFile.DeletedKeyCount == 0 ? FileMetadata.InvalidTimeStamp : timeStamp,
                    checkpointFile.DeletedKeyCount == 0 ? FileMetadata.InvalidTimeStamp : timeStamp,
                    this.MergeHelper.PercentageOfInvalidEntriesPerFile);
                fileMetadata.CheckpointFile = checkpointFile;

                MetadataManager.AddFile(tempMetadataTable.Table, fileId, fileMetadata);

                // The new file needs to be deleted on the next complete checkpoint.
                fileMetadata.CanBeDeleted = true;

                fileMetadata.AddRef();
                bool add = this.filesToBeDeleted.TryAdd(fileMetadata, /*immediate:*/ false);
                Diagnostics.Assert(add, traceType, "Failed to add file id {0}", fileMetadata.FileId);
            }

            // Checkpoint current MT, this is needed only when copy happens followed by clear.
            await MetadataManager.WriteAsync(tempMetadataTable, this.TmpDiskMetadataFilePath).ConfigureAwait(false);

            this.NextMetadataTable = tempMetadataTable;
        }

        private void ProcessConsolidationTask(MetadataTable tempMetadataTable)
        {
            PostMergeMetadatableInformation mergeMetadataTableInformation = null;

            // Check if previus consolidation task has completed.
            if (this.consolidationTask != null && this.consolidationTask.IsCompleted)
            {
                if (this.consolidationTask.Status != TaskStatus.RanToCompletion)
                {
                    // trace, throw excption
                    if (this.consolidationTask.Exception != null)
                    {
                        throw this.consolidationTask.Exception;
                    }
                }
                else
                {
                    mergeMetadataTableInformation = this.consolidationTask.Result;

                    this.consolidationTask = null;
                }
            }

            if (mergeMetadataTableInformation != null)
            {
                if (mergeMetadataTableInformation.NewMergedFile != null)
                {
                    MetadataManager.AddFile(tempMetadataTable.Table, mergeMetadataTableInformation.NewMergedFile.FileId, mergeMetadataTableInformation.NewMergedFile);
                }

                foreach (var fileId in mergeMetadataTableInformation.DeletedFileIds)
                {
                    // Mark the file as about to be deleted, and remove it from the metadata table.
                    var fileMetadata = tempMetadataTable.Table[fileId];
                    fileMetadata.CanBeDeleted = true;

                    // Add ref for files to be deleted.  These are commented out because the AddRef and ReleaseRef cancel each other out.
                    //fileMetadata.AddRef();
                    bool add = this.FilesToBeDeleted.TryAdd(fileMetadata, /*immediate:*/ true);
                    Diagnostics.Assert(add, traceType, "Failed to add file id {0}", fileMetadata.FileId);

                    // Release ref for file metadata, but it will be deleted on complete checkpoint along with the current metadatable being replaced.
                    //fileMetadata.ReleaseRef();
                    MetadataManager.RemoveFile(tempMetadataTable.Table, fileId, this.traceType);
                }
            }
        }

        /// <summary>
        /// Caller is expected to synchronize
        /// </summary>
        /// <returns></returns>
        private async Task ResetConsolidationManagerAsync()
        {
            this.ConsolidationManager.Reset();

            // Reset consolidation task.
            await this.ResetConsolidationTaskAsync().ConfigureAwait(false);

            // Stop any ongoing sweep
            await this.CancelSweepAsync().ConfigureAwait(false);
        }

        private void Sweep()
        {
            try
            {
                CancellationToken cancellationToken = this.SweepTaskCancellationSource.Token;
                if (!cancellationToken.IsCancellationRequested)
                {
                    // todo: Preethas, keep this trace around for a few weeks and remove or make it noise after background sweep has become stable.
                    FabricEvents.Events.SweepStarting(this.traceType);

                    // Reset the load counter.
                    this.LoadValueCounter.ResetCounter();

                    // Pass in cancellation token.
                    this.ConsolidationManager.Sweep(cancellationToken);

                    FabricEvents.Events.SweepCompleted(this.traceType);
                }
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("{0}@{1}", "TStore.Sweep", this.traceType), e, "failed");

                // Do not assert.
            }
            finally
            {
                var result = Interlocked.CompareExchange(ref this.sweepInProgress, 0, 1);
                Diagnostics.Assert(result == 1, this.TraceType, "Sweep should have been in progress");
            }
        }

        /// <summary>
        /// Initialize metrics
        /// </summary>
        private void InitializeMetrics()
        {
            this.metrics = this.transactionalReplicator.MetricManager.GetProvider(MetricProviderType.TStore) as TStoreMetricProvider;
            Diagnostics.Assert(this.metrics != null, this.TraceType, "TStore metrics could not be found");
            
            this.metrics.KeyTypeMetric.RegisterKeyType(typeof(TKey));
            this.metrics.StoreCountMetric.RegisterStore();
        }

        /// <summary>
        /// Uninitialize metrics
        /// </summary>
        private void UninitializeMetrics()
        {
            this.metrics.StoreCountMetric.UnregisterStore();
            this.metrics.KeyTypeMetric.UnregisterKeyType(typeof(TKey));

            this.metrics.DiskSizeMetric.Add(-this.lastRecordedDiskSize);
            this.lastRecordedDiskSize = 0;

            this.metrics.ItemCountMetric.Add(-this.lastRecordedItemCount);
            this.lastRecordedItemCount = 0;
        }

        /// <summary>
        /// Constructs all performance counters
        /// </summary>
        private void InitializePerformanceCounters()
        {
            this.perfCounterSetInstance = TStorePerformanceCounterSet.CreateCounterSetInstance(
                this.partitionId,
                this.replicaId,
                this.stateProviderId);

            this.itemCountPerfCounterWriter = new TStoreItemCountCounterWriter(this.perfCounterSetInstance);
            this.diskSizePerfCounterWriter = new TStoreDiskSizeCounterWriter(this.perfCounterSetInstance);
        }

        /// <summary>
        /// Closes all performance counters
        /// </summary>
        private void UninitializePerformanceCounters()
        {
            this.perfCounterSetInstance.Close();
        }

        /// <summary>
        /// Initializes the lock managers for this store.
        /// </summary>
        private void InitializeLockManager()
        {
            // Initialize lock managers.
            this.lockManager.Open(this.TraceType);
        }

        /// <summary>
        /// Uninitializes the lock managers for this store.
        /// </summary>
        private void UninitializeLockManager()
        {
            // Uninitialize lock managers.
            this.lockManager.Close();
        }

        /// <summary>
        /// Performs cleanup that might be needed in case copy fails.
        /// </summary>
        private async Task CleanupAsync()
        {
            FabricEvents.Events.OnCleanup(this.traceType, DifferentialStoreConstants.Cleanup_Starting);

            await this.CancelBackgroundConnsolidationTaskAsync();
            await this.CancelSweepTaskAsync();

            try
            {
                // Set isClosing flag to true before acquiring prime lock.
                this.isClosing = true;

                var rwtx = new StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(
                               ReplicatorTransactionBase.CreateTransactionId(),
                               null,
                               this,
                               this.traceType,
                               null,
                               false,
                               this.enableStrict2PL);
                try
                {
                    await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Exclusive, TStoreConstants.TimeoutForClosePrimeLockInMilliseconds, true).ConfigureAwait(false);
                }
                catch (TimeoutException)
                {
                }

                if (this.MergeMetadataTable != null)
                {
                    this.MergeMetadataTable.Dispose();
                }

                if (this.NextMetadataTable != null)
                {
                    FabricEvents.Events.OnCleanup(this.traceType, DifferentialStoreConstants.Cleanup_NextInProgressMasterTable_Closing);

                    this.NextMetadataTable.Dispose();
                }

                if (this.CurrentMetadataTable != null)
                {
                    this.CurrentMetadataTable.Dispose();
                }

                if (this.MetadataTableLock != null)
                {
                    this.MetadataTableLock.Dispose();
                }

                if (this.SnapshotContainer != null)
                {
                    this.SnapshotContainer.Dispose();
                }

                // Release files that need to be deleted.
                foreach (var fileMetadata in this.filesToBeDeleted.Keys)
                {
                    // Reset because close/abort can come in anytime  before completecheckpoint and these files cannot be deleted.
                    fileMetadata.CanBeDeleted = false;
                    fileMetadata.ReleaseRef();

                    // Assert that this should be zero
                    Diagnostics.Assert(fileMetadata.ReferenceCount == 0, this.TraceType, "Unexpected file metadata refernce count {0} for file {1}", fileMetadata.ReferenceCount, fileMetadata.CheckpointFile.KeyCheckpointFileName);
                }

                if (this.copyManager != null)
                {
                    this.copyManager.Dispose();
                }

                rwtx.Dispose();

                this.UninitializeLockManager();
                this.UninitializePerformanceCounters();
                this.UninitializeMetrics();

                FabricEvents.Events.OnCleanup(this.traceType, DifferentialStoreConstants.Cleanup_Completed);
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("TStore.CleanupAsync@{0}", this.traceType), e, "clean up failed");
                throw;
            }
        }

        private async Task CancelSweepTaskAsync()
        {
            // Complete the sweep task, if started.
            if (this.SweepTaskCancellationSource != null)
            {
                this.SweepTaskCancellationSource.Cancel();
            }

            // Await the sweep task following cancellation
            if (this.SweepTask != null)
            {
                await this.SweepTask.ConfigureAwait(false);
            }
        }

        private async Task CancelBackgroundConnsolidationTaskAsync()
        {
            // If consolidation task is in progress, then cancel it. 
            this.ConsolidateTaskCancellationSource.Cancel();

            try
            {
                if (this.consolidationTask != null)
                {
                    await this.consolidationTask;
                }
            }
            catch (Exception e)
            {
                // trace and swallow the exception.
                Diagnostics.ProcessException(string.Format("TStore.Cleanup@{0}", this.traceType), e, "cancelling consolidation task");
            }
        }

        /// <summary>
        /// Apply operation on primary.
        /// </summary>
        /// <param name="sequenceNumber"></param>
        /// <param name="replicatorTransaction"></param>
        /// <param name="metadataOperationData"></param>
        /// <returns></returns>
        private async Task<IStoreReadWriteTransaction> OnPrimaryApplyAsync(long sequenceNumber, ReplicatorTransactionBase replicatorTransaction, MetadataOperationData metadataOperationData)
        {
            // Find the store transaction that corresponds to this replicator transaction.
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx = null;
            this.inflightReadWriteStoreTransactions.TryGetValue(replicatorTransaction.Id, out rwtx);
            Diagnostics.Assert(null != rwtx, traceType, "unexpected transaction {0}", replicatorTransaction.Id);

            FabricEvents.Events.OnPrimaryApplyAsync(this.traceType, sequenceNumber, rwtx.Id);

            // Decrement primary operation count.
            rwtx.DecrementPrimaryOperationCount();

            if (metadataOperationData.Modification == StoreModificationType.Clear)
            {
                // Clear on Primary takes all prime locks for all components in Exclusive mode.
                // At this time, there should be no other transactions (read or read/write) active in the store.

                // Re-create store components as empty.
                this.DifferentialState = new TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(
                    this.transactionalReplicator,
                    this.stateProviderId,
                    this.SnapshotContainer,
                    this.LoadValueCounter,
                    this.isValueAReferenceType,
                    this.traceType);

                // Re-create consolidated state as empty.
                await this.ResetConsolidationManagerAsync().ConfigureAwait(false);

                this.ResetCount(sequenceNumber);
                this.itemCountPerfCounterWriter.ResetCount();

                // Fire notifications.
                this.FireClearNotification(replicatorTransaction);

                this.IsClearApplied = true;
            }
            else
            {
                MetadataOperationData<TKey> cachedKeyMetadataOperationData = metadataOperationData as MetadataOperationData<TKey>;
                Diagnostics.Assert(cachedKeyMetadataOperationData != null, this.TraceType, "metadataOperationData is MetadataOperationData<TKey>");

                var key = cachedKeyMetadataOperationData.Key;

                // If the key has been added in this tx, then removing this key does not have to be added to the differential state.
                var value = rwtx.Component.Read(key);
                Diagnostics.Assert(
                    value != null,
                    traceType, 
                    "value cannot be null, please check the implementation of {0} and ensure that it compares the original and de-serialized keys correctly (finds them to be equal)",
                    typeof(TKey).AssemblyQualifiedName);

                // Update commit sequence number
                value.VersionSequenceNumber = sequenceNumber;
                this.DifferentialState.Add(key, value, this.ConsolidationManager);

                if (cachedKeyMetadataOperationData.Modification == StoreModificationType.Add)
                {
                    this.IncrementCount(replicatorTransaction.Id, sequenceNumber);
                }
                else if (cachedKeyMetadataOperationData.Modification == StoreModificationType.Remove)
                {
                    this.DecrementCount(replicatorTransaction.Id, sequenceNumber);
                }
                
                this.FireSingleItemNotificationOnPrimary(replicatorTransaction, key, metadataOperationData);
            }

            return rwtx;
        }

        /// <summary>
        /// Applies a remove operation to a secondary replica.
        /// </summary>
        /// <param name="sequenceNumber">Replication sequence number for this operation.</param>
        /// <param name="rwtx">Store transaction.</param>
        /// <param name="metadataOperationData"></param>
        /// <param name="storeModificationType"></param>
        /// <returns></returns>
        private void OnUndoFalseProgressAsync(
            long sequenceNumber, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx, MetadataOperationData metadataOperationData,
            StoreModificationType storeModificationType)
        {
            // Retrieve key and value.
            var key = default(TKey);
            try
            {
                key = this.GetKeyFromBytes(metadataOperationData.KeyBytes);
            }
            catch (Exception e)
            {
                FabricEvents.Events.Error_OnUndoFalseProgressAsync(
                    this.traceType,
                    rwtx.Id,
                    DifferentialStoreConstants.OnUndoFalseProgress_Deserialization);

                FabricEvents.Events.ProcessExceptionWarning(
                    this.traceType,
                    e.Message,
                    e.GetType().ToString());

                // Rethrow exception.
                throw;
            }

            // Compute locking constructs.
            var keyLockResourceNameHash = CRC64.ToCRC64(metadataOperationData.KeyBytes);

            try
            {
                // Modify differential state. This is done before any apply comes in the replication stream.
                var dupeKeyInTxn = this.DifferentialState.UndoFalseProgress(key, keyLockResourceNameHash, sequenceNumber, storeModificationType);
                if (dupeKeyInTxn)
                {
                    return;
                }

                var currentVersionedItem = this.DifferentialState.Read(key);
                if (currentVersionedItem == null)
                {
                    currentVersionedItem = this.ConsolidationManager.Read(key);
                }

                this.UpdateCountForUndoOperation(rwtx.Id, sequenceNumber, metadataOperationData, currentVersionedItem);
                this.FireUndoNotifications(rwtx.ReplicatorTransactionBase, key, metadataOperationData, currentVersionedItem);
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.OnUndoFalseProgressAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.Assert(e is TimeoutException || e is FabricObjectClosedException, traceType, "unexpected exception {0}", e.GetType());

                // Rethrow inner exception.
                throw;
            }
        }

        private void UpdateCountForUndoOperation(long transactionId, long commitSequenceNumber, MetadataOperationData metadataOperationData, TVersionedItem<TValue> previousVersionedItem)
        {
            // Last operation on the key in txn is Add
            if (metadataOperationData.Modification == StoreModificationType.Add)
            {
                if (previousVersionedItem == null || previousVersionedItem.Kind == RecordKind.DeletedVersion)
                {
                    this.DecrementCount(transactionId, commitSequenceNumber);
                    return;
                }

                return;
            }

            // Last operation on the key in txn is Update
            if (metadataOperationData.Modification == StoreModificationType.Update)
            {
                if (previousVersionedItem == null || previousVersionedItem.Kind == RecordKind.DeletedVersion)
                {
                    this.DecrementCount(transactionId, commitSequenceNumber);
                    return;
                }

                return;
            }

            // Last operation on the key in txn is Remove
            if (metadataOperationData.Modification == StoreModificationType.Remove)
            {
                if (previousVersionedItem == null || previousVersionedItem.Kind == RecordKind.DeletedVersion)
                {
                    // If item previously did not exist, since the last notification for the given key is remove, this must be nooped.
                    return;
                }

                this.IncrementCount(transactionId, commitSequenceNumber);
                return;
            }
        }

        /// <summary>
        /// Applies an operation on the secondary.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="replicatorTransaction">Replicator transaction.</param>
        /// <param name="metadataOperationData"></param>
        /// <param name="operationRedoUndo">Data bytes to be applied.</param>
        /// <param name="isIdempotent"></param>
        /// <returns></returns>
        private async Task<IStoreReadWriteTransaction> OnSecondaryApplyAsync(
            long sequenceNumber, ReplicatorTransactionBase replicatorTransaction, MetadataOperationData metadataOperationData,
            RedoUndoOperationData operationRedoUndo, bool isIdempotent)
        {
            // Retrieve redo/undo operation.
            Diagnostics.ThrowIfNotValid(
                StoreModificationType.Add == metadataOperationData.Modification || StoreModificationType.Remove == metadataOperationData.Modification ||
                StoreModificationType.Clear == metadataOperationData.Modification || StoreModificationType.Update == metadataOperationData.Modification,
                "unexpected store operation type {0}",
                metadataOperationData.Modification);
            if (StoreModificationType.Clear == metadataOperationData.Modification)
            {
                Diagnostics.Assert(null == metadataOperationData.KeyBytes.Array, this.TraceType, "unexpected key bytes");
            }
            else
            {
                Diagnostics.Assert(null != metadataOperationData.KeyBytes.Array, this.TraceType, "unexpected key bytes");
            }

            // Find or create the store transaction correspondent to this replicator transaction.
            // MCoskun: Note that the transaction ids are not unique on secondary. This may not be the transaction you are looking for.
            bool firstCreated = false;
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx = this.FindOrCreateTransaction(replicatorTransaction, metadataOperationData.StoreTransaction, out firstCreated);
            FabricEvents.Events.OnSecondaryApplyAsync(this.traceType, firstCreated ? DifferentialStoreConstants.OnSecondaryApply_Starting : DifferentialStoreConstants.OnSecondaryApply_Completed, sequenceNumber, rwtx.Id);

            // Apply operation.
            switch (metadataOperationData.Modification)
            {
                case StoreModificationType.Add:
                    this.OnApplyAdd(replicatorTransaction, metadataOperationData, operationRedoUndo, isIdempotent, DifferentialStoreConstants.OnSecondaryApply);
                    break;
                case StoreModificationType.Remove:
                    this.OnApplyRemove(replicatorTransaction, metadataOperationData, isIdempotent, DifferentialStoreConstants.OnSecondaryApply);
                    break;
                case StoreModificationType.Clear:
                    // Idempotent flag is not needed for clear.
                    await this.OnApplyClearAsync(replicatorTransaction.CommitSequenceNumber, rwtx).ConfigureAwait(false);
                    return rwtx;
                case StoreModificationType.Update:
                    this.OnApplyUpdate(replicatorTransaction, metadataOperationData, operationRedoUndo, isIdempotent, DifferentialStoreConstants.OnSecondaryApply);
                    break;
            }

            return firstCreated ? rwtx : null;
        }

        /// <summary>
        /// Applies an add operation to a secondary replica.
        /// </summary>
        /// <param name="txn">Replicator transaction.</param>
        /// <param name="metadataOperationData"></param>
        /// <param name="operationRedoUndo">Data.</param>
        /// <param name="isIdempotent"></param>
        /// <param name="applyType"></param>
        /// <returns></returns>
        private void OnApplyAdd(
            ReplicatorTransactionBase txn, 
            MetadataOperationData metadataOperationData,
            RedoUndoOperationData operationRedoUndo, 
            bool isIdempotent,
            string applyType)
        {
            // Retrieve key and value.
            var key = default(TKey);
            var value = default(TValue);
            try
            {
                key = this.GetKeyFromBytes(metadataOperationData.KeyBytes);
                value = this.GetValueFromBytes(operationRedoUndo.ValueBytes);
            }
            catch (Exception e)
            {
                FabricEvents.Events.Error_OnApplyAddAsync(
                    this.traceType,
                    txn.Id,
                    DifferentialStoreConstants.OnApplyAdd_Deserialization,
                    applyType);

                FabricEvents.Events.ProcessExceptionWarning(
                    this.traceType,
                    e.Message,
                    e.GetType().ToString());

                // Rethrow exception.
                throw;
            }

            // Compute locking constructs.
            var keyLockResourceNameHash = CRC64.ToCRC64(metadataOperationData.KeyBytes);

            try
            {
                // If !couldBeDuplicate
                if (!isIdempotent)
                {
                    var currentVersionedItem = this.DifferentialState.Read(key);
                    if (currentVersionedItem == null)
                    {
                        currentVersionedItem = this.ConsolidationManager.Read(key);
                    }

                    // Assert: TVersionedItem must not exist or be a DeletedVersion.
                    if (currentVersionedItem != null)
                    {
                        Diagnostics.Assert(
                            currentVersionedItem.Kind == RecordKind.DeletedVersion,
                            this.TraceType,
                            "Cannot add an item that already exist. Txn: {0} CommitLSN: {1} CheckpointLSN: {2} key: {3} RecordKind: {4}",
                            txn.Id, txn.CommitSequenceNumber, this.CurrentMetadataTable.CheckpointLSN, keyLockResourceNameHash, currentVersionedItem.Kind);
                    }
                }

                if (!isIdempotent || this.ShouldValueBeAddedToDifferentialState(key, txn.CommitSequenceNumber))
                {
                    // Add the change to the store transaction write-set.
                    TVersionedItem<TValue> insertedVersion = new TInsertedItem<TValue>(
                        txn.CommitSequenceNumber,
                        value,
                        this.GetValueSize(operationRedoUndo.ValueBytes),
                        this.CanItemBeSweepedToDisk(value));
                    this.DifferentialState.Add(key, insertedVersion, this.ConsolidationManager);

                    long newCount = this.IncrementCount(txn.Id, txn.CommitSequenceNumber);

                    this.FireItemAddedNotificationOnSecondary(txn, key, value);

                    FabricEvents.Events.OnApplyAddAsync(
                        this.traceType,
                        txn.CommitSequenceNumber,
                        txn.Id,
                        keyLockResourceNameHash,
                        !IsNull(value) ? value.GetHashCode() : int.MinValue,
                        newCount,
                        applyType);
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.OnApplyAddAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.Assert(e is TimeoutException || e is FabricObjectClosedException, traceType, "TStore.OnApplyAddAsync: Unexpected exception {0}", e.ToString());

                // Rethrow inner exception.
                throw;
            }
        }

        /// <summary>
        /// Applies an add operation to a secondary replica.
        /// </summary>
        /// <param name="txn">Replicator transaction.</param>
        /// <param name="metadataOperationData"></param>
        /// <param name="operationRedoUndo">Data.</param>
        /// <param name="isIdempotent"></param>
        /// <param name="applyType"></param>
        /// <returns></returns>
        private void OnApplyUpdate(
            ReplicatorTransactionBase txn, 
            MetadataOperationData metadataOperationData,
            RedoUndoOperationData operationRedoUndo, 
            bool isIdempotent,
            string applyType)
        {
            // Retrieve key and value.
            var key = default(TKey);
            var value = default(TValue);
            try
            {
                key = this.GetKeyFromBytes(metadataOperationData.KeyBytes);
                value = this.GetValueFromBytes(operationRedoUndo.ValueBytes);
            }
            catch (Exception e)
            {
                FabricEvents.Events.Error_OnApplyUpdateAsync(
                    this.traceType,
                    txn.Id,
                    DifferentialStoreConstants.OnApplyUpdate_Deserialization,
                    applyType);

                FabricEvents.Events.ProcessExceptionWarning(
                    this.traceType,
                    e.Message,
                    e.GetType().ToString());

                // Rethrow exepction.
                throw;
            }

            // Compute locking constructs.
            var keyLockResourceNameHash = CRC64.ToCRC64(metadataOperationData.KeyBytes);

            try
            {
                // If !couldBeDuplicate
                if (!isIdempotent)
                {
                    var currentVersionedItem = this.DifferentialState.Read(key);
                    if (currentVersionedItem == null)
                    {
                        currentVersionedItem = this.ConsolidationManager.Read(key);
                    }

                    Diagnostics.Assert(
                        currentVersionedItem != null,
                        this.traceType,
                        "Cannot update an item that does not exist. Txn: {0} CommitLSN: {1} CheckpointLSN: {2} key: {3} RecordKind: Null",
                        txn.Id, txn.CommitSequenceNumber, this.CurrentMetadataTable.CheckpointLSN, keyLockResourceNameHash);

                    Diagnostics.Assert(
                        currentVersionedItem.Kind != RecordKind.DeletedVersion,
                        this.traceType,
                        "Cannot update an item that does not exist. Txn: {0} CommitLSN: {1} CheckpointLSN: {2} key: {3} RecordKind: {4}",
                        txn.Id, txn.CommitSequenceNumber, this.CurrentMetadataTable.CheckpointLSN, keyLockResourceNameHash, currentVersionedItem.Kind);
                }

                if (!isIdempotent || (isIdempotent && this.ShouldValueBeAddedToDifferentialState(key, txn.CommitSequenceNumber)))
                {
                    // Add the change to the store transaction write-set.
                    TVersionedItem<TValue> updatedVersion = new TUpdatedItem<TValue>(
                        txn.CommitSequenceNumber,
                        value,
                        this.GetValueSize(operationRedoUndo.ValueBytes),
                        this.CanItemBeSweepedToDisk(value));
                    this.DifferentialState.Add(key, updatedVersion, this.ConsolidationManager);

                    this.FireItemUpdatedNotificationOnSecondary(txn, key, value);

                    FabricEvents.Events.OnApplyUpdateAsync(
                        this.traceType,
                        txn.CommitSequenceNumber,
                        txn.Id,
                        keyLockResourceNameHash,
                        null != value ? value.GetHashCode() : int.MinValue,
                        applyType);
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.OnApplyUpdateAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.Assert(e is TimeoutException || e is FabricObjectClosedException, traceType, "TStore.OnApplyUpdateAsync: Unexpected exception {0}", e.ToString());

                // Rethrow inner exception.
                throw;
            }
        }

        /// <summary>
        /// Applies a remove operation to a secondary replica.
        /// </summary>
        /// <param name="txn">Replicator transaction.</param>
        /// <param name="metadataOperationData"></param>
        /// <param name="isIdempotent"></param>
        /// <param name="applyType"></param>
        /// <returns></returns>
        private void OnApplyRemove(
            ReplicatorTransactionBase txn, 
            MetadataOperationData metadataOperationData,
            bool isIdempotent,
            string applyType)
        {
            // Retrieve key.
            var key = default(TKey);
            try
            {
                key = this.GetKeyFromBytes(metadataOperationData.KeyBytes);
            }
            catch (Exception e)
            {
                FabricEvents.Events.Error_OnApplyRemoveAsync(
                    this.traceType,
                    txn.Id,
                    DifferentialStoreConstants.OnApplyRemove_Deserialization,
                    applyType);

                FabricEvents.Events.ProcessExceptionWarning(
                    this.traceType,
                    e.Message,
                    e.GetType().ToString());

                // Rethrow exception.
                throw;
            }

            // Compute locking constructs.
            var keyLockResourceNameHash = CRC64.ToCRC64(metadataOperationData.KeyBytes);

            try
            {
                if (!isIdempotent)
                {
                    var currentVersionedItem = this.DifferentialState.Read(key);
                    if (currentVersionedItem == null)
                    {
                        currentVersionedItem = this.ConsolidationManager.Read(key);
                    }

                    Diagnostics.Assert(
                        currentVersionedItem != null,
                        this.traceType,
                        "Cannot remove an item that does not exist. Txn: {0} CommitLSN: {1} CheckpointLSN: {2} key: {3} RecordKind: Null",
                        txn.Id, txn.CommitSequenceNumber, this.CurrentMetadataTable.CheckpointLSN, keyLockResourceNameHash);
                    Diagnostics.Assert(
                        currentVersionedItem.Kind != RecordKind.DeletedVersion,
                        this.traceType,
                        "Cannot remove an item that does not exist. Txn: {0} CommitLSN: {1} CheckpointLSN: {2} key: {3} RecordKind: {4}",
                        txn.Id, txn.CommitSequenceNumber, this.CurrentMetadataTable.CheckpointLSN, keyLockResourceNameHash, currentVersionedItem.Kind);
                }

                if (!isIdempotent || this.ShouldValueBeAddedToDifferentialState(key, txn.CommitSequenceNumber))
                {
                    // Add the change to the store transaction write-set.
                    TVersionedItem<TValue> deletedVersion = new TDeletedItem<TValue>(txn.CommitSequenceNumber);
                    this.DifferentialState.Add(key, deletedVersion, this.ConsolidationManager);

                    long newCount = this.DecrementCount(txn.Id, txn.CommitSequenceNumber);

                    this.FireItemRemovedNotificationOnSecondary(txn, key);

                    FabricEvents.Events.OnApplyRemoveAsync(this.traceType, txn.CommitSequenceNumber, txn.Id, keyLockResourceNameHash, newCount, applyType);
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.OnApplyRemoveAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.Assert(e is TimeoutException || e is FabricObjectClosedException, traceType, "TStore.OnApplyRemoveAsync: Unexpected exception {0}", e.ToString());

                // Rethrow inner exception.
                throw;
            }
        }

        /// <summary>
        /// Applies a clear operation to a secondary replica.
        /// </summary>
        /// <param name="sequenceNumber">Replication sequence number for this operation.</param>
        /// <param name="rwtx">Store transaction.</param>
        /// <returns></returns>
        private async Task OnApplyClearAsync(long sequenceNumber, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx)
        {
            // Used for tracing exceptions.
            var currentStoreComponent = string.Empty;

            try
            {
                // Zero lock timeout since there can be no conflict.
                await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Exclusive, Timeout.Infinite, true).ConfigureAwait(false);

                this.ThrowIfFaulted();

                // At this time, there should be no other transactions (read or read/write) active in the store.

                // Re-create store components as empty.
                this.DifferentialState = new TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(
                    this.transactionalReplicator,
                    this.stateProviderId,
                    this.SnapshotContainer,
                    this.LoadValueCounter,
                    this.isValueAReferenceType,
                    this.traceType);

                // Re-create consolidated state as empty.
                await this.ResetConsolidationManagerAsync().ConfigureAwait(false);

                long oldCount = Interlocked.Exchange(ref this.count, 0);
                Diagnostics.Assert(oldCount >= 0, traceType, "Old count {0} must not be negative.", oldCount);

                this.FireClearNotification(rwtx.ReplicatorTransactionBase);

                // Set clear flag, but not clear the metadata table yet.
                this.IsClearApplied = true;

                FabricEvents.Events.OnApplyClearAsync(this.traceType, sequenceNumber, rwtx.Id);
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.OnApplyClear@{0}", this.traceType), e, "store={0} ", currentStoreComponent);
                Diagnostics.Assert(e is TimeoutException, traceType, "TStore.OnApplyClear: Unexpected exception {0}", e.ToString());

                // Rethrow inner exception.
                throw e;
            }
        }

        /// <summary>
        /// Applies an operation during recovery.
        /// </summary>
        /// <param name="sequenceNumber">Operation sequence number.</param>
        /// <param name="replicatorTransaction">Replicator transaction.</param>
        /// <param name="metadata"></param>
        /// <param name="operationData">Data bytes to be applied.</param>
        /// <returns></returns>
        private async Task<IStoreReadWriteTransaction> OnRecoveryApplyAsync(
            long sequenceNumber, ReplicatorTransactionBase replicatorTransaction, OperationData metadata, OperationData operationData)
        {
            // Retrieve redo/undo operation.
            var metadataOperationData = MetadataOperationData.FromBytes(TStoreConstants.SerializedVersion, metadata);
            var operationRedoUndo = RedoUndoOperationData.FromBytes(operationData, this.traceType);
            Diagnostics.ThrowIfNotValid(
                StoreModificationType.Add == metadataOperationData.Modification || StoreModificationType.Remove == metadataOperationData.Modification ||
                StoreModificationType.Clear == metadataOperationData.Modification || StoreModificationType.Update == metadataOperationData.Modification,
                "unexpected store operation type {0}",
                metadataOperationData.Modification);
            if (StoreModificationType.Clear == metadataOperationData.Modification)
            {
                Diagnostics.Assert(null == metadataOperationData.KeyBytes.Array, this.TraceType, "unexpected key bytes");
            }
            else
            {
                Diagnostics.Assert(null != metadataOperationData.KeyBytes.Array, this.TraceType, "unexpected key bytes");
            }

            // Find or create the store transaction correspondent to this replicator transaction.
            bool firstCreated = false;
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx = this.FindOrCreateTransaction(replicatorTransaction, metadataOperationData.StoreTransaction, out firstCreated);
            FabricEvents.Events.OnRecoveryApplyAsync(this.traceType, firstCreated ? DifferentialStoreConstants.OnRecoveryApply_Starting : DifferentialStoreConstants.OnRecoveryApply_Completed, sequenceNumber, rwtx.Id);

            bool couldBeDuplicate = this.CurrentMetadataTable.CheckpointLSN == DifferentialStoreConstants.InvalidLSN;

            // Apply operation.
            switch (metadataOperationData.Modification)
            {
                case StoreModificationType.Add:
                    this.OnApplyAdd(replicatorTransaction, metadataOperationData, operationRedoUndo, couldBeDuplicate, DifferentialStoreConstants.OnRecoveryApply);
                    break;
                case StoreModificationType.Remove:
                    this.OnApplyRemove(replicatorTransaction, metadataOperationData, couldBeDuplicate, DifferentialStoreConstants.OnRecoveryApply);
                    break;
                case StoreModificationType.Update:
                    this.OnApplyUpdate(replicatorTransaction, metadataOperationData, operationRedoUndo, couldBeDuplicate, DifferentialStoreConstants.OnRecoveryApply);
                    break;
                case StoreModificationType.Clear:
                    await this.OnApplyClearAsync(sequenceNumber, rwtx).ConfigureAwait(false);
                    break;
            }

            return firstCreated ? rwtx : null;
        }

        /// <summary>
        /// Used to make sure that the store transaction is cleaned up on boundary cases.
        /// </summary>
        /// <param name="rwtx">Store transaction.</param>
        private bool FixupDanglingStoreTransactionIfNeeded(StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx)
        {
            // Check if the store transaction never replicated anything.
            if (0 == rwtx.PrimaryOperationCount)
            {
                FabricEvents.Events.FixupDanglingStoreTransactionIfNeeded(this.traceType, rwtx.Id);

                // Release locks.
                rwtx.Dispose();

                return true;
            }

            return false;
        }

        /// <summary>
        /// Checks if a key can be added.
        /// </summary>
        /// <param name="storeTransaction">Store transaction.</param>
        /// <param name="key">Key to be added.</param>
        /// <returns></returns>
        private bool CanKeyBeAdded(StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> storeTransaction, TKey key)
        {
            // Check to see if this key was already added as part of this store transaction.
            var versionedItem = storeTransaction.Component.Read(key);
            if (versionedItem == null)
            {
                // Capture the differential state. Use it in read-only mode.
                versionedItem = this.DifferentialState.Read(key);

                if (versionedItem == null)
                {
                    // Capture the consolidated state. Use it in read-only mode.
                    versionedItem = this.ConsolidationManager.Read(key);
                }
            }

            if (versionedItem == null || versionedItem is TDeletedItem<TValue>)
            {
                // Key does not exist. It can be added.
                return true;
            }

            // Key exists.  It cannot be added.
            return false;
        }

        /// <summary>
        /// Checks to see if a key can be updated or deleted.
        /// </summary>
        /// <param name="storeTransaction">Store transaction</param>
        /// <param name="key">Key to check.</param>
        /// <param name="conditionalVersion">Check version.</param>
        /// <param name="performVersionCheck">True if the version should be checked.</param>
        /// <param name="isVersionMismatch">Cannot be updated or deleted due to a version mismatch.</param>
        /// <returns></returns>
        private bool CanKeyBeUpdatedOrDeleted(
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> storeTransaction,
            TKey key,
            long conditionalVersion,
            bool performVersionCheck,
            out bool isVersionMismatch)
        {
            // Initialize out parameters.
            isVersionMismatch = false;

            // Check to see if this key exists in the write set.
            var versionedItem = storeTransaction.Component.Read(key);
            if (versionedItem == null)
            {
                // Capture the differential state. Use it in read-only mode.
                versionedItem = this.DifferentialState.Read(key);
                if (versionedItem == null)
                {
                    // Capture the consolidated state. Use it in read-only mode.
                    versionedItem = this.ConsolidationManager.Read(key);
                }
            }

            if (versionedItem == null || versionedItem is TDeletedItem<TValue>)
            {
                // The key does not exist, or was deleted. It cannot be updated or deleted.
                return false;
            }

            if (performVersionCheck)
            {
                // If the version does not match, the item cannot be updated or deleted.
                if (versionedItem.VersionSequenceNumber != conditionalVersion)
                {
                    isVersionMismatch = true;
                    return false;
                }
            }
            
            return true;
        }

        /// <summary>
        /// Get the current versioned and value for the given key
        /// </summary>
        /// <param name="storeTransaction">Store transaction</param>
        /// <param name="key">Key to check.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns></returns>
        private async Task<StoreComponentReadResult<TValue>> TryGetValueForReadWriteTransactionAsync(
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> storeTransaction,
            TKey key, 
            ReadMode readMode,
            CancellationToken cancellationToken)
        {
            try
            {
                // Check to see if this key exists in the write set.
                StoreComponentReadResult<TValue> readResult = new StoreComponentReadResult<TValue>(null, default(TValue));
                TVersionedItem<TValue> versionedItem = storeTransaction.Component.Read(key);
                if (versionedItem == null)
                {
                    // Capture the differential state. Use it in read-only mode.
                    versionedItem = this.DifferentialState.Read(key);
                    if (versionedItem == null)
                    {
                        // Capture the consolidated state. Use it in read-only mode.
                        readResult = await this.ReadFromConsolidatedStateAsync(key, readMode, cancellationToken).ConfigureAwait(false);
                    }
                }

                if (versionedItem != null)
                {
                    // Read from write-set or differential state so value will be present in memory.
                    readResult = new StoreComponentReadResult<TValue>(versionedItem, versionedItem.Value);
                }

                this.AssertIfUnexpectedVersionedItemValue(readResult, readMode);
                return readResult;
            }
            catch (ObjectDisposedException e)
            {
                Diagnostics.ProcessException(string.Format("TStore.TryGetValueForReadWriteTransactionAsync@{0}", this.traceType), e, "Reading value failed");
                this.ThrowIfFaulted();

                throw;
            }
        }

        /// <summary>
        /// Get the current versioned and value for the given key
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <param name="visibilitySequenceNumber">Sequence number that is visible for this read operation.</param>
        /// <param name="includeSnapshotContainer">Indiacte if snapshot container needs to be included or not.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns></returns>
        private async Task<StoreComponentReadResult<TValue>> TryGetValueForReadOnlyTransactionsAsync(
            TKey key,
            long visibilitySequenceNumber,
            bool includeSnapshotContainer,
            ReadMode readMode,
            CancellationToken cancellationToken)
        {
            try
            {
                bool itemPresentInSnapshotComponent = false;
                StoreComponentReadResult<TValue> readResult = new StoreComponentReadResult<TValue>(null, default(TValue));
                TVersionedItem<TValue> versionedItem = null;

                // Read from differential state.
                if (visibilitySequenceNumber == LogicalSequenceNumber.InvalidLsn.LSN)
                {
                    versionedItem = this.DifferentialState.Read(key);
                }
                else
                {
                    versionedItem = this.DifferentialState.Read(key, visibilitySequenceNumber);
                }

                if (versionedItem != null)
                {
                    readResult = new StoreComponentReadResult<TValue>(versionedItem, versionedItem.Value);
                }
                else
                {
                    if (includeSnapshotContainer)
                    {
                        // Assert that the visibility lsn is valid.
                        Diagnostics.Assert(visibilitySequenceNumber != LogicalSequenceNumber.InvalidLsn.LSN,
                            traceType,
                            "Invalid visibility sequence numner {0} for snapshot reads",
                            visibilitySequenceNumber);

                        // Check the snapshot container if there is a match to the visibility lsn.
                        var snapshotComponent = this.SnapshotContainer.Read(visibilitySequenceNumber);

                        if (snapshotComponent != null)
                        {
                            var readResultFromSnaphotComponent = await snapshotComponent.ReadAsync(key, visibilitySequenceNumber, readMode).ConfigureAwait(false);
                            if (readResultFromSnaphotComponent.HasValue)
                            {
                                TVersionedItem<TValue> versionedItemFromSnapshotComponent = readResultFromSnaphotComponent.VersionedItem;
                                Diagnostics.Assert(versionedItemFromSnapshotComponent != null, traceType, "versionedItemFromSnapshotComponent != null");

                                readResult = readResultFromSnaphotComponent;
                                itemPresentInSnapshotComponent = true;
                            }
                        }
                    }

                    if (!itemPresentInSnapshotComponent)
                    {
                        // Read from consolidated state.
                        readResult = await this.ReadFromConsolidatedStateAsync(key, visibilitySequenceNumber, readMode, cancellationToken).ConfigureAwait(false);
                    }
                }

                this.AssertIfUnexpectedVersionedItemValue(readResult, readMode);
                return readResult;
            }
            catch (ObjectDisposedException e)
            {
                Diagnostics.ProcessException(string.Format("TStore.TryGetValueForReadOnlyTransactionsAsync@{0}", this.traceType), e, "Reading value failed");
                this.ThrowIfFaulted();

                throw;
            }
        }

        /// <summary>
        /// Acquires necessary store and key locks in a given read/write transaction.
        /// </summary>
        /// <param name="lockingContext"></param>
        /// <param name="lockManager">Lock manager that owns the locks.</param>
        /// <param name="operationType">Store operation type.</param>
        /// <param name="keyLockResourceNameHash">Logical key hash to lock.</param>
        /// <param name="rwtx">Read/write transaction.</param>
        /// <param name="milliseconds">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns></returns>
        private async Task<ILock> AcquireKeyModificationLocksAsync(
            string lockingContext,
            LockManager lockManager,
            StoreModificationType operationType,
            ulong keyLockResourceNameHash,
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx,
            int milliseconds)
        {
            Diagnostics.Assert(
                StoreModificationType.Add == operationType || StoreModificationType.Remove == operationType || StoreModificationType.Update == operationType,
                this.traceType,
                "Error_TStore_OperationType. operationType got : {0}",
                operationType);

            // If the caller assumes locking responsability, then return immediately.
            if (0 != (rwtx.LockingHints & LockingHints.NoLock))
            {
                FabricEvents.Events.NoLock_AcquireKeyModificationLocksAsync(this.traceType, lockingContext, keyLockResourceNameHash, rwtx.Id);
                return null;
            }

            FabricEvents.Events.AcquireKeyModificationLocksAsync(
                this.traceType,
                lockingContext,
                (byte) LockMode.Exclusive,
                keyLockResourceNameHash,
                rwtx.Id,
                milliseconds,
                (long) rwtx.LockingHints);

            try
            {
                // Exclusive lock on this key allows a single write to occur on this key at any given time.
                return await rwtx.LockAsync(lockManager, keyLockResourceNameHash, LockMode.Exclusive, milliseconds).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(
                    string.Format("TStore.{0}", lockingContext),
                    e,
                    "could not lock key={0} txn={1} ",
                    keyLockResourceNameHash,
                    rwtx.Id);
                Diagnostics.Assert(e is TimeoutException || e is FabricObjectClosedException || e is TransactionFaultedException, traceType, "unexpected exception {0}", e.GetType());

                // Rethrow inner exception.
                throw e;
            }
        }

        /// <summary>
        /// Acquires necessary store and key lock in a given read/write transaction.
        /// </summary>
        /// <param name="lockingContext"></param>
        /// <param name="lockManager">Lock manager that owns the locks.</param>
        /// <param name="keyLockResourceNameHash">Logical key hash to lock.</param>
        /// <param name="rtx">Read transaction.</param>
        /// <param name="milliseconds">The timeout in milliseconds allowed for the operation to proceed.</param>
        /// <returns>
        /// Returns (false, null) in the case of unsuccessful readpast.
        /// Returns (true, null) in the case of no lock hints.
        /// Returns (true, lock) in the case of successful lock acquisition.
        /// </returns>
        private async Task<ConditionalValue<ILock>> AcquireKeyReadLockAsync(
            string lockingContext,
            LockManager lockManager,
            ulong keyLockResourceNameHash,
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rtx,
            int milliseconds)
        {
            var keyLockMode = (0 == (rtx.LockingHints & LockingHints.UpdateLock)) ? LockMode.Shared : LockMode.Update;

            // If the caller assumes locking responsibility, then return immediately.
            if (0 != (rtx.LockingHints & LockingHints.NoLock))
            {
                FabricEvents.Events.NoLock_AcquireKeyReadLocksAsync(this.traceType, lockingContext, keyLockResourceNameHash, rtx.Id, milliseconds, (long)rtx.LockingHints, (byte)rtx.Isolation);
                return new ConditionalValue<ILock>(true, null);
            }

            FabricEvents.Events.AcquireKeyReadLocksAsync(
                this.traceType,
                lockingContext,
                (byte) keyLockMode,
                keyLockResourceNameHash,
                rtx.Id,
                milliseconds,
                (long) rtx.LockingHints,
                (byte) rtx.Isolation);

            // Recompute short duration.
            var shortDurationLocks = (StoreTransactionReadIsolationLevel.ReadCommitted == rtx.Isolation) && (0 == (rtx.LockingHints & LockingHints.UpdateLock));
            var readPastLocks = 0 != (rtx.LockingHints & LockingHints.ReadPast);

            // Short duration locks are not added to the lock container.
            ILock keyLock = null;

            // Compute timeout if needed.
            var lockTimeout = readPastLocks ? 0 : milliseconds;

            try
            {
                // Shared/Update lock on this key allows a no write to occur on this key at this time.
                keyLock = shortDurationLocks
                    ? await lockManager.AcquireLockAsync(rtx.Id, keyLockResourceNameHash, keyLockMode, lockTimeout).ConfigureAwait(false)
                    : await rtx.LockAsync(lockManager, keyLockResourceNameHash, keyLockMode, lockTimeout).ConfigureAwait(false);
                if (shortDurationLocks)
                {
                    Diagnostics.Assert(
                        LockStatus.Invalid != keyLock.Status,
                        this.traceType,
                        "keyLock.Status = LockStatus.Invalid, TStore.AcquireKeyReadLockAsync");

                    if (LockStatus.Timeout == keyLock.Status)
                    {
#if DEBUG
                        Diagnostics.Assert(keyLock.OldestGrantee != -1, "OldestGrantee should not be -1. There should be at least one grantee");
#endif

                        throw new TimeoutException(
                            string.Format(
                                CultureInfo.CurrentCulture,
                                SR.LockTimeout_TStore_KeyLock,
                                keyLockMode,
                                this.traceType,
                                lockTimeout,
                                rtx.Id,
                                keyLockResourceNameHash,
                                keyLock.OldestGrantee));
                    }
                }
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(
                    string.Format("TStore.{0}", lockingContext),
                    e,
                    "could not lock key: {0} txn: {1} shortDurationLock: {2} lockHint: {3}",
                    keyLockResourceNameHash,
                    rtx.Id, 
                    shortDurationLocks,
                    rtx.LockingHints);
                
                // Note : ArgumentException should not assert in this case. 
                // ArgumentException is thrown by the AcquireLockAsync and LockAsync apis for the following reasons :
                //  1. If the timeout given is not in range, both APIs will throw.
                //  2. If the lockmode is Free, both APIs will throw.
                //  3. If the store transaction is readonly and we do not have a shared lock, LockAsync will throw.
                // In all the above cases, assert
                Diagnostics.Assert(
                    e is TimeoutException || e is FabricObjectClosedException || e is TransactionFaultedException, 
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Done if a lock with timeout zero failed.
                if (readPastLocks && !(e is FabricObjectClosedException))
                {
                    return new ConditionalValue<ILock>(false, null);
                }

                // Rethrow inner exception.
                throw e;
            }

            // Return lock acquired
            return new ConditionalValue<ILock>(true, keyLock);
        }

        /// <summary>
        /// Updates the value for a given key.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to update.</param>
        /// <param name="value">Value to update.</param>
        /// <param name="conditionalCheckVersion">Desired version of the key to update.</param>
        /// <param name="performVersionCheck">True to do a version check before attempting the update.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>Indicates whether the conditional update succeeded.</returns>
        private async Task<bool> ConditionalUpdateAsync(
            IStoreWriteTransaction transaction, TKey key, TValue value, long conditionalCheckVersion, bool performVersionCheck, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;

            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key bytes.
            var keyBytes = this.GetKeyBytes(key);
            if (null == keyBytes.Array)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Compute redo/undo information. This part can be done outside the lock.
            var valueBytes = this.GetValueBytes(default(TValue), value);
            var valueSize = this.GetValueSize(valueBytes);
            MetadataOperationData metadata = new MetadataOperationData<TKey, TValue>(key, value, TStoreConstants.SerializedVersion, StoreModificationType.Update, keyBytes, rwtx.Id, this.traceType);
            RedoUndoOperationData redoUpdate = new RedoUndoOperationData(valueBytes, null, this.traceType);

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            if (this.metrics.KeyValueSizeMetric.GetSamplingSuggestion())
            {
                this.metrics.KeyValueSizeMetric.Update(valueSize);
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await
                    this.AcquireKeyModificationLocksAsync(
                        "ConditionalUpdateAsync",
                        this.lockManager,
                        StoreModificationType.Update,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalUpdateAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            // Check to see if this key exists.
            if (!this.CanKeyBeUpdatedOrDeleted(rwtx, key, conditionalCheckVersion, performVersionCheck, out bool isVersionMismatch))
            {
                if (isVersionMismatch)
                {
                    FabricEvents.Events.Error_ConditionalUpdateAsync(this.traceType, keyLockResourceNameHash, rwtx.Id, DifferentialStoreConstants.VersionDoesnotMatch);
                }
                else
                {
                    FabricEvents.Events.Error_ConditionalUpdateAsync(this.traceType, keyLockResourceNameHash, rwtx.Id, DifferentialStoreConstants.KeyDoesnotExist);
                }

                // Key cannot be updated.
                return false;
            }

            // Compute redo/undo information. This cannot be done outside the lock because we need the previous value.
            IOperationData undoUpdate = null;

            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redoUpdate, undoUpdate, millisecondsRemaining, CancellationToken.None).ConfigureAwait(false);

                FabricEvents.Events.AcceptConditionalUpdateAsync(
                    this.traceType,
                    rwtx.Id,
                    keyLockResourceNameHash,
                    null != value ? value.GetHashCode() : int.MinValue);

                // Add the change to the store transaction write-set.
                var updatedVersion = new TUpdatedItem<TValue>(value, valueSize, this.CanItemBeSweepedToDisk(value));
                rwtx.Component.Add(false, key, updatedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalUpdateAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            // Return the appropriate version of the modification.
            return true;
        }

        /// <summary>
        /// Removes a given key and value from the store.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to remove.</param>
        /// <param name="conditionalCheckVersion">Specifies the desired version of the key to remove.</param>
        /// <param name="performVersionCheck">True to do a version check before attempting the remove.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was cancelled.</exception>
        /// <returns>True, existent version, and existent value of the item with that key, if it exists.</returns>
        private async Task<ConditionalValue<TValue>> ConditionalRemoveAsync(IStoreWriteTransaction transaction, TKey key, long conditionalCheckVersion, bool performVersionCheck, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;

            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure writes are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotWritable(rwtx.Id);

            // Extract key bytes.
            var keyBytes = this.GetKeyBytes(key);
            if (null == keyBytes.Array)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Compute redo information. This can be done outside the lock.
            MetadataOperationData metadata = new MetadataOperationData<TKey>(key, TStoreConstants.SerializedVersion, StoreModificationType.Remove, keyBytes, rwtx.Id, this.traceType);
            RedoUndoOperationData redo = new RedoUndoOperationData(null, null, this.traceType);
            TVersionedItem<TValue> deletedVersion = null;

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterWrite();
            }

            // Compute locking constructs.
            var milliseconds = this.GetTimeoutInMilliseconds(timeout);
            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
            int millisecondsRemaining;

            try
            {
                // Initialize store transaction.
                millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();

                // Acquire required locks for this operation.
                await
                    this.AcquireKeyModificationLocksAsync(
                        "ConditionalRemoveAsync",
                        this.lockManager,
                        StoreModificationType.Remove,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                // If the operation was cancelled during the lock wait, then terminate.
                cancellationToken.ThrowIfCancellationRequested();
            }
            catch (Exception e)
            {
                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalRemoveAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    e is TimeoutException || e is OperationCanceledException || e is FabricObjectClosedException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }

            StoreComponentReadResult<TValue> readResult = await this.TryGetValueForReadWriteTransactionAsync(rwtx, key, ReadMode.ReadValue, cancellationToken).ConfigureAwait(false);
            TVersionedItem<TValue> versionedItem = readResult.VersionedItem;
            bool isVersionedItemFound = readResult.HasValue;

            // An item that does not exist cannot be removed.
            if (isVersionedItemFound == false || versionedItem.Kind == RecordKind.DeletedVersion)
            {
                // Key cannot be removed.
                FabricEvents.Events.Error_ConditionalRemoveAsync(this.traceType, keyLockResourceNameHash, rwtx.Id, DifferentialStoreConstants.KeyDoesnotExist);
                return new ConditionalValue<TValue>();
            }

            // Item cannot be removed if the remove conditions are not met.
            if (performVersionCheck && versionedItem.VersionSequenceNumber != conditionalCheckVersion)
            {
                // Key cannot be removed.
                FabricEvents.Events.Error_ConditionalRemoveAsync(this.traceType, keyLockResourceNameHash, rwtx.Id, DifferentialStoreConstants.VersionDoesnotMatch);
                return new ConditionalValue<TValue>();
            }

            // Compute undo information. This cannot be done outside the lock because we need the previous value.
            IOperationData undo = null;
            try
            {
                // Replicate.
                await this.ReplicateOperationAsync(rwtx, metadata, redo, undo, millisecondsRemaining, cancellationToken).ConfigureAwait(false);

                FabricEvents.Events.AcceptConditionalRemoveAsync(this.traceType, rwtx.Id, keyLockResourceNameHash);

                // Add the change to the store transaction write-set.
                deletedVersion = new TDeletedItem<TValue>();
                rwtx.Component.Add(false, key, deletedVersion);
            }
            catch (Exception e)
            {
                // Release locks if this store transaction never replicated anything and the exception is not retryable.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalRemoveAsync@{0}", this.traceType), e, "replication key={0} ", keyLockResourceNameHash);
                Diagnostics.TraceIfNot(
                    this.IsNonRetryableReplicationException(e) || e is OperationCanceledException || e is TimeoutException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow.
                throw e;
            }

            // Return the appropriate version of the modification.
            this.AssertIfUnexpectedVersionedItemValue(readResult, ReadMode.ReadValue);
            return new ConditionalValue<TValue>(true, readResult.UserValue);
        }

        /// <summary>
        /// Gets the value for a given key. The read isolation level is observed.
        /// </summary>
        /// <param name="transaction">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the store is closed or deleted.</exception>
        /// <returns>1. Snapshot - last committed value according to snapshot isolation visibility rules as of when the store transaction started.</returns>
        /// <returns>2. ReadCommittedSnapshot - last committed value according to snapshot isolation visibility rules as of when this call is made.</returns>
        /// <returns>3. ReadCommitted - last committed value using short duration locks (locks kept for the duration of the call).</returns>
        /// <returns>4. ReadRepeatable - last committed value using long duration locks (locks kept until the end of transaction).</returns>
        private async Task<ConditionalValue<TValue>> ConditionalGetAsync(IStoreTransaction transaction, TKey key, ReadMode readMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            Stopwatch stopwatch = null;
            var readLatencyMetric = this.metrics.ReadLatencyMetric;
            bool shouldSampleMetricMeasurement = readLatencyMetric.GetSamplingSuggestion();

            if (shouldSampleMetricMeasurement)
            {
                stopwatch = Stopwatch.StartNew();
            }

            // Check arguments.
            var rwtx = transaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;

            if (null == rwtx || rwtx.Owner != this)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (null == key)
            {
                throw new ArgumentNullException(SR.Error_Key);
            }

            // Make sure reads are allowed.
            this.ThrowIfFaulted(rwtx);
            this.ThrowIfNotReadable(rwtx);

            if (this.metrics.ReadWriteMetric.GetSamplingSuggestion())
            {
                this.metrics.ReadWriteMetric.RegisterRead();
            }

            var milliseconds = this.GetTimeoutInMilliseconds(timeout);

            try
            {
                // Initialize store transaction.
                var millisecondsRemaining = await rwtx.AcquirePrimeLocksAsync(this.lockManager, LockMode.Shared, milliseconds, false).ConfigureAwait(false);

                this.ThrowIfFaulted(rwtx);

                // Check if cancellation was issued.
                cancellationToken.ThrowIfCancellationRequested();

                // Returns the read version for this key.
                var readReturn = new ConditionalValue<TValue>();

                // Check to see if the key was written as part of this store transaction, if this is a write store transaction.
                if (!rwtx.IsWriteSetEmpty)
                {
                    // Read the value for this key.
                    // Since this is in the transaction write set, there is a lock already taken for this key.
                    var versionItem = rwtx.Component.Read(key);
                    if (versionItem != null)
                    {
                        // Return.
                        if (versionItem is TDeletedItem<TValue>)
                        {
                            // Key was deleted.
                            // Extract key bytes.
                            var keyBytes = this.GetKeyBytes(key);
                            if (null == keyBytes.Array)
                            {
                                throw new ArgumentNullException(SR.Error_Key);
                            }
                            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);

                            FabricEvents.Events.NotFound_ConditionalGetAsync(
                                this.traceType,
                                DifferentialStoreConstants.Lock_Free,
                                rwtx.Id, keyLockResourceNameHash,
                                (byte)rwtx.Isolation,
                                FabricReplicator.UnknownSequenceNumber,
                                "keyfounddeleted");
                        }
                        else
                        {
                            // Key exists.
                            // Safe to get the value from versioned item since it is in the write set.
                            TValue value = versionItem.Value;
                            this.AssertIfUnexpectedVersionedItemValue(versionItem, value, readMode);
                            readReturn = new ConditionalValue<TValue>(true, value);
                        }

                        if (shouldSampleMetricMeasurement)
                        {
                            stopwatch.Stop();
                            readLatencyMetric.Update(stopwatch.ElapsedTicks);
                        }

                        return readReturn;
                    }

                    // Continue search in the differential and consolidated/consolidating components.
                }

                // The key was modified outside of this store tranasaction.
                if (StoreTransactionReadIsolationLevel.Snapshot == rwtx.Isolation || StoreTransactionReadIsolationLevel.ReadCommittedSnapshot == rwtx.Isolation)
                {
                    // Single version cannot allow snapshot reads. The visible version enumerators might not work since they execute twice.
                    if (this.storeBehavior == StoreBehavior.SingleVersion && StoreTransactionReadIsolationLevel.Snapshot == rwtx.Isolation)
                    {
                        throw new InvalidOperationException();
                    }

                    var visibilitySequenceNumber = await ((ITransaction)rwtx.ReplicatorTransactionBase).GetVisibilitySequenceNumberAsync().ConfigureAwait(false);

                    var readResult = await this.TryGetValueForReadOnlyTransactionsAsync(key, visibilitySequenceNumber, true, readMode, cancellationToken);

                    // Check if transaction has been disposed since replicator can dispose  a long running tx or 
                    // on change role and it can race with a read, 
                    // so checking here coz read from the snapshot container has already completed.
                    this.ThrowIfFaulted(rwtx);

                    if (readResult.HasValue)
                    {
                        TVersionedItem<TValue> versionedItem = readResult.VersionedItem;

                        // Key is visible.
                        if (versionedItem is TDeletedItem<TValue>)
                        {
                            // Key was deleted.
                            // Extract key bytes.
                            var keyBytes = this.GetKeyBytes(key);
                            if (null == keyBytes.Array)
                            {
                                throw new ArgumentNullException(SR.Error_Key);
                            }

                            var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);

                            FabricEvents.Events.NotFound_ConditionalGetAsync(
                                this.traceType,
                                DifferentialStoreConstants.Lock_Free,
                                rwtx.Id,
                                keyLockResourceNameHash,
                                (byte)rwtx.Isolation,
                                visibilitySequenceNumber,
                                "keyfounddeleted");
                        }
                        else
                        {
                            // Key exists.
                            // We don't need to load from disk here, because above we got it from
                            // differential (in memory) or consolidated (loaded from disk) or snapshot (loaded from disk)
                            this.AssertIfUnexpectedVersionedItemValue(readResult, readMode);
                            readReturn = new ConditionalValue<TValue>(true, readResult.UserValue);
                        }
                    }
                    else
                    {
                        // The key does not exist or is not visible for this reader.
                        // Extract key bytes.
                        var keyBytes2 = this.GetKeyBytes(key);
                        if (null == keyBytes2.Array)
                        {
                            throw new ArgumentNullException(SR.Error_Key);
                        }
                        var keyLockResourceNameHash2 = CRC64.ToCRC64(keyBytes2);

                        FabricEvents.Events.NotFound_ConditionalGetAsync(
                            this.traceType,
                            DifferentialStoreConstants.Lock_Free,
                            rwtx.Id,
                            keyLockResourceNameHash2,
                            (byte)rwtx.Isolation,
                            visibilitySequenceNumber,
                            "keydoesnotexist");
                    }
                }
                else if (StoreTransactionReadIsolationLevel.ReadCommitted == rwtx.Isolation ||
                         StoreTransactionReadIsolationLevel.ReadRepeatable == rwtx.Isolation)
                {
                    // Extract key bytes.
                    var keyBytes = this.GetKeyBytes(key);
                    if (null == keyBytes.Array)
                    {
                        throw new ArgumentNullException(SR.Error_Key);
                    }
                    var keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);

                    // Acquire read lock (short duration for read committed, long duration for read repeatable).
                    var readLock = await this.AcquireKeyReadLockAsync(
                        "ConditionalGetAsync",
                        this.lockManager,
                        keyLockResourceNameHash,
                        rwtx,
                        millisecondsRemaining).ConfigureAwait(false);

                    // Check if cancellation was issued.
                    cancellationToken.ThrowIfCancellationRequested();

                    if (readLock.HasValue)
                    {
                        var readResult = await this.TryGetValueForReadOnlyTransactionsAsync(key, LogicalSequenceNumber.InvalidLsn.LSN, false, readMode, cancellationToken);

                        // Read the value for this key.
                        if (readResult.HasValue)
                        {
                            var versionedItem = readResult.VersionedItem;

                            if (false == versionedItem is TDeletedItem<TValue>)
                            {
                                // Key exists.
                                this.AssertIfUnexpectedVersionedItemValue(readResult, readMode);
                                readReturn = new ConditionalValue<TValue>(true, readResult.UserValue);
                            }
                            #region preethas: re-enable again.
                            //else
                            //{
                            //    // Key was deleted.
                            //    FabricEvents.Events.ConditionalGetNotFoundTraceEvent(this.traceType, "lock-based", rtx.Id, keyLockResourceNameHash, (byte)rtx.Isolation, rtx.SnapshotVisibilitySequenceNumber, "keyfounddeleted");
                            //}
                            #endregion
                        }
                        #region preethas: enable after you get visibilty lsn from replicator.
                        //else
                        //{
                        //    // The key never existed or was truncated.
                        //    FabricEvents.Events.ConditionalGetNotFoundTraceEvent(this.traceType, "lock-based", rtx.Id, keyLockResourceNameHash, (byte)rtx.Isolation, rtx.SnapshotVisibilitySequenceNumber, "keydoesnotexist");
                        //}
                        #endregion

                        // Release short duration locks if necessary.
                        if (StoreTransactionReadIsolationLevel.ReadCommitted == transaction.Isolation && LockingHints.UpdateLock != transaction.LockingHints)
                        {
                            if (readLock.Value != null)
                            {
                                this.lockManager.ReleaseLock(readLock.Value);
                            }
                        }
                    }
                    #region preethas : re-enable again.
                    //else
                    //{
                    //    // The lock list is null only in the readpast case when the key could not be locked instantly.
                    //    FabricEvents.Events.ConditionalGetNotFoundTraceEvent(this.traceType, "lock-based", rtx.Id, keyLockResourceNameHash, (byte)rtx.Isolation, rtx.SnapshotVisibilitySequenceNumber, "keynotfoundreadpast");
                    //}
                    #endregion

                    // Make sure a read does not start in primary role and completes in secondary role.
                    this.ThrowIfNotReadable(rwtx);
                }

                if (shouldSampleMetricMeasurement)
                {
                    stopwatch.Stop();
                    readLatencyMetric.Update(stopwatch.ElapsedTicks);
                }

                return readReturn;
            }
            catch (Exception e)
            {
                // Extract key bytes.
                var keyBytes = this.GetKeyBytes(key);
                ulong keyLockResourceNameHash;
                if (null == keyBytes.Array)
                {
                    // Since we cannot throw in a catch block.
                    // 0 indicates that the key serialization might have failed.
                    keyLockResourceNameHash = 0;
                }
                else
                {
                    keyLockResourceNameHash = CRC64.ToCRC64(keyBytes);
                }

                // Check inner exception.
                e = Diagnostics.ProcessException(string.Format("TStore.ConditionalGetAsync@{0}", this.traceType), e, "key={0} ", keyLockResourceNameHash);

                // If GetKeyBytes returned a null array, then there is an issue in the user's serializer. In that case ArgumentException would be thrown.
                // Hence, we cannot assert and must throw the exception back to the user.
                Diagnostics.TraceIfNot(
                    e is FabricObjectClosedException || e is TimeoutException || e is OperationCanceledException || e is ArgumentException ||
                    e is FabricNotReadableException || e is TransactionFaultedException,
                    traceType,
                    "unexpected exception {0}",
                    e.GetType());

                // Rethrow inner exception.
                throw e;
            }
        }

        private Task<StoreComponentReadResult<TValue>> ReadFromConsolidatedStateAsync(TKey key, ReadMode readMode, CancellationToken cancellationToken)
        {
            return this.ReadFromConsolidatedStateAsync(key, LogicalSequenceNumber.InvalidLsn.LSN, readMode, cancellationToken);
        }

        private async Task<StoreComponentReadResult<TValue>> ReadFromConsolidatedStateAsync(TKey key, long visibilitySequenceNumber, ReadMode readMode, CancellationToken cancellationToken)
        {
            TVersionedItem<TValue> versionedItem = null;
            TValue value = default(TValue);

            // Note: Retry needs to be done at this layer since every time a load fails due to AddRef failure, the versioned item needs to be re-read.
            while (true)
            {
                if (visibilitySequenceNumber == LogicalSequenceNumber.InvalidLsn.LSN)
                {
                    versionedItem = this.ConsolidationManager.Read(key);
                }
                else
                {
                    versionedItem = this.ConsolidationManager.Read(key, visibilitySequenceNumber);
                }

                if (versionedItem != null && versionedItem.Kind != RecordKind.DeletedVersion && readMode == ReadMode.CacheResult)
                {
                    // If the TValue is a reference type, TrySet the inUse flag to 1.
                    versionedItem.SetInUseToTrue(this.IsValueAReferenceType);
                }

                if (versionedItem == null || readMode == ReadMode.Off)
                {
                    break;
                }

                value = versionedItem.Value;
                if (!versionedItem.ShouldValueBeLoadedFromDisk(this.isValueAReferenceType, value, this.traceType))
                {
                    break;
                }

                var result = await this.TryLoadValueAsync(versionedItem, readMode, cancellationToken);
                if (result.HasValue)
                {
                    value = result.Value;
                    break;
                }
            }

            if (versionedItem != null)
            {
                this.AssertIfUnexpectedVersionedItemValue(versionedItem, value, readMode);
            }

            return new StoreComponentReadResult<TValue>(versionedItem, value);
        }

        private async Task<ConditionalValue<TValue>> TryLoadValueAsync(TVersionedItem<TValue> versionedItem, ReadMode readMode, CancellationToken cancellationToken)
        {
            // Snap the current and next table upfront in the order of current first and then next.
            bool currentAddRefSucceeded = false;
            bool nextAddRefSucceeded = false;
            bool mergeAddRefSucceeded = false;
            TValue value = default(TValue);

            // Snap current and then next upfront so that current does not become next by the time this function completes.
            var currentMetadataTable = this.CurrentMetadataTable;
            Diagnostics.Assert(currentMetadataTable != null, this.TraceType, "current metadata table cannot be null");
            var nextMetadataTable = this.NextMetadataTable;
            var mergeMetadataTable = this.MergeMetadataTable;

            try
            {
                currentAddRefSucceeded = currentMetadataTable.TryAddRef();
                if (!currentAddRefSucceeded)
                {
                    // Current metadata table got disposed before we could read from it.
                    return new ConditionalValue<TValue>(false, value);
                }

                if (nextMetadataTable != null)
                {
                    nextAddRefSucceeded = nextMetadataTable.TryAddRef();
                    if (!nextAddRefSucceeded)
                    {
                        return new ConditionalValue<TValue>(false, value);
                    }
                }

                if (mergeMetadataTable != null)
                {
                    mergeAddRefSucceeded = mergeMetadataTable.TryAddRef();
                    if (!mergeAddRefSucceeded)
                    {
                        return new ConditionalValue<TValue>(false, value);
                    }
                }

                // Order is important. Check the merge table first and then next.
                if (mergeMetadataTable != null && mergeMetadataTable.Table.ContainsKey(versionedItem.FileId))
                {
                    value = await versionedItem.GetValueAsync(mergeMetadataTable, this.valueConverter, this.isValueAReferenceType, readMode, this.LoadValueCounter, cancellationToken, this.traceType).ConfigureAwait(false);
                    return new ConditionalValue<TValue>(true, value);
                }

                // Load the value from disk.
                // Load from next metadata table first.
                if (nextMetadataTable != null && nextMetadataTable.Table.ContainsKey(versionedItem.FileId))
                {
                    value = await versionedItem.GetValueAsync(nextMetadataTable, this.valueConverter, this.isValueAReferenceType, readMode, this.LoadValueCounter, cancellationToken, this.traceType).ConfigureAwait(false);
                    return new ConditionalValue<TValue>(true, value);
                }

                // Load using current metadata table.
                Diagnostics.Assert(currentMetadataTable.Table.ContainsKey(versionedItem.FileId), this.TraceType, "Current metadata table must contain the file id.");
                value = await versionedItem.GetValueAsync(currentMetadataTable, this.valueConverter, this.isValueAReferenceType, readMode, this.LoadValueCounter, cancellationToken, this.traceType).ConfigureAwait(false); // DISK-LOAD
                return new ConditionalValue<TValue>(true, value);
            }
            finally
            {
                if (currentAddRefSucceeded)
                {
                    currentMetadataTable.ReleaseRef();
                }

                if (nextAddRefSucceeded)
                {
                    nextMetadataTable.ReleaseRef();
                }
                if(mergeAddRefSucceeded)
                {
                    mergeMetadataTable.ReleaseRef();
                }
            }
        }

        /// <summary>
        /// Returns the timeout in milliseconds for a given timespan.
        /// </summary>
        /// <param name="timeout">Timeout.</param>
        /// <returns>Timeout in milliseconds.</returns>
        private  int GetTimeoutInMilliseconds(TimeSpan timeout)
        {
            if (Timeout.InfiniteTimeSpan == timeout)
            {
                return Timeout.Infinite;
            }

            if (TimeSpan.Zero == timeout)
            {
                return 0;
            }

            var milliseconds = timeout.TotalMilliseconds;
            if (milliseconds > int.MaxValue)
            {
                return Timeout.Infinite;
            }

            if (TimeSpan.Zero > timeout)
            {
                throw new ArgumentException(SR.Error_Timeout);
            }

            if (0.0 < milliseconds && DefaultSystemTimerResolution > milliseconds)
            {
                milliseconds = 16;
            }

            return checked((int)milliseconds);
        }

        /// <summary>
        /// Throws exception exception if the transaction has been aborted or committed.
        /// </summary>
        /// <exception cref="FabricObjectClosedException"></exception>
        /// <exception cref="InvalidOperationException"></exception>
        /// <devnote> This can be used for all argument checkes for read and write operations, not just for committed/aborted transaction, hence a generic name here</devnote>
        private void ThrowIfFaulted(StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx)
        {
            this.ThrowIfFaulted();
            rwtx.ReplicatorTransactionBase.ThrowIfTransactionIsNotActive();
        }

        /// <summary>
        /// Throws FabricObjectClosedException if the store is closing
        /// </summary>
        private void ThrowIfFaulted()
        {
            if (this.isClosing)
            {
                throw new FabricObjectClosedException();
            }
        }

        /// <summary>
        /// Throws exception if the state provider is not writable.
        /// </summary>
        private void ThrowIfNotWritable(long tracer)
        {
            var status = this.transactionalReplicator.StatefulPartition.WriteStatus;
            Diagnostics.Assert(PartitionAccessStatus.Invalid != status, this.TraceType, "invalid partition status");

            if (PartitionAccessStatus.NotPrimary == status)
            {
                FabricEvents.Events.ThrowIfNot(this.traceType, DifferentialStoreConstants.Writable, tracer, status.ToString(), this.transactionalReplicator.Role.ToString());
                throw new FabricNotPrimaryException();
            }
        }

        /// <summary>
        /// Throws exception if the state provider is not in the primary role.
        /// </summary>
        private void ThrowIfNotReadable(IStoreReadWriteTransaction rtx)
        {
            var status = this.transactionalReplicator.StatefulPartition.ReadStatus;
            Diagnostics.Assert(PartitionAccessStatus.Invalid != status, this.TraceType, "invalid partition status");

            var role = this.transactionalReplicator.Role;
            var grantReadStatus = PartitionAccessStatus.Granted == status;

            // If the replica is readable, no need to throw.
            // Note that this takes dependency on RA not making a replica readable
            // during Unknown and Idle.
            if (grantReadStatus)
            {
                return;
            }

            // MCoskun: If Primary, ReadStatus must be adhered to.
            // Not doing so will cause the replica to be readable during dataloss.
            if (role == ReplicaRole.Primary)
            {
                FabricEvents.Events.ThrowIfNotReadable(
                    this.traceType,
                    DifferentialStoreConstants.PrimaryNotReadable,
                    rtx.Id,
                    status.ToString(),
                    role.ToString(),
                    true);
                throw new FabricNotReadableException("Primary store is currently not readable.");
            }

            // If role is Active Secondary, we need to ignore ReadStatus since it is never granted on Active Secondary.
            // Instead we will use IsReadable which indicates that logging replicator has ensured the state is locally consistent.
            var isConsistent = this.transactionalReplicator.IsReadable;
            if (role == ReplicaRole.ActiveSecondary)
            {
                if (!this.isAlwaysReadable)
                {
                    grantReadStatus = false;
                }
                else
                {
                    grantReadStatus = isConsistent;
                }

                if (grantReadStatus)
                {
                    grantReadStatus = (StoreTransactionReadIsolationLevel.Snapshot == rtx.Isolation ||
                                       StoreTransactionReadIsolationLevel.ReadCommittedSnapshot == rtx.Isolation) && rtx.LockingHints == LockingHints.None;
                }
            }

            if (!grantReadStatus)
            {
                FabricEvents.Events.ThrowIfNotReadable(this.traceType, DifferentialStoreConstants.Readable, rtx.Id, status.ToString(), role.ToString(), isConsistent);
                throw new FabricNotReadableException();
            }
        }

        /// <summary>
        /// Utility routine that wraps the replication calls to avoid dealing with transient exceptions.
        /// </summary>
        /// <param name="rwtx">Store transaction that is replicating.</param>
        /// <param name="metaData"></param>
        /// <param name="redo">Redo info.</param>
        /// <param name="undo">Undo info.</param>
        /// <param name="timeoutInMilliseconds">Left over time.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        private async Task ReplicateOperationAsync(
            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx,
            MetadataOperationData metaData,
            RedoUndoOperationData redo,
            IOperationData undo,
            int timeoutInMilliseconds,
            CancellationToken cancellationToken)
        {
            Diagnostics.Assert(undo == null, this.traceType, "All TStore operations have null undo operationData.");

            var start = DateTime.UtcNow;

            var countRetry = 0;
            OperationData metaDataOperation = metaData;
            OperationData redoOperation = redo;

            // Exponential backoff starts at 16ms and stops at 4s.
            var exponentialBackoff = ExponentialBackoffMin;
            var isDone = false;
            var tx = rwtx.ReplicatorTransactionBase as ReplicatorTransaction;
            Diagnostics.Assert(null != tx, this.TraceType, "transaction cannot be null");
            while (!isDone)
            {
                try
                {
                    // Assume success.
                    isDone = true;

                    // Create operation context.
                    object context = 0 < rwtx.PrimaryOperationCount ? null : rwtx;

                    // Issue replication operation.
                    tx.AddOperation(metaDataOperation, null, redoOperation, context, this.stateProviderId);

                    // Increment primary write opration count.
                    rwtx.IncrementPrimaryOperationCount();
                }
                catch (Exception e)
                {
                    var ex = e is AggregateException ? e.InnerException : e;
                    if (!this.IsRetryableReplicationException(ex) && !this.IsNonRetryableReplicationException(ex) && !(ex is OperationCanceledException))
                    {
                        // There might be serialization exceptions.
                        FabricEvents.Events.Warning_ReplicateOperationAsync(this.traceType, rwtx.Id, ex.GetType() + " " + ex.Message);
                        throw ex;
                    }

                    // Retry replication if possible.
                    if (this.IsRetryableReplicationException(ex))
                    {
                        if (exponentialBackoff * 2 < ExponentialBackoffMax)
                        {
                            exponentialBackoff *= 2;
                        }
                        else
                        {
                            exponentialBackoff = ExponentialBackoffMax;
                        }

                        isDone = false;
                    }
                    else
                    {
                        // Trace the failure.
                        FabricEvents.Events.Error_ReplicateOperationAsync(this.traceType, rwtx.Id, ex.Message);

                        // Rethrow inner exception.
                        throw ex;
                    }
                }

                // Postpone the completion of this task in case of backoff.
                if (!isDone)
                {
                    if (timeoutInMilliseconds != Timeout.Infinite)
                    {
                        var end = DateTime.UtcNow;
                        var duration = end.Subtract(start);

                        if (duration.TotalMilliseconds > timeoutInMilliseconds)
                        {
                            throw new TimeoutException(
                                string.Format(
                                    CultureInfo.CurrentCulture,
                                    SR.ReplicationTimeout_TStore,
                                    this.traceType,
                                    timeoutInMilliseconds,
                                    rwtx.Id));
                        }
                    }

                    // Trace only after a few retries have failed.
                    countRetry++;
                    if (16 == countRetry)
                    {
                        FabricEvents.Events.Retry_ReplicateOperationAsync(this.traceType, rwtx.Id, exponentialBackoff);
                        countRetry = 0;
                    }

                    // Async sleep.
                    await Task.Delay(exponentialBackoff, cancellationToken).ConfigureAwait(false);

                    // Make sure writes are allowed.
                    this.ThrowIfFaulted(rwtx);
                    this.ThrowIfNotWritable(rwtx.Id);

                    // Stop processing if cancellation was requested.
                    cancellationToken.ThrowIfCancellationRequested();
                }
            }
        }

        /// <summary>
        /// Checks if a replication exception is retryable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        private bool IsRetryableReplicationException(Exception e)
        {
            return e is FabricTransientException;
        }

        /// <summary>
        /// Checks if a replication exception is non-retryable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        private bool IsNonRetryableReplicationException(Exception e)
        {
            if (e is FabricNotPrimaryException || e is FabricObjectClosedException || e is InvalidOperationException)
            {
                return true;
            }

            var fe = e as FabricException;
            if (null != fe)
            {
                return FabricErrorCode.ReplicationOperationTooLarge == fe.ErrorCode;
            }

            return false;
        }

        /// <summary>
        /// Creates an ordered enumerable over the store items within a specified range.
        /// </summary>
        /// <param name="storeTransaction">Transaction that owns this operation.</param>
        /// <param name="firstKey">Key range start.</param>
        /// <param name="useFirstKey"></param>
        /// <param name="lastKey">Key range end.</param>
        /// <param name="useLastKey"></param>
        /// <param name="filter">Key filter routine.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns></returns>
        private async Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            IStoreTransaction storeTransaction,
            TKey firstKey,
            bool useFirstKey,
            TKey lastKey,
            bool useLastKey,
            Func<TKey, bool> filter,
            bool isOrdered,
            ReadMode readMode)
        {
            // Check arguments.
            if (storeTransaction == null)
            {
                throw new ArgumentException(SR.Error_Transaction);
            }

            if (filter == null)
            {
                throw new ArgumentException(SR.Error_Filter);
            }

            if (useFirstKey)
            {
                if (firstKey == null)
                {
                    throw new ArgumentNullException(SR.Error_FirstKey);
                }
            }

            if (useLastKey)
            {
                if (lastKey == null)
                {
                    throw new ArgumentNullException(SR.Error_LastKey);
                }
            }

            if (useFirstKey && useLastKey)
            {
                if (this.keyComparer.Compare(firstKey, lastKey) > 0)
                {
                    throw new ArgumentException(SR.Error_FirstKey);
                }
            }

            // Enumerate all components. Skip over the keys that have been removed.
            var componentEnumerable = await this.CreateComponentEnumerableAsync(storeTransaction, firstKey, useFirstKey, lastKey, useLastKey, isOrdered, readMode, filter).ConfigureAwait(false);

            return new KeyValueAsyncEnumerable<TKey, TValue>(componentEnumerable, this.TraceType);
        }

        /// <summary>
        /// Provides and enumerable for a pair of store components.
        /// </summary>
        /// <param name="storeTransaction">Store transaction to use.</param>
        /// <param name="firstKey">First key in the range.</param>
        /// <param name="useFirstKey">Specifies if there is a low boundary.</param>
        /// <param name="lastKey">Last key in the range.</param>
        /// <param name="useLastKey">Specifies if there is a high boundary.</param>
        /// <param name="isOrdered">Specifies if the enumerable is required to be ordered.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <param name="keyFilter">Pushdown filter.</param>
        /// <returns></returns>
        private async Task<IEnumerable<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>>> CreateComponentEnumerableAsync(
            IStoreTransaction storeTransaction,
            TKey firstKey,
            bool useFirstKey,
            TKey lastKey,
            bool useLastKey,
            bool isOrdered,
            ReadMode readMode,
            Func<TKey, bool> keyFilter)
        {
            // Key enumerables.
            IEnumerable<TKey> differentialStateEnumerable = null;
            IEnumerable<TKey> consolidatedStateEnumerable = null;
            IEnumerable<TKey> snapshotStateEnumerable = null;
            IEnumerable<TKey> rwtxStateEnumerable = null;

            // Include the transaction write set if needed.
            var rwtx = storeTransaction as StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>;
            this.ThrowIfFaulted(rwtx);

            Diagnostics.Assert(rwtx != null, this.TraceType, "transaction cannot be null");

            SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> snapshotState = null;
            // Capture the snaphot state. Use it in read-only mode.
            ITransaction transaction = null;
            transaction = (ITransaction)rwtx.ReplicatorTransactionBase;

            var visibilityLsn = await transaction.GetVisibilitySequenceNumberAsync().ConfigureAwait(false);

            // If we try to read the visbility lsn before contents have been moved to snapshot, then this can be empty. We could miss a key if a key got deleted,
            // since we do not move deleted keys into consolidated state.
            if (rwtx.Isolation == StoreTransactionReadIsolationLevel.Snapshot || rwtx.Isolation == StoreTransactionReadIsolationLevel.ReadCommittedSnapshot)
            {
                snapshotState = this.SnapshotContainer.Read(visibilityLsn);
            }

            // Retrieve enumerables.
            differentialStateEnumerable = this.DifferentialState.GetEnumerableNewKeys();
            consolidatedStateEnumerable = this.ConsolidationManager.GetSortedKeyEnumerable(keyFilter, useFirstKey, firstKey, useLastKey, lastKey);

            if (snapshotState != null)
            {
                snapshotStateEnumerable = snapshotState.GetEnumerable();
            }

            // Add the store transaction enumerable.
            if (!rwtx.IsWriteSetEmpty)
            {
                rwtxStateEnumerable = rwtx.Component.GetSortedKeyEnumerable(keyFilter, useFirstKey, firstKey, useLastKey, lastKey);
            }

            FabricEvents.Events.CreateComponentEnumerableAsync(this.traceType, storeTransaction.Id, (byte)storeTransaction.Isolation, (int)readMode, visibilityLsn);

            // Build enumerable.
            SortedSequenceMergeManager<TKey> mergeManager = new SortedSequenceMergeManager<TKey>(this.KeyComparer, keyFilter, useFirstKey, firstKey, useLastKey, lastKey);
            mergeManager.Add(differentialStateEnumerable);
            mergeManager.Add(consolidatedStateEnumerable);

            if (null != rwtxStateEnumerable)
            {
                mergeManager.Add(rwtxStateEnumerable);
            }

            if (null != snapshotStateEnumerable)
            {
                mergeManager.Add(snapshotStateEnumerable);
            }

            var orderedKeyEnumerable = mergeManager.Merge();
            return this.EnumerateComponentKeys(orderedKeyEnumerable, storeTransaction, readMode);
        }

        private IEnumerable<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> EnumerateComponentKeys(IEnumerable<TKey> keys, IStoreTransaction storeTransaction, ReadMode readMode)
        {
            foreach (var x in keys)
            {
                // This is executed once for each move next call.
                var getTask = this.GetAsync(storeTransaction, x, readMode, storeTransaction.DefaultTimeout, CancellationToken.None);

                yield return getTask.ContinueWith(
                    t =>
                    {
                        // Return only if the previous task completed successfully.
                        if (!t.IsFaulted && !t.IsCanceled)
                        {
                            return new KeyValuePair<TKey, ConditionalValue<TValue>>(x, t.Result);
                        }

                        // Rethrow inner exception.
                        var e = Diagnostics.ProcessException(
                            string.Format("TStore.CreateComponentEnumerable@{0}", this.traceType),
                            t.Exception,
                            "GetAsync failed");
                        throw e;
                    },
                    TaskContinuationOptions.ExecuteSynchronously);
            }
        }

        /// <summary>
        /// Find and existent or create a read/write store transaction. Called during recovery and secondary operations.
        /// </summary>
        /// <param name="replicatorTransaction">Replicator transaction associated with the writes of this store transaction.</param>
        /// <param name="storeTransaction">Store transaction associated with the writes of this store transaction.</param>
        /// <param name="firstCreated">True, if store transaction was just created.</param>
        /// <returns></returns>
        private StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> FindOrCreateTransaction(
            ReplicatorTransactionBase replicatorTransaction, long storeTransaction, out bool firstCreated)
        {
            firstCreated = false;

            // Check arguments.
            if (null == replicatorTransaction)
            {
                throw new ArgumentNullException(SR.Error_ReplicatorTransaction);
            }

            StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> rwtx = null;

            // Attempt to retrieve the store transaction.
            this.inflightReadWriteStoreTransactions.TryGetValue(replicatorTransaction.Id, out rwtx);
            if (null == rwtx)
            {
                // Created store transaction. Notifications are fire donly if the store has become consistent.
                firstCreated = true;
                rwtx = new StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(
                    storeTransaction,
                    replicatorTransaction,
                    this,
                    this.traceType,
                    this.inflightReadWriteStoreTransactions,
                    true,
                    this.enableStrict2PL);

                ((IDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>>) this.inflightReadWriteStoreTransactions).Add(
                    rwtx.Id,
                    rwtx);

                // Reset default isolation level, if needed.
                if (this.storeBehavior == StoreBehavior.SingleVersion)
                {
                    rwtx.Isolation = StoreTransactionReadIsolationLevel.ReadCommittedSnapshot;
                }
            }

            return rwtx;
        }

        /// <summary>
        /// Computes the remaining operation time.
        /// </summary>
        /// <param name="availableTime">total available time for operation.</param>
        /// <param name="operationStartTime">operation start time.</param>
        /// <param name="operationEndTime">operation end time.</param>
        /// <returns>remaining time left over for the rest of the operation.</returns>
        private TimeSpan ComputeTotalRemainingTimeForOperation(TimeSpan availableTime, DateTime operationStartTime, DateTime operationEndTime)
        {
            var nextAvailableTime = TimeSpan.FromMilliseconds(0);
            if (availableTime == Timeout.InfiniteTimeSpan)
            {
                return availableTime;
            }

            if (availableTime.TotalMilliseconds > 0)
            {
                var duration = operationEndTime.Subtract(operationStartTime);

                // Compute next available time
                if (availableTime.Subtract(duration).TotalMilliseconds > 0)
                {
                    nextAvailableTime = availableTime.Subtract(duration);
                }
            }

            return nextAvailableTime;
        }

        /// <summary>
        /// Initializes the store in a Service Fabric context.
        /// </summary>
        /// <param name="transactionalReplicator">transactional replicator</param>
        /// <param name="partition">partition</param>
        /// <param name="initializationParameters">initialization parameters</param>
        /// <param name="name">state provider name</param>
        /// <param name="storeBehavior">store behavior</param>
        /// <param name="allowReadableSecondary"></param>
        /// <param name="stateProviderId"></param>
        /// <param name="workDirectory"></param>
        private void Initialize(
            TransactionalReplicator transactionalReplicator, IStatefulServicePartition partition, StatefulServiceContext initializationParameters, Uri name,
            StoreBehavior storeBehavior, bool allowReadableSecondary, long stateProviderId, string workDirectory)
        {
            this.Initialize(
                transactionalReplicator,
                partition,
                initializationParameters.PartitionId,
                initializationParameters.ReplicaId,
                initializationParameters.ServiceName,
                name,
                storeBehavior,
                allowReadableSecondary,
                stateProviderId,
                workDirectory);
        }

        /// <summary>
        /// Initializes the store
        /// </summary>
        /// <param name="transactionalReplicator">transactional replicator</param>
        /// <param name="partition">partition</param>
        /// <param name="partitionId">service partition id</param>
        /// <param name="replicaId">service replica id</param>
        /// <param name="serviceName">service name</param>
        /// <param name="name">state provider name</param>
        /// <param name="storeBehavior">store behavior</param>
        /// <param name="allowReadableSecondary">allow secondary replicas to perform read operations</param>
        /// <param name="stateProviderId">state provider unique id</param>
        /// <param name="workDirectory">working directory for the store</param>
        private void Initialize(
            ITransactionalReplicator transactionalReplicator,
            IStatefulServicePartition partition,
            Guid partitionId,
            long replicaId,
            Uri serviceName,
            Uri name,
            StoreBehavior storeBehavior,
            bool allowReadableSecondary,
            long stateProviderId,
            string workDirectory)
        {
            // Initialize hosting related members.
            this.partitionId = partitionId;
            this.replicaId = replicaId;
            this.stateProviderId = stateProviderId;
            this.serviceInstanceName = serviceName;
            this.Name = name;
            this.workingDirectory = workDirectory;
            this.transactionalReplicator = transactionalReplicator;
            this.storeBehavior = storeBehavior;

            this.keyConverter = transactionalReplicator.GetStateSerializer<TKey>(name);
            this.valueConverter = transactionalReplicator.GetStateSerializer<TValue>(name);
            this.CurrentMetadataTable = new MetadataTable(this.TraceType);

            //todo: this should be disposed on dispose.
            this.MetadataTableLock = new SemaphoreSlim(1);

            if (null == this.keyConverter)
            {
                throw new NotSupportedException(SR.Error_KeyConverter);
            }

            if (null == this.valueConverter)
            {
                throw new NotSupportedException(SR.Error_ValueConverter);
            }

            // Set the appropriate flag for secondary reads.
            this.isAlwaysReadable = allowReadableSecondary;

            this.traceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}@{1}@{2}@{3}",
                this.partitionId,
                this.replicaId,
                this.Name,
                stateProviderId);

            FabricEvents.Events.Constructor(
                this.traceType,
                typeof(TKey).ToString(),
                typeof(TValue).ToString(),
                storeBehavior.ToString(),
                allowReadableSecondary);

            this.isValueAReferenceType = !typeof(TValue).IsValueType;
            this.LoadValueCounter = new LoadValueCounter(this, this.TraceType);
            this.SweepTaskCancellationSource = new CancellationTokenSource();
            this.ConsolidateTaskCancellationSource = new CancellationTokenSource();

            // Ensure the state provider's working sub-directory exists (no-op if it already exists).
            FabricDirectory.CreateDirectory(this.workingDirectory);

            this.CurrentDiskMetadataFilePath = Path.Combine(workDirectory, currentDiskMetadataFileName);
            this.TmpDiskMetadataFilePath = Path.Combine(workDirectory, tempDiskMetadataFileName);
            this.BkpDiskMetadataFilePath = Path.Combine(workDirectory, bkpDiskMetadataFileName);

            this.SnapshotContainer = new SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(this.valueConverter, this.TraceType);

            // Create aggregated store components.
            this.DifferentialState =
                new TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(
                    this.transactionalReplicator,
                    this.stateProviderId,
                    this.SnapshotContainer,
                    this.LoadValueCounter,
                    this.isValueAReferenceType,
                    this.traceType);

            this.MergeHelper = new MergeHelper(this.TraceType);
            this.ConsolidationManager = new ConsolidationManager<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(this);
            this.EnableBackgroundConsolidation = true;

            this.storeSettings = new StoreSettings();
            StoreSettingsUtil.LoadFromClusterManifest(this.traceType, ref this.storeSettings);
            Diagnostics.Assert(storeSettings != null, this.traceType, "store settings cannot be null");
            Diagnostics.Assert(storeSettings.SweepThreshold != null, this.traceType, "storeSettings.SweepThreshold cannot be null");
            Diagnostics.Assert(this.storeSettings.EnableStrict2PL != null, this.traceType, "storeSettings.EnableStrict2PL cannot be null");

            if (IsSweepThresholdInValidRange(storeSettings.SweepThreshold.Value))
            {
                this.EnableSweep = true;
                this.SweepThreshold = storeSettings.SweepThreshold.Value;
            }
            else
            {
                Diagnostics.Assert(storeSettings.SweepThreshold.Value == TStoreConstants.SweepThresholdDisabledFlag, this.TraceType, "Sweep threshold value {0} cannot be less than -1", storeSettings.SweepThreshold.Value);

                // Disable when it is -1.
                this.EnableSweep = false;
            }

            this.enableStrict2PL = this.storeSettings.EnableStrict2PL.Value;

            FabricEvents.Events.TStoreEnableFlags(
                this.traceType,
                this.EnableSweep,
                this.SweepThreshold.ToString(),
                this.EnableBackgroundConsolidation,
                this.EnableStrict2PL);
        }

        private bool IsSweepThresholdInValidRange(int sweepThrehsoldValueFromSettings)
        {
            if (sweepThrehsoldValueFromSettings >= TStoreConstants.SweepThresholdDefaultFlag &&
                sweepThrehsoldValueFromSettings <= TStoreConstants.MaxSweepThreshold)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Appends to differential state.
        /// </summary>
        private void AppendDeltaDifferantialState()
        {
            Diagnostics.Assert(this.DeltaDifferentialState != null, this.TraceType, "Delta differential state cannot be null");
            foreach (var key in this.DifferentialState.EnumerateKeys())
            {
                var versions = this.DifferentialState.ReadVersions(key);
                if (versions.PreviousVersion != null)
                {
                    this.DeltaDifferentialState.Add(key, versions.PreviousVersion, this.ConsolidationManager);
                }

                Diagnostics.Assert(versions.CurrentVersion != null, this.TraceType, "current version cannot be null");
                this.DeltaDifferentialState.Add(key, versions.CurrentVersion, this.ConsolidationManager);
            }
        }

        /// <summary>
        /// Checks if the lsn higher than the given lsn is present in consolidated state.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="versionSequenceNumber"></param>
        /// <returns></returns>
        private bool ShouldValueBeAddedToDifferentialState(TKey key, long versionSequenceNumber)
        {
            // Check if a higher version sequence number is present in the consolidated state.
            var versionedItem = this.ConsolidationManager.Read(key);
            if (versionedItem != null)
            {
                if (versionedItem.VersionSequenceNumber >= versionSequenceNumber)
                {
                    return false;
                }
            }

            return true;
        }

        private long IncrementCount(long txnId, long commitLSN)
        {
            long newCount = Interlocked.Increment(ref this.count);
            Diagnostics.Assert(newCount >= 0, this.traceType, "New count {0} cannot be negative. TxnId: {1} CommitLSN: {2}", newCount, txnId, commitLSN);
            return newCount;
        }

        private long DecrementCount(long txnId, long commitLSN)
        {
            long newCount = Interlocked.Decrement(ref this.count);
            Diagnostics.Assert(newCount >= 0, this.traceType, "New count {0} cannot be negative. TxnId: {1} CommitLSN: {2}", newCount, txnId, commitLSN);
            return newCount;
        }

        private void ResetCount(long commitLSN)
        {
            long oldCount = Interlocked.Exchange(ref this.count, 0);
            Diagnostics.Assert(oldCount >= 0, this.traceType, "Old count {0} cannot be negative. CommitLSN: {1}", oldCount, commitLSN);
        }

        /// <summary>
        /// Gets the LSN for the next checkpoint.
        /// 
        /// In cases where newer checkpoint then expected is copied, it is possible for the lastPrepareCheckpointLSN to be less than the checkpoint lsn of the current checkpoint.
        /// </summary>
        /// <returns>LSN for the next checkpoint.</returns>
        private long GetCheckpointLSN()
        {
            Diagnostics.Assert(
                this.lastPrepareCheckpointLSN > DifferentialStoreConstants.InvalidLSN, 
                this.traceType, 
                "PrepareCheckpoint must have been called. lastPrepareCheckpointLSN: {0}",
                this.lastPrepareCheckpointLSN);
            Diagnostics.Assert(this.CurrentMetadataTable != null, this.traceType, "CurrentMetadataTable cannot be null");

            if (this.CurrentMetadataTable.CheckpointLSN > this.lastPrepareCheckpointLSN)
            {
                return this.CurrentMetadataTable.CheckpointLSN;
            }

            return this.lastPrepareCheckpointLSN;
        }

        private long GetDiskSize(MetadataTable metadataTable)
        {
            Diagnostics.Assert(
                metadataTable != null,
                this.TraceType,
                "TStore.GetDiskSize(): metadataTable is null.");

            var result = metadataTable.MetadataFileSize;

            foreach (var fileMetadata in metadataTable.Table.Values)
            {
                result += fileMetadata.GetFileSize();
            }

            return result;
        }

        /// <summary>
        /// Update store counters where we want to get the most recent count measurement.
        /// </summary>
        private void UpdateInstantaneousStoreCounters(MetadataTable metadataTable, out long itemCount, out long diskSize)
        {
            itemCount = this.Count;
            this.itemCountPerfCounterWriter.SetCount(itemCount);
            this.metrics.ItemCountMetric.Add(itemCount - this.lastRecordedItemCount);
            this.lastRecordedItemCount = itemCount;

            diskSize = GetDiskSize(metadataTable);
            this.diskSizePerfCounterWriter.SetSize(diskSize);
            this.metrics.DiskSizeMetric.Add(diskSize - this.lastRecordedDiskSize);
            this.lastRecordedDiskSize = diskSize;
        }

        #region Notifications

        /*
            Notifications are implemented as State Provider events.
            Changes are fired during Apply under the lock.
            All operations belonging to a transaction will generate a notification event.
            Only exception is false progress undo in secondary.
            In which case, at most one event per undo key will be fired to get consumer's state back in to state before false progressed transactions.
            This is because of the undoOperationData optimization (it does not have the previous value any more)
            TODO: Add configuration to give all or at most one notification during undo per key.

            Rebuild notification is fired after recovery and copy under a table lock.
            Rebuild notification contains a snapshot of the state to allow customer to reinitialize their state.
        */

        private void FireClearNotification(ReplicatorTransactionBase txn)
        {
            var eventHandler = this.DictionaryChanged;
            if (eventHandler == null)
            {
                return;
            }

            try
            {
                eventHandler(this, new NotifyDictionaryClearEventArgs<TKey, TValue>(txn.CommitSequenceNumber));
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("TStore.ClearNotification@{0}", this.traceType), e, string.Empty);
                throw;
            }
        }

        private void FireSingleItemNotificationOnPrimary(ReplicatorTransactionBase txn, TKey key, MetadataOperationData metadataOperationData)
        {
            var eventHandler = this.DictionaryChanged;
            if (eventHandler == null)
            {
                return;
            }

            try
            {
                var publicTxn = txn as ITransaction;
                Diagnostics.Assert(publicTxn != null, this.TraceType, "Must be an ITransaction");

                if (metadataOperationData.Modification == StoreModificationType.Remove)
                {
                    eventHandler(this, new NotifyDictionaryItemRemovedEventArgs<TKey, TValue>(publicTxn, key));

                    return;
                }

                MetadataOperationData<TKey, TValue> cachedKeyValueMetadataOperationData = metadataOperationData as MetadataOperationData<TKey, TValue>;
                Diagnostics.Assert(cachedKeyValueMetadataOperationData != null, this.traceType, "metadataOperationData is MetadataOperationData<TKey>. TxnId: {0}", txn.Id);

                // Retrieve value.
                if (metadataOperationData.Modification == StoreModificationType.Add)
                {
                    eventHandler(this, new NotifyDictionaryItemAddedEventArgs<TKey, TValue>(publicTxn, key, cachedKeyValueMetadataOperationData.Value));
                }
                else if (metadataOperationData.Modification == StoreModificationType.Update)
                {
                    eventHandler(this, new NotifyDictionaryItemUpdatedEventArgs<TKey, TValue>(publicTxn, key, cachedKeyValueMetadataOperationData.Value));
                }
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(
                    string.Format("TStore.PrimaryNotification@{0}@{1}", metadataOperationData.Modification, this.traceType),
                    e,
                    string.Empty);
                throw;
            }
        }

        private void FireItemAddedNotificationOnSecondary(ReplicatorTransactionBase txn, TKey key, TValue value)
        {
            var eventHandler = this.DictionaryChanged;
            if (eventHandler == null)
            {
                return;
            }

            try
            {
                var publicTxn = txn as ITransaction;
                Diagnostics.Assert(publicTxn != null, this.traceType, "Must be an ITransaction. TxnId: {0}", txn.Id);

                eventHandler(this, new NotifyDictionaryItemAddedEventArgs<TKey, TValue>(publicTxn, key, value));
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("TStore.PrimaryNotification@{0}@{1}", StoreModificationType.Add, this.traceType), e, string.Empty);
                throw;
            }
        }

        private void FireItemUpdatedNotificationOnSecondary(ReplicatorTransactionBase txn, TKey key, TValue value)
        {
            var eventHandler = this.DictionaryChanged;
            if (eventHandler == null)
            {
                return;
            }

            try
            {
                var publicTxn = txn as ITransaction;
                Diagnostics.Assert(publicTxn != null, this.traceType, "Must be an ITransaction. TxnId: {0}", txn.Id);

                eventHandler(this, new NotifyDictionaryItemUpdatedEventArgs<TKey, TValue>(publicTxn, key, value));
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("TStore.PrimaryNotification@{0}@{1}", StoreModificationType.Update, this.traceType), e, string.Empty);
                throw;
            }
        }

        private void FireItemRemovedNotificationOnSecondary(ReplicatorTransactionBase txn, TKey key)
        {
            var eventHandler = this.DictionaryChanged;
            if (eventHandler == null)
            {
                return;
            }

            try
            {
                var publicTxn = txn as ITransaction;
                Diagnostics.Assert(publicTxn != null, traceType, "Must be an ITransaction. TxnId: {0}", txn.Id);

                eventHandler(this, new NotifyDictionaryItemRemovedEventArgs<TKey, TValue>(publicTxn, key));
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("TStore.PrimaryNotification@{0}@{1}", StoreModificationType.Remove, this.traceType), e, string.Empty);
                throw;
            }
        }

        private async Task FireRebuildNotificationCallerHoldsLockAsync()
        {
            Stopwatch stopwatch = Stopwatch.StartNew();
            FabricEvents.Events.RebuildNotification(this.traceType, true, 0);

            var snappedRebuildNotificationAsyncCallback = this.rebuildNotificationsAsyncCallback;
            if (snappedRebuildNotificationAsyncCallback == null)
            {
                return;
            }

            try
            {
                var state = this.AsyncEnumerateCallerHoldsLock(ReadMode.ReadValue);
                await snappedRebuildNotificationAsyncCallback.Invoke(this, new NotifyDictionaryRebuildEventArgs<TKey, TValue>(state)).ConfigureAwait(false);

                AsyncEnumerable<TKey, TValue> asyncEnumerable = state as AsyncEnumerable<TKey, TValue>;
                Diagnostics.Assert(asyncEnumerable!=null, this.traceType, "must be an AsyncEnumerator");

                // Ensures that the enumerable cannot be used any more.
                asyncEnumerable.InvalidateEnumerable();
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(string.Format("TStore.RebuildNotification@{0}", this.traceType), e, string.Empty);
                throw;
            }

            FabricEvents.Events.RebuildNotification(this.traceType, true, stopwatch.ElapsedMilliseconds);
        }

        /// <summary>
        /// Fire notifications for operations undone during false progress.
        /// </summary>
        /// <param name="txn"></param>
        /// <param name="key"></param>
        /// <param name="metadataOperationData"></param>
        /// <param name="previousVersionedItem"></param>
        /// <remarks>
        /// Invariant: For a given key at max one committed transaction can be undone. 
        /// Previous state is last committed transaction on the key before the one being undone.
        /// Previous State can be from differential or consolidated.
        /// 
        /// Previous State  |   This Txn Last Operation |   Expected Result
        /// NA|Remove           Add                         Remove
        /// NA|Remove           Update                      Remove
        /// NA|Remove           Remove                      Noop
        /// 
        /// A|U                 Add                         Update
        /// A|U                 Update                      Update
        /// A|U                 Remove                      Add
        /// 
        /// </remarks>
        private void FireUndoNotifications(
            ReplicatorTransactionBase txn, TKey key, MetadataOperationData metadataOperationData, TVersionedItem<TValue> previousVersionedItem)
        {
            var eventHandler = this.DictionaryChanged;

            if (eventHandler == null)
            {
                return;
            }

            try
            {
                var publicTxn = txn as ITransaction;
                Diagnostics.Assert(publicTxn != null, this.traceType, "Must be an ITransaction");

                // Last operation on the key in txn is Add
                if (metadataOperationData.Modification == StoreModificationType.Add)
                {
                    if (previousVersionedItem == null || previousVersionedItem.Kind == RecordKind.DeletedVersion)
                    {
                        eventHandler(this, new NotifyDictionaryItemRemovedEventArgs<TKey, TValue>(publicTxn, key));
                        return;
                    }

                    eventHandler(this, new NotifyDictionaryItemUpdatedEventArgs<TKey, TValue>(publicTxn, key, previousVersionedItem.Value));
                    return;
                }

                // Last operation on the key in txn is Update
                if (metadataOperationData.Modification == StoreModificationType.Update)
                {
                    if (previousVersionedItem == null || previousVersionedItem.Kind == RecordKind.DeletedVersion)
                    {
                        eventHandler(this, new NotifyDictionaryItemRemovedEventArgs<TKey, TValue>(publicTxn, key));
                        return;
                    }

                    eventHandler(this, new NotifyDictionaryItemUpdatedEventArgs<TKey, TValue>(publicTxn, key, previousVersionedItem.Value));
                    return;
                }

                // Last operation on the key in txn is Remove
                if (metadataOperationData.Modification == StoreModificationType.Remove)
                {
                    if (previousVersionedItem == null || previousVersionedItem.Kind == RecordKind.DeletedVersion)
                    {
                        // If item previously did not exist, since the last notification for the given key is remove, this must be nooped.
                        return;
                    }

                    eventHandler(this, new NotifyDictionaryItemAddedEventArgs<TKey, TValue>(publicTxn, key, previousVersionedItem.Value));
                    return;
                }
            }
            catch (Exception e)
            {
                Diagnostics.ProcessException(
                    string.Format("TStore.UndoNotification@{0}@{1}", metadataOperationData.Modification, this.traceType),
                    e,
                    string.Empty);
                throw;
            }
        }

        private bool CanItemBeSweepedToDisk(TValue value)
        {
            if (!this.isValueAReferenceType)
            {
                return false;
            }
            else
            {
                if (value == null)
                {
                    return false;
                }
            }

            return true;
        }

#endregion

        private bool IsNull(TKey key)
        {
            return !typeof(TKey).IsValueType && key == null;
        }

        private bool IsNull(TValue value)
        {
            return this.isValueAReferenceType && value == null;
        }
    }
}