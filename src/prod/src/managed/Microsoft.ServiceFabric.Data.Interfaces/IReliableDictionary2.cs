// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents a reliable collection of key/value pairs that are persisted and replicated.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the reliable dictionary.</typeparam>
    /// <typeparam name="TValue">
    /// The type of the values in the reliable dictionary.</typeparam>
    /// <remarks>Keys or values stored in this dictionary MUST NOT be mutated outside the context of an operation on the 
    /// dictionary.  It is highly recommended to make both <typeparamref name="TKey"/> and <typeparamref name="TValue"/> 
    /// immutable in order to avoid accidental data corruption.
    /// 
    /// <para>
    /// The transaction is the unit of concurrency. Users can have multiple transactions in-flight at any given point of time, but for a given transaction each API must be called one at a time.
    /// When calling any asynchronous Reliable Collection method that takes an <see cref="ITransaction"/>, you must wait for completion of the returned Task before calling
    /// another method using the same transaction.
    /// </para>
    /// </remarks>
    public interface IReliableDictionary2<TKey, TValue> : IReliableDictionary<TKey, TValue>
        where TKey : IComparable<TKey>, IEquatable<TKey>
    {
        /// <summary>
        /// Creates an async enumerator over the <see cref="IReliableDictionary2{TKey,TValue}"/> to enumerate the keys.
        /// </summary>
        /// <param name="txn">Transaction to associate this operation with.</param>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the Reliable Dictionary cannot serve reads at the moment.
        /// <see cref="FabricNotReadableException"/> can be thrown in all <see cref="ReplicaRole"/>s.
        /// One example for it being thrown in the <see cref="ReplicaRole.Primary"/> is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <see cref="ReplicaRole.ActiveSecondary"/> is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <remarks>
        /// The enumerarable returned from the reliable dictionary is safe to use concurrently with reads and writes
        /// to the dictionary. It represents a snapshot consistent view of the dictionary.
        /// </remarks>
        /// <returns>An enumerable for the reliable dictionary keys.</returns>
        Task<IAsyncEnumerable<TKey>> CreateKeyEnumerableAsync(ITransaction txn);

        /// <summary>
        /// Creates an async enumerator over the <see cref="IReliableDictionary2{TKey,TValue}"/> to enumerate the keys.
        /// </summary>
        /// <param name="txn">Transaction to associate this operation with.</param>
        /// <param name="enumerationMode">The enumeration mode to use. The default is Unordered.</param>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the Reliable Dictionary cannot serve reads at the moment.
        /// <see cref="FabricNotReadableException"/> can be thrown in all <see cref="ReplicaRole"/>s.
        /// One example for it being thrown in the <see cref="ReplicaRole.Primary"/> is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <see cref="ReplicaRole.ActiveSecondary"/> is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <remarks>
        /// The enumerarable returned from the <see cref="IReliableDictionary2{TKey,TValue}"/> is safe to use concurrently with reads and writes
        /// to the dictionary. It represents a snapshot consistent view of the dictionary.
        /// </remarks>
        /// <returns>An enumerable for the <see cref="IReliableDictionary2{TKey,TValue}"/> keys.</returns>
        Task<IAsyncEnumerable<TKey>> CreateKeyEnumerableAsync(ITransaction txn, EnumerationMode enumerationMode);

        /// <summary>
        /// Creates an async enumerator over the <see cref="IReliableDictionary2{TKey,TValue}"/> to enumerate the keys.
        /// </summary>
        /// <param name="txn">Transaction to associate this operation with.</param>
        /// <param name="enumerationMode">The enumeration mode to use. The default is Unordered.</param>
        /// <param name="timeout">
        /// The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.
        /// </param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the Reliable Dictionary cannot serve reads at the moment.
        /// <see cref="FabricNotReadableException"/> can be thrown in all <see cref="ReplicaRole"/>s.
        /// One example for it being thrown in the <see cref="ReplicaRole.Primary"/> is loss of <see cref="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <see cref="ReplicaRole.ActiveSecondary"/> is that Reliable Collection's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="System.Fabric.FabricObjectClosedException">Indicates that the Reliable Dictionary is closed or deleted.</exception>
        /// <remarks>
        /// The enumerarable returned from the <see cref="IReliableDictionary2{TKey,TValue}"/> is safe to use concurrently with reads and writes
        /// to the dictionary. It represents a snapshot consistent view of the dictionary.
        /// </remarks>
        /// <returns>An enumerable for the <see cref="IReliableDictionary2{TKey,TValue}"/> keys.</returns>
        Task<IAsyncEnumerable<TKey>> CreateKeyEnumerableAsync(
            ITransaction txn, 
            EnumerationMode enumerationMode,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Gets the number of key-value pairs contained in the <see cref="IReliableDictionary2{TKey,TValue}"/>.
        /// </summary>
        /// <remarks>
        /// This property does not have transactional semantics. It represents the best effort number of items 
        /// in the dictionary at the moment when the property was accessed.
        /// </remarks>
        long Count { get; }
    }
}