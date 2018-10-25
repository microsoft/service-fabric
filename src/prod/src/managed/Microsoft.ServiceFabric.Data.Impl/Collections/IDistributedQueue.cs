// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Interface of FIFO collection
    /// </summary>
    /// <typeparam name="T">Specifies the type of elements in the queue.</typeparam>
    internal interface IDistributedQueue<T> : IDistributedCollection
    {
        /// <summary>
        /// Adds an object to the end of the queue.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <param name="item">
        /// The object to add to the end of the queue.
        /// The value can be a null reference for reference types.
        /// </param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        Task EnqueueAsync(Transaction tx, T item);

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
        Task EnqueueAsync(Transaction tx, T item, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to remove and return the object at the beginning of the queue.
        /// </summary>
        /// <param name="tx">Transaction object</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the removed object.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        Task<ConditionalValue<T>> TryDequeueAsync(Transaction tx);

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
        Task<ConditionalValue<T>> TryDequeueAsync(Transaction tx, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Attempts to return an object from the beginning of the queue without removing it.
        /// </summary>
        /// <param name="tx">Transaction object.</param>
        /// <returns>
        /// If the operation was successful, output Tuple contains true indicating the 
        /// operation was successful and the object from the beginning of the queue.
        /// If the operation was unsuccessful, output Tuple contains false indicating the operation was unsuccessful.
        /// </returns>
        Task<ConditionalValue<T>> TryPeekAsync(Transaction tx);

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
        Task<ConditionalValue<T>> TryPeekAsync(Transaction tx, TimeSpan timeout, CancellationToken cancellationToken);

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
        Task<ConditionalValue<T>> TryPeekAsync(Transaction tx, LockMode lockMode);

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
        Task<ConditionalValue<T>> TryPeekAsync(Transaction tx, LockMode lockMode, TimeSpan timeout, CancellationToken cancellationToken);

        /// <summary>
        /// Creates an async enumerable over the DistributedQueue.
        /// </summary>
        /// <returns>IAsyncEnumerable that represents all the values.</returns>
        Task<IAsyncEnumerable<T>> CreateEnumerableAsync(Transaction tx);

        // TODO: Asynchronous T[] ToArray().
        // TODO: Asynchronous void CopyTo(T[] array, int index).
    }
}