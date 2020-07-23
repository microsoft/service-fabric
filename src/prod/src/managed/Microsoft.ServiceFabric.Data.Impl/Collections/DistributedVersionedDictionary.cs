// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using Microsoft.ServiceFabric.Replicator;
    using System.Fabric.Store;
    using System.Threading;
    using System.Threading.Tasks;
    
    using TransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;
    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;

    /// <summary>
    /// Replicated Versioned Dictionary with default TKeyEqualityComparer, TKeyComparer and singleton partition.
    /// </summary>
    /// <typeparam name="TKey">The type of the values in the dictionary.</typeparam>
    /// <typeparam name="TValue">The type of the keys in the dictionary.</typeparam>
    internal class DistributedVersionedDictionary<TKey, TValue> :
        IStateProvider2,
        IDistributedVersionedDictionary<TKey, TValue>
        where TKey : IComparable<TKey>
    {
        /// <summary>
        /// Internal distributed dictionary.
        /// </summary>
        private readonly InternalDistributedDictionary<TKey, TValue, DefaultComparer<TKey>, DefaultEqualityComparer<TKey>, EmptyRangePartitioner<TKey>> internalDistributedDictionary;

        /// <summary>
        /// The store behavior.
        /// </summary>
        private readonly StoreBehavior storeBehavior;

        /// <summary>
        /// Initializes a new instance of the DistributedVersionedDictionary class.
        /// </summary>
        public DistributedVersionedDictionary()
        {
            this.storeBehavior = StoreBehavior.Historical;

            this.internalDistributedDictionary = new InternalDistributedDictionary<TKey, TValue, DefaultComparer<TKey>, DefaultEqualityComparer<TKey>, EmptyRangePartitioner<TKey>>(this.storeBehavior);
        }

        /// <summary>
        /// Gets the name.
        /// </summary>
        Uri IStateProvider2.Name
        {
            get
            {
                return this.internalDistributedDictionary.Name;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the distributed versioned dictionary is registered or not.
        /// </summary>
        public bool IsRegistered
        {
            get { return this.internalDistributedDictionary.IsRegistered; }
        }

        /// <summary>
        /// Gets the store behavior setting.
        /// </summary>
        public StoreBehavior StoreBehavior
        {
            get
            {
                return this.storeBehavior;
            }
        }

        /// <summary>
        /// Gets initialization context.
        /// </summary>
        /// <returns>byte array of initialization context.</returns>
        byte[] IStateProvider2.InitializationContext
        {
            get { return this.internalDistributedDictionary.InitializationContext; }
        }

        #region IDistributedVersionedDictionary
        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, DateTime since, DateTime until)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, DateTime since, DateTime until, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, IVersion since, IVersion until)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, IVersion since, IVersion until, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until, timeout, cancellationToken);
        }
        #endregion

        #region IStateProvider2
        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for abrupt closure.
        /// </summary>
        void IStateProvider2.Abort()
        {
            this.internalDistributedDictionary.Abort();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for applying an operation.
        /// </summary>
        /// <param name="logicalSequenceNumber">
        /// The logical sequence number.
        /// </param>
        /// <param name="transaction">
        /// The transaction.
        /// </param>
        /// <param name="data">
        /// The data.
        /// </param>
        /// <param name="applyContext">
        /// The apply context.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task<object> IStateProvider2.ApplyAsync(long logicalSequenceNumber, TransactionBase transaction, OperationData data, ApplyContext applyContext)
        {
            return this.internalDistributedDictionary.ApplyAsync(logicalSequenceNumber, transaction, data, applyContext);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The begin setting current state.
        /// </summary>
        void IStateProvider2.BeginSettingCurrentState()
        {
            this.internalDistributedDictionary.BeginSettingCurrentState();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called when the state provider's role is changing.
        /// </summary>
        /// <param name="newRole">
        /// The new role.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ChangeRoleAsync(newRole, cancellationToken);
        }

        /// <summary>
        /// Prepares for checkpoint.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PrepareCheckpointAsync(CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.PrepareCheckpointAsync(cancellationToken);
        }

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.PerformCheckpointAsync(performMode, cancellationToken);
        }

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.CompleteCheckpointAsync(cancellationToken);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for graceful close.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.CloseAsync()
        {
            return this.internalDistributedDictionary.CloseAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called during the end of copy.
        /// </summary>
        Task IStateProvider2.EndSettingCurrentStateAsync()
        {
            return this.internalDistributedDictionary.EndSettingCurrentStateAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The get current state.
        /// </summary>
        /// <returns>
        /// The <see cref="IOperationDataStream"/>.
        /// </returns>
        IOperationDataStream IStateProvider2.GetCurrentState()
        {
            return this.internalDistributedDictionary.GetCurrentState();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The initialize.
        /// </summary>
        /// <param name="txn">
        /// The transactional replicator.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <param name="initializationContext">
        /// The initialization context.
        /// </param>
        /// <param name="stateProviderId">
        /// The state provider id.
        /// </param>
        void IStateProvider2.Initialize(TransactionalReplicator txn, Uri name, byte[] initializationContext, Guid stateProviderId)
        {
            this.internalDistributedDictionary.Initialize(
                txn,
                name,
                initializationContext,
                stateProviderId);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called when a data loss is suspected.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.OnDataLossAsync()
        {
            return this.internalDistributedDictionary.OnDataLossAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The on recovery completed async.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.OnRecoveryCompletedAsync()
        {
            return this.internalDistributedDictionary.OnRecoveryCompletedAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for opening the state provider.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.OpenAsync()
        {
            return this.internalDistributedDictionary.OpenAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The recover checkpoint async.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.RecoverCheckpointAsync()
        {
            return this.internalDistributedDictionary.RecoverCheckpointAsync();
        }

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.BackupCheckpointAsync(backupDirectory, cancellationToken);
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
        Task IStateProvider2.RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.RestoreCheckpointAsync(backupDirectory, cancellationToken);
        }

        /// <summary>
        /// Prepares for removing state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.PrepareForRemoveAsync(transaction, timeout, cancellationToken);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The remove state async.
        /// </summary>
        /// <param name="stateProviderId">
        /// The state provider id.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.RemoveStateAsync(Guid stateProviderId)
        {
            return this.internalDistributedDictionary.RemoveStateAsync(stateProviderId);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The set current state.
        /// </summary>
        /// <param name="stateRecordNumber">
        /// The state record number.
        /// </param>
        /// <param name="data">
        /// The data.
        /// </param>
        void IStateProvider2.SetCurrentState(long stateRecordNumber, OperationData data)
        {
            this.internalDistributedDictionary.SetCurrentState(stateRecordNumber, data);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The unlock.
        /// </summary>
        /// <param name="state">
        /// The state.
        /// </param>
        void IStateProvider2.Unlock(object state)
        {
            this.internalDistributedDictionary.Unlock(state);
        }

        /// <summary>
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <param name="name">Name of the parent state provider</param>
        /// <returns>Collection of state providers.</returns>
        IEnumerable<IStateProvider2> IStateProvider2.GetChildren(Uri name)
        {
            return this.internalDistributedDictionary.GetChildren(name);
        }

        #endregion

        #region IDistributedDictionary <TKey, TValue>
        /// <summary>
        /// Adds a key/value pair to the Distributed Dictionary if the key does not already exist, or updates a key/value pair if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValueFactory,
                updateValueFactory);
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
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValueFactory,
                updateValueFactory,
                timeout,
                cancellationToken);
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
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValue,
                updateValueFactory);
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
        public Task<TValue> AddOrUpdateAsync(
            Transaction tx,
            TKey key,
            TValue addValue,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValue,
                updateValueFactory,
                timeout,
                cancellationToken);
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ClearAsync()
        {
            return this.internalDistributedDictionary.ClearAsync();
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ClearAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key, timeout, cancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key, lockMode);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key, lockMode, timeout, cancellationToken);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary. 
        /// Uses unordered enumeration mode.
        /// </summary>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable()
        {
            return this.internalDistributedDictionary.CreateEnumerable();
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary. 
        /// Uses unordered enumeration mode.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(Transaction txn)
        {
            return this.internalDistributedDictionary.CreateEnumerable(txn);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(enumerationMode);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(Transaction txn, EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(txn, enumerationMode);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="filter">Filtering the output.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(filter, enumerationMode);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="filter">Filtering the output.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(
            Transaction txn,
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(txn, filter, enumerationMode);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        /// <remarks>Currently GetOrAdd will always take an exclusive lock.</remarks>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, valueFactory);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        /// <remarks>Currently GetOrAdd will always take an exclusive lock.</remarks>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, valueFactory, timeout, cancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, value);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        public Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.internalDistributedDictionary.TryAddAsync(tx, key, value);
        }

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        public Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryAddAsync(tx, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key);
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
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key, timeout, cancellationToken);
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
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key, lockMode);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to get.</param>
        /// <param name="lockMode">Lock to be used for the get operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key, lockMode, timeout, cancellationToken);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the boolean indicating whether
        /// the key/value pair was removed.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.TryRemoveAsync(tx, key);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the boolean indicating whether
        /// the key/value pair was removed.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryRemoveAsync(tx, key, timeout, cancellationToken);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue)
        {
            return this.internalDistributedDictionary.TryUpdateAsync(tx, key, newValue, comparisionValue);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryUpdateAsync(tx, key, newValue, comparisionValue, timeout, cancellationToken);
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
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task AddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.internalDistributedDictionary.AddAsync(tx, key, value);
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
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task AddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.AddAsync(tx, key, value, timeout, cancellationToken);
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
            return this.internalDistributedDictionary.UpdateAsync(tx, key, value);
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
        public Task UpdateAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.UpdateAsync(tx, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <exception cref="FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        public Task<long> GetCountAsync()
        {
            return this.internalDistributedDictionary.GetCountAsync();
        }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        public Task<long> GetCountAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.GetCountAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return this.internalDistributedDictionary.GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
        #endregion
    }

    /// <summary>
    /// Replicated Versioned Dictionary
    /// </summary>
    /// <typeparam name="TKey">The type of the values in the dictionary.</typeparam>
    /// <typeparam name="TValue">The type of the keys in the dictionary.</typeparam>
    /// <typeparam name="TKeyComparer">Comparer for the keys.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Equality Comparer for the keys.</typeparam>
    /// <typeparam name="TKeyRangePartitioner">Used for dividing the key space into multiple partitions.</typeparam>
    [SuppressMessage("StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "Reviewed.")]
    internal class DistributedVersionedDictionary<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> : 
        IStateProvider2,
        IDistributedVersionedDictionary<TKey, TValue>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        /// <summary>
        /// Internal distributed dictionary.
        /// </summary>
        private readonly InternalDistributedDictionary<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> internalDistributedDictionary;

        /// <summary>
        /// The store behavior.
        /// </summary>
        private StoreBehavior storeBehavior;

        /// <summary>
        /// Initializes a new instance of the DistributedVersionedDictionary class.
        /// </summary>
        public DistributedVersionedDictionary()
        {
            this.storeBehavior = StoreBehavior.Historical;

            this.internalDistributedDictionary = new InternalDistributedDictionary<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(this.storeBehavior);
        }

        /// <summary>
        /// Gets the name.
        /// </summary>
        Uri IStateProvider2.Name
        {
            get
            {
                return this.internalDistributedDictionary.Name;
            }
        }

        /// <summary>
        /// Gets the store behavior setting.
        /// </summary>
        public StoreBehavior StoreBehavior
        {
            get
            {
                return this.storeBehavior;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the distributed versioned dictionary is registered or not.
        /// </summary>
        public bool IsRegistered
        {
            get { return this.internalDistributedDictionary.IsRegistered; }
        }

        /// <summary>
        /// Gets initialization context.
        /// </summary>
        /// <returns>byte array of initialization context.</returns>
        byte[] IStateProvider2.InitializationContext
        {
            get { return this.internalDistributedDictionary.InitializationContext; }
        }

        #region IDistributedVersionedDictionary
        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, DateTime since, DateTime until)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, DateTime since, DateTime until, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, IVersion since, IVersion until)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until);
        }

        /// <summary>
        /// Gets all versions for a given key.
        /// </summary>
        /// <param name="tx">Transaction that owns this operation.</param>
        /// <param name="key">Key to search.</param>
        /// <param name="since">Version bounding the versions returned.</param>
        /// <param name="until">Upper bound for the versions to be returned.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>True, all versions for the key, if it exists.</returns>
        public Task<Tuple<bool, IEnumerable<Tuple<bool, IVersion, TValue>>>> TryGetVersionsAsync(Transaction tx, TKey key, IVersion since, IVersion until, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetVersionsAsync(tx, key, since, until, timeout, cancellationToken);
        }
        #endregion

        #region IStateProvider2
        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for abrupt closure.
        /// </summary>
        void IStateProvider2.Abort()
        {
            this.internalDistributedDictionary.Abort();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for applying an operation.
        /// </summary>
        /// <param name="logicalSequenceNumber">
        /// The logical sequence number.
        /// </param>
        /// <param name="transaction">
        /// The transaction.
        /// </param>
        /// <param name="data">
        /// The data.
        /// </param>
        /// <param name="applyContext">
        /// The apply context.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task<object> IStateProvider2.ApplyAsync(long logicalSequenceNumber, TransactionBase transaction, OperationData data, ApplyContext applyContext)
        {
            return this.internalDistributedDictionary.ApplyAsync(logicalSequenceNumber, transaction, data, applyContext);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The begin setting current state.
        /// </summary>
        void IStateProvider2.BeginSettingCurrentState()
        {
            this.internalDistributedDictionary.BeginSettingCurrentState();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called when the state provider's role is changing.
        /// </summary>
        /// <param name="newRole">
        /// The new role.
        /// </param>
        /// <param name="cancellationToken">
        /// The cancellation token.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ChangeRoleAsync(newRole, cancellationToken);
        }

        /// <summary>
        /// Prepares for checkpoint.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PrepareCheckpointAsync(CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.PrepareCheckpointAsync(cancellationToken);
        }

        /// <summary>
        /// Checkpoints state to local disk.
        /// </summary>
        /// <param name="performMode">Represents different mode to perform checkpoint.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PerformCheckpointAsync(PerformCheckpointMode performMode, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.PerformCheckpointAsync(performMode, cancellationToken);
        }

        /// <summary>
        /// Completes checkpoint.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.CompleteCheckpointAsync(CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.CompleteCheckpointAsync(cancellationToken);
        }

        /// <summary>
        /// Prepares for removing state provider.
        /// </summary>
        /// <param name="transaction">transaction that this operation is a part of.</param>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.PrepareForRemoveAsync(Transaction transaction, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.PrepareForRemoveAsync(transaction, timeout, cancellationToken);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for graceful close.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.CloseAsync()
        {
            return this.internalDistributedDictionary.CloseAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called during the end of copy.
        /// </summary>
        Task IStateProvider2.EndSettingCurrentStateAsync()
        {
            return this.internalDistributedDictionary.EndSettingCurrentStateAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The get current state.
        /// </summary>
        /// <returns>
        /// The <see cref="IOperationDataStream"/>.
        /// </returns>
        IOperationDataStream IStateProvider2.GetCurrentState()
        {
            return this.internalDistributedDictionary.GetCurrentState();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The initialize.
        /// </summary>
        /// <param name="txn">
        /// The transactional replicator.
        /// </param>
        /// <param name="name">
        /// The name.
        /// </param>
        /// <param name="initializationContext">
        /// The initialization context.
        /// </param>
        /// <param name="stateProviderId">
        /// The state provider id.
        /// </param>
        void IStateProvider2.Initialize(TransactionalReplicator txn, Uri name, byte[] initializationContext, Guid stateProviderId)
        {
            this.internalDistributedDictionary.Initialize(
                txn,
                name,
                initializationContext,
                stateProviderId);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called when a data loss is suspected.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.OnDataLossAsync()
        {
            return this.internalDistributedDictionary.OnDataLossAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The on recovery completed async.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.OnRecoveryCompletedAsync()
        {
            return this.internalDistributedDictionary.OnRecoveryCompletedAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// Called for opening the state provider.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.OpenAsync()
        {
            return this.internalDistributedDictionary.OpenAsync();
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The recover checkpoint async.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.RecoverCheckpointAsync()
        {
            return this.internalDistributedDictionary.RecoverCheckpointAsync();
        }

        /// <summary>
        /// Backup the existing checkpoint state on local disk (if any) to the given directory.
        /// </summary>
        /// <param name="backupDirectory">The directory where the checkpoint backup is to be stored.</param>
        /// <param name="cancellationToken">Request cancellation of the checkpoint backup.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task IStateProvider2.BackupCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.BackupCheckpointAsync(backupDirectory, cancellationToken);
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
        Task IStateProvider2.RestoreCheckpointAsync(string backupDirectory, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.RestoreCheckpointAsync(backupDirectory, cancellationToken);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The remove state async.
        /// </summary>
        /// <param name="stateProviderId">
        /// The state provider id.
        /// </param>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        Task IStateProvider2.RemoveStateAsync(Guid stateProviderId)
        {
            return this.internalDistributedDictionary.RemoveStateAsync(stateProviderId);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The set current state.
        /// </summary>
        /// <param name="stateRecordNumber">
        /// The state record number.
        /// </param>
        /// <param name="data">
        /// The data.
        /// </param>
        void IStateProvider2.SetCurrentState(long stateRecordNumber, OperationData data)
        {
            this.internalDistributedDictionary.SetCurrentState(stateRecordNumber, data);
        }

        /// <summary>
        /// Warning: Should only be called by Windows Fabric.
        /// The unlock.
        /// </summary>
        /// <param name="state">
        /// The state.
        /// </param>
        void IStateProvider2.Unlock(object state)
        {
            this.internalDistributedDictionary.Unlock(state);
        }

        /// <summary>
        /// Get all the state providers contained within this state provider instance.
        /// </summary>
        /// <param name="name">Name of the parent state provider</param>
        /// <returns>Collection of state providers.</returns>
        IEnumerable<IStateProvider2> IStateProvider2.GetChildren(Uri name)
        {
            return this.internalDistributedDictionary.GetChildren(name);
        }
        #endregion

        #region IDistributedDictionary <TKey, TValue>
        /// <summary>
        /// Adds a key/value pair to the Distributed Dictionary if the key does not already exist, or updates a key/value pair if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to be added or whose value should be updated</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValueFactory,
                updateValueFactory);
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
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task<TValue> AddOrUpdateAsync(Transaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValueFactory,
                updateValueFactory,
                timeout,
                cancellationToken);
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
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValue,
                updateValueFactory);
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
        public Task<TValue> AddOrUpdateAsync(
            Transaction tx,
            TKey key,
            TValue addValue,
            Func<TKey, TValue, TValue> updateValueFactory,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.AddOrUpdateAsync(
                tx,
                key,
                addValue,
                updateValueFactory,
                timeout,
                cancellationToken);
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ClearAsync()
        {
            return this.internalDistributedDictionary.ClearAsync();
        }

        /// <summary>
        /// Removes all keys and values.
        /// </summary>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ClearAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key, timeout, cancellationToken);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key, lockMode);
        }

        /// <summary>
        /// Determines whether the DistributedDictionary Contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key to locate.s</param>
        /// <param name="lockMode">Lock to be used for the operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains a boolean that indicates whether the key exists.
        /// </returns>
        public Task<bool> ContainsKeyAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.ContainsKeyAsync(tx, key, lockMode, timeout, cancellationToken);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary. 
        /// Uses unordered enumeration mode.
        /// </summary>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable()
        {
            return this.internalDistributedDictionary.CreateEnumerable();
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary. 
        /// Uses unordered enumeration mode.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(Transaction txn)
        {
            return this.internalDistributedDictionary.CreateEnumerable(txn);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(enumerationMode);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(Transaction txn, EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(txn, enumerationMode);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="filter">Filtering the output.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(filter, enumerationMode);
        }

        /// <summary>
        /// Creates an enumerable over the DistributedDictionary.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="filter">Filtering the output.</param>
        /// <param name="enumerationMode">Enumeration mode for the enumeration.</param>
        /// <returns>IEnumerable that represents all the key values.</returns>
        /// <remarks>Isolation level is Snapshot.</remarks>
        public IEnumerable<KeyValuePair<TKey, TValue>> CreateEnumerable(
            Transaction txn,
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode)
        {
            return this.internalDistributedDictionary.CreateEnumerable(txn, filter, enumerationMode);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        /// <remarks>Currently GetOrAdd will always take an exclusive lock.</remarks>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, valueFactory);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exists.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the value of the key. This will be either the existing value for the key if the key is already in the dictionary, or the new value for the key as returned by valueFactory if the key was not in the dictionary.
        /// </returns>
        /// <remarks>Currently GetOrAdd will always take an exclusive lock.</remarks>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, Func<TKey, TValue> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, valueFactory, timeout, cancellationToken);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, value);
        }

        /// <summary>
        /// Adds a key/value pair to the DistributedDictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the value of the key
        /// if the key is already in the dictionary, 
        /// or the new value if the key was not in the dictionary.
        /// </returns>
        public Task<TValue> GetOrAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.GetOrAddAsync(tx, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        public Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.internalDistributedDictionary.TryAddAsync(tx, key, value);
        }

        /// <summary>
        /// Add the specified key value pair.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a boolean indicating whether the
        /// the key/value pair was added.
        /// </returns>
        public Task<bool> TryAddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryAddAsync(tx, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key);
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
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key, timeout, cancellationToken);
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
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key, lockMode);
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to get.</param>
        /// <param name="lockMode">Lock to be used for the get operation.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains a ConditionalValue of TValue.
        /// ConditionalValue.HasValue indicates whether the get operation was successful.
        /// ConditionalValue.Value is set to the value if the get operation was successful.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryGetValueAsync(Transaction tx, TKey key, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryGetValueAsync(tx, key, lockMode, timeout, cancellationToken);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the boolean indicating whether
        /// the key/value pair was removed.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key)
        {
            return this.internalDistributedDictionary.TryRemoveAsync(tx, key);
        }

        /// <summary>
        /// Attempts to remove and return the value with the specified key
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">Timeout for the operation.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <exception cref="System.ArgumentException">Invalid transaction or invalid timeout.</exception>
        /// <exception cref="System.ArgumentNullException">Key is null or transaction is null.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.OperationCanceledException">Operation was canceled.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// The value of the TResult parameter contains the boolean indicating whether
        /// the key/value pair was removed.
        /// </returns>
        public Task<ConditionalValue<TValue>> TryRemoveAsync(Transaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryRemoveAsync(tx, key, timeout, cancellationToken);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue)
        {
            return this.internalDistributedDictionary.TryUpdateAsync(tx, key, newValue, comparisionValue);
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
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<bool> TryUpdateAsync(Transaction tx, TKey key, TValue newValue, TValue comparisionValue, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.TryUpdateAsync(tx, key, newValue, comparisionValue, timeout, cancellationToken);
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
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task AddAsync(Transaction tx, TKey key, TValue value)
        {
            return this.internalDistributedDictionary.AddAsync(tx, key, value);
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
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task AddAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.AddAsync(tx, key, value, timeout, cancellationToken);
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
            return this.internalDistributedDictionary.UpdateAsync(tx, key, value);
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
        public Task UpdateAsync(Transaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.UpdateAsync(tx, key, value, timeout, cancellationToken);
        }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <exception cref="FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        public Task<long> GetCountAsync()
        {
            return this.internalDistributedDictionary.GetCountAsync();
        }

        /// <summary>
        /// Gets the number of elements contained in the IDistributedCollection.
        /// </summary>
        /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <exception cref="FabricException">StateProvider has not been registered.</exception>
        /// <returns>
        /// A task that represents the asynchronous operation. The value of the TResult parameter contains the total number of elements in the distributed collection.
        /// </returns>
        public Task<long> GetCountAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.internalDistributedDictionary.GetCountAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            return this.internalDistributedDictionary.GetEnumerator();
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.internalDistributedDictionary.GetEnumerator();
        }
        #endregion
    }
}