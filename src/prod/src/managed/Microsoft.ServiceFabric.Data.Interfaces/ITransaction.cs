// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;

    /// <summary>
    /// A sequence of operations performed as a single logical unit of work.
    /// </summary>
    /// <remarks>
    /// A transaction must exhibit the following ACID properties. (see: https://technet.microsoft.com/en-us/library/ms190612)
    /// <list type="bullet">
    ///     <item>
    ///         <description>
    ///             Atomicity - A transaction must be an atomic unit of work; either all of its data modifications are
    ///             performed, or none of them is performed.
    ///         </description>
    ///     </item>
    ///     <item>
    ///         <description>
    ///             Consistency - When completed, a transaction must leave all data in a consistent state. All
    ///             internal data structures must be correct at the end of the transaction.
    ///         </description>
    ///     </item>
    ///     <item>
    ///         <description>
    ///             Isolation - Modifications made by concurrent transactions must be isolated from the
    ///             modifications made by any other concurrent transactions. 
    ///             The isolation level used for an operation within an <see cref="ITransaction"/> is determined by the
    ///             <see cref="IReliableState"/> performing the operation.
    ///         </description>
    ///     </item>
    ///     <item>
    ///         <description>
    ///             Durability - After a transaction has completed, its effects are permanently in place in the system.
    ///             The modifications persist even in the event of a system failure.
    ///         </description>
    ///     </item>
    /// </list>
    /// 
    /// <para>
    /// Any instance member of this type is not guaranteed to be thread safe. 
    /// This makes transactions the unit of concurrency: Users can have multiple transactions in-flight at any given point of time, but for a given transaction each API must be called one at a time.
    /// All <see cref="T:IReliableCollection{T}"/> APIs that accept a transaction and return a Task must be awaited one at a time.
    /// </para>
    /// 
    /// 
    /// <para>
    /// Following is an example of a correct usage.
    /// <code>
    /// <![CDATA[
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
    ///
    ///         // Delay and retry.
    ///         await Task.Delay(TimeSpan.FromMilliseconds(100), cancellationToken);
    ///     }
    /// ]]>
    /// </code>
    /// </para>
    /// 
    /// <para>
    /// The following is an example of incorrect usage that has undefined behavior.
    /// <code>
    /// <![CDATA[
    /// using (var txn = this.StateManager.CreateTransaction())
    /// {
    ///     List<Task> taskList = new List<Task>();
    ///     taskList.Add(concurrentQueue.DequeueAsync(txn, cancellationToken));
    ///     taskList.Add(concurrentQueue.DequeueAsync(txn, cancellationToken));
    /// 
    ///     await Task.WhenAll(taskList);
    ///     await txn.CommitAsync();
    /// }
    /// ]]>
    /// </code>
    /// </para>
    /// 
    /// </remarks>
    public interface ITransaction : IDisposable
    {
        /// <summary>
        /// Sequence number for the commit operation.
        /// </summary>
        /// <value>
        /// The sequence number at which the the transaction was committed.
        /// </value>
        long CommitSequenceNumber { get; }

        /// <summary>
        /// Commit the transaction.
        /// </summary>
        /// <remarks>
        /// You cannot abort a transaction once it has been committed, because all modifications
        /// have been persisted and replicated.
        /// </remarks>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="FabricNotPrimaryException">
        /// The transaction includes updates to <see cref="IReliableState"/> and the <see cref="System.Fabric.ReplicaRole"/> is not Primary.
        /// Only Primary replicas are given write status.
        /// </exception>
        /// <returns>
        /// A task that represents the asynchronous commit operation. 
        /// </returns>
        Task CommitAsync();

        /// <summary>
        /// Abort (rolls back) the transaction.
        /// </summary>
        /// <remarks>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// <exception cref="FabricNotPrimaryException">
        /// The transaction includes updates to <see cref="IReliableState"/> and the <see cref="System.Fabric.ReplicaRole"/> is not Primary.
        /// Only Primary replicas are given write status.
        /// </exception>
        /// </remarks>
        void Abort();

        /// <summary>
        /// Gets a value identifying the transaction.
        /// </summary>
        /// <returns>The transaction id.</returns>
        long TransactionId { get; }

        /// <summary>
        /// Gets the visibility sequence number.
        /// </summary>
        /// <remarks>
        /// <exception cref="TransactionFaultedException">The transaction has been internally faulted by the system. Retry the operation on a new transaction</exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown when a method call is invalid for the object's current state.
        /// Example, transaction used is already terminated: committed or aborted by the user.
        /// If this exception is thrown, it is highly likely that there is a bug in the service code of the use of transactions.
        /// </exception>
        /// </remarks>
        /// <returns>The visibility sequence number.</returns>
        Task<long> GetVisibilitySequenceNumberAsync();
    }
}