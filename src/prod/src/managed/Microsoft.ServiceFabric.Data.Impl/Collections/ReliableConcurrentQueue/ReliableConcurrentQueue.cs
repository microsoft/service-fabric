// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;

    using TimeoutHelper = Microsoft.ServiceFabric.Data.TimeoutHelper;
    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using TransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    // todo: document: class summary, design overview, invariants, contract
    internal class ReliableConcurrentQueue<T> : IReliableConcurrentQueue<T>, IStateProvider2
    {
        internal PrimeLockThrows PrimeLock { get; private set; }

        private string traceType;

        private IdGenerator idGenerator;

        private List<IListElement<T>> checkpointSnap;

        private ITransactionalReplicator transactionalReplicator;

        internal IStateSerializer<T> ValueSerializer { get; private set; }

        internal string WorkDirectory { get; private set; }

        private Uri stateProviderName;

        private long stateProviderId;

        private readonly int queueMaximumSize;

        private int queueCurrentSize;

        private CopyFileWriter copyFileWriter;

        private ReplicaRole currentRole = ReplicaRole.Unknown;

        internal CheckpointManager<T> CurrentCheckpointManager { get; private set; }

        private CheckpointManager<T> nextCheckpointManager;

        private static readonly ConditionalValue<T> EmptyDequeue = new ConditionalValue<T>();

        private long lastPrepareCheckpointLsn;

        private long lastPerformCheckpointLsn;

        private long lastCompleteCheckpointLsn;

        private long lastMaxApplyLsn;

        private volatile bool isNoopCheckpoint;

        private readonly RCQMode QueueMode;

        /// <summary>
        /// Dictionary to keep track of the transaction and the first unlock call that comes in for it.
        /// Used to reduce the number of traces. Now only the first unlock would be traces for every transaction.
        /// </summary>
        private ConcurrentDictionary<long, bool> transactionContext;

        /// <summary>
        /// Size with respect to the capacity of the queue.  In-flight enqueues increase the size and in-flight dequeues
        /// do not decrease it, so that the size will not exceed the capacity.
        /// </summary>
        internal int QueueCurrentSize
        {
            get
            {
                var size = this.queueCurrentSize;
                if (!(size >= 0))
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "QueueCurrentSize",
                        "queue size must never be negative");
                }

                this.ThrowIfNotInitialized("QueueCurrentSize");
                this.ThrowIfNotOpened("QueueCurrentSize");
                this.ThrowIfClosing();

                // On secondary, this value will not be correct, but is only used on Primary and will be corrected 
                // during ChangeRole.
                if (this.transactionalReplicator.Role != ReplicaRole.Primary)
                {
                    throw new FabricNotPrimaryException();
                }

                return size;
            }
        }

        // volatile to make sure old values (false) of these flags are not mistakenly read by another thread.

        private volatile bool isInitialized;

        private volatile bool isOpened;

        private volatile bool isRecovered;

        private volatile bool isClosing;


        /// <summary>
        /// Returns all items which should be included in the next checkpoint:
        ///     - Enqueue has been applied
        ///     - Dequeue has NOT been applied
        /// <see cref="IStateProvider2.ApplyAsync"/> cannot race with <see cref="IStateProvider2.PrepareCheckpointAsync"/>,
        /// so listElements cannot transition between states which would change whether or not it should be included or excluded.
        /// 
        /// Also drops all dequeued items with commit lsn less than or equal to the stable lsn, as undo will not be seen.
        /// </summary>
        internal List<IListElement<T>> GetPersistedItemsAndDropStableDequeues()
        {
            var persistedItems = new List<IListElement<T>>();

            var stableLsn = this.transactionalReplicator.GetLastStableSequenceNumber();
            foreach (var listElement in this.DataStore.Items)
            {
                if (Constants.PersistentStateDictionary[listElement.State])
                {
                    persistedItems.Add(listElement);
                }
                else if (listElement.State == ListElementState.DequeueApplied
                         && listElement.DequeueCommittingTransaction.CommitSequenceNumber <= stableLsn)
                {
                    // once the stable lsn has increased beyond the commit lsn of this dequeue, we will never see Undo, so it is safe to forget about this item
                    this.DataStore.RemoveListElement(listElement);
                }
            }

            return persistedItems;
        }

        /// <summary>
        /// The data structure holding on to the listElements themselves.
        /// </summary>
        internal DataStore<T> DataStore { get; private set; }

        public ReliableConcurrentQueue()
            : this(Constants.DefaultQueueMaximumSize)
        {
        }

        // todo: how to configure?  
        public ReliableConcurrentQueue(int queueMaximumSize)
        {
            this.QueueMode = Constants.QueueMode;
            this.queueMaximumSize = queueMaximumSize;
            this.queueCurrentSize = 0;
            this.idGenerator = new IdGenerator(long.MinValue + 1);
        }

        internal IEnumerable<string> GetCheckpointFiles()
        {
            return CheckpointFileHelper.GetCheckpointFiles(this.WorkDirectory);
        }

        /// <summary>
        /// Enumerate over the visible items outside the lock (does not support concurrent enqueue or dequeue).  For testing only.
        /// </summary>
        internal IEnumerable<T> GetVisibleListElementsUnsafe()
        {
            var current = this.DataStore.Head;

            while (current != null)
            {
                yield return current.Value;
                current = current.Next;
            }
        }

        /// <summary>
        /// Gets a value indicating the unique name for the <see cref="IReliableState"/> instance.
        /// </summary>
        /// <returns>The <see cref="System.Uri"/> name of this <see cref="IReliableState"/> instance.</returns>
        Uri IReliableState.Name
        {
            get
            {
                return this.stateProviderName;
            }
        }

        long IReliableConcurrentQueue<T>.Count
        {
            get
            {
                this.ThrowIfNotInitialized("Count");
                this.ThrowIfNotOpened("Count");
                this.ThrowIfNotRecovered("Count");
                this.ThrowIfClosing();
                this.ThrowIfNotReadable("Count");

                return this.DataStore.LinkedCount;
            }
        }

        async Task IReliableConcurrentQueue<T>.EnqueueAsync(ITransaction tx, T value, CancellationToken cancellationToken, TimeSpan? timeout)
        {
            // todo sangarg : RDBug 8887951
            this.ThrowIfTransactionIsNullOrInvalid(tx);
            this.ThrowIfNotInitialized("EnqueueAsync", tx.TransactionId);
            this.ThrowIfNotOpened("EnqueueAsync", tx.TransactionId);
            this.ThrowIfNotRecovered("EnqueueAsync", tx.TransactionId);
            this.ThrowIfClosing();

            // todo sangarg : RDBug 9563497
            // Enable checks for Read Write status

            Stopwatch enqueueTimer = new Stopwatch();
            enqueueTimer.Start();
            var startTime = DateTime.UtcNow;

            IListElement<T> listElement = null;
            var addedCapacity = false;
            var primeLockTaken = false;
            var transaction = tx as Transaction;

            if (transaction == null)
            {
                FabricEvents.Events.ReliableConcurrentQueue_InvalidTransaction(this.traceType, tx.TransactionId);
                throw new ArgumentException(
                          string.Format(SR.ReliableConcurrentQueue_InvalidTransaction, tx.TransactionId));
            }

            try
            {
                {
                    // throw if queue is full
                    int currentSize;
                    int newSize;

                    do
                    {
                        currentSize = this.queueCurrentSize;
                        newSize = currentSize + 1;

                        if (newSize > this.queueMaximumSize)
                        {
                            FabricEvents.Events.ReliableConcurrentQueue_EnqueueQueueFull(
                                this.traceType,
                                this.queueMaximumSize);
                            throw new QueueFullException(
                                      string.Format(
                                          CultureInfo.CurrentCulture,
                                          SR.ReliableConcurrentQueue_EnqueueQueueFull,
                                          this.queueMaximumSize));
                        }
                    }
                    while (currentSize != Interlocked.CompareExchange(ref this.queueCurrentSize, newSize, currentSize));
                    addedCapacity = true;
                }

                listElement = DataStore<T>.CreateQueueListElement(this.idGenerator.Next(), value, this.traceType);
                if (this.DataStore.HasListElement(listElement.Id))
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "EnqueueAsync",
                        "the newly created listElement must not exist in the dictionary");
                }

                await this.PrimeLock.AcquireReadAsync(timeout, cancellationToken).ConfigureAwait(false);
                primeLockTaken = true;

                // if this fails, no need to compensate
                await
                    this.ReplicateEnqueueAsync(transaction, listElement, cancellationToken, timeout)
                        .ConfigureAwait(false);

                enqueueTimer.Stop();

                // not expected to throw
                FabricEvents.Events.ReliableConcurrentQueue_Enqueue(
                    this.traceType,
                    transaction.TransactionId,
                    listElement.Id,
                    enqueueTimer.ElapsedMilliseconds,
                    this.DataStore.LinkedCount,
                    this.DataStore.ItemsCount);

                this.DataStore.AssertUnlinked(listElement);
                listElement.StartEnqueue();
                this.DataStore.AddListElement(listElement);

                // not expected to throw
                transaction.AddLockContext(
                    new OperationLockContext<T>(this, listElement, UnlockSource.EnqueueLockContext, tx, startTime));

                // Add the transaction id to the transactionContext dictionary
                this.transactionContext.TryAdd(tx.TransactionId, false);
            }
            catch (Exception e)
            {
                FabricEvents.Events.ReliableConcurrentQueue_EnqueueFailed(
                    this.traceType,
                    tx.TransactionId,
                    listElement == null ? "null" : listElement.ToString(),
                    e.ToString());

                if (addedCapacity)
                {
                    Interlocked.Decrement(ref this.queueCurrentSize);
                }

                if (primeLockTaken)
                {
                    this.PrimeLock.ReleaseRead();
                }

                throw;
            }
        }

        async Task<ConditionalValue<T>> IReliableConcurrentQueue<T>.TryDequeueAsync(ITransaction tx, CancellationToken cancellationToken, TimeSpan? timeout)
        {
            this.ThrowIfTransactionIsNullOrInvalid(tx);
            this.ThrowIfNotInitialized("TryDequeueAsync", tx.TransactionId);
            this.ThrowIfNotOpened("TryDequeueAsync", tx.TransactionId);
            this.ThrowIfNotRecovered("TryDequeueAsync", tx.TransactionId);
            this.ThrowIfClosing();

            // todo sangarg : RDBug 9563497
            // Enable checks for Read Write status
            
            IListElement<T> listElement = null;
            var primeLockTaken = false;
            var transaction = tx as Transaction;
            var startTime = DateTime.UtcNow;
            var dequeueTimer = new Stopwatch();
            dequeueTimer.Start();

            if (transaction == null)
            {
                FabricEvents.Events.ReliableConcurrentQueue_InvalidTransaction(this.traceType, tx.TransactionId);
                throw new ArgumentException(string.Format(SR.ReliableConcurrentQueue_InvalidTransaction, tx.TransactionId));
            }

            bool isInfiniteTime = false;

            int timeoutMs = TimeoutHelper.GetTimeSpanInMilliseconds(timeout ?? TimeoutHelper.DefaultReplicationTimeout);
            var totalTime = TimeSpan.FromMilliseconds(timeoutMs);

            if (totalTime == Timeout.InfiniteTimeSpan)
            {
                isInfiniteTime = true;
            }

            var timeLeft = totalTime;
            var stopWatch = new Stopwatch();

            try
            {
                stopWatch.Start();

                // Try to acquire the PrimeLock
                await this.PrimeLock.AcquireReadAsync(timeLeft, cancellationToken).ConfigureAwait(false);

                stopWatch.Stop();
            }
            catch (TimeoutException)
            {
                FabricEvents.Events.ReliableConcurrentQueue_DequeueFailed(
                           this.traceType,
                           transaction.TransactionId,
                           "null",
                           "PrimeLock acquisition timed out");
                throw;
            }
            primeLockTaken = true;

            try
            {
                ConditionalValue<IListElement<T>> listElementResult;

                switch (this.QueueMode)
                {
                    // IReliableConcurrentQueue
                    case RCQMode.NonBlocking:
                    {
                        listElementResult = this.DataStore.TryUnlinkHead();
                        break;
                    }

                    // IReliableConcurrentBlockingQueue
                    case RCQMode.Blocking:
                    {
                        // todo sangarg : RDBug 8829568
                        var tcs = new TaskCompletionSource<ConditionalValue<IListElement<T>>>();

                        stopWatch.Start();

                        this.DataStore.TryDequeue(transaction, tcs, timeLeft, cancellationToken);
                        listElementResult = await tcs.Task.ConfigureAwait(false);

                        stopWatch.Stop();
                        break;
                    }

                    default:
                        TestableAssertHelper.FailInvalidOperation(this.traceType, "TryDequeueAsync",
                            "Invalid QueueMode : {0}", this.QueueMode);
                        return ReliableConcurrentQueue<T>.EmptyDequeue; // unreachable
                }

                if (!listElementResult.HasValue)
                {
                    // If there was nothing to dequeue, release the read lock
                    this.PrimeLock.ReleaseRead();
                    primeLockTaken = false;

                    if (listElementResult.Value != null)
                    {
                        TestableAssertHelper.FailInvalidOperation(
                            this.traceType,
                            "TryDequeueAsync",
                            "value must be null if HasValue is false");
                    }

                    if (this.QueueMode == RCQMode.Blocking)
                    {
                        // Blocking RCQ would only return empty queue in time out
                        FabricEvents.Events.ReliableConcurrentQueue_DequeueTimeout(this.traceType, transaction.TransactionId);
                    }

                    // If nothing was returned, return conditional value false
                    return ReliableConcurrentQueue<T>.EmptyDequeue;
                }

                if (listElementResult.Value == null)
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "TryDequeueAsync",
                        "value must not be null as HasValue was true");
                }

                listElement = listElementResult.Value;

                listElement.StartDequeue();

                if (!isInfiniteTime)
                {
                    timeLeft = totalTime - stopWatch.Elapsed;

                    if (timeLeft < TimeSpan.Zero)
                    {
                        timeLeft = TimeSpan.Zero;
                    }
                }

                await this.ReplicateDequeueAsync(transaction, listElement, cancellationToken, timeLeft).ConfigureAwait(false);

                dequeueTimer.Stop();

                // not expected to throw
                FabricEvents.Events.ReliableConcurrentQueue_Dequeue(
                    this.traceType,
                    transaction.TransactionId,
                    listElement.Id,
                    dequeueTimer.ElapsedMilliseconds,
                    this.DataStore.LinkedCount,
                    this.DataStore.ItemsCount);

                // does not throw today but can throw in the future
                transaction.AddLockContext(
                    new OperationLockContext<T>(this, listElement, UnlockSource.DequeueLockContext, tx, startTime));

                return new ConditionalValue<T>(true, listElement.Value);
            }
            catch (Exception e)
            {
                FabricEvents.Events.ReliableConcurrentQueue_DequeueFailed(
                    this.traceType,
                    tx.TransactionId,
                    listElement == null ? "null" : listElement.ToString(),
                    e.ToString());

                if (listElement != null)
                {
                    listElement.AbortDequeue();
                    this.DataStore.LinkHead(listElement);
                }

                if (!primeLockTaken)
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "TryDequeueAsync",
                        "PrimeLock should have been taken");
                }

                this.PrimeLock.ReleaseRead();

                throw;
            }
        }

        internal void Unlock(OperationLockContext<T> context)
        {
            var listElement = context.ListElement;
            var unlockSource = context.UnlockSource;

            DateTime startTime = context.OperationStartTime;
            var totalTime = (long)(DateTime.UtcNow - startTime).TotalMilliseconds;

            bool temp;
            var txnId = context.Transaction.TransactionId;

            // If we could not remove the value from the transactionContext dictionary, then we have already traces unlock for this transaction.
            bool traced = this.transactionContext.TryRemove(txnId, out temp);

            if (!traced)
            {
                FabricEvents.Events.ReliableConcurrentQueue_Unlock(
                    this.traceType,
                    (short)unlockSource,
                    txnId,
                    listElement.Id,
                    (short)listElement.State,
                    totalTime);
            }

            try
            {
                if (unlockSource == UnlockSource.EnqueueLockContext)
                {
                    // ABORT-ENQUEUE
                    if (listElement.State == ListElementState.EnqueueInFlight)
                    {
                        // Transaction was aborted, so the enqueue was not applied
                        this.DataStore.AssertUnlinked(listElement);
                        listElement.AbortEnqueue();

                        // Replication succeeded so we added the item in memory, so we need to compensate
                        this.DataStore.RemoveListElement(listElement);
                        Interlocked.Decrement(ref this.queueCurrentSize);
                    }
                 
                    // Once the enqueue has been applied, we can no longer make any inferences about what state the listElement is in with respect to
                    // this unlock call.
                }
                else if (unlockSource == UnlockSource.ApplyEnqueue)
                {
                    // UNLOCK-ENQUEUE
                    if (listElement.State != ListElementState.EnqueueApplied)
                    {
                        TestableAssertHelper.FailInvalidOperation(
                            this.traceType,
                            "Unlock",
                            "listElement.State == ListElementState.EnqueueApplied. listElement id: {0}, unlockSource: {1}",
                            listElement.Id, unlockSource);
                    }

                    this.DataStore.AssertUnlinked(listElement);
                    this.DataStore.LinkTail(listElement);
                }
                else if (unlockSource == UnlockSource.DequeueLockContext)
                {
                    // ABORT-DEQUEUE
                    if (listElement.State == ListElementState.DequeueInFlight)
                    {
                        if (!this.DataStore.HasListElement(listElement))
                        {
                            TestableAssertHelper.FailInvalidOperation(
                                this.traceType,
                                "Unlock",
                                "if dequeue is in flight, the dictionary must have the listElement {0}. unlockSource: {1}",
                                listElement.Id, unlockSource);
                        }

                        // Transaction was aborted, so the dequeue was not applied
                        this.DataStore.AssertUnlinked(listElement);
                        listElement.AbortDequeue();
                        this.DataStore.LinkHead(listElement); // Add the item back to the head of the queue

                        return;
                    }

                    // The list element can never transition back to EnqueueInFlight.
                    if (listElement.State == ListElementState.EnqueueInFlight)
                    {
                        TestableAssertHelper.FailInvalidOperation(
                            this.traceType,
                            "Unlock",
                            "listElement.State != ListElementState.EnqueueInFlight. listElement id: {0}, unlockSource: {1}",
                            listElement.Id, unlockSource);
                    }

                    // Cannot make any other inferences about the list element state. "Dequeue-Unlocked" is not a tracked state, and the state machine may have transitioned already.
                }
            }
            finally
            {
                this.PrimeLock.ReleaseRead();
            }
        }

        private async Task ReplicateEnqueueAsync(
            Transaction tx,
            IListElement<T> listElement,
            CancellationToken token,
            TimeSpan? timeout)
        {
            var valueBytes = this.GetValueBytes(listElement.Value);
            var metaData = new OperationData(new MetadataOperationData(Constants.SerializedVersion, OperationType.Enqueue, listElement.Id));
            var redo = new OperationData(new RedoUndoOperationData(valueBytes));

            await
                ReplicationHelper.ReplicateOperationAsync(
                    tx: tx,
                    stateProviderId: this.stateProviderId,
                    traceType: this.traceType,
                    operationContext: null,
                    metaData: metaData,
                    redo: redo,
                    undo: null,
                    cancellationToken: token,
                    timeout: timeout).ConfigureAwait(false);
        }

        internal ArraySegment<byte> GetValueBytes(T value)
        {
            using (var memoryStream = new MemoryStream())
            {
                using (var binaryWriter = new BinaryWriter(memoryStream))
                {
                    this.ValueSerializer.Write(value, binaryWriter);

                    return new ArraySegment<byte>(memoryStream.GetBuffer(), 0, (int)memoryStream.Position);
                }
            }
        }

        private T GetValueFromBytes(ArraySegment<byte> valueBytes)
        {
            using (var memoryStream = new MemoryStream(valueBytes.Array, valueBytes.Offset, valueBytes.Count, writable: false))
            {
                using (var binaryReader = new BinaryReader(memoryStream))
                {
                    return this.ValueSerializer.Read(binaryReader);
                }
            }
        }

        private async Task ReplicateDequeueAsync(
            Transaction tx,
            IListElement<T> listElement,
            CancellationToken token,
            TimeSpan? timeout)
        {
            var metaData = new OperationData(new MetadataOperationData(Constants.SerializedVersion, OperationType.Dequeue, listElement.Id));

            await
                ReplicationHelper.ReplicateOperationAsync(
                    tx: tx,
                    stateProviderId: this.stateProviderId,
                    traceType: this.traceType,
                    operationContext: null,
                    metaData: metaData,
                    redo: null,
                    undo: null,
                    cancellationToken: token,
                    timeout: timeout).ConfigureAwait(false);
        }

        private void ThrowIfTransactionIsNullOrInvalid(ITransaction tx)
        {
            if (tx == null)
            {
                FabricEvents.Events.ReliableConcurrentQueue_NullTransaction(this.traceType);
                throw new ArgumentNullException(SR.ReliableConcurrentQueue_NullTransaction);
            }

            if (!((TransactionBase)tx).IsValidTransaction)
            {
                FabricEvents.Events.ReliableConcurrentQueue_InvalidTransaction(this.traceType, tx.TransactionId);
                throw new ArgumentException(string.Format(SR.ReliableConcurrentQueue_InvalidTransaction, tx.TransactionId));
            }
        }

        private void ThrowIfNotInitialized(string api, long transactionId = -1)
        {
            if (!this.isInitialized)
            {
                FabricEvents.Events.ReliableConcurrentQueue_ThrowIfNot(
                    this.traceType,
                    "Initialized",
                    api,
                    transactionId,
                    "n/a",
                    this.currentRole.ToString());

                throw new InvalidOperationException(SR.ReliableConcurrentQueue_NotInitialized);
            }
        }

        private void ThrowIfNotOpened(string api, long transactionId = -1)
        {
            if (!this.isOpened)
            {
                FabricEvents.Events.ReliableConcurrentQueue_ThrowIfNot(
                   this.traceType,
                    "Opened",
                    api,
                    transactionId,
                    this.transactionalReplicator.StatefulPartition.ReadStatus.ToString(),
                    this.currentRole.ToString());

                throw new InvalidOperationException(SR.ReliableConcurrentQueue_NotOpened);
            }
        }

        private void ThrowIfNotRecovered(string api, long transactionId = -1)
        {
            if (!this.isRecovered)
            {
                FabricEvents.Events.ReliableConcurrentQueue_ThrowIfNot(
                  this.traceType,
                   "Recovered",
                   api,
                   transactionId,
                   this.transactionalReplicator.StatefulPartition.ReadStatus.ToString(),
                   this.currentRole.ToString());

                throw new InvalidOperationException(SR.ReliableConcurrentQueue_NotRecovered);
            }
        }

        private void ThrowIfClosing()
        {
            if (this.isClosing)
            {
                FabricEvents.Events.ReliableConcurrentQueue_Closing(this.traceType);
                throw new FabricObjectClosedException(SR.ReliableConcurrentQueue_Closing);
            }
        }

        private void ThrowIfNotReadable(string api, long transactionId = -1)
        {
            if (!this.transactionalReplicator.IsReadable)
            {
                FabricEvents.Events.ReliableConcurrentQueue_ThrowIfNot(
                    this.traceType,
                     "Readable",
                     api,
                     transactionId,
                     this.transactionalReplicator.StatefulPartition.ReadStatus.ToString(),
                     this.currentRole.ToString());

                throw new FabricNotReadableException();
            }
        }

        private class IdGenerator
        {
            private long nextId;

            public IdGenerator(long nextId)
            {
                this.nextId = nextId;
            }

            public long Next()
            {
                return Interlocked.Increment(ref this.nextId) - 1;
            }
        }

        #region IStateProvider2

        /// <summary>
        /// Gets Initialization context.
        /// </summary>
        byte[] IStateProvider2.InitializationContext
        {
            get
            {
                return null;
            }
        }

        void IStateProvider2.Initialize(
            TransactionalReplicator replicator,
            Uri name,
            byte[] initializationContext,
            long id,
            string stateProviderWorkDirectory,
            IEnumerable<IStateProvider2> childrenStateProviders)
        {
            this.Initialize(replicator, name, initializationContext, id, stateProviderWorkDirectory, childrenStateProviders);
        }

        internal void Initialize(
            ITransactionalReplicator replicator,
            Uri name,
            byte[] initializationContext,
            long id,
            string stateProviderWorkDirectory,
            IEnumerable<IStateProvider2> childrenStateProviders)
        {
            this.transactionalReplicator = replicator;
            this.stateProviderName = name;
            this.stateProviderId = id;
            this.WorkDirectory = stateProviderWorkDirectory;
            this.ValueSerializer = replicator.GetStateSerializer<T>(name);
            if (initializationContext != null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "Initialize", "initializationContext == null");
            }
            if (childrenStateProviders != null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "Initialize", "childrenStateProviders == null");
            }

            this.traceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}@{1}@{2}@{3}",
                this.transactionalReplicator.ServiceContext.PartitionId,
                this.transactionalReplicator.ServiceContext.ReplicaId,
                this.stateProviderName,
                this.stateProviderId);

            this.DataStore = new DataStore<T>(this.QueueMode, this.traceType);

            // Ensure the state provider's working sub-directory exists (no-op if it already exists).
            FabricDirectory.CreateDirectory(this.WorkDirectory);

            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "Initialize");

            this.transactionContext = new ConcurrentDictionary<long, bool>();

            this.lastMaxApplyLsn = -1;
            this.lastPrepareCheckpointLsn = -1;
            this.lastPerformCheckpointLsn = -1;
            this.lastCompleteCheckpointLsn = -1;
            this.isNoopCheckpoint = false;

            this.isInitialized = true;
        }

        Task IStateProvider2.OpenAsync()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "OpenAsync");

            this.PrimeLock = new PrimeLockThrows();

            // Ensure the state provider's working sub-directory exists
            if (!FabricDirectory.Exists(this.WorkDirectory))
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "OpenAsync", "Work directory does not exist");
            }

            this.isOpened = true;
            return Task.FromResult(true);
        }

        Task IStateProvider2.CloseAsync()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "CloseAsync");

            this.DataStore.Close();
            this.Cleanup("CloseAsync");

            return Task.FromResult(true);
        }

        void IStateProvider2.Abort()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "Abort");

            this.DataStore.Abort();

            this.Cleanup("Abort");
        }

        private void Cleanup(string api)
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, string.Format("Cleanup => {0}", api));

            if (this.isClosing)
            {
                return;
            }

            try
            {
                this.isClosing = true;
                this.PrimeLock.Close();

                if (this.copyFileWriter != null)
                {
                    // Close/Abort are happening before copy completed.
                    this.copyFileWriter.Dispose();
                    this.copyFileWriter = null;
                }
            }
            catch (Exception e)
            {
                // Ignore all exceptions thrown during cleanup.
                FabricEvents.Events.ReliableConcurrentQueue_ExceptionWarning(
                    this.traceType,
                    string.Format("Cleanup => {0} ignored Exception. Exception : {1}", api, e));
            }
        }

        async Task IStateProvider2.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_ChangeRole(this.traceType, this.currentRole.ToString(), newRole.ToString());

            this.DataStore.ChangeRole(this.currentRole, newRole);

            if (this.currentRole == ReplicaRole.Primary)
            {
                if (newRole != ReplicaRole.None)
                {
                    // drain unlocks during ChangeRole to P -> !N
                    // If going to None, then we can ignore the Unlocks as we are trying to remove the state.
                    await
                        this.PrimeLock.AcquireWriteAsync(TimeSpan.MaxValue, CancellationToken.None)
                            .ConfigureAwait(false);
                    this.PrimeLock.ReleaseWrite();
                }
            }

            this.currentRole = newRole;

            if (newRole == ReplicaRole.None)
            {
                this.Cleanup("ChangeRoleToNone");
            }
            else if (newRole == ReplicaRole.Primary)
            {
                if (this.isClosing)
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "ChangeRoleAsync",
                        "Should not see ChangeRole to primary once the state provider is being closed or the state removed.");
                }

                // all the applies from the old primary have been seen
                var startingValue = this.DataStore.HighestId + 1;
                this.idGenerator = new IdGenerator(startingValue);
                this.queueCurrentSize = this.DataStore.LinkedCount;
            }
        }

        Task IStateProvider2.OnDataLossAsync()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "OnDataLossAsync");

            return Task.FromResult(true);
        }

        Task IStateProvider2.PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_PrepareCheckpointStarted(this.traceType, checkpointLSN);

            var stopWatch = new Stopwatch();
            stopWatch.Start();

            // if the lastCompleteCheckpoint is greater that the lastApply seen by RCQ, then noop the current checkpoint as there is no state change.
            // todo : remove isNoopCheckpoint once incremental checkpoint is added.
            this.isNoopCheckpoint = this.lastMaxApplyLsn < this.lastCompleteCheckpointLsn;

            this.lastPrepareCheckpointLsn = checkpointLSN;

            // if it is not a noop checkpoint, find all persisted elements
            if (!this.isNoopCheckpoint)
            {
                this.checkpointSnap = this.GetPersistedItemsAndDropStableDequeues();
            }
            // todo sangarg : else checkpointSnap == null, check prepare-prepare condition

            stopWatch.Stop();

            FabricEvents.Events.ReliableConcurrentQueue_PrepareCheckpointCompleted(this.traceType, this.lastPrepareCheckpointLsn, this.DataStore.LinkedCount, this.DataStore.ItemsCount, this.isNoopCheckpoint, stopWatch.ElapsedMilliseconds);

            return Task.FromResult(true);
        }

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        async Task IStateProvider2.PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_PerformCheckpointStarted(this.traceType, this.lastPrepareCheckpointLsn);

            var stopWatch = new Stopwatch();
            stopWatch.Start();

            // Snap the lastPrepareCheckpointLsn in lastPerformCheckpointLsn
            this.lastPerformCheckpointLsn = this.lastPrepareCheckpointLsn;

            if (!this.isNoopCheckpoint)
            {
                this.nextCheckpointManager = CheckpointManager<T>.CreateNewCheckpointManager(this.checkpointSnap, this.WorkDirectory, this.ValueSerializer, this.traceType);
                await this.nextCheckpointManager.CurrentCheckpoint.WriteAsync().ConfigureAwait(false);
                await this.nextCheckpointManager.WriteAsync().ConfigureAwait(false);
            }

            stopWatch.Stop();

            FabricEvents.Events.ReliableConcurrentQueue_PerformCheckpointCompleted(this.traceType, this.lastPerformCheckpointLsn, this.checkpointSnap.Count, stopWatch.ElapsedMilliseconds);
        }

        async Task IStateProvider2.CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_CompleteCheckpointStarted(this.traceType, this.lastPerformCheckpointLsn);

            var stopWatch = new Stopwatch();
            stopWatch.Start();

            // Snap the lastPerformCheckpointLsn in lastCompleteCheckpointLsn
            this.lastCompleteCheckpointLsn = this.lastPerformCheckpointLsn;
            
            // Noop Complete if no new checkpoint metadata exists, if it was a noop checkpoint, then nothing would be written during perform.
            // Also If the complete came in as part of the Open call before recovery, but the metadata were actually replaced,
            // Nothing to be done, return
            // todo sangarg : add extra asserts to check that it is in one of the above conditions.
            if (this.nextCheckpointManager == null)
            {
                var res = await CheckpointManager<T>.TryReadNewFile(this.WorkDirectory, this.ValueSerializer, this.traceType).ConfigureAwait(false);
                if (!res.HasValue)
                {
                    // No new manager file is present, so no-op
                    return;
                }
                this.nextCheckpointManager = res.Value;
            }

            if (this.CurrentCheckpointManager == null)
            {
                if (!this.isOpened || this.isRecovered)
                {
                    // Assert that the CurrentCheckpointManager was null, but the CompleteCheckpointAsync call did not come in between Open and Recover
                    // Replica should be opened but not recovered.
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "CompleteCheckpointAsync",
                        "CurrentCheckpointMetadata does not exist but the CompleteCheckpointAsync call did not come in between Open and Recover. isOpen {0}, isRecovered {1}",
                        this.isOpened, this.isRecovered);
                }

                // Else try and read it from disk
                var res = await CheckpointManager<T>.TryReadCurrentFile(this.WorkDirectory, this.ValueSerializer, this.traceType).ConfigureAwait(false);
                if (!res.HasValue)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "CompleteCheckpointAsync",
                        "CurrentCheckpointMetadata does not exist. WorkingDirectory = {0}",
                        this.WorkDirectory);
                }

                this.CurrentCheckpointManager = res.Value;
            }

            // todo: if a copy occurred, no-op the performcheckpoint since the copy file was already written, and simply
            // rename the copy file and point the metadata at it

            await this.nextCheckpointManager.ReplaceCurrentAsync().ConfigureAwait(false);
            this.CurrentCheckpointManager.CurrentCheckpoint.ReleaseRef();
            this.CurrentCheckpointManager = this.nextCheckpointManager;
            this.nextCheckpointManager = null;

            // Once CompleteCheckpoint has finished, we won't need the copy file anymore (e.g. for disaster recovery).
            CheckpointFileHelper.TryDeleteCopyFile(this.WorkDirectory, this.traceType);

            stopWatch.Stop();
            FabricEvents.Events.ReliableConcurrentQueue_CompleteCheckpointCompleted(this.traceType, this.lastPerformCheckpointLsn, stopWatch.ElapsedMilliseconds);
        }

        private async Task LoadStateFromCheckpointFileAsync(Checkpoint<T> checkpoint)
        {
            var hasListElements = await checkpoint.TryReadAsync().ConfigureAwait(false);
            var listElementList = hasListElements ? new DataStore<T>(checkpoint.ListElements, this.QueueMode, this.traceType) : new DataStore<T>(this.QueueMode, this.traceType);
            this.LoadState(listElementList);
        }

        private async Task LoadStateFromCopyFileAsync()
        {
            this.LoadState(await CheckpointFileHelper.ReadCopyFileAsync(this.WorkDirectory, this.ValueSerializer, this.QueueMode, this.traceType).ConfigureAwait(false));
        }

        private void LoadState(DataStore<T> dataStore)
        {
            if (dataStore == null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "LoadState", "dataStore != null");
            }
            this.DataStore = dataStore;
            this.idGenerator = new IdGenerator(this.DataStore.HighestId + 1);

            // upon recovery, there will be no items with applied/unlocked dequeue, so every item should count towards 
            // the capacity
            // todo: verify that operations CANNOT come to a secondary before copy
            this.queueCurrentSize = this.DataStore.LinkedCount;
            if (this.DataStore.LinkedCount != this.DataStore.ItemsCount)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "LoadState", "dataStore != null");
            }
        }

        async Task IStateProvider2.RecoverCheckpointAsync()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "RecoverCheckpointAsync");

            var stopWatch = new Stopwatch();
            stopWatch.Start();

            // Reset the last prepared and last performed checkpoint lsn.
            this.lastPerformCheckpointLsn = -1;
            this.lastPrepareCheckpointLsn = -1;

            var res = await CheckpointManager<T>.TryReadCurrentFile(this.WorkDirectory, this.ValueSerializer, this.traceType).ConfigureAwait(false);
            if (res.HasValue)
            {
                this.CurrentCheckpointManager = res.Value;
                await this.LoadStateFromCheckpointFileAsync(this.CurrentCheckpointManager.CurrentCheckpoint).ConfigureAwait(false);
            }
            else
            {
                this.CurrentCheckpointManager = CheckpointManager<T>.CreateCurrentCheckpointManager(
                    new List<IListElement<T>>(),
                    this.WorkDirectory,
                    this.ValueSerializer, 
                    this.traceType);
                await this.CurrentCheckpointManager.CurrentCheckpoint.WriteAsync().ConfigureAwait(false);
                await this.CurrentCheckpointManager.WriteAsync().ConfigureAwait(false);
            }

            foreach (var filePath in FabricDirectory.GetFiles(this.WorkDirectory))
            {
                // leave the copy file around in case it is the last copy of the state and needs to be manually recovered
                if (!filePath.Equals(this.CurrentCheckpointManager.FilePath)
                    && !filePath.Equals(this.CurrentCheckpointManager.CurrentCheckpoint.FilePath)
                    && !filePath.Equals(CheckpointFileHelper.GetCopyFilePath(this.WorkDirectory)))
                {
                    FabricFile.Delete(filePath);
                }
            }

            stopWatch.Stop();

            FabricEvents.Events.ReliableConcurrentQueue_RecoverCheckpointCompleted(this.traceType, this.DataStore.ItemsCount, stopWatch.ElapsedMilliseconds);
        }

        Task IStateProvider2.OnRecoveryCompletedAsync()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "OnRecoveryCompletedAsync");

            this.isRecovered = true;

            return Task.FromResult(true);
        }

        Task IStateProvider2.BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "BackupCheckpointAsync");
            this.CurrentCheckpointManager.Copy(backupDirectory, false);
            return Task.FromResult(true);
        }

        async Task IStateProvider2.RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "RestoreCheckpointAsync");
            var res = await CheckpointManager<T>.TryReadCurrentFile(backupDirectory, this.ValueSerializer, this.traceType).ConfigureAwait(false);
            if (!res.HasValue)
            {
                throw new ArgumentException(string.Format("RestoreCheckpointAsync : Failed to read checkpoint file(s) from directory {0}", backupDirectory));
            }

            // todo: assert that there is not an existing checkpoint once StateManager stops leaking
            res.Value.Copy(this.WorkDirectory, true);
        }

        IOperationDataStream IStateProvider2.GetCurrentState()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "GetCurrentState");

            // Create a copy stream from the state of the disk as of this logical snapshot.
            // the act of creating the copy stream implicitly AddRefs the checkpoint file, and will be released when the copy stream is disposed
            IOperationDataStream copyStream = this.CurrentCheckpointManager.CurrentCheckpoint.GetCopyStream();

            return copyStream;
        }

        void IStateProvider2.BeginSettingCurrentState()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "GetCurrentState");

            this.DataStore = new DataStore<T>(this.QueueMode, this.traceType);

            CheckpointFileHelper.TryDeleteCopyFile(this.WorkDirectory, this.traceType);

            if (this.copyFileWriter != null)
            {
                // todo: verify
                TestableAssertHelper.FailInvalidOperation(this.traceType, "BeginSettingCurrentState", "Another copy is already in flight");
            } 

            this.copyFileWriter = new CopyFileWriter(CheckpointFileHelper.GetCopyFilePath(this.WorkDirectory), this.traceType);
        }

        async Task IStateProvider2.SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            var complete = await this.copyFileWriter.WriteNextAsync(data).ConfigureAwait(false);
            if (complete)
            {
                this.copyFileWriter.Dispose();
                this.copyFileWriter = null;
                await this.LoadStateFromCopyFileAsync().ConfigureAwait(false);
            }

            // todo sangarg : Add stateRecordNumber to trace
            FabricEvents.Events.ReliableConcurrentQueue_SetCurrentState(this.traceType, complete);
        }

        Task IStateProvider2.EndSettingCurrentStateAsync()
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "EndSettingCurrentState");
            return Task.FromResult(true);
        }

        async Task IStateProvider2.PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "PrepareForRemoveAsync");

            await this.PrimeLock.AcquireWriteAsync(timeout, cancellationToken).ConfigureAwait(false);

            try
            {
                transaction.AddLockContext(new PrepareForRemoveStateLockContext<T>(this.PrimeLock));
            }
            catch (Exception)
            {
                this.PrimeLock.ReleaseWrite();
                throw;
            }
        }

        Task IStateProvider2.RemoveStateAsync(long stateProviderId)
        {
            FabricEvents.Events.ReliableConcurrentQueue_ApiInfo(this.traceType, "RemoveStateAsync");

            // todo sangarg : bug RDBug 8927523
            //if (this.currentRole != ReplicaRole.None)
            //{
            //    TestableAssertHelper.FailInvalidOperation(
            //        this.traceType,
            //        "RemoveStateAsync",
            //        "Current replica role = {0} (!= ReplicaRole.None",
            //        __arglist(this.currentRole));
            //}

            this.Cleanup("RemoveStateAsync");

            if (!this.isClosing)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "RemoveStateAsync", "this.IsClosing");
            }
            if (!this.PrimeLock.IsClosed)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "RemoveStateAsync", "this.PrimeLock.IsClosed");
            }

            // Get rid of all persisted files.
            if (FabricDirectory.Exists(this.WorkDirectory))
            {
                FabricDirectory.Delete(this.WorkDirectory, recursive: true);
            }

            return Task.FromResult(true);
        }

        // Assumption: any exception thrown from ApplyAsync will cause the replica to be faulted or the process to go down; progress will not continue.
        async Task<object> IStateProvider2.ApplyAsync(
            long lsn,
            TransactionBase tx,
            OperationData metaData,
            OperationData operationData,
            ApplyContext applyContext)
        {
            // Check arguments.
            if (tx == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "ApplyAsync", "tx != null");
            }
            if (metaData == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "ApplyAsync", "metaData != null");
            }
            if (metaData.Count == 0)
            {
                TestableAssertHelper.FailInvalidData(this.traceType, "ApplyAsync", "metaData.Count != 0");
            }
            if (applyContext == ApplyContext.Invalid)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "ApplyAsync", "applyContext != ApplyContext.Invalid");
            }

            var roleContext = applyContext & ApplyContext.ROLE_MASK;
            var operationContext = applyContext & ApplyContext.OPERATION_MASK;

            long current = Interlocked.Read(ref this.lastMaxApplyLsn);
           
            while (current < lsn)
            {
                // todo sangarg : define previous outside to avoid early break, and do all of your checks in the while conditional.
                var previous = Interlocked.CompareExchange(ref this.lastMaxApplyLsn, lsn, current);
             
                if (previous == current || previous >= lsn)
                    break;

                // todo sangarg: check if these need to be interlocked reads
                current = Interlocked.Read(ref this.lastMaxApplyLsn);
            }

            if (roleContext == ApplyContext.Invalid)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "ApplyAsync", "roleContext != ApplyContext.Invalid");
            }
            if (operationContext == ApplyContext.Invalid)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "ApplyAsync", "operationContext != ApplyContext.Invalid");
            }

            // must not be in conflict according to replicator contract
            var queueMetaData = new MetadataOperationData(metaData);

            FabricEvents.Events.ReliableConcurrentQueue_ApplyAsync(this.traceType, lsn, tx.CommitSequenceNumber, tx.Id, (long)roleContext, (long)operationContext, (short)queueMetaData.OperationType);

            switch (queueMetaData.OperationType)
            {
                case OperationType.Enqueue:
                    return await this.ApplyEnqueueAsync(roleContext, operationContext, queueMetaData, operationData, (ITransaction)tx).ConfigureAwait(false);
                case OperationType.Dequeue:
                    this.ApplyDequeue(roleContext, operationContext, queueMetaData, (ITransaction)tx);
                    return null;
                case OperationType.Invalid:
                default:
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "ApplyAsync", "Invalid operationType during Apply: {0}", queueMetaData.OperationType);
                    throw new InvalidDataException("Invalid operationType during Apply."); // unreachable
            }
        }

        private async Task<OperationLockContext<T>> ApplyEnqueueAsync(
            ApplyContext roleContext,
            ApplyContext operationContext,
            MetadataOperationData metaData,
            OperationData operationData,
            ITransaction commitTx)
        {
            // if Redo-Enqueue
            if (operationContext == ApplyContext.REDO)
            {
                return await this.ApplyRedoEnqueueAsync(roleContext, metaData, operationData, commitTx).ConfigureAwait(false);
            }

            // else Undo-Enqueue
            if (operationContext != ApplyContext.UNDO && operationContext != ApplyContext.FALSE_PROGRESS)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "ApplyEnqueueAsync",
                    "must be UNDO or FALSE_PROGRESS operation context.  operationContext: {0}",
                    operationContext);
            }

            this.ApplyUndoEnqueue(metaData);
            return null;
        }

        private async Task<OperationLockContext<T>> ApplyRedoEnqueueAsync(
            ApplyContext roleContext,
            MetadataOperationData metaData,
            OperationData operationData,
            ITransaction commitTx)
        {
            var startTime = DateTime.UtcNow;

            IListElement<T> listElement;

            if (operationData == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "ApplyRedoEnqueueAsync", "operationData != null");
            }
            if (commitTx == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "ApplyRedoEnqueueAsync", "commitTx != null");
            }
            if (operationData.Count == 0)
            {
                TestableAssertHelper.FailInvalidData(this.traceType, "ApplyRedoEnqueueAsync", "operationData.Count != 0");
            }

            var queueOperationData = new RedoUndoOperationData(operationData);

            if (!this.DataStore.HasListElement(metaData.ItemId))
            {
                // If the DataStore does not have the list element, we should be on the secondary
                if (roleContext == ApplyContext.PRIMARY)
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "ApplyRedoEnqueueAsync",
                        "If the listElement is not already in the dictionary, we must not be the primary, as it was added at enqueue time.  roleContext: {0}");
                }

                var value = this.GetValueFromBytes(queueOperationData.ValueBytes);
                listElement = DataStore<T>.CreateQueueListElement(metaData.ItemId, value, this.traceType, ListElementState.EnqueueInFlight);

                this.DataStore.AddListElement(listElement);
                listElement.ApplyEnqueue();
                this.DataStore.LinkTail(listElement);

                // Secondary does not return any lock context
                return null;
            }
            else
            {
                listElement = this.DataStore.GetListElement(metaData.ItemId);

                // ignore replay
                if (roleContext != ApplyContext.PRIMARY)
                {
                    // assumption: this is during copy or recovery.  todo?: assert this
                    if (listElement.State != ListElementState.EnqueueApplied)
                    {
                        TestableAssertHelper.FailInvalidOperation(
                            this.traceType,
                            "ApplyRedoEnqueueAsync",
                            "listElement.State == ListElementState.EnqueueUnlocked.  state: {0}",
                            listElement.State);
                    }

                    this.DataStore.AssertLinked(listElement);

                    FabricEvents.Events.ReliableConcurrentQueue_IgnoreReplay(this.traceType, listElement.ToString(), "apply-redo-enqueue");

                    return null;
                }

                this.DataStore.AssertUnlinked(listElement);

                listElement.ApplyEnqueue();

                await this.PrimeLock.AcquireReadAsync(TimeSpan.MaxValue, CancellationToken.None).ConfigureAwait(false);

                // Add the transaction id to the transactionContext dictionary
                this.transactionContext.TryAdd(commitTx.TransactionId, false);

                // take the read lock, to be released on unlock so that the linking is done during unlock
                return new OperationLockContext<T>(this, listElement, UnlockSource.ApplyEnqueue, commitTx, startTime);
            }
        }

        private void ApplyUndoEnqueue(MetadataOperationData metaData)
        {
            var listElement = this.DataStore.GetListElement(metaData.ItemId);

            if (listElement.State != ListElementState.EnqueueApplied)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "ApplyUndoEnqueue",
                   "Invalid listElement state: {0}",
                   listElement.ToString());
            }

            this.DataStore.AssertLinked(listElement);
            listElement.UndoEnqueue();

            this.DataStore.Unlink(listElement);
            this.DataStore.RemoveListElement(listElement);
        }

        private void ApplyDequeue(ApplyContext roleContext, ApplyContext operationContext, MetadataOperationData metaData, ITransaction commitTx)
        {
            // if Redo-Dequeue
            if (operationContext == ApplyContext.REDO)
            {
                this.ApplyRedoDequeue(roleContext, metaData, commitTx);
                return;
            }

            // else Undo-Dequeue
            if (operationContext != ApplyContext.UNDO && operationContext != ApplyContext.FALSE_PROGRESS)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "ApplyDequeue",
                    "must be UNDO or FALSE_PROGRESS operation context.  operationContext: {0}",
                    operationContext);
            }

            this.ApplyUndoDequeue(metaData);
        }

        private void ApplyRedoDequeue(ApplyContext roleContext, MetadataOperationData metaData, ITransaction commitTx)
        {
            if (!this.DataStore.HasListElement(metaData.ItemId))
            {
                // ignore replay
                FabricEvents.Events.ReliableConcurrentQueue_IgnoreReplay(this.traceType, "id = " + metaData.ItemId, "apply-redo-dequeue");
                return;
            }

            var listElement = this.DataStore.GetListElement(metaData.ItemId);

            if (roleContext != ApplyContext.PRIMARY)
            {
                // ignore replay
                if (!this.DataStore.IsLinked(listElement))
                {
                    if (listElement.State != ListElementState.DequeueApplied)
                    {
                        TestableAssertHelper.FailInvalidOperation(this.traceType, "ApplyRedoDequeue", "state != DequeueApplied");
                    }

                    FabricEvents.Events.ReliableConcurrentQueue_IgnoreReplay(this.traceType, listElement.ToString(), "apply-redo-dequeue");

                    return;
                }

                this.DataStore.Unlink(listElement);
                listElement.StartDequeue();
            }
            else
            {
                if (listElement.State != ListElementState.DequeueInFlight)
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "ApplyRedoDequeue",
                        "Invalid listElement state: {0}",
                        listElement.ToString());
                }

                this.DataStore.AssertUnlinked(listElement);
            }

            Interlocked.Decrement(ref this.queueCurrentSize);
            listElement.ApplyDequeue(commitTx);
        }

        private void ApplyUndoDequeue(MetadataOperationData metaData)
        {
            var listElement = this.DataStore.GetListElement(metaData.ItemId);

            long commitLsn = listElement.DequeueCommittingTransaction.CommitSequenceNumber;
            long stableLsn = this.transactionalReplicator.GetLastStableSequenceNumber();
            if (commitLsn <= stableLsn)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "ApplyUndoDequeue",
                    "commit LSN must be greater than stable if this is false progress. commitLSN: {0}, stableLSN: {1}",
                    commitLsn, stableLsn);
            }

            this.DataStore.AssertUnlinked(listElement);
            listElement.UndoDequeue();

            this.DataStore.LinkHead(listElement);
        }

        void IStateProvider2.Unlock(object state)
        {
            if (state == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "Unlock", "state");
            }
            var context = state as OperationLockContext<T>;
            if (context == null)
            {
                TestableAssertHelper.FailInvalidData(
                    this.traceType,
                    "Unlock",
                    "Unable to cast unlock state to ConcurrentQueueOperationLockContext. state type: {0}",
                    state.GetType());
            }

            this.Unlock(context);
        }

        IEnumerable<IStateProvider2> IStateProvider2.GetChildren(Uri name)
        {
            return new IStateProvider2[] { };
        }

        #endregion IStateProvider2

        #region TestHooks

        public RCQMode Test_GetQueueMode()
        {
            return this.QueueMode;
        }

        #endregion
    }
}