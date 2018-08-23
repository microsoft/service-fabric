// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data.Notifications;

    /// <summary>
    /// <para>Represents a Reliable Collection of key/value pairs that are persisted and replicated.</para>
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the Reliable Dictionary.</typeparam>
    /// <typeparam name="TValue">The type of the values in the Reliable Dictionary.</typeparam>
    /// <remarks>
    /// <para>Keys or values stored in this dictionary MUST NOT be mutated outside the context of an operation on the 
    /// dictionary. It is highly recommended to make both <typeparamref name="TKey"/> and <typeparamref name="TValue"/> 
    /// immutable in order to avoid accidental data corruption.
    /// See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-work-with-reliable-collections">here</see> for common pitfalls.</para>
    /// <para>The transaction is the unit of concurrency. Users can have multiple transactions in-flight at any given point of time, but for a given transaction each API must be called one at a time.
    /// When calling any asynchronous Reliable Collection method that takes an <see cref="ITransaction"/>, you must wait for completion of the returned Task before calling
    /// another method using the same transaction. See examples of transactions <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-work-with-reliable-collections">here</see>.</para>
    /// </remarks>
    public interface IReliableDictionary<TKey, TValue> : IReliableCollection<KeyValuePair<TKey, TValue>>
        where TKey : IComparable<TKey>, IEquatable<TKey>
    {
        /// <summary>
        /// A function that is called when the Reliable Dictionary is being rebuilt during copy, restore or recovery.
        /// </summary>
        /// <remarks>
        /// <see cref="NotifyDictionaryChangedEventArgs{TKey, TValue}"/> can only be used within this callback.
        /// Once the asynchronous callback completes, the <see cref="NotifyDictionaryRebuildEventArgs{TKey, TValue}"/> becomes invalid. 
        /// See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-notifications">here</see> for more information. 
        /// </remarks>
        /// <value>
        /// The asynchronous rebuild notification function. 
        /// Function takes in IReliableDictionary and NotifyDictionaryRebuildEventArgs token and returns a Task that represents the asynchronous processing of the rebuild notification.
        /// </value>
        Func<IReliableDictionary<TKey, TValue>, NotifyDictionaryRebuildEventArgs<TKey, TValue>, Task> RebuildNotificationAsyncCallback
        {
            set;
        }

        /// <summary>
        /// Occurs when the Reliable Dictionary changes.
        /// For example, addition, update or removal of an item.
        /// </summary>
        event EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> DictionaryChanged;

        /// <summary>
        /// Adds the specified key/value pair to the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to be added.</param>
        /// <param name="value">The value to be added. The value can be null for reference types.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException">A value with the same key already exists in the Reliable Dictionary.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous add operation.</returns>
        Task AddAsync(ITransaction tx, TKey key, TValue value);

        /// <summary>
        /// Adds the specified key/value pair to the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to be added.</param>
        /// <param name="value">The value to be added. The value can be null for reference types.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException">A value with the same key already exists in the Reliable Dictionary, or <paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous add operation.</returns>
        Task AddAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the Reliable Dictionary if the key does not already exist,
        /// or to update a key/value pair in the Reliable Dictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized, or <paramref name="addValueFactory"/> is null, or <paramref name="updateValueFactory"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// Task that represents the asynchronous add or update operation. The task result is the new value for the key. This will be
        /// either the result of addValueFactory (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        Task<TValue> AddOrUpdateAsync(
            ITransaction tx, 
            TKey key, 
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary if the key does not already exist, or updates a key/value pair 
        /// in the Reliable Dictionary by using the specified function if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="addValue">The value to be added for an absent key. The value can be null for reference types.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized, or <paramref name="updateValueFactory"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// Task that represents the asynchronous add or update operation. The task result is the new value for the key. This will be
        /// either addValue (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        Task<TValue> AddOrUpdateAsync(
            ITransaction tx, 
            TKey key, 
            TValue addValue,
            Func<TKey, TValue, TValue> updateValueFactory);

        /// <summary>
        /// Uses the specified functions to add a key/value pair to the Reliable Dictionary if the key does not already exist,
        /// or to update a key/value pair in the Reliable Dictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="addValueFactory">The function used to generate a value for an absent key.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized, or <paramref name="addValueFactory"/> is null, or <paramref name="updateValueFactory"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// Task that represents the asynchronous add or update operation. The task result is the new value for the key. This will be
        /// either the result of addValueFactory (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        Task<TValue> AddOrUpdateAsync(
            ITransaction tx, 
            TKey key, 
            Func<TKey, TValue> addValueFactory,
            Func<TKey, TValue, TValue> updateValueFactory, 
            TimeSpan timeout, 
            CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary if the key does not already exist, or updates a key/value pair 
        /// in the Reliable Dictionary by using the specified function if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to be added or whose value should be updated.</param>
        /// <param name="addValue">The value to be added for an absent key. The value can be null for reference types.</param>
        /// <param name="updateValueFactory">The function used to generate a new value for an existing key based on the key's existing value.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized, or <paramref name="updateValueFactory"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// Task that represents the asynchronous add or update operation. The task result is the new value for the key. This will be
        /// either addValue (if the key was absent) or the result of updateValueFactory (if the key was present).
        /// </returns>
        Task<TValue> AddOrUpdateAsync(
            ITransaction tx, 
            TKey key, 
            TValue addValue,
            Func<TKey, TValue, TValue> updateValueFactory, 
            TimeSpan timeout, 
            CancellationToken cancellationToken);

        /// <summary>
        /// Removes all keys and values from the Reliable Dictionary.
        /// </summary>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <returns>Task that represents the asynchronous clear operation.</returns>
        Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Determines whether the Reliable Dictionary contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to locate in the Reliable Dictionary.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous operation. The task result indicates whether the key exists.</returns>
        Task<bool> ContainsKeyAsync(ITransaction tx, TKey key);

        /// <summary>
        /// Determines whether the Reliable Dictionary contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to locate in the Reliable Dictionary.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous operation. The task result indicates whether the key exists.</returns>
        Task<bool> ContainsKeyAsync(ITransaction tx, TKey key, LockMode lockMode);

        /// <summary>
        /// Determines whether the Reliable Dictionary contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to locate in the Reliable Dictionary.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous operation. The task result indicates whether the key exists.</returns>
        Task<bool> ContainsKeyAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Determines whether the Reliable Dictionary contains the specified key.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key to locate in the Reliable Dictionary.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous operation. The task result indicates whether the key exists.</returns>
        Task<bool> ContainsKeyAsync(
            ITransaction tx, 
            TKey key, 
            LockMode lockMode, 
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Creates an asynchronous enumerator over the <see cref="IReliableDictionary{TKey,TValue}"/>.
        /// </summary>
        /// <param name="txn">The transaction to associate this operation with.</param>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>Indicates that the Reliable Dictionary is closed or deleted.</para>
        /// </exception>
        /// <remarks>
        /// <para>The returned enumerator is safe to use concurrently with reads and writes to the Reliable Dictionary.
        /// It represents a snapshot consistent view. Please note that <see cref="IAsyncEnumerable{T}.GetAsyncEnumerator"/> needs to be called on
        /// the returned IAsyncEnumerable in order to enumerate. Example usage can be
        /// seen <see href="https://github.com/Azure-Samples/service-fabric-dotnet-web-reference-app/blob/master/ReferenceApp/Inventory.Service/InventoryService.cs">here</see>.</para>
        /// </remarks>
        /// <returns>
        /// <para>A task that represents the asynchronous create enumerable operation. The task result is an enumerator for the Reliable Dictionary.</para>
        /// </returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(ITransaction txn);

        /// <summary>
        /// Creates an asynchronous enumerator over the <see cref="IReliableDictionary{TKey,TValue}"/>.
        /// </summary>
        /// <param name="txn">The transaction to associate this operation with.</param>
        /// <param name="enumerationMode">The enumeration mode to use. The default is Unordered. Ordered enumeration is ascending only. </param>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>Indicates that the Reliable Dictionary is closed or deleted.</para>
        /// </exception>
        /// <remarks>
        /// <para>The returned enumerator is safe to use concurrently with reads and writes to the Reliable Dictionary.
        /// It represents a snapshot consistent view. Please note that <see cref="IAsyncEnumerable{T}.GetAsyncEnumerator"/> needs to be called on
        /// the returned IAsyncEnumerable in order to enumerate. Example usage can be
        /// seen <see href="https://github.com/Azure-Samples/service-fabric-dotnet-web-reference-app/blob/master/ReferenceApp/Inventory.Service/InventoryService.cs">here</see>.</para>
        /// </remarks>
        /// <returns>
        /// <para>A task that represents the asynchronous create enumerable operation. The task result is an enumerator for the Reliable Dictionary.</para>
        /// </returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(ITransaction txn, EnumerationMode enumerationMode);

        /// <summary>
        /// Creates an asynchronous enumerator over the <see cref="IReliableDictionary{TKey,TValue}"/>.
        /// </summary>
        /// <param name="txn">The transaction to associate this operation with.</param>
        /// <param name="filter">Predicate that filters the key-value pairs to include in the enumeration based on the key.</param>
        /// <param name="enumerationMode">The enumeration mode to use. The default is Unordered. Ordered enumeration is ascending only.</param>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>Indicates that the Reliable Dictionary is closed or deleted.</para>
        /// </exception>
        /// <remarks>
        /// <para>The returned enumerator is safe to use concurrently with reads and writes to the Reliable Dictionary.
        /// It represents a snapshot consistent view. Please note that <see cref="IAsyncEnumerable{T}.GetAsyncEnumerator"/> needs to be called on
        /// the returned IAsyncEnumerable in order to enumerate. Example usage can be
        /// seen <see href="https://github.com/Azure-Samples/service-fabric-dotnet-web-reference-app/blob/master/ReferenceApp/Inventory.Service/InventoryService.cs">here</see>.</para>
        /// </remarks>
        /// <returns>
        /// <para>A task that represents the asynchronous create enumerable operation. The task result is an enumerator for the Reliable Dictionary.</para>
        /// </returns>
        Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(
            ITransaction txn, 
            Func<TKey, bool> filter,
            EnumerationMode enumerationMode);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized, or <paramref name="valueFactory"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous add operation. The task result is the value for the key. This will be
        /// either the existing value for the key if the key is already in the Reliable Dictionary, or the new value for the key
        /// as returned by valueFactory if the key was not in the Reliable Dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(ITransaction tx, TKey key, Func<TKey, TValue> valueFactory);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary if the key does not already exist.
        /// If the key exists no updates will be made to the value.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous add operation. The task result is the value for the key. This will be either the
        /// existing value for the key if the key is already in the Reliable Dictionary, or the new value if the key was not in the Reliable Dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(ITransaction tx, TKey key, TValue value);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary by using the specified function, if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="valueFactory">The function used to generate a value for the key.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized, or <paramref name="valueFactory"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous add operation. The task result is the value for the key. This will be
        /// either the existing value for the key if the key is already in the Reliable Dictionary, or the new value for the key
        /// as returned by valueFactory if the key was not in the Reliable Dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(
            ITransaction tx, 
            TKey key, 
            Func<TKey, TValue> valueFactory, 
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary if the key does not already exist.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value to be added, if the key does not already exist.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous add operation. The task result is the value for the key. This will be either the
        /// existing value for the key if the key is already in the Reliable Dictionary, or the new value if the key was not in the Reliable Dictionary.
        /// </returns>
        Task<TValue> GetOrAddAsync(
            ITransaction tx, 
            TKey key, 
            TValue value, 
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to add the specified key and value to the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous add operation. The task result indicates whether the key/value pair was added.</returns>
        Task<bool> TryAddAsync(ITransaction tx, TKey key, TValue value);

        /// <summary>
        /// Attempts to add the specified key and value to the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to add.</param>
        /// <param name="value">The value of the element to add. The value can be null for reference types.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous add operation. The task result indicates whether the key/value pair was added.</returns>
        Task<bool> TryAddAsync(
            ITransaction tx, 
            TKey key, 
            TValue value, 
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to get the value associated with the specified key from the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the value to get.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous read operation. The task result is a tuple indicating
        /// whether the key was found in the Reliable Dictionary and if so, the value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(ITransaction tx, TKey key);

        /// <summary>
        /// Attempts to get the value associated with the specified key from the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the value to get.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous read operation. The task result is a tuple indicating
        /// whether the key was found in the Reliable Dictionary and if so, the value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(ITransaction tx, TKey key, LockMode lockMode);

        /// <summary>
        /// Attempts to get the value associated with the specified key from the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the value to get.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous read operation. The task result is a tuple indicating
        /// whether the key was found in the Reliable Dictionary and if so, the value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(
            ITransaction tx, 
            TKey key, 
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to get the value associated with the specified key from the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the value to get.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Indicates that the IReliableDictionary cannot serve reads at the moment.
        /// This exception can be thrown in all <see cref="ReplicaRole"/>s.
        /// One reason it may be thrown in the <see cref="ReplicaRole.Primary"/> role is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One reason it may be thrown in the <see cref="ReplicaRole.ActiveSecondary"/> role is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// A task that represents the asynchronous read operation. The task result is a tuple indicating
        /// whether the key was found in the Reliable Dictionary and if so, the value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryGetValueAsync(
            ITransaction tx, 
            TKey key, 
            LockMode lockMode,
            TimeSpan timeout, 
            CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to remove the value with the specified key from the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// Task that represents the asynchronous remove operation. The task result is a tuple indicating
        /// whether the key was removed from the Reliable Dictionary and if so, the value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryRemoveAsync(ITransaction tx, TKey key);

        /// <summary>
        /// Attempts to remove the value with the specified key from the Reliable Dictionary.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key of the element to remove.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>
        /// Task that represents the asynchronous remove operation. The task result is a tuple indicating
        /// whether the key was removed from the Reliable Dictionary and if so, the value.
        /// </returns>
        Task<ConditionalValue<TValue>> TryRemoveAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key whose value is compared with <paramref name="comparisonValue"/> and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified <paramref name="key"/> if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified <paramref name="key"/>.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous update operation. The task result indicates whether the object was updated.</returns>
        Task<bool> TryUpdateAsync(ITransaction tx, TKey key, TValue newValue, TValue comparisonValue);

        /// <summary>
        /// Compares the existing value for the specified key with a specified value, and if they are equal, updates the key with a third value.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key whose value is compared with <paramref name="comparisonValue"/> and possibly replaced.</param>
        /// <param name="newValue">The value that replaces the value of the element that has the specified <paramref name="key"/> if the comparison results in equality.</param>
        /// <param name="comparisonValue">The value that is compared to the value of the element that has the specified <paramref name="key"/>.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous update operation. The task result indicates whether the object was updated.</returns>
        Task<bool> TryUpdateAsync(
            ITransaction tx, 
            TKey key, 
            TValue newValue, 
            TValue comparisonValue, 
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary if the key does not already exist, or updates a key/value pair
        /// in the Reliable Dictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key whose value should be updated.</param>
        /// <param name="value">The value that replaces the value of the element that has the specified <paramref name="key"/>.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous update operation.</returns>
        Task SetAsync(ITransaction tx, TKey key, TValue value);

        /// <summary>
        /// Adds a key/value pair to the Reliable Dictionary if the key does not already exist, or updates a key/value pair
        /// in the Reliable Dictionary if the key already exists.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="key">The key whose value should be updated.</param>
        /// <param name="value">The value that replaces the value of the element that has the specified <paramref name="key"/>.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null, or <paramref name="key"/> is null or cannot be serialized.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <see cref="IReliableDictionary{TKey, TValue}"/> is not in <see cref="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <returns>A task that represents the asynchronous update operation.</returns>
        Task SetAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken);
    }
}