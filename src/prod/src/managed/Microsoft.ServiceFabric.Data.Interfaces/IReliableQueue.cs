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
    /// Represents a reliable first-in, first-out collection of objects that are persisted and replicated.
    /// </summary>
    /// <typeparam name="T">The type of the elements contained in the reliable queue.</typeparam>
    /// <remarks>
    /// <para>
    /// Values stored in this queue MUST NOT be mutated outside the context of an operation on the queue. It is
    /// highly recommended to make <typeparamref name="T"/> immutable in order to avoid accidental data corruption.
    /// </para>
    /// <para>
    /// Transaction is the unit of concurrency: Users can have multiple transactions in-flight at any given point of time but for a given transaction each API must be called one at a time.
    /// So all Reliable Collection APIs that take in a transaction and return a Task, must be awaited one at a time.
    /// <seealso cref="ITransaction"/>
    /// </para>
    /// </remarks>
    public interface IReliableQueue<T> : IReliableCollection<T>
    {
        /// <summary>
        /// Adds an object to the end of the reliable queue.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="item">The object to add to the end of the queue. The value can be null for reference types.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <cref name="IReliableQueue{T}"/> is not in <cref name="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <remarks>If a retriable exception is thrown by this method, it is recommended to dispose the transaction <paramref name="tx"/> and try again with a new transaction.</remarks>
        /// <returns>Task that represents the asynchronous enqueue operation.</returns>
        Task EnqueueAsync(ITransaction tx, T item);

        /// <summary>
        /// Adds an object to the end of the reliable queue.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="item">The object to add to the end of the queue. The value can be null for reference types.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <cref name="IReliableQueue{T}"/> is not in <cref name="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <remarks>If a retriable exception is thrown by this method, it is recommended to dispose the transaction <paramref name="tx"/> and try again with a new transaction.</remarks>
        /// <returns>Task that represents the asynchronous enqueue operation.</returns>
        Task EnqueueAsync(ITransaction tx, T item, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Tries to remove and return the object at the beginning of the reliable queue.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <cref name="IReliableQueue{T}"/> is not in <cref name="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <remarks>If a retriable exception is thrown by this method, it is recommended to dispose the transaction <paramref name="tx"/> and try again with a new transaction.</remarks>
        /// <returns>
        /// Task that represents the asynchronous dequeue operation. The task result is a tuple indicating
        /// whether an object was removed and if so, the object.
        /// </returns>
        Task<ConditionalValue<T>> TryDequeueAsync(ITransaction tx);

        /// <summary>
        /// Tries to remove and return the object at the beginning of the reliable queue.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotPrimaryException">The exception that is thrown when the <cref name="IReliableQueue{T}"/> is not in <cref name="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <remarks>If a retriable exception is thrown by this method, it is recommended to dispose the transaction <paramref name="tx"/> and try again with a new transaction.</remarks>
        /// <returns>
        /// Task that represents the asynchronous dequeue operation. The task result is a tuple indicating
        /// whether an object was removed and if so, the object.
        /// </returns>
        Task<ConditionalValue<T>> TryDequeueAsync(ITransaction tx, TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Tries to return an object from the beginning of the reliable queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the <cref name="IReliableQueue{T}"/> cannot serve reads at the moment.
        /// <cref name="FabricNotReadableException"/> can be thrown in all <cref name="ReplicaRole"/>s.
        /// One example for it being thrown in the <cref name="ReplicaRole.Primary"/> is loss of <cref name="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <cref name="ReplicaRole.ActiveSecondary"/> is that Reliable Queue's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <returns>
        /// Task that represents the asynchronous peek operation. The task result is a tuple indicating
        /// whether an object was found at the beginning of the queue and if so, the object.
        /// </returns>
        Task<ConditionalValue<T>> TryPeekAsync(ITransaction tx);

        /// <summary>
        /// Tries to return an object from the beginning of the reliable queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the <cref name="IReliableQueue{T}"/> cannot serve reads at the moment.
        /// <cref name="FabricNotReadableException"/> can be thrown in all <cref name="ReplicaRole"/>s.
        /// One example for it being thrown in the <cref name="ReplicaRole.Primary"/> is loss of <cref name="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <cref name="ReplicaRole.ActiveSecondary"/> is that Reliable Queue's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <returns>
        /// Task that represents the asynchronous peek operation. The task result is a tuple indicating
        /// whether an object was found at the beginning of the queue and if so, the object.
        /// </returns>
        Task<ConditionalValue<T>> TryPeekAsync(ITransaction tx, TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Tries to return an object from the beginning of the reliable queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the default timeout.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the <cref name="IReliableQueue{T}"/> cannot serve reads at the moment.
        /// <cref name="FabricNotReadableException"/> can be thrown in all <cref name="ReplicaRole"/>s.
        /// One example for it being thrown in the <cref name="ReplicaRole.Primary"/> is loss of <cref name="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <cref name="ReplicaRole.ActiveSecondary"/> is that Reliable Queue's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <returns>
        /// Task that represents the asynchronous peek operation. The task result is a tuple indicating
        /// whether an object was found at the beginning of the queue and if so, the object.
        /// </returns>
        Task<ConditionalValue<T>> TryPeekAsync(ITransaction tx, LockMode lockMode);

        /// <summary>
        /// Tries to return an object from the beginning of the reliable queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="lockMode">Type of locking to use for this read operation.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. Primarily used to prevent deadlocks. The default is 4 seconds.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is None.</param>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="ArgumentException"><paramref name="timeout"/> is negative.</exception>
        /// <exception cref="TimeoutException">The operation failed to complete within the given timeout.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled.</exception>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the <cref name="IReliableQueue{T}"/> cannot serve reads at the moment.
        /// <cref name="FabricNotReadableException"/> can be thrown in all <cref name="ReplicaRole"/>s.
        /// One example for it being thrown in the <cref name="ReplicaRole.Primary"/> is loss of <cref name="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <cref name="ReplicaRole.ActiveSecondary"/> is that Reliable Queue's state is not yet consistent.
        /// </exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <returns>
        /// Task that represents the asynchronous peek operation. The task result is a tuple indicating
        /// whether an object was found at the beginning of the queue and if so, the object.
        /// </returns>
        Task<ConditionalValue<T>> TryPeekAsync(ITransaction tx, LockMode lockMode, TimeSpan timeout,
            CancellationToken cancellationToken);

        /// <summary>
        /// Creates an async enumerable over the <see cref="IReliableQueue{T}"/>.
        /// </summary>
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <exception cref="FabricNotReadableException">
        /// Exception indicates that the <cref name="IReliableQueue{T}"/> cannot serve reads at the moment.
        /// <cref name="FabricNotReadableException"/> can be thrown in all <cref name="ReplicaRole"/>s.
        /// One example for it being thrown in the <cref name="ReplicaRole.Primary"/> is loss of <cref name="IStatefulServicePartition.ReadStatus"/>.
        /// One example for it being thrown in the <cref name="ReplicaRole.ActiveSecondary"/> is that Reliable Queue's state is not yet consistent.
        /// </exception>
        /// <returns>IEnumerable that represents all the values.</returns>
        Task<IAsyncEnumerable<T>> CreateEnumerableAsync(ITransaction tx);
    }
}