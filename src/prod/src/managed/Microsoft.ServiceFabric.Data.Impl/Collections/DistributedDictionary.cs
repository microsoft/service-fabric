// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
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
    using Microsoft.ServiceFabric.Data.Notifications;
    using Microsoft.ServiceFabric.Replicator;

    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;
    using TransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Replicated Dictionary
    /// </summary>
    /// <typeparam name="TKey">The type of the values in the dictionary.</typeparam>
    /// <typeparam name="TValue">The type of the keys in the dictionary.</typeparam>
    internal class DistributedDictionary<TKey, TValue> :
        IStateProvider2,
        IDistributedDictionary<TKey, TValue>,
        IReliableDictionary2<TKey, TValue>
        where TKey : IComparable<TKey>, IEquatable<TKey>
    {
        /// <summary>
        /// Default cancellation token.
        /// </summary>
        public readonly CancellationToken DefaultCancellationToken = CancellationToken.None;

        /// <summary>
        /// Default timeout
        /// </summary>
        public readonly TimeSpan DefaultTimeout = TimeSpan.FromSeconds(4);

        /// <summary>
        /// Type name for tracing.
        /// </summary>
        private const string ClassName = "DistributedDictionary";

        /// <summary>
        /// Version number. Starts from 1.
        /// </summary>
        private const int Version = 1;

        /// <summary>
        /// Type name in the trace.
        /// </summary>
        private const StoreBehavior DefaultStateBehavior = StoreBehavior.MultiVersion;

        /// <summary>
        /// User's function to be called during rebuild: copy, restore and recovery.
        /// </summary>
        private Func<IReliableDictionary<TKey, TValue>, NotifyDictionaryRebuildEventArgs<TKey, TValue>, Task> userRebuildAsyncCallback;

        /// <summary>
        /// User's dictionary changed event handler.
        /// </summary>
        private EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> userDictionaryChangedEventHandler;

        /// <summary>
        /// Relative path for data store.
        /// </summary>
        private readonly Uri relativePathForDataStore = new Uri("dataStore", UriKind.Relative);

        /// <summary>
        /// The store behavior.
        /// </summary>
        private readonly StoreBehavior storeBehavior;

        /// <summary>
        /// Gets or sets the internal store.
        /// </summary>
        private TStore<TKey, TValue, DefaultComparer<TKey>, DefaultEqualityComparer<TKey>, EmptyRangePartitioner<TKey>> dataStore;

        /// <summary>
        /// Transactional Replicator.
        /// </summary>
        private ITransactionalReplicator transactionalReplicator;

        /// <summary>
        /// Gets or sets the State Provider Id.
        /// </summary>
        private long stateProviderId;

        /// <summary>
        /// Indicates whether event handler is registered with the data store.
        /// </summary>
        private bool isDataStoreEventHandlerRegistered;

        /// <summary>
        /// Lock for the data store event handler registration.
        /// </summary>
        private object dataStoreEventhHandlerRegisterationLock = new object();

        /// <summary>
        /// Initializes a new instance of the DistributedDictionary class.
        /// </summary>
        public DistributedDictionary()
        {
            this.ShouldCloneKey = false;
            this.ShouldCloneValue = false;
            this.IsAutomaticCloningEnabled = false;

            this.storeBehavior = DefaultStateBehavior;
            this.IsRegistered = false;
        }

        /// <summary>
        /// Initializes a new instance of the DistributedDictionary class.
        /// </summary>
        /// <param name="isAutomaticCloningEnabled">Is automatic cloning enabled.</param>
        public DistributedDictionary(bool isAutomaticCloningEnabled)
        {
            var result = this.SetInternalCloningVariablesFromFlag(isAutomaticCloningEnabled);

            if (true == result)
            {
                this.ShouldCloneKey = false;
                this.ShouldCloneValue = false;
                this.IsAutomaticCloningEnabled = false;

                throw new ArgumentException(SR.Error_DD_ICloneable_Expected);
            }

            this.storeBehavior = DefaultStateBehavior;
            this.IsRegistered = false;
        }

        /// <summary>
        /// Initializes a new instance of the DistributedDictionary class.
        /// </summary>
        /// <param name="storeBehavior">Store behavior</param>
        public DistributedDictionary(StoreBehavior storeBehavior)
        {
            this.storeBehavior = storeBehavior;
            this.IsRegistered = false;
        }

        /// <summary>
        /// Notification event for dictionary changes.
        /// </summary>
        public event EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> DictionaryChanged
        {
            add
            {
                // MCoskun: Order of the following operations is significant.
                // user event handler is set before the data store event handler is registered to ensure if a notification comes it can be plumbed.
                this.userDictionaryChangedEventHandler += value;

                lock (this.dataStoreEventhHandlerRegisterationLock)
                {
                    if (this.isDataStoreEventHandlerRegistered == false)
                    {
                        this.dataStore.DictionaryChanged += this.OnDictionaryChangedHandler;
                        this.isDataStoreEventHandlerRegistered = true;
                    }
                }
            }
            remove
            {
                this.userDictionaryChangedEventHandler -= value;
            }
        }

        /// <summary>
        /// Notification for rebuild.
        /// </summary>
        public Func<IReliableDictionary<TKey, TValue>, NotifyDictionaryRebuildEventArgs<TKey, TValue>, Task> RebuildNotificationAsyncCallback
        {
            set
            {
                // MCoskun: users callback must be set before registering the notification below.
                this.userRebuildAsyncCallback = value;

                if (this.dataStore.RebuildNotificationsAsyncCallback == null)
                {
                    // Ensures that the compiler does not reorder the two instructions.
                    Thread.MemoryBarrier();

                    // Register the notification with the TStore.
                    this.dataStore.RebuildNotificationsAsyncCallback = this.RebuildNotificationForwarderAsync;
                }
            }

            private get
            {
                return this.userRebuildAsyncCallback;
            }
        }

        /// <summary>
        /// Gets a value indicating whether automatic cloning is enabled for all returned values.
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
        /// Gets the name of the state provider
        /// </summary>
        public Uri StateProviderName { get; internal set; }

        /// <summary>
        /// Gets the store behavior.
        /// </summary>
        public StoreBehavior StoreBehavior
        {
            get { return this.storeBehavior; }
        }

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
                var context = new DictionaryInitializationContext(this.IsAutomaticCloningEnabled);
                return context.ToByteArray();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether key should be cloned.
        /// </summary>
        internal bool ShouldCloneKey { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether value should be cloned.
        /// </summary>
        internal bool ShouldCloneValue { get; set; }

        /// <summary>
        /// Gets the trace type name
        /// </summary>
        internal string TraceTypeName { get; private set; }

        /// <summary>
        /// For Testing only.
        /// </summary>
        internal TStore<TKey, TValue, DefaultComparer<TKey>, DefaultEqualityComparer<TKey>, EmptyRangePartitioner<TKey>> DataStore
        {
            get { return this.dataStore; }
        }

        /// <summary>
        /// Gets or sets the name of the data store.
        /// </summary>
        private Uri DataStoreName { get; set; }

        #region IReliableDictionary <TKey, TValue>

        Task<long> IReliableCollection<KeyValuePair<TKey, TValue>>.GetCountAsync(ITransaction tx)
        {
            return this.GetCountAsync((Transaction) tx);
        }

        Task IReliableDictionary<TKey, TValue>.AddAsync(ITransaction tx, TKey key, TValue value)
        {
            return this.AddAsync((Transaction) tx, key, value);
        }

        Task IReliableDictionary<TKey, TValue>.AddAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddAsync((Transaction) tx, key, value, timeout, cancellationToken);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.AddOrUpdateAsync(
            ITransaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return this.AddOrUpdateAsync((Transaction) tx, key, addValueFactory, updateValueFactory);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.AddOrUpdateAsync(
            ITransaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return this.AddOrUpdateAsync((Transaction) tx, key, addValue, updateValueFactory);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.AddOrUpdateAsync(
            ITransaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.AddOrUpdateAsync((Transaction) tx, key, addValueFactory, updateValueFactory, timeout, cancellationToken);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.AddOrUpdateAsync(
            ITransaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddOrUpdateAsync((Transaction) tx, key, addValue, updateValueFactory, timeout, cancellationToken);
        }

        Task IReliableCollection<KeyValuePair<TKey, TValue>>.ClearAsync()
        {
            return this.ClearAsync();
        }

        Task IReliableDictionary<TKey, TValue>.ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ClearAsync(timeout, cancellationToken);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.ContainsKeyAsync(ITransaction tx, TKey key)
        {
            return this.ContainsKeyAsync((Transaction) tx, key);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.ContainsKeyAsync(ITransaction tx, TKey key, LockMode lockMode)
        {
            return this.ContainsKeyAsync((Transaction) tx, key, lockMode);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.ContainsKeyAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ContainsKeyAsync((Transaction) tx, key, timeout, cancellationToken);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.ContainsKeyAsync(
            ITransaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ContainsKeyAsync((Transaction) tx, key, lockMode, timeout, cancellationToken);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.GetOrAddAsync(ITransaction tx, TKey key, Func<TKey, TValue> valueFactory)
        {
            return this.GetOrAddAsync((Transaction) tx, key, valueFactory);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.GetOrAddAsync(ITransaction tx, TKey key, TValue value)
        {
            return this.GetOrAddAsync((Transaction) tx, key, value);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.GetOrAddAsync(
            ITransaction tx, TKey key, Func<TKey, TValue> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetOrAddAsync((Transaction) tx, key, valueFactory, timeout, cancellationToken);
        }

        Task<TValue> IReliableDictionary<TKey, TValue>.GetOrAddAsync(
            ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.GetOrAddAsync((Transaction) tx, key, value, timeout, cancellationToken);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.TryAddAsync(ITransaction tx, TKey key, TValue value)
        {
            return this.TryAddAsync((Transaction) tx, key, value);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.TryAddAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryAddAsync((Transaction) tx, key, value, timeout, cancellationToken);
        }

        Task<ConditionalValue<TValue>> IReliableDictionary<TKey, TValue>.TryGetValueAsync(ITransaction tx, TKey key)
        {
            return this.TryGetValueAsync((Transaction) tx, key);
        }

        Task<ConditionalValue<TValue>> IReliableDictionary<TKey, TValue>.TryGetValueAsync(ITransaction tx, TKey key, LockMode lockMode)
        {
            return this.TryGetValueAsync((Transaction) tx, key, lockMode);
        }

        Task<ConditionalValue<TValue>> IReliableDictionary<TKey, TValue>.TryGetValueAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryGetValueAsync((Transaction) tx, key, timeout, cancellationToken);
        }

        Task<ConditionalValue<TValue>> IReliableDictionary<TKey, TValue>.TryGetValueAsync(ITransaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryGetValueAsync((Transaction) tx, key, lockMode, timeout, cancellationToken);
        }

        Task<ConditionalValue<TValue>> IReliableDictionary<TKey, TValue>.TryRemoveAsync(ITransaction tx, TKey key)
        {
            return this.TryRemoveAsync((Transaction) tx, key);
        }

        Task<ConditionalValue<TValue>> IReliableDictionary<TKey, TValue>.TryRemoveAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryRemoveAsync((Transaction) tx, key, timeout, cancellationToken);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.TryUpdateAsync(ITransaction tx, TKey key, TValue newValue, TValue comparisonValue)
        {
            return this.TryUpdateAsync((Transaction) tx, key, newValue, comparisonValue);
        }

        Task<bool> IReliableDictionary<TKey, TValue>.TryUpdateAsync(
            ITransaction tx, TKey key, TValue newValue, TValue comparisonValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryUpdateAsync((Transaction) tx, key, newValue, comparisonValue, timeout, cancellationToken);
        }

        Task IReliableDictionary<TKey, TValue>.SetAsync(ITransaction tx, TKey key, TValue value)
        {
            return this.AddOrUpdateAsync((Transaction) tx, key, value, (k, oldValue) => value);
        }

        Task IReliableDictionary<TKey, TValue>.SetAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.AddOrUpdateAsync((Transaction) tx, key, value, (k, oldValue) => value, timeout, cancellationToken);
        }

        #endregion

        #region IReliableDictionary2<TKey, TValue>

        public Task<IAsyncEnumerable<TKey>> CreateKeyEnumerableAsync(ITransaction txn)
        {
            return this.CreateKeyEnumerableAsync(txn, EnumerationMode.Unordered);
        }

        public async Task<IAsyncEnumerable<TKey>> CreateKeyEnumerableAsync(ITransaction txn, EnumerationMode enumerationMode)
        {
            var enumerable = await this.CreateEnumerableAsync(txn, null, enumerationMode, ReadMode.Off).ConfigureAwait(false);

            var keyEnumerable = enumerable as IAsyncEnumerable<TKey>;

            Diagnostics.Assert(keyEnumerable != null, ClassName, "keyEnumerable cannot be null.");

            return keyEnumerable;
        }

        public Task<IAsyncEnumerable<TKey>> CreateKeyEnumerableAsync(
            ITransaction txn,
            EnumerationMode enumerationMode,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.CreateKeyEnumerableAsync(txn, enumerationMode);
        }

        public long Count
        {
            get
            {
                this.ThrowIfNotRegistered();
                Diagnostics.Assert(this.dataStore != null, ClassName, "this.dataStore != null");
                return this.dataStore.Count;
            }
        }

        #endregion

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <exception cref="System.Fabric.FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        /// TODO: keep the count in memory.
        public Task<long> GetCountAsync(Transaction tx)
        {
            return this.GetCountAsync(tx, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="System.Fabric.FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        /// TODO: keep the count in memory.
        public async Task<long> GetCountAsync(Transaction tx, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            // Do not read the value.
            // Isolation level is set by CreateEnumerableAsync
            var enumerable = await this.CreateEnumerableAsync(tx, null, EnumerationMode.Unordered, ReadMode.Off).ConfigureAwait(false);
            var result = await enumerable.GetCountAsync().ConfigureAwait(false);
            return result;
        }

        /// <summary>
        /// Adds a key/value pair to the Distributed Dictionary if the key does not already exist, or updates a key/value pair if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return this.AddOrUpdateAsync(tx, key, addValueFactory, updateValueFactory, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the Distributed Dictionary if the key does not already exist, or updates a key/value pair if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task<TValue> AddOrUpdateAsync(
            Transaction tx,
            TKey key,
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Cloning inputs
            var tmpKey = this.CloneKeyIfNecessary(ref key);

            var reply = await this.dataStore.AddOrUpdateAsync(
                rwtx,
                tmpKey,
                (funcKey) =>
                {
                    var value = addValueFactory.Invoke(this.CloneKeyIfNecessary(ref funcKey));
                    return this.CloneValueIfNecessary(ref value);
                },
                (funcKey, funcValue) =>
                {
                    var value = updateValueFactory.Invoke(
                        this.CloneKeyIfNecessary(ref funcKey),
                        this.CloneValueIfNecessary(ref funcValue));

                    return this.CloneValueIfNecessary(ref value);
                },
                timeout,
                cancellationToken).ConfigureAwait(false);

            var tmpValueBeforeCopy = reply;
            var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

            return tmpValue;
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedCollection if they key does not already exist, or updates a key/value pair in the DistributedDictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValue">The value to be added for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns>
        /// The new value for the key. This will be either be addValue (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        public Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return this.AddOrUpdateAsync(
                tx,
                key,
                addValue,
                updateValueFactory,
                this.DefaultTimeout,
                this.DefaultCancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedCollection if they key does not already exist, or updates a key/value pair in the DistributedDictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValue">The value to be added for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// The new value for the key. This will be either be addValue (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        public async Task<TValue> AddOrUpdateAsync(
            Transaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Cloning inputs
            var tmpKey = this.CloneKeyIfNecessary(ref key);
            var tmpAddValue = this.CloneValueIfNecessary(ref addValue);

            var reply = await this.dataStore.AddOrUpdateAsync(
                rwtx,
                tmpKey,
                tmpAddValue,
                (funcKey, funcValue) =>
                {
                    var value = updateValueFactory.Invoke(
                        this.CloneKeyIfNecessary(ref funcKey),
                        this.CloneValueIfNecessary(ref funcValue));

                    return this.CloneValueIfNecessary(ref value);
                },
                timeout,
                cancellationToken).ConfigureAwait(false);

            var tmpValueBeforeCopy = reply;
            var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

            return tmpValue;
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ClearAsync()
        {
            return this.ClearAsync(this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            await this.dataStore.ClearAsync(timeout, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key)
        {
            return this.ContainsKeyAsync(tx, key, LockMode.Default, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.ContainsKeyAsync(tx, key, LockMode.Default, timeout, cancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode)
        {
            return this.ContainsKeyAsync(tx, key, lockMode, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            rwtx.Isolation = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.SingleEntity);

            this.SetLockMode(ref rwtx, lockMode);

            // Cloning if necessary.
            var tmpKey = this.CloneKeyIfNecessary(ref key);

            return this.dataStore.ContainsKeyAsync(rwtx, tmpKey, timeout, cancellationToken);
        }

        /// <summary>
        /// Creates an async enumerable over the DistributedDictionary. Uses unordered enumeration mode.
        /// Uses snapshot isolation.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        public async Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(ITransaction txn)
        {
            return await this.CreateEnumerableAsync(txn, null, EnumerationMode.Unordered).ConfigureAwait(false);
        }

        /// <summary>
        /// Creates an async enumerable over the DistributedDictionary.
        /// Uses snapshot isolation.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        public async Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn,
            EnumerationMode enumerationMode)
        {
            return await this.CreateEnumerableAsync(txn, null, enumerationMode).ConfigureAwait(false);
        }

        /// <summary>
        /// Creates an async enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="filter">Filtering the output.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn,
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode)
        {
            return this.CreateEnumerableAsync(txn, filter, enumerationMode, ReadMode.ReadValue);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        /// <remarks>Currently GetOrAdd will always take an exclusive lock.</remarks>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory)
        {
            return this.GetOrAddAsync(tx, key, valueFactory, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        /// <remarks>Currently GetOrAdd will always take an exclusive lock.</remarks>
        public async Task<TValue> GetOrAddAsync(
            Transaction tx,
            TKey key,
            Func<TKey, TValue> valueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Set the isolation we want for the read.  Today, TStore will ignore this and take an exclusive lock
            // on the Key, however in the future it may support e.g. snapshot.
            rwtx.Isolation = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.SingleEntity);

            var tmpKey = this.CloneKeyIfNecessary(ref key);

            // TODO: Today TStore only supports exclusive lock for operations that are ReadOrWrite.
            // So this API will always take an exclusive lock.
            // Once TStore supports getting other type of locks for the "get", we need to add support here.
            var reply = await this.dataStore.GetOrAddAsync(
                rwtx,
                tmpKey,
                (funcKey) =>
                {
                    var value = valueFactory.Invoke(this.CloneKeyIfNecessary(ref funcKey));
                    return this.CloneValueIfNecessary(ref value);
                },
                timeout,
                cancellationToken).ConfigureAwait(false);

            var tmpValueBeforeCopy = reply;
            var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

            return tmpValue;
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.GetOrAddAsync(tx, key, value, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        public async Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Set the isolation we want for the read.  Today, TStore will ignore this and take an exclusive lock
            // on the Key, however in the future it may support e.g. snapshot.
            rwtx.Isolation = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.SingleEntity);

            var tmpKey = this.CloneKeyIfNecessary(ref key);
            var tmpAddValue = this.CloneValueIfNecessary(ref value);

            // TODO: Today TStore only supports exclusive lock for operations that are ReadOrWrite.
            // So this API will always take an exclusive lock.
            // Once TStore supports getting other type of locks for the "get", we need to add support here.
            var reply = await this.dataStore.GetOrAddAsync(rwtx, tmpKey, tmpAddValue, timeout, cancellationToken).ConfigureAwait(false);

            var tmpValueBeforeCopy = reply;
            var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

            return tmpValue;
        }

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        public Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.TryAddAsync(tx, key, value, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        public Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Do not set isolation level for conditional writes, as TStore does the right thing

            var tmpKey = this.CloneKeyIfNecessary(ref key);
            var tmpAddValue = this.CloneValueIfNecessary(ref value);

            return this.dataStore.TryAddAsync(rwtx, tmpKey, tmpAddValue, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key)
        {
            return this.TryGetValueAsync(tx, key, LockMode.Default, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.TryGetValueAsync(tx, key, LockMode.Default, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to get.</param>
        /// <param name="lockMode">Lock to be used for the get operation.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, LockMode lockMode)
        {
            return this.TryGetValueAsync(tx, key, lockMode, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to get.</param>
        /// <param name="lockMode">Lock to be used for the get operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public async Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            rwtx.Isolation = IsolationHelper.GetIsolationLevel(tx, IsolationHelper.OperationType.SingleEntity);

            this.SetLockMode(ref rwtx, lockMode);

            var tmpKey = this.CloneKeyIfNecessary(ref key);

            var reply = await this.dataStore.GetAsync(rwtx, tmpKey, timeout, cancellationToken).ConfigureAwait(false);

            var tmpValueBeforeCopy = reply.Value;
            var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

            return new ConditionalValue<TValue>(reply.HasValue, tmpValue);
        }

        /// <summary>
        /// Attempts to remove and return the value with the specified key
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the boolean indicating whether
        /// the key/value pair was removed.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key)
        {
            return this.TryRemoveAsync(tx, key, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Attempts to remove and return the value with the specified key
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the boolean indicating whether
        /// the key/value pair was removed.
        /// </returns>
        public async Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Do not set isolation level for conditional writes, as TStore does the right thing

            var tmpKey = this.CloneKeyIfNecessary(ref key);

            var reply = await this.dataStore.TryRemoveAsync(rwtx, tmpKey, timeout, cancellationToken).ConfigureAwait(false);

            var tmpValueBeforeCopy = reply.Value;
            var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

            return new ConditionalValue<TValue>(reply.HasValue, tmpValue);
        }

        /// <summary>
        /// Compares the existing version for the specified key with a specified version,
        /// and if they are equal, updates the key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The value that replaces the value of the element with key.</param>
        /// <param name="newValue">
        /// The value that replaces the value of the element with key if the comparison
        /// results in equality.</param>
        /// <param name="comparisionValue">The value that is compared to the value of the element with key.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue)
        {
            return this.TryUpdateAsync(tx, key, newValue, comparisionValue, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Compares the existing version for the specified key with a specified version,
        /// and if they are equal, updates the key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The value that replaces the value of the element with key.</param>
        /// <param name="newValue">
        /// The value that replaces the value of the element with key if the comparison
        /// results in equality.</param>
        /// <param name="comparisionValue">The value that is compared to the value of the element with key.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<bool> TryUpdateAsync(
            Transaction tx, TKey key, TValue newValue, TValue comparisionValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Do not set isolation level for conditional writes, as TStore does the right thing

            var tmpKey = this.CloneKeyIfNecessary(ref key);
            var tmpNewValue = this.CloneValueIfNecessary(ref newValue);
            var tmpComparisonValue = this.CloneValueIfNecessary(ref comparisionValue);

            return this.dataStore.TryUpdateAsync(rwtx, tmpKey, tmpNewValue, tmpComparisonValue, timeout, cancellationToken);
        }

        /// <summary>
        /// Add the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task AddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.AddAsync(tx, key, value, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Add the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous operation.</returns>
        public async Task AddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Do not set isolation level for writes

            var tmpKey = this.CloneKeyIfNecessary(ref key);
            var tmpAddValue = this.CloneValueIfNecessary(ref value);

            await this.dataStore.AddAsync(rwtx, tmpKey, tmpAddValue, timeout, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Update the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to update.</param>
        /// <param name="value">The value to be added.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task UpdateAsync(Transaction tx, TKey key, TValue value)
        {
            return this.UpdateAsync(tx, key, value, this.DefaultTimeout, this.DefaultCancellationToken);
        }

        /// <summary>
        /// Update the specified key/value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to update.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.ArgumentException">Invalid store transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <returns>A task that represents the asynchronous operation.</returns>
        public async Task UpdateAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var replyTx = this.dataStore.CreateOrFindTransaction(tx);

            var rwtx = replyTx.Value;

            // Do not set isolation level for writes

            var tmpKey = this.CloneKeyIfNecessary(ref key);
            var tmpAddValue = this.CloneValueIfNecessary(ref value);

            await this.dataStore.UpdateAsync(rwtx, tmpKey, tmpAddValue, timeout, cancellationToken).ConfigureAwait(false);
        }

        #region Life cycle

        /// <summary>
        /// Initialize state provider.
        /// </summary>
        /// <param name="transactionalReplicator">Transactional Replicator.</param>
        /// <param name="name">Name of the state provider.</param>
        /// <param name="initializationContext">Initialization context.</param>
        /// <param name="stateProviderId">State provider Id.</param>
        /// <param name="workDirectory">Work directory.</param>
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
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Change the role of the state provider.
        /// </summary>
        /// <param name="newRole">New Role</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Close the state provider.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task CloseAsync()
        {
            lock (this.dataStoreEventhHandlerRegisterationLock)
            {
                if (this.isDataStoreEventHandlerRegistered)
                {
                    this.dataStore.DictionaryChanged -= this.OnDictionaryChangedHandler;
                    this.isDataStoreEventHandlerRegistered = false;
                }
            }
            
            var tcs = new TaskCompletionSource<object>();
            tcs.SetResult(null);
            return tcs.Task;
        }

        /// <summary>
        /// Abort of the service.
        /// </summary>
        public void Abort()
        {
            lock (this.dataStoreEventhHandlerRegisterationLock)
            {
                if (this.isDataStoreEventHandlerRegistered)
                {
                    this.dataStore.DictionaryChanged -= this.OnDictionaryChangedHandler;
                    this.isDataStoreEventHandlerRegistered = false;
                }
            }
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
        /// This API should only be called by Windows Fabric.
        /// </summary>
        /// <param name="stateProviderId"> Id that uniquely identifies state provider.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task RemoveStateAsync(long stateProviderId)
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
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <param name="name">Name of the parent state provider</param>
        /// <returns>Collection of state providers.</returns>
        public IEnumerable<IStateProvider2> GetChildren(Uri name)
        {
            Uri dataStoreUri;

            var resultTryCombineForDataStore = Common.TryCombine(name, this.relativePathForDataStore, out dataStoreUri);
            Diagnostics.Assert(
                resultTryCombineForDataStore,
                ClassName,
                "Uri.TryCreate failed for dataStore with parent name {0}",
                (name));

            // 1. dataStore.
            var dataStore = new TStore<TKey, TValue, DefaultComparer<TKey>, DefaultEqualityComparer<TKey>, EmptyRangePartitioner<TKey>>(
                this.transactionalReplicator,
                dataStoreUri,
                this.StoreBehavior,
                true);

            var children = new List<IStateProvider2>(1);
            children.Add(dataStore);
            return children;
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
                "ApplyAsync should never be called on {0}.",
                ClassName);

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
                "Unlock should never be called on {0}.",
                ClassName);
        }

        /// <summary>
        /// Prepares for checkpoint by snapping the state.
        /// </summary>
        /// <param name="checkpointLSN">Checkpoint logical sequence number.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PrepareCheckpointAsync(long checkpointLSN, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            return Task.FromResult(true);
        }

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
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
        /// <param name="children">State provider's children.</param>
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

            Uri resultUri;

            var resultTryCombine = Common.TryCombine(this.StateProviderName, this.relativePathForDataStore, out resultUri);

            Diagnostics.Assert(
                resultTryCombine,
                ClassName,
                "Uri.TryCreate failed for dataStore with parent name {0}",
                this.StateProviderName);

            this.DataStoreName = resultUri;

            try
            {
                var intializationContext = DictionaryInitializationContext.FromByteArray(initializationContext);

                Diagnostics.Assert(
                    null != intializationContext,
                    ClassName,
                    "Initialization context must not be null");

                Diagnostics.Assert(
                    intializationContext.Version >= 1,
                    ClassName,
                    "Version number must exist and be greater or equal to 1. {0}",
                    intializationContext.Version);

                var shouldAssert = this.SetInternalCloningVariablesFromFlag(intializationContext.IsAutomaticCloningEnabled);

                Diagnostics.Assert(false == shouldAssert, ClassName, "Is not cloneable.");
            }
            catch (Exception e)
            {
                Diagnostics.Assert(
                    false,
                    ClassName,
                    "Deserialization failed with {0}",
                    e);
            }

            var keySerializer = this.transactionalReplicator.GetStateSerializer<TKey>(name);
            var valueSerializer = this.transactionalReplicator.GetStateSerializer<TValue>(name);

            ReleaseAssert.AssertIf(children == null, "Children cannot be null");

            foreach (var child in children)
            {
                var result = Uri.Compare(child.Name, this.DataStoreName, UriComponents.AbsoluteUri, UriFormat.SafeUnescaped, StringComparison.Ordinal);

                if (result == 0)
                {
                    this.dataStore = (TStore<TKey, TValue, DefaultComparer<TKey>, DefaultEqualityComparer<TKey>, EmptyRangePartitioner<TKey>>) child;
                }
            }

            ReleaseAssert.AssertIf(this.dataStore == null, "Data store must have been in the children list.");

            this.TraceTypeName = string.Format(
                CultureInfo.InvariantCulture,
                "{0}@{1}@{2}@{3}",
                partitionId,
                replicaId,
                this.StateProviderName,
                this.stateProviderId);

            FabricEvents.Events.Initialize_DistributedDictionary(
                this.TraceTypeName,
                typeof(TKey).ToString(),
                typeof(TValue).ToString(),
                keySerializer.GetType().ToString(),
                valueSerializer.GetType().ToString(),
                this.StoreBehavior.ToString());

            // This method is only called if the State Provider is already registered.
            // Note that this code takes a dependency on the fact that Initialize is called on Primary and Secondary same way.
            this.IsRegistered = true;
        }

        /// <summary>
        /// Creates an async enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="filter">Filtering the output.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <param name="readMode">Specifies the read mode.</param>
        /// <returns>IAsyncEnumerable that represents all the key values.</returns>
        private async Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn,
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode,
            ReadMode readMode)
        {
            // Filter can be null.
            var replicatorTxn = txn as Transaction;
            Requires.ThrowIfNull(replicatorTxn, "txn is not a replicator transaction.");

            // Can throw InvalidOperationException.
            this.ThrowIfNotRegistered();

            var isOrdered = this.IsOrderedEnumeration(enumerationMode);

            var replyTx = this.dataStore.CreateOrFindTransaction(replicatorTxn);

            var readWriteTxn = replyTx.Value;
            readWriteTxn.Isolation = IsolationHelper.GetIsolationLevel(replicatorTxn, IsolationHelper.OperationType.MultiEntity);

            IAsyncEnumerable<KeyValuePair<TKey, TValue>> txnEnumerable;

            if (null == filter)
            {
                txnEnumerable = await this.dataStore.CreateEnumerableAsync(readWriteTxn, isOrdered, readMode).ConfigureAwait(false);
            }
            else
            {
                txnEnumerable = await this.dataStore.CreateEnumerableAsync(
                    readWriteTxn,
                    key => filter.Invoke(this.CloneKeyIfNecessary(ref key)),
                    isOrdered,
                    readMode).ConfigureAwait(false);
            }

            return new SnapshotAsyncEnumerable<TKey, TValue>(this.ShouldCloneKey, readWriteTxn, txnEnumerable);
        }

        /// <summary>
        /// Sets the lockMode.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="lockMode">Lock mode.</param>
        private void SetLockMode(ref IStoreReadWriteTransaction tx, LockMode lockMode)
        {
            switch (lockMode)
            {
                case LockMode.Default:
                    tx.LockingHints = LockingHints.None;
                    break;
                case LockMode.Update:
                    tx.LockingHints = LockingHints.UpdateLock;
                    break;
                default:
                    throw new ArgumentException(SR.Error_DD_LockMode_Invalid, SR.Error_LockMode);
            }
        }

        /// <summary>
        /// Throw InvalidOperationException if Collections is not registered.
        /// </summary>
        /// <exception cref="InvalidOperationException">StateProvider has not been registered.</exception>
        private void ThrowIfNotRegistered()
        {
            if (false == this.IsRegistered)
            {
                throw new InvalidOperationException(SR.Error_DD_Registration);
            }
        }

        /// <summary>
        /// Decides whether enumeration mode dictates ordering.
        /// </summary>
        /// <param name="enumerationMode">Enumeration mode.</param>
        /// <returns>Boolean indicating whether enumeration should be ordered.</returns>
        private bool IsOrderedEnumeration(EnumerationMode enumerationMode)
        {
            var isOrdered = false;

            if (EnumerationMode.Unordered == enumerationMode)
            {
                isOrdered = false;
            }
            else if (EnumerationMode.Ordered == enumerationMode)
            {
                isOrdered = true;
            }
            else
            {
                Diagnostics.Assert(false, ClassName, "Unknown enumerationMode");
            }

            return isOrdered;
        }

        /// <summary>
        /// Set internal variables according to the cloning flag.
        /// </summary>
        /// <param name="automaticCloningFlag">Is automatic cloning enabled.</param>
        /// <returns>Should throw or assert.</returns>
        private bool SetInternalCloningVariablesFromFlag(bool automaticCloningFlag)
        {
            if (false == automaticCloningFlag)
            {
                this.ShouldCloneKey = false;
                this.ShouldCloneValue = false;
                this.IsAutomaticCloningEnabled = false;

                return false;
            }

            var shouldThrow = true;

            if (true == typeof(TKey).GetTypeInfo().IsValueType)
            {
                this.ShouldCloneKey = false;
                shouldThrow = false;
            }
            else if (typeof(TKey) == typeof(string))
            {
                this.ShouldCloneKey = false;
                shouldThrow = false;
            }
            else if (false == typeof(ICloneable).IsAssignableFrom(typeof(TKey)))
            {
                this.ShouldCloneKey = false;
            }
            else
            {
                this.ShouldCloneKey = true;
                shouldThrow = false;
            }

            if (true == typeof(TValue).GetTypeInfo().IsValueType)
            {
                this.ShouldCloneValue = false;
                shouldThrow = false;
            }
            else if (typeof(TValue) == typeof(string))
            {
                this.ShouldCloneValue = false;
                shouldThrow = false;
            }
            else if (false == typeof(ICloneable).IsAssignableFrom(typeof(TValue)))
            {
                this.ShouldCloneValue = false;
            }
            else
            {
                this.ShouldCloneValue = true;
                shouldThrow = false;
            }

            this.IsAutomaticCloningEnabled = !shouldThrow;

            return shouldThrow;
        }

        /// <summary>
        /// Clone the input key if necessary.
        /// </summary>
        /// <param name="input">The input value.</param>
        /// <returns>Cloned value if necessary.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private TKey CloneKeyIfNecessary(ref TKey input)
        {
            if (false == this.ShouldCloneKey)
            {
                return input;
            }

            if (null == input)
            {
                return default(TKey);
            }

            var cloneable = input as ICloneable;

            Diagnostics.Assert(null != cloneable, ClassName, "input must be cloneable.");

            return (TKey) cloneable.Clone();
        }

        /// <summary>
        /// Clone the input value if necessary.
        /// </summary>
        /// <param name="input">The input value.</param>
        /// <returns>Cloned value if necessary.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private TValue CloneValueIfNecessary(ref TValue input)
        {
            if (false == this.ShouldCloneValue)
            {
                return input;
            }

            if (null == input)
            {
                // default(T) must return null.
                return default(TValue);
            }

            var cloneable = input as ICloneable;

            Diagnostics.Assert(null != cloneable, ClassName, "input must be cloneable.");

            return (TValue) cloneable.Clone();
        }

        private async Task RebuildNotificationForwarderAsync(IReliableState reliableState, NotifyDictionaryRebuildEventArgs<TKey, TValue> args)
        {
            var snappedAsyncCallback = this.RebuildNotificationAsyncCallback;
            if (snappedAsyncCallback != null)
            {
                await snappedAsyncCallback.Invoke(this, args).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Forwards the notification from data store to the user.
        /// </summary>
        /// <param name="sender">dataStore.</param>
        /// <param name="e">Event.</param>
        /// <remarks>
        /// This method is required to switch sender from TStore to this.
        /// </remarks>
        private void OnDictionaryChangedHandler(object sender, NotifyDictionaryChangedEventArgs<TKey, TValue> e)
        {
            var snappedUserDictionaryChangedEventHandler = this.userDictionaryChangedEventHandler;

            if (snappedUserDictionaryChangedEventHandler != null)
            {
                snappedUserDictionaryChangedEventHandler.Invoke(this, e);
            }
        }
    }
}