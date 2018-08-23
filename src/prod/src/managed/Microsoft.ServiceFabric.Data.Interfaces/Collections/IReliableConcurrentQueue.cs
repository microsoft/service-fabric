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

    // todo: when queue capacity constraint is configurable, update the remark to link to it directly if possible
    // todo: when retriable and non-retriable FabricNotReadableException are split (#5052175), update the exception documentation
    /// <summary>
    /// <para>
    /// Represents a reliable collection of persisted, replicated values with best-effort first-in first-out ordering.
    /// </para>
    /// </summary>
    /// 
    /// <typeparam name="T">
    /// The type of the values contained in the reliable queue slim.
    /// </typeparam>
    /// 
    /// <remarks>
    /// <para>
    /// Intended as an alternative to <see cref="IReliableQueue{T}"/> for workloads where strict ordering is not required, as by relaxing
    /// the ordering constraint, concurrency can be greatly improved.  <see cref="IReliableQueue{T}"/> restricts concurrent consumers
    /// and producers to a maximum of one each, while <see cref="IReliableConcurrentQueue{T}"/> imposes no such restriction.
    /// </para>
    /// <para>
    /// <see cref="IReliableConcurrentQueue{T}"/> does not offer the same transaction isolation semantics as the other reliable data structures.  See the 
    /// individual operations and properties (<see cref="EnqueueAsync"/>, <see cref="TryDequeueAsync"/> and <see cref="Count"/>) for details on what isolation, 
    /// if any, they provide.
    /// </para>
    /// <para>
    /// It is expected that values will be relatively short-lived in the queue; in other words, that the egress (<see cref="TryDequeueAsync"/>) rate is 
    /// equal to or greater than the ingress (<see cref="EnqueueAsync"/>) rate.  Violating this expectation 
    /// may worsen system performance.  A planned queue capacity constraint which will throttle incoming Enqueues once the capacity is reached
    /// will help in maintaining this property.
    /// property.
    /// </para>
    /// <para>
    /// As the ordering of elements is not strictly guaranteed, assumptions about the ordering of any two values in the queue MUST NOT
    /// be made.  The best-effort first-in first-out ordering is provided for fairness; the time that an value spends in the queue should
    /// be related to the failure rate (failures may alter the queue's ordering) and the dequeue rate, but not the enqueue rate.
    /// </para>
    /// <para>
    /// <see cref="IReliableConcurrentQueue{T}"/> does not offer a Peek operation, however by combining <see cref="TryDequeueAsync"/> and <see cref="ITransaction.Abort"/>
    /// the same semantic can be achieved.  See <see cref="TryDequeueAsync"/> for additional details and an example.
    /// </para>
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
    public interface IReliableConcurrentQueue<T> : IReliableState
    {
        // todo: when queue capacity constraint is configurable, add QueueFullException to EnqueueAsync exceptions and document capacity behavior (including example)

        /// <summary>
        /// <para>
        /// Stage the enqueue of a value into the queue.
        /// </para>
        /// </summary>
        /// 
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="value">The value to add to the end of the queue. The value can be null for reference types.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is <see cref="CancellationToken.None"/>.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete before throwing a TimeoutException. The default is null.  If null is passed, a default timeout will be used.</param>
        /// 
        /// <returns>Task that represents the asynchronous enqueue operation.</returns>
        /// 
        /// <remarks>
        /// A <see cref="TryDequeueAsync"/> operation cannot return any value for which its <see cref="EnqueueAsync"/> has not yet been committed.
        /// This includes the transaction in which the value was enqueued; as a consequence, <see cref="IReliableConcurrentQueue{T}"/> does not support Read-Your-Writes.
        /// </remarks>
        /// 
        /// <exception cref="FabricNotPrimaryException">The replica is no longer in <cref name="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricNotReadableException">The replica is currently not readable.</exception>
        /// <exception cref="FabricObjectClosedException">The <see cref="IReliableConcurrentQueue{T}"/> was closed by the runtime.</exception>
        /// <exception cref="FabricTransientException">The replica saw a transient failure. Retry the operation on a new transaction</exception>
        /// <exception cref="FabricException">The replica saw a non retriable failure other than the types defined above. Cleanup and rethrow the exception</exception>
        /// <exception cref="TimeoutException">
        /// The operation was unable to be completed within the given timeout.  The transaction should be aborted and
        /// a new transaction should be created to retry.
        /// </exception>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled via <paramref name="cancellationToken"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// 
        /// <example>
        /// This example shows how to use <see cref="EnqueueAsync"/> to enqueue a value with retry.
        /// <code>
        /// <![CDATA[
        /// protected override async Task RunAsync(CancellationToken cancellationToken)
        /// {
        ///     var concurrentQueue = await this.StateManager.GetOrAddAsync<IReliableConcurrentQueue<long>>(new Uri("fabric:/concurrentQueue"));
        ///
        ///     while (true)
        ///     {
        ///         cancellationToken.ThrowIfCancellationRequested();
        /// 
        ///         try
        ///         {
        ///             using (var tx = this.StateManager.CreateTransaction())
        ///             {
        ///                 await concurrentQueue.EnqueueAsync(tx, 12L, cancellationToken);
        ///                 await tx.CommitAsync();
        ///
        ///                 return;
        ///             }
        ///         }
        ///         catch (TransactionFaultedException e)
        ///         {
        ///             // This indicates that the transaction was internally faulted by the system. One possible cause for this is that the transaction was long running
        ///             // and blocked a checkpoint. Increasing the "ReliableStateManagerReplicatorSettings.CheckpointThresholdInMB" will help reduce the chances of running into this exception
        ///             Console.WriteLine("Transaction was internally faulted, retrying the transaction: " + e);
        ///         }
        ///         catch (FabricNotPrimaryException e)
        ///         {
        ///             // Gracefully exit RunAsync as the new primary should have RunAsync invoked on it and continue work.
        ///             // If instead enqueue was being executed as part of a client request, the client would be signaled to re-resolve.
        ///             Console.WriteLine("Replica is not primary, exiting RunAsync: " + e);
        ///             return;
        ///         }
        ///         catch (FabricNotReadableException e)
        ///         {
        ///             // Retry until the queue is readable or a different exception is thrown.
        ///             Console.WriteLine("Queue is not readable, retrying the transaction: " + e);
        ///         }
        ///         catch (FabricObjectClosedException e)
        ///         {
        ///             // Gracefully exit RunAsync as this is happening due to replica close.
        ///             // If instead enqueue was being executed as part of a client request, the client would be signaled to re-resolve.
        ///             Console.WriteLine("Replica is closing, exiting RunAsync: " + e);
        ///             return;
        ///         }
        ///         catch (TimeoutException e)
        ///         {
        ///             Console.WriteLine("Encountered TimeoutException during EnqueueAsync, retrying the transaction: " + e);
        ///         }
        ///         catch (FabricTransientException e)
        ///         {
        ///             // Retry until the queue is writable or a different exception is thrown.
        ///             Console.WriteLine("Queue is currently not writable, retrying the transaction: " + e);
        ///         }
        ///
        ///         // Delay and retry.
        ///         await Task.Delay(TimeSpan.FromMilliseconds(100), cancellationToken);
        ///     }
        /// }
        /// ]]>
        /// </code>
        /// </example>
        Task EnqueueAsync(ITransaction tx, T value, CancellationToken cancellationToken = default(CancellationToken), TimeSpan? timeout = null);

        /// <summary>
        /// <para>
        /// Tentatively dequeue a value from the queue. If the queue is empty, the dequeue operation will wait for an item to become available.
        /// </para>
        /// </summary>
        /// 
        /// <param name="tx">Transaction to associate this operation with.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests. The default is <see cref="CancellationToken.None"/>.</param>
        /// <param name="timeout">The amount of time to wait for the operation to complete. The default is null.  If null is passed, a default timeout will be used.</param>
        /// 
        /// <returns>
        /// A task that represents the asynchronous dequeue operation. The task's result is a ConditionalValue of type T.
        /// If a value was dequeued within the given time, return a ConditionalValue with HasValue as false, else it returns a ConditionalValue with HasValue as true and the Value as the dequeued item of Type T
        /// </returns>
        /// 
        /// <remarks>
        /// <para>
        /// While <see cref="TryDequeueAsync"/> can only return values for which the corresponding <see cref="EnqueueAsync"/> was committed, <see cref="TryDequeueAsync"/> operations are not isolated
        /// from one another.  Once a transaction has dequeued a value, other transactions cannot dequeue it, but are not blocked from dequeuing other values.
        /// </para>
        /// <para>
        /// When a transaction or transactions including one or more <see cref="TryDequeueAsync"/> operations aborts, the dequeued values will be added back at
        /// the head of the queue in an arbitrary order.  This will ensure that these values will be dequeued again soon, improving the fairness of the
        /// data structure, but without enforcing strict ordering (which would require reducing the allowed concurrency, as in <see cref="IReliableQueue{T}"/>).
        /// </para>
        /// </remarks>
        /// 
        /// <exception cref="FabricNotPrimaryException">The replica is no longer in <cref name="ReplicaRole.Primary"/>.</exception>
        /// <exception cref="FabricNotReadableException">The replica is currently not readable.</exception>
        /// <exception cref="FabricObjectClosedException">The <see cref="IReliableConcurrentQueue{T}"/> was closed by the runtime.</exception>
        /// <exception cref="FabricTransientException">The replica saw a transient failure. Retry the operation on a new transaction</exception>
        /// <exception cref="FabricException">The replica saw a non retriable failure other than the types defined above. Cleanup and rethrow the exception</exception>
        /// <exception cref="TimeoutException">
        /// The operation was unable to be completed within the given timeout.  The transaction should be aborted and
        /// a new transaction should be created to retry.
        /// </exception>
        /// <exception cref="ArgumentNullException"><paramref name="tx"/> is null. Do not handle this exception.</exception>
        /// <exception cref="OperationCanceledException">The operation was canceled via <paramref name="cancellationToken"/>.</exception>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <example>
        /// This example shows how to dequeue and log infinitely with retry, until the cancellation token is canceled.  
        /// <code>
        /// <![CDATA[
        /// protected override async Task RunAsync(CancellationToken cancellationToken)
        /// {
        ///     var concurrentQueue = await this.StateManager.GetOrAddAsync<IReliableConcurrentQueue<long>>(new Uri("fabric:/concurrentQueue"));
        /// 
        ///     // Assumption: values are being enqueued by another source (e.g. the communication listener).
        ///     while (true)
        ///     {
        ///         cancellationToken.ThrowIfCancellationRequested();
        /// 
        ///         try
        ///         {
        ///             using (var tx = this.StateManager.CreateTransaction())
        ///             {
        ///                 var dequeueOutput = await concurrentQueue.TryDequeueAsync(tx, cancellationToken, TimeSpan.FromMilliseconds(100));
        ///                 await tx.CommitAsync();
        /// 
        ///                 if (dequeueOutput.HasValue)
        ///                 {
        ///                     Console.WriteLine("Dequeue # " + dequeueOutput);
        ///                 }
        ///                 else
        ///                 {
        ///                     Console.WriteLine("Could not dequeue in the given time");
        ///                 }
        ///             }
        ///         }
        ///         catch (TransactionFaultedException e)
        ///         {
        ///             // This indicates that the transaction was internally faulted by the system. One possible cause for this is that the transaction was long running
        ///             // and blocked a checkpoint. Increasing the "ReliableStateManagerReplicatorSettings.CheckpointThresholdInMB" will help reduce the chances of running into this exception
        ///             Console.WriteLine("Transaction was internally faulted, retrying the transaction: " + e);
        ///         }
        ///         catch (FabricNotPrimaryException e)
        ///         {
        ///             // Gracefully exit RunAsync as the new primary should have RunAsync invoked on it and continue work.
        ///             // If instead dequeue was being executed as part of a client request, the client would be signaled to re-resolve.
        ///             Console.WriteLine("Replica is not primary, exiting RunAsync: " + e);
        ///             return;
        ///         }
        ///         catch (FabricNotReadableException e)
        ///         {
        ///             // Retry until the queue is readable or a different exception is thrown.
        ///             Console.WriteLine("Queue is not readable, retrying the transaction: " + e);
        ///         }
        ///         catch (FabricObjectClosedException e)
        ///         {
        ///             // Gracefully exit RunAsync as this is happening due to replica close.
        ///             // If instead dequeue was being executed as part of a client request, the client would be signaled to re-resolve.
        ///             Console.WriteLine("Replica is closing, exiting RunAsync: " + e);
        ///             return;
        ///         }
        ///         catch (TimeoutException e)
        ///         {
        ///             Console.WriteLine("Encountered TimeoutException during DequeueAsync, retrying the transaction: " + e);
        ///         }
        ///         catch (FabricTransientException e)
        ///         {
        ///             // Retry until the queue is writable or a different exception is thrown.
        ///             Console.WriteLine("Queue is currently not writable, retrying the transaction: " + e);
        ///         }
        /// 
        ///         // Delay and retry.
        ///         await Task.Delay(TimeSpan.FromMilliseconds(100), cancellationToken);
        ///     }
        /// }
        /// ]]>
        /// </code>
        /// </example>
        Task<ConditionalValue<T>> TryDequeueAsync(ITransaction tx, CancellationToken cancellationToken = default(CancellationToken), TimeSpan? timeout = null);

        /// <summary>
        /// <para>
        /// Gets the number of values in the <see cref="IReliableConcurrentQueue{T}"/>.
        /// </para>
        /// </summary>
        ///  
        /// <remarks>
        /// <para>
        /// This count represents the number of values currently visible to <see cref="TryDequeueAsync"/>.  Uncommitted Enqueues will not
        /// increase the count, however uncommitted Dequeues will decrease the count.
        /// </para>
        /// <para>
        /// Note that this API does not take a transaction parameter.  Since the effects of <see cref="TryDequeueAsync"/> are not isolated
        /// from other transactions, the count also cannot be isolated from other transactions.  
        /// </para>
        /// </remarks>
        /// <value>The number of values in  the <see cref="IReliableConcurrentQueue{T}"/>.</value>
        /// <exception cref="FabricNotReadableException">The replica is currently not readable.</exception>
        /// <exception cref="FabricObjectClosedException">The <see cref="IReliableConcurrentQueue{T}"/> was closed by the runtime.</exception>
        ///  
        /// <example>
        /// This example shows how to monitor the queue's count infinitely, until the cancellation token is canceled.
        /// <code>
        /// <![CDATA[
        /// protected override async Task RunAsync(CancellationToken cancellationToken)
        /// {
        ///     var concurrentQueue = await this.StateManager.GetOrAddAsync<IReliableConcurrentQueue<long>>(new Uri("fabric:/concurrentQueue"));
        ///
        ///     // Assumption: values are being enqueued/dequeued in another place (e.g. the communication listener).
        ///     var observer = Task.Run(
        ///         async () =>
        ///             {
        ///                 while (true)
        ///                 {
        ///                     cancellationToken.ThrowIfCancellationRequested();
        ///
        ///                     try
        ///                     {
        ///                         Console.WriteLine("Count: " + concurrentQueue.Count);
        ///                     }
        ///                     catch (FabricNotReadableException e)
        ///                     {
        ///                         // Retry until the queue is readable or a different exception is thrown.
        ///                         Console.WriteLine("Queue is not readable, retrying the observation: " + e);
        ///                     }
        ///                     catch (FabricObjectClosedException e)
        ///                     {
        ///                         // Gracefully exit as this is happening due to replica close.
        ///                         Console.WriteLine("Replica is closing, stopping observer: " + e);
        ///                         return;
        ///                     }
        ///                     
        ///                     await Task.Delay(TimeSpan.FromMilliseconds(100), cancellationToken);
        ///                 }
        ///             },
        ///         cancellationToken);
        /// }
        /// ]]>
        /// </code>
        /// </example>
        long Count { get; }
    }
}