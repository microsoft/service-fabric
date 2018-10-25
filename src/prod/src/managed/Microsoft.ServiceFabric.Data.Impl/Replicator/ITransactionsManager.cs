// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Notifications;

    /// <summary>
    /// Interface for replicator.
    /// </summary>
    internal interface ITransactionManager
    {
        /// <summary>
        /// EventHandler for publishing changes in transactions.
        /// For example, notifying that the transaction committed.
        /// </summary>
        event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged;

        /// <summary>
        /// Aborts transaction.
        /// </summary>
        Task<long> AbortTransactionAsync(Transaction transaction);

        /// <summary>
        /// Replicates transaction.
        /// </summary>
        void AddOperation(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext);

        /// <summary>
        /// Replicates atomic operation.
        /// </summary>
        Task<long> AddOperationAsync(
            AtomicOperation atomicOperation,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext);

        /// <summary>
        /// Replicates atomic operation.
        /// </summary>
        Task<long> AddOperationAsync(
            AtomicRedoOperation atomicRedoOperation,
            OperationData metaData,
            OperationData redo,
            object operationContext);

        /// <summary>
        /// Initiates processing of records.
        /// </summary>
        void BeginTransaction(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext);

        /// <summary>
        /// Used for single operation transactions
        /// </summary>
        /// <param name="transaction"></param>
        /// <param name="metaData"></param>
        /// <param name="undo"></param>
        /// <param name="redo"></param>
        /// <param name="operationContext"></param>
        /// <returns></returns>
        Task<long> BeginTransactionAsync(
            Transaction transaction,
            OperationData metaData,
            OperationData undo,
            OperationData redo,
            object operationContext);

        /// <summary>
        /// Commits transaction.
        /// </summary>
        Task<long> CommitTransactionAsync(Transaction transaction);
    }
}