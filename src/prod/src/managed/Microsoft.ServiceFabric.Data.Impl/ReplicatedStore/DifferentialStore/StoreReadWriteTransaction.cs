// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;
// Alias.
    using ReplicatorTransactionBase = Microsoft.ServiceFabric.Replicator.TransactionBase;

    /// <summary>
    /// Encapsulates read/write functionality for a store transaction.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    internal class StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer> : StoreTransaction, IStoreReadWriteTransaction
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
    {
        /// <summary>
        /// Set for any transaction that does not commit.
        /// </summary>
        public const long UnknownCommitSequenceNumber = long.MinValue;

        /// <summary>
        /// Write set. Contains all key modifications done by this transaction.
        /// </summary>
        private TWriteSetStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> writeSet;

        /// <summary>
        /// Specifies if the store transaction has terminated.
        /// </summary>
        private bool hasCompleted;

        /// <summary>
        /// Number of operations replicated by the primary.
        /// </summary>
        private long primaryOperationCount;

        /// <summary>
        /// Store that contains this read transaction.
        /// </summary>
        private IDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>> container;

        /// <summary>
        /// Keeps the current isolation level for this store transaction.
        /// </summary>
        private StoreTransactionReadIsolationLevel isolationlevel = StoreTransactionReadIsolationLevel.Snapshot;

        /// <summary>
        /// Keeps the current locking hints for this store transaction.
        /// </summary>
        private LockingHints lockingHints = LockingHints.None;

        private readonly string traceType;

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="id"></param>
        /// <param name="replicatorTransaction"></param>
        /// <param name="owner"></param>
        /// <param name="tracer"></param>
        /// <param name="container"></param>
        /// <param name="disposeDuringUnlock"></param>
        /// <param name="enableStrict2PL">Indicates whether strict 2PL is enabled.</param>
        public StoreReadWriteTransaction(
            long id,
            ReplicatorTransactionBase replicatorTransaction,
            IStoreTransactionProvider owner,
            string tracer,
            IDictionary<long, StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>> container,
            bool disposeDuringUnlock,
            bool enableStrict2PL)
            : base(id, false, owner, replicatorTransaction, tracer, enableStrict2PL)
        {
            this.container = container;
            this.traceType = tracer;
        }

        /// <summary>
        /// Store transaction state.
        /// </summary>
        private enum TerminationStatus
        {
            None,
            Committed,
            Completed
        }

        #region IStoreReadTransaction Members

        /// <summary>
        /// Gets/Sets transaction isolation level.
        /// </summary>
        public StoreTransactionReadIsolationLevel Isolation
        {
            get { return this.isolationlevel; }

            set { this.isolationlevel = value; }
        }

        /// <summary>
        /// Gets/Sets locking hints.
        /// </summary>
        public LockingHints LockingHints
        {
            get { return this.lockingHints; }

            set { this.lockingHints = value; }
        }

        #endregion

        /// <summary>
        /// Returns the write set for this transaction.
        /// </summary>
        public TWriteSetStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer> Component
        {
            get
            {
                if (this.writeSet != null)
                {
                    return this.writeSet;
                }
                else
                {
                    this.writeSet = new TWriteSetStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(this.traceType);
                    return this.writeSet;
                }
            }
        }

        /// <summary>
        /// Returns true if the write set is empty.
        /// </summary>
        public bool IsWriteSetEmpty
        {
            get { return this.writeSet == null; }
        }

        /// <summary>
        /// Specifies whether a store transaction was completed.
        /// </summary>
        public override bool IsCompleted
        {
            get { return this.hasCompleted; }

            set { this.hasCompleted = value; }
        }

        /// <summary>
        /// Used on the primary.
        /// </summary>
        public long PrimaryOperationCount
        {
            get { return this.primaryOperationCount; }
        }

        /// <summary>
        /// Used on the primary.
        /// </summary>
        public void IncrementPrimaryOperationCount()
        {
            Interlocked.Increment(ref this.primaryOperationCount);
        }

        /// <summary>
        /// Used on the primary.
        /// </summary>
        public void DecrementPrimaryOperationCount()
        {
            var value = Interlocked.Decrement(ref this.primaryOperationCount);
            Diagnostics.Assert(0 <= value, this.traceType, "unexpected primary operation count for txn={0}", this.Id);
        }

        /// <summary>
        /// Custom dispose implementation.
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Release all locks.
                this.ClearLocks();

                // Remove this store transaction from its container.
                if (null != this.container)
                {
                    this.container.Remove(this.Id);
                }
            }
        }
    }
}