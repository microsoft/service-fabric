// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Fabric.Store;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Replicator;
    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using TransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Distributed Queue
    /// </summary>
    /// <typeparam name="T">Specifies the type of elements in the queue</typeparam>
    /// <remarks>
    /// Rules:
    ///     1) Always update data store before metadata store
    ///         Items must be added or removed from the data store before head and tail indices in the metadata store can be updated.
    ///     2) Always hold an update lock on the relevant metadata store index, before writing on the data store.
    ///         We want to make sure that the metadata store write operations do not fail with transient exception (e.g. timing out because 
    ///     3) If write  on the metadata store fails, either the operation will be rolledback or the transaction will be rolledback.
    ///     Above three rules are used to make sure there are no silent data corruptions or torn writes.
    ///     E.g. An enqueue adding an item in to the data store but failing to update the meta data store (tail index),
    ///     would leave the queue with a corrupted state. Possibly causing all future enqueues to fail (because index ++ is taken).
    /// Assumptions:
    ///     1) TStore will never return AggregateExceptions that are embedded in other AggregateExceptions.
    ///     2) TStore will never throw TimeoutException if the lock is held
    ///     3) TStore infinitely retries. Never throw FabricTransientException.
    /// </remarks>
    [SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1650:ElementDocumentationMustBeSpelledCorrectly",
        Justification = "Reviewed. Suppression is OK here.")]
    internal class DistributedQueue<T> :
        IStateProvider2,
        IDistributedQueue<T>,
        IReliableQueue<T>
    {
        /// <summary>
        /// Default cancellation token.
        /// </summary>
        public readonly CancellationToken DefaultCancellationToken = CancellationToken.None;

        /// <summary>
        /// Default timeout.
        /// </summary>
        public readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(4);

        /// <summary>
        /// Type name for tracing.
        /// </summary>
        private const string ClassName = "DistributedQueue";

        /// <summary>
        /// Version number. Starts from 1.
        /// </summary>
        private const int Version = 1;

        /// <summary>
        /// Initial row id for the tail pointer. long.MinValue is used as invalid.
        /// </summary>
        private const long InitialRowIdForTailPointer = long.MinValue + 1;

        /// <summary>
        /// Initial row id for the tail pointer. long.MinValue is used as invalid.
        /// </summary>
        private const long InitialRowIdForHeadPointer = long.MinValue + 1;

        /// <summary>
        /// Initial row id for the tail pointer.
        /// </summary>
        private const long KeyIdOfTheRowThatPointsToTailInMetadataStore = long.MinValue;

        /// <summary>
        /// Initial row id for the tail pointer.
        /// </summary>
        private const long KeyIdOfTheRowThatPointsToHeadInMetadataStore = long.MinValue + 1;

        /// <summary>
        /// Relative path for data store.
        /// </summary>
        private readonly Uri relativePathForDataStore = new Uri("dataStore", UriKind.Relative);

        /// <summary>
        /// Relative path for metadata store.
        /// </summary>
        private readonly Uri relativePathForMetadataStore = new Uri("metadataStore", UriKind.Relative);

        /// <summary>
        /// Gets or sets the State Manager.
        /// </summary>
        private ITransactionalReplicator transactionalReplicator;

        /// <summary>
        /// Gets or sets the State Provider Id.
        /// </summary>
        private long stateProviderId;


        /// <summary>
        /// Initializes a new instance of the DistributedQueue class.
        /// </summary>
        public DistributedQueue()
        {
            // initialize.
            this.IsRegistered = false;
            this.IsAutomaticCloningEnabled = false;
        }

        /// <summary>
        /// Initializes a new instance of the DistributedQueue class.
        /// </summary>
        /// <param name="isAutomaticCloningEnabled">Is automatic deep copy enabled.</param>
        public DistributedQueue(bool isAutomaticCloningEnabled)
        {
            this.IsRegistered = false;

            if (false == isAutomaticCloningEnabled)
            {
                // initialize.
                this.IsAutomaticCloningEnabled = false;
                this.ShouldCloneValue = false;
            }
            else if (true == typeof(T).GetTypeInfo().IsValueType)
            {
                this.ShouldCloneValue = false;
                this.IsAutomaticCloningEnabled = true;
            }
            else if (typeof(T) == typeof(string))
            {
                this.ShouldCloneValue = false;
                this.IsAutomaticCloningEnabled = true;
            }
            else if (true == typeof(ICloneable).IsAssignableFrom(typeof(T)))
            {
                this.ShouldCloneValue = true;
                this.IsAutomaticCloningEnabled = true;
            }
            else
            {
                throw new ArgumentException(SR.Error_ICloneableOrValueType_Expected);
            }
        }

        

        /// <summary>
        /// Gets a value indicating whether automatic deep copy is enabled for all returned values.
        /// </summary>
        public bool IsAutomaticCloningEnabled { get; private set; }

        /// <summary>
        /// Gets the name of the state provider.
        /// </summary>
        public Uri Name
        {
            get { return this.StateProviderName; }
        }

        /// <summary>
        /// Gets name of the state provider
        /// </summary>
        public Uri StateProviderName { get; internal set; }

        /// <summary>
        /// Gets a value indicating whether the object is initialized.
        /// </summary>
        public bool IsRegistered { get; private set; }

        /// <summary>
        /// Gets initialization context.
        /// </summary>
        public byte[] InitializationContext
        {
            get
            {
                var context = new QueueInitializationContext(this.IsAutomaticCloningEnabled);
                return context.ToByteArray();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether value should be cloned.
        /// </summary>
        internal bool ShouldCloneValue { get; set; }

        /// <summary>
        /// Gets or sets the data store.
        /// </summary>
        internal Uri DataStoreName { get; set; }

        /// <summary>
        /// Gets or sets the name of the metadata store.
        /// </summary>
        internal Uri MetadataStoreName { get; set; }

        /// <summary>
        /// Gets or sets the trace type used for structured tracing.
        /// </summary>
        private string TraceType { get; set; }

        /// <summary>
        /// Gets or sets the internal metadata store.
        /// </summary>
        private TStore<long, long, LongComparer, LongComparer, LongRangePartitioner> MetadataStore { get; set; }

        /// <summary>
        /// Gets or sets the internal data store.
        /// </summary>
        private TStore<long, T, LongComparer, LongComparer, LongRangePartitioner> DataStore { get; set; }

        #region IReliableQueue

        public Task<IAsyncEnumerable<T>> CreateEnumerableAsync(ITransaction tx)
        {
            return this.CreateEnumerableAsync((Transaction) tx);
        }

        Task<long> IReliableCollection<T>.GetCountAsync(ITransaction tx)
        {
            return this.GetCountAsync((Transaction) tx);
        }

        Task IReliableCollection<T>.ClearAsync()
        {
            // todo: DistributedQueue must support Clear
            throw new NotImplementedException();
        }

        Task IReliableQueue<T>.EnqueueAsync(ITransaction tx, T item)
        {
            return this.EnqueueAsync((Transaction) tx, item);
        }

        Task IReliableQueue<T>.EnqueueAsync(ITransaction tx, T item, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.EnqueueAsync((Transaction) tx, item, timeout, cancellationToken);
        }

        Task<ConditionalValue<T>> IReliableQueue<T>.TryDequeueAsync(ITransaction tx)
        {
            return this.TryDequeueAsync((Transaction) tx);
        }

        Task<ConditionalValue<T>> IReliableQueue<T>.TryDequeueAsync(ITransaction tx, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryDequeueAsync((Transaction) tx, timeout, cancellationToken);
        }

        Task<ConditionalValue<T>> IReliableQueue<T>.TryPeekAsync(ITransaction tx)
        {
            return this.TryPeekAsync((Transaction) tx);
        }

        Task<ConditionalValue<T>> IReliableQueue<T>.TryPeekAsync(ITransaction tx, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryPeekAsync((Transaction) tx, timeout, cancellationToken);
        }

        Task<ConditionalValue<T>> IReliableQueue<T>.TryPeekAsync(ITransaction tx, LockMode lockMode)
        {
            return this.TryPeekAsync((Transaction) tx, lockMode);
        }

        Task<ConditionalValue<T>> IReliableQueue<T>.TryPeekAsync(ITransaction tx, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryPeekAsync((Transaction) tx, lockMode, timeout, cancellationToken);
        }

        #endregion

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <exception cref="System.Fabric.FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// Retriable exception.
        /// Example reason: Distributed Collection is not yet ready.
        /// </exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">
        /// Distributed Collection is current not readable.
        /// </exception>
        public Task<long> GetCountAsync(Transaction tx)
        {
            return this.GetCountAsync(tx, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// Uses snapshot isolation.
        /// </summary>
        /// <param name="tx"></param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// Retriable exception.
        /// Example reason: Distributed Collection is not yet ready.
        /// </exception>
        /// <exception cref="System.Fabric.FabricNotReadableException">
        /// Distributed Collection is current not readable.
        /// </exception>
        public async Task<long> GetCountAsync(Transaction tx, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var conditionalValueStoreTxn = this.DataStore.CreateOrFindTransaction(tx);

            // #6882249: Whether this is the first time the store txn was created or not, GetCountAsync is always SNAPSHOT.
            var dataStoreTxn = conditionalValueStoreTxn.Value;
            dataStoreTxn.Isolation = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.MultiEntity);

            // #7728567: Read Your Own Write with Snapshot Isolation breaks snapshot Isolation.
            // Must get count by enumerating the data store instead of the metadatastore since otherwise snapshot isolation can be broken 
            // when write set is not empty.
            // Using ReadMode off since the value's are not required for calculating the count.
            long count = 0;
            var asyncEnumable = await this.DataStore.CreateEnumerableAsync(dataStoreTxn, false, ReadMode.Off).ConfigureAwait(false);
            var asyncEnumerator = asyncEnumable.GetAsyncEnumerator();
            while (await asyncEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false) == true)
            {
                count++;
            }

            FabricEvents.Events.GetCountAsync(this.TraceType, dataStoreTxn.Id, count);

            Diagnostics.Assert(count >= 0, ClassName, "Size of the queue can never be negative");
            Diagnostics.Assert(count <= int.MaxValue, ClassName, "This method can only return upto int.MaxValue");

            return count;
        }

        /// <summary>
        /// Adds an object to the end of the queue.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="item">
        /// The object to add to the end of the queue.
        /// The value can be a null reference for reference types.
        /// </param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Supports concurrent calls within a single transaction.
        public Task EnqueueAsync(Transaction tx, T item)
        {
            return this.EnqueueAsync(tx, item, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Adds an object to the end of the queue.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="item">
        /// The object to add to the end of the queue.
        /// The value can be a null reference for reference types.
        /// </param>
        /// <param name="timeout">
        /// A TimeSpan that represents the number of milliseconds to wait for the operation to complete 
        /// before throwing TimeoutException.
        /// </param>
        /// <param name="cancellationToken">The CancellationToken token to observe.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// Retriable exception.
        /// Example reason: Distributed Collection is not yet ready.
        /// </exception>
        /// Supports concurrent calls within a single transaction.
        /// <remarks>
        /// Rules:
        ///     1) Within enqueue, add can never fail.
        /// </remarks>
        public async Task EnqueueAsync(Transaction tx, T item, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var metadataStoreTransactionResult = this.MetadataStore.CreateOrFindTransaction(tx);
            var dataStoreTransactionResult = this.DataStore.CreateOrFindTransaction(tx);

            var metadataStoreRwTransaction = metadataStoreTransactionResult.Value;
            var dataStoreRwTransaction = dataStoreTransactionResult.Value;

            // 6882249: Enqueue always uses Repeatable Read for metadata store reads.
            // Note that since Enqueue is a write operations, RR is ok.
            // Enqueue does not read from data store so Isolation level of the data store txn does not need to be set.
            metadataStoreRwTransaction.Isolation = StoreTransactionReadIsolationLevel.ReadRepeatable;

            // To make sure the update does not deadlock/timeout.
            metadataStoreRwTransaction.LockingHints = LockingHints.UpdateLock;

            var getResult = await this.MetadataStore.GetAsync(
                metadataStoreRwTransaction,
                KeyIdOfTheRowThatPointsToTailInMetadataStore,
                timeout,
                cancellationToken).ConfigureAwait(false);

            var tailPointer = getResult.HasValue ? getResult.Value + 1 : InitialRowIdForTailPointer;

            FabricEvents.Events.EnqueueAsync(this.TraceType, tx.Id, tailPointer);

            var enqueueItem = this.CloneIfNecessary(ref item);

            try
            {
                // Add the item to the data store.
                await this.DataStore.AddAsync(
                    dataStoreRwTransaction,
                    tailPointer,
                    enqueueItem,
                    timeout,
                    cancellationToken).ConfigureAwait(false);
            }
            catch (ArgumentException e)
            {
                // Add cannot fail with "key already exists"
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "EnqueueAsync: Unexpected exception: {0}. Add of the item into data store must not fail with item already exists. {1}",
                    e.GetType(), e);
            }

            var shouldRollbackTheOperationAndThrowOperationAborted = false;
            Exception innerException = null;

            try
            {
                // Update the tail.
                // This operation is not cancellable.
                await this.MetadataStore.AddOrUpdateAsync(
                    metadataStoreRwTransaction,
                    KeyIdOfTheRowThatPointsToTailInMetadataStore,
                    tailPointer,
                    tailPointer,
                    timeout,
                    CancellationToken.None).ConfigureAwait(false);
            }
            catch (FabricTransientException e)
            {
                // Assumption: TStore does not throw transient exception.
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "EnqueueAsync: Unexpected exception: {0}. Assumption: TStore does not throw transient exception. {1}",
                    e.GetType(), e);
            }
            catch (Exception e)
            {
                this.AssertIfUnexpectedNonRetriableException(e, "EnqueueAsync");

                // Since as part of AddOperation, BeginReplicate is called, replicator can throw an FabricTransientException (e.g. QueueFull).
                shouldRollbackTheOperationAndThrowOperationAborted = true;
                innerException = e;
            }

            if (false == shouldRollbackTheOperationAndThrowOperationAborted)
            {
                Diagnostics.Assert(
                    null == innerException,
                    ClassName,
                    "EnqueueAsync: If an exception happened, compensation should have been initiated.");

                // Happy path completion.
                return;
            }

            // Compensation Code path.
            Diagnostics.Assert(
                null != innerException,
                ClassName,
                "EnqueueAsync: If compensation code path is called, exception must have been captured.");

            Exception compensationException = null;
            try
            {
                // Compensation
                var removeResult = await this.DataStore.TryRemoveAsync(
                    dataStoreRwTransaction,
                    tailPointer,
                    timeout,
                    CancellationToken.None).ConfigureAwait(false);

                // Since the item was added in this transaction, it must be removable.
                Diagnostics.Assert(
                    removeResult.HasValue,
                    ClassName,
                    "EnqueueAsync: Since the item was added in this transaction, it must be removable");
            }
            catch (FabricTransientException e)
            {
                // All "must not throw back to user" exceptions are handled as catch.
                // In this case we must not throw a transient exception to make sure customer does not consider the error benign.
                // Since the lock has been taken, this method cannot throw TimeoutException.
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "EnqueueAsync: Unexpected exception: {0}. Assumption: TStore does not throw transient exception. {1}",
                    e.GetType(), e);
            }
            catch (Exception e)
            {
                this.AssertIfUnexpectedNonRetriableException(e, "EnqueueAsync");

                compensationException = e;
            }

            if (null != compensationException)
            {
                FabricEvents.Events.EnqueueAsyncCompensationFailed(
                    this.TraceType,
                    tx.Id,
                    tailPointer,
                    compensationException.ToString());

                // Abort the user's transaction, as committing would result in data corruption (inconsistent state between
                // the data and metadata stores, as the data store add was unable to be compensated)
                // It is assumed that if Abort throws, it is a permanent exception and should not be handled
                await tx.AbortAsync().ConfigureAwait(false);

                if (this.IsExpectedNonRetriableException(compensationException))
                {
                    // Bubble up of expected permanent errors.
                    throw compensationException;
                }

                // TODO: Add tracing here.
                throw compensationException;
            }

            // Compensation happy path completion.
            throw innerException;
        }

        /// <summary>
        /// Attempts to remove and return the object at the beginning of the queue.
        /// </summary>
        /// <param name="tx">Transaction object</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the removed object.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Synchronizes concurrent calls within a single or across multiple transactions.
        public Task<ConditionalValue<T>> TryDequeueAsync(Transaction tx)
        {
            return this.TryDequeueAsync(tx, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Attempts to remove and return the object at the beginning of the queue.
        /// </summary>
        /// <param name="tx">Transaction object</param>
        /// <param name="timeout">
        /// A TimeSpan that represents the number of milliseconds to wait for the operation to complete 
        /// before throwing TimeoutException.
        /// </param>
        /// <param name="cancellationToken">The CancellationToken token to observe.</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the removed object.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Synchronizes concurrent calls within a single or across multiple transactions.
        public async Task<ConditionalValue<T>> TryDequeueAsync(Transaction tx, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var metadataStoreTransactionResult = this.MetadataStore.CreateOrFindTransaction(tx);
            var dataStoreTransactionResult = this.DataStore.CreateOrFindTransaction(tx);

            var metadataStoreRwTransaction = metadataStoreTransactionResult.Value;
            var dataStoreRwTransaction = dataStoreTransactionResult.Value;

            // 6882249: Dequeue always uses Repeatable Read for metadata store reads.
            // Note that since Dequeue is a write operations, RR is ok.
            // Dequeue does not read from data store so Isolation level of the data store txn does not need to be set.
            metadataStoreRwTransaction.Isolation = StoreTransactionReadIsolationLevel.ReadRepeatable;

            // Following lock hints provide strict first in first out guarantees.
            // This code is kept for if in future GetOrAdd does not take Xlock for Get.
            metadataStoreRwTransaction.LockingHints = LockingHints.UpdateLock;

            // Read the head pointer. Item3 is the value and Item1 indicates whether value was there.
            var getResult = await this.MetadataStore.GetAsync(
                metadataStoreRwTransaction,
                KeyIdOfTheRowThatPointsToHeadInMetadataStore,
                timeout,
                cancellationToken).ConfigureAwait(false);

            // If the head pointer has not been initialized assume it is Initial Row Id For Head Pointer.
            var headPointer = getResult.HasValue ? getResult.Value : InitialRowIdForHeadPointer;

            FabricEvents.Events.DequeueAsync(
                this.TraceType,
                tx.Id,
                headPointer);

            var itemToBeRemoved = await this.DataStore.TryRemoveAsync(
                dataStoreRwTransaction,
                headPointer,
                timeout,
                cancellationToken).ConfigureAwait(false);

            // Item1 stands for whether the item existed or not (was row removed).
            if (!itemToBeRemoved.HasValue)
            {
                // Catch if the key does not exist.
                // This implies that the queue is empty.
                return new ConditionalValue<T>();
            }

            var shouldRollbackTheOperationAndThrowOperationAborted = false;
            Exception innerException = null;

            try
            {
                // Update the head. We already have the update lock on the row.
                // Note that CancellationToken.None is used since the operation is not cancellable.
                await this.MetadataStore.AddOrUpdateAsync(
                    metadataStoreRwTransaction,
                    KeyIdOfTheRowThatPointsToHeadInMetadataStore,
                    headPointer + 1,
                    headPointer + 1,
                    timeout,
                    CancellationToken.None).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                this.AssertIfUnexpectedNonRetriableException(e, "TryDequeueAsync");

                // Since as part of AddOperation, BeginReplicate is called, replicator can throw an FabricTransientException (e.g. QueueFull).
                shouldRollbackTheOperationAndThrowOperationAborted = true;
                innerException = e;
            }

            if (false == shouldRollbackTheOperationAndThrowOperationAborted)
            {
                Diagnostics.Assert(
                    null == innerException,
                    ClassName,
                    "TryDequeueAsync: If an exception happened, compensation should have been initiated.");

                var result = itemToBeRemoved.Value;

                var removedItem = this.CloneIfNecessary(ref result);

                // Happy path completion.
                return new ConditionalValue<T>(true, removedItem);
            }

            // Compensation Code path.
            Diagnostics.Assert(
                null != innerException,
                ClassName,
                "TryDequeueAsync: If compensation code path is called, exception must have been captured.");

            Exception compensationException = null;
            try
            {
                // Note that CancellationToken.None is used since the operation is not cancellable.
                await this.DataStore.AddAsync(
                    dataStoreRwTransaction,
                    headPointer,
                    itemToBeRemoved.Value,
                    timeout,
                    CancellationToken.None).ConfigureAwait(false);
            }
            catch (FabricTransientException e)
            {
                // All "must not throw back to user" exceptions are handled as catch.
                // In this case we must not throw a transient exception to make sure customer does not consider the error benign.
                // Since the lock has been taken, this method cannot throw TimeoutException.
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "TryDequeueAsync: Unexpected exception: {0}. Assumption: TStore does not throw transient exception. {1}",
                    e.GetType(), e);
            }
            catch (Exception e)
            {
                this.AssertIfUnexpectedNonRetriableException(e, "TryDequeueAsync");

                compensationException = e;
            }

            if (null != compensationException)
            {
                FabricEvents.Events.DequeueAsyncCompensationFailed(
                    this.TraceType,
                    tx.Id,
                    headPointer,
                    compensationException.ToString());

                // Abort the user's transaction, as committing would result in data corruption (inconsistent state between
                // the data and metadata stores, as the data store add was unable to be compensated)
                // It is assumed that if Abort throws, it is a permanent exception and should not be handled
                await tx.AbortAsync().ConfigureAwait(false);

                if (this.IsExpectedNonRetriableException(compensationException))
                {
                    // let go of expected permanent errors.
                    throw compensationException;
                }

                // TODO: Add tracing here.
                throw compensationException;
            }

            // Compensation happy path completion.
            throw innerException;
        }

        /// <summary>
        /// Attempts to return an object from the beginning of the queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the object from the beginning of the queue.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        /// <exception cref="ArgumentNullException">transaction is null.</exception>
        /// <exception cref="ArgumentException">transaction, timeout, cancellation token.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">Retriable error.</exception>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Synchronizes concurrent calls within a single or across multiple transactions.
        public Task<ConditionalValue<T>> TryPeekAsync(Transaction tx)
        {
            return this.TryPeekAsync(tx, LockMode.Default, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Attempts to return an object from the beginning of the queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="timeout">
        /// A TimeSpan that represents the number of milliseconds to wait for the operation to complete 
        /// before throwing TimeoutException.
        /// </param>
        /// <param name="cancellationToken">The CancellationToken token to observe.</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the object from the beginning of the queue.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        /// <exception cref="ArgumentNullException">transaction is null.</exception>
        /// <exception cref="ArgumentException">transaction, timeout, cancellation token.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">Retriable error.</exception>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Synchronizes concurrent calls within a single or across multiple transactions.
        public Task<ConditionalValue<T>> TryPeekAsync(
            Transaction tx,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.TryPeekAsync(tx, LockMode.Default, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to return an object from the beginning of the queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the object from the beginning of the queue.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        /// <exception cref="ArgumentNullException">transaction is null.</exception>
        /// <exception cref="ArgumentException">transaction, timeout, cancellation token.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">Retriable error.</exception>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Synchronizes concurrent calls within a single or across multiple transactions.
        public Task<ConditionalValue<T>> TryPeekAsync(Transaction tx, LockMode lockMode)
        {
            return this.TryPeekAsync(
                tx,
                lockMode,
                this.DefaultTimeout,
                this.DefaultCancellationToken);
        }

        /// <summary>
        /// Attempts to return an object from the beginning of the queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <param name="timeout">
        /// A TimeSpan that represents the number of milliseconds to wait for the operation to complete 
        /// before throwing TimeoutException.
        /// </param>
        /// <param name="cancellationToken">The CancellationToken token to observe.</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the object from the beginning of the queue.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        /// <exception cref="ArgumentNullException">transaction is null.</exception>
        /// <exception cref="ArgumentException">transaction, timeout, cancellation token, lockMode.</exception>
        /// <exception cref="System.Fabric.FabricTransientException">Retriable error.</exception>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        /// Argument null check is done by TStore.
        /// Synchronizes concurrent calls within a single or across multiple transactions.
        public async Task<ConditionalValue<T>> TryPeekAsync(
            Transaction tx,
            LockMode lockMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            // Will throw ArgumentNullException if tx is null.
            var metadataStoreTransactionResult = this.MetadataStore.CreateOrFindTransaction(tx);
            var dataStoreTransactionResult = this.DataStore.CreateOrFindTransaction(tx);

            var metadataStoreRwTransaction = metadataStoreTransactionResult.Value;
            var dataStoreRwTransaction = dataStoreTransactionResult.Value;

            // 6882249: Ensure that different operations cannot change the expected isolation level.
            var isolationLevel = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.SingleEntity);
            metadataStoreRwTransaction.Isolation = isolationLevel;
            dataStoreRwTransaction.Isolation = isolationLevel;

            switch (lockMode)
            {
                case LockMode.Default:
                    // Taking shared lock by default.
                    break;
                case LockMode.Update:
                    metadataStoreRwTransaction.LockingHints = LockingHints.UpdateLock;
                    dataStoreRwTransaction.LockingHints = LockingHints.UpdateLock;
                    break;
                default:
                    Diagnostics.Assert(false, ClassName, "Invalid lockMode");
                    break;
            }

            // Read the head pointer.
            var getHeadResult = await this.MetadataStore.GetAsync(
                metadataStoreRwTransaction,
                KeyIdOfTheRowThatPointsToHeadInMetadataStore,
                timeout,
                cancellationToken).ConfigureAwait(false);

            // To make sure Peeks do not block each other we are not using GetOrAdd.
            var headPointer = false == getHeadResult.HasValue
                ? InitialRowIdForHeadPointer
                : getHeadResult.Value;

            var itemToBePeekedResult = await this.DataStore.GetAsync(
                dataStoreRwTransaction,
                headPointer,
                timeout,
                cancellationToken).ConfigureAwait(false);

            var value = itemToBePeekedResult.Value;

            var peekedItem = this.CloneIfNecessary(ref value);

            return false == itemToBePeekedResult.HasValue ?
                new ConditionalValue<T>() : new ConditionalValue<T>(true, peekedItem);
        }

        /// <summary>
        /// Returns an async enumerable that iterates through a collection.
        /// </summary>
        /// <returns>Enumerator that iterates through a collection.</returns>
        /// <remarks>
        /// Enumeration is ordered (FIFO) and unfiltered.
        /// Snapshot isolation is used for the enumeration.
        /// </remarks>
        public async Task<IAsyncEnumerable<T>> CreateEnumerableAsync(Transaction tx)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.DataStore.CreateOrFindTransaction(tx);
            var dataStoreTxn = replyTx.Value;
            dataStoreTxn.Isolation = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.MultiEntity);

            // Enumeration needs to be ordered to preserve FIFO.
            var enumerable = await this.DataStore.CreateEnumerableAsync(dataStoreTxn, true, ReadMode.ReadValue).ConfigureAwait(false);

            return new QueueSnapshotAsyncEnumerable<T>(this.ShouldCloneValue, dataStoreTxn, enumerable);
        }

        #region Life cycle

        /// <summary>
        /// Initialize state provider.
        /// </summary>
        /// <param name="transactionalReplicator">Transactional Replicator.</param>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="initializationContext">Initialization context.</param>
        /// <param name="stateProviderId">State provider Id.</param>
        /// <param name="workDirectory">Path to the work folder.</param>
        /// <param name="children">State provider's children.</param>
        public void Initialize(
            TransactionalReplicator transactionalReplicator,
            Uri name,
            byte[] initializationContext,
            long stateProviderId,
            string workDirectory,
            IEnumerable<IStateProvider2> children)
        {
            this.Initialize(
                transactionalReplicator,
                name,
                initializationContext,
                stateProviderId,
                transactionalReplicator.ServiceContext.PartitionId,
                transactionalReplicator.ServiceContext.ReplicaId,
                children);
        }

        /// <summary>
        /// Open the state provider.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task OpenAsync()
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Change the role of the state provider.
        /// </summary>
        /// <param name="newRole">New Role</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Close the state provider.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task CloseAsync()
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Abort of the service.
        /// </summary>
        public void Abort()
        {
        }

        /// <summary>
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <param name="name">Name of the parent state provider</param>
        /// <returns>Collection of state providers.</returns>
        public IEnumerable<IStateProvider2> GetChildren(Uri name)
        {
            // set names
            Uri tmpDataStoreName;
            Uri tmpMetadataStoreName;

            var resultDataStoreNameTryCombine = Common.TryCombine(
                name,
                this.relativePathForDataStore,
                out tmpDataStoreName);

            var resultMetadataStoreNameTryCombine = Common.TryCombine(
                name,
                this.relativePathForMetadataStore,
                out tmpMetadataStoreName);

            Diagnostics.Assert(
                resultDataStoreNameTryCombine,
                ClassName,
                "Uri.TryCreate failed for dataStore with parent name {0}",
                name);

            Diagnostics.Assert(
                resultMetadataStoreNameTryCombine,
                ClassName,
                "Uri.TryCreate failed for metadataStore with parent name {0}",
                name);

            var dataStoreName = tmpDataStoreName;
            var metadataStoreName = tmpMetadataStoreName;

            // 1. Data store.
            var dataStore = new TStore<long, T, LongComparer, LongComparer, LongRangePartitioner>(
                this.transactionalReplicator,
                dataStoreName,
                StoreBehavior.MultiVersion,
                true);

            // 2. Metadata store.
            var metadataStore = new TStore<long, long, LongComparer, LongComparer, LongRangePartitioner>(
                this.transactionalReplicator,
                metadataStoreName,
                StoreBehavior.MultiVersion,
                true);

            var children = new List<IStateProvider2>(2);
            children.Add(dataStore);
            children.Add(metadataStore);
            return children;
        }

        #endregion

        #region Copy related operations

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <returns>Empty operation stream.</returns>
        public IOperationDataStream GetCurrentState()
        {
            return new EmptyOperationDataStream();
        }

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        public void BeginSettingCurrentState()
        {
        }

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <param name="stateRecordNumber">Record number.</param>
        /// <param name="data">Data to set to.</param>
        public Task SetCurrentStateAsync(long stateRecordNumber, OperationData data)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        public Task EndSettingCurrentStateAsync()
        {
            return Task.FromResult(0);
        }

        #endregion

        #region Recovery

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task OnRecoveryCompletedAsync()
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task RecoverCheckpointAsync()
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
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
        public Task RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task OnDataLossAsync()
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Prepares for removing state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <param name="stateProviderId">
        /// Id that uniquely identifies state provider.
        /// </param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task RemoveStateAsync(long stateProviderId)
        {
            return Task.FromResult(true);
        }

        #endregion

        #region State Operations

        /// <summary>
        /// Apply operations
        /// </summary>
        /// <param name="lsn">Sequence number.</param>
        /// <param name="transaction">Transaction object.</param>
        /// <param name="metadata">metadata bytes pertaining to the operation.</param>
        /// <param name="data">The operation.</param>
        /// <param name="applyContext">Lock context.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task<object> ApplyAsync(long lsn, TransactionBase transaction, OperationData metadata, OperationData data, ApplyContext applyContext)
        {
            Diagnostics.Assert(
                false,
                ClassName,
                "ApplyAsync should never be called on DistributedQueue.");

            return Task.FromResult<object>(null);
        }

        /// <summary>
        /// Release the relevant locks.
        /// </summary>
        /// <param name="state">Lock context.</param>
        public void Unlock(object state)
        {
            Diagnostics.Assert(
                false,
                ClassName,
                "Unlock should never be called on DistributedQueue.");
        }

        /// <summary>
        /// Prepares for checkpoint.
        /// </summary>
        /// <param name="checkpointLSN">Checkpoint logical sequence number.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        #endregion

        /// <summary>
        /// Initialize state provider.
        /// </summary>
        /// <param name="transactionalReplicator">Transactional Replicator.</param>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="initializationContext">Initialization context.</param>
        /// <param name="stateProviderId">State provider Id.</param>
        /// <param name="partitionId">Service partition Id.</param>
        /// <param name="replicaId">Service replica Id.</param>
        /// <param name="children">The children</param>
        internal void Initialize(
            ITransactionalReplicator transactionalReplicator,
            Uri name,
            byte[] initializationContext,
            long stateProviderId,
            Guid partitionId,
            long replicaId,
            IEnumerable<IStateProvider2> children)
        {
            this.transactionalReplicator = transactionalReplicator;
            this.StateProviderName = name;
            this.stateProviderId = stateProviderId;

            Uri tmpDataStoreName;
            Uri tmpMetadataStoreName;

            var resultDataStoreNameTryCombine = Common.TryCombine(
                this.StateProviderName,
                this.relativePathForDataStore,
                out tmpDataStoreName);

            var resultMetadataStoreNameTryCombine = Common.TryCombine(
                this.StateProviderName,
                this.relativePathForMetadataStore,
                out tmpMetadataStoreName);

            Diagnostics.Assert(
                resultDataStoreNameTryCombine,
                ClassName,
                "Uri.TryCreate failed for dataStore with parent name {0}",
                this.StateProviderName);

            Diagnostics.Assert(
                resultMetadataStoreNameTryCombine,
                ClassName,
                "Uri.TryCreate failed for metadataStore with parent name {0}",
                this.StateProviderName);

            this.DataStoreName = tmpDataStoreName;
            this.MetadataStoreName = tmpMetadataStoreName;

            try
            {
                var initContext = QueueInitializationContext.FromByteArray(initializationContext);

                Diagnostics.Assert(
                    null != initContext,
                    ClassName,
                    "Initialization context must not be null");

                Diagnostics.Assert(
                    initContext.Version >= 1,
                    ClassName,
                    "Version number must exist and be greater or equal to 1. {0}",
                    initContext.Version);

                this.IsAutomaticCloningEnabled = initContext.IsAutomaticCloningEnabled;

                if (initContext.IsAutomaticCloningEnabled)
                {
                    if (true == typeof(T).GetTypeInfo().IsValueType)
                    {
                        this.ShouldCloneValue = false;
                    }
                    else if (typeof(T) == typeof(string))
                    {
                        this.ShouldCloneValue = false;
                    }
                    else
                    {
                        this.ShouldCloneValue = true;
                    }
                }
                else
                {
                    this.ShouldCloneValue = false;
                }
            }
            catch (Exception e)
            {
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "Deserialization failed with {0}",
                    e);
            }

            // Ignore output.
            // TODO: Consider having the LongByteConverter as static.
            this.transactionalReplicator.TryAddStateSerializer<long>(name, new LongByteConverter());

            var keySerializer = this.transactionalReplicator.GetStateSerializer<long>(name);
            var valueSerializer = this.transactionalReplicator.GetStateSerializer<T>(name);

            ReleaseAssert.AssertIf(children == null, "Children cannot be null");

            foreach (var child in children)
            {
                var result = Uri.Compare(child.Name, this.DataStoreName, UriComponents.AbsoluteUri, UriFormat.SafeUnescaped, StringComparison.Ordinal);

                if (result == 0)
                {
                    this.DataStore = (TStore<long, T, LongComparer, LongComparer, LongRangePartitioner>) child;
                    continue;
                }

                result = Uri.Compare(child.Name, this.MetadataStoreName, UriComponents.AbsoluteUri, UriFormat.SafeUnescaped, StringComparison.Ordinal);

                if (result == 0)
                {
                    this.MetadataStore = (TStore<long, long, LongComparer, LongComparer, LongRangePartitioner>) child;
                    continue;
                }
            }

            ReleaseAssert.AssertIf(this.DataStore == null, "Data store must have been in the children list.");
            ReleaseAssert.AssertIf(this.MetadataStore == null, "Metadata store must have been in the children list.");

            this.TraceType = string.Format(
                CultureInfo.InvariantCulture,
                "{0}@{1}@{2}@{3}",
                partitionId,
                replicaId,
                this.StateProviderName,
                this.stateProviderId);

            FabricEvents.Events.Initialize_DistributedQueue(
                this.TraceType,
                keySerializer.GetType().ToString(),
                typeof(T).ToString(),
                valueSerializer.GetType().ToString());

            // This method is only called if the State Provider is already registered.
            // Note that this code takes a dependency on the fact that Initialize is called on Primary and Secondary same way.
            this.IsRegistered = true;
        }

        /// <summary>
        /// Throw InvalidOperationException if Collections is not registered.
        /// </summary>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        private void ThrowIfNotRegistered()
        {
            if (false == this.IsRegistered)
            {
                throw new InvalidOperationException(SR.Error_DQ_NotRegistered);
            }
        }

        /// <summary>
        /// Is this an expected non retriable exception.
        /// </summary>
        /// <param name="e">The Exception.</param>
        /// <returns>The result.</returns>
        private bool IsExpectedNonRetriableException(Exception e)
        {
            return e is FabricNotPrimaryException ||
                   e is FabricObjectClosedException ||
                   e is ObjectDisposedException ||
                   e is InvalidOperationException ||
                   e is OperationCanceledException;
        }

        /// <summary>
        /// Assert if unexpected non retriable exception.
        /// </summary>
        /// <param name="e">The exception</param>
        /// <param name="method">Name of the method</param>
        private void AssertIfUnexpectedNonRetriableException(Exception e, string method)
        {
            if (
                e is ArgumentNullException ||
                e is ArgumentException ||
                e is FabricReplicationOperationTooLargeException ||
                e is FabricNotReadableException)
            {
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "{0}: Unexpected exception: {1}. {2}",
                    method, e.GetType(), e);
            }
        }

        /// <summary>
        /// Deep copy the input value if necessary.
        /// </summary>
        /// <param name="input">The input value.</param>
        /// <returns>Deep copied value if necessary.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private T CloneIfNecessary(ref T input)
        {
            if (false == this.ShouldCloneValue)
            {
                return input;
            }

            if (null == input)
            {
                return default(T);
            }

            var cloneable = input as ICloneable;

            Diagnostics.Assert(null != cloneable, ClassName, "input must be cloneable.");

            return (T) cloneable.Clone();
        }
    }
}