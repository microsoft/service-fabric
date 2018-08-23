// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// <para>Represents a transaction. <see cref="System.Fabric.KeyValueStoreReplica"/></para>
    /// </summary>
    public class Transaction : TransactionBase
    {
        private NativeRuntime.IFabricTransaction nativeTransaction;

        internal Transaction(NativeRuntime.IFabricTransaction nativeTransaction)
            : base((NativeRuntime.IFabricTransactionBase)nativeTransaction)
        {
            this.nativeTransaction = nativeTransaction;
        }

        /// <summary>
        /// <para>Rolls back the set of operations that are associated with this transaction.</para>
        /// </summary>
        public void Rollback()
        {
            this.ThrowIfDisposed();
            Utility.WrapNativeSyncInvoke(
                () => this.RollbackHelper(),
                "Transaction.Rollback");
        }

        /// <summary>
        /// <para>Commits the set of operations associated with the transaction. Replication and local disk writes are performed asynchronously</para>
        /// </summary>
        /// <returns>
        /// <para>
        /// A Task whose result is the Logical Sequence Number of the transaction. 
        /// The sequence number is associated with all keys written in this transaction 
        /// and can be passed into APIs accepting a check sequence number parameter to implement optimistic concurrency:
        /// <list type="bullet">
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Update(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Remove(TransactionBase, string, long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryUpdate(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryRemove(TransactionBase, string, long)" /></description></item>
        /// </list>
        /// </para>
        /// </returns>
        public Task<long> CommitAsync()
        {
            return this.InternalCommitAsync(TimeSpan.MaxValue, CancellationToken.None);
        }

        /// <summary>
        /// <para>Commits the set of operations that are associated with the transaction with an optional timeout. Replication and local disk 
        /// writes are performed asynchronously. Note that the underlying replication operation may not have been cancelled even in the event of a timeout.</para>
        /// </summary>
        /// <param name="timeout">
        /// <para>The amount of time to wait for replication and local disk writes to complete before <see cref="System.TimeoutException" /> is thrown.</para>
        /// </param>
        /// <returns>
        /// <para>
        /// A Task whose result is the Logical Sequence Number of the transaction. 
        /// The sequence number is associated with all keys written in this transaction 
        /// and can be passed into APIs accepting a check sequence number parameter to implement optimistic concurrency:
        ///
        /// <list type="bullet">
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Update(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Remove(TransactionBase, string, long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryUpdate(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryRemove(TransactionBase, string, long)" /></description></item>
        /// </list>
        /// </para>
        /// </returns>
        public Task<long> CommitAsync(TimeSpan timeout)
        {
            return this.InternalCommitAsync(timeout, CancellationToken.None);
        }

        /// <summary>
        /// <para>Commits the set of operations that are associated with the transaction with an optional timeout. Replication and local disk 
        /// writes are performed asynchronously. Note that the underlying replication operation may not have been cancelled even in the event of a timeout.</para>
        /// </summary>
        /// <param name="timeout">
        /// <para>The amount of time to wait for replication and local disk writes to complete before <see cref="System.TimeoutException" /> is thrown.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>Currently not used. Reserved for future use.</para>
        /// </param>
        /// <returns>
        /// <para>
        /// A Task whose result is the Logical Sequence Number of the transaction. 
        /// The sequence number is associated with all keys written in this transaction 
        /// and can be passed into APIs accepting a check sequence number parameter to implement optimistic concurrency:
        ///
        /// <list type="bullet">
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Update(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.Remove(TransactionBase, string, long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryUpdate(TransactionBase, string, byte[], long)" /></description></item>
        /// <item><description><see cref="System.Fabric.KeyValueStoreReplica.TryRemove(TransactionBase, string, long)" /></description></item>
        /// </list>
        /// </para>
        /// </returns>
        [Obsolete("Use CommitAsync() or CommitAsync(TimeSpan) overloads")]
        public Task<long> CommitAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.InternalCommitAsync(timeout, cancellationToken);
        }

        /// <summary>
        /// <para>The dispose event occurs when the transaction is disposed of through the <languageKeyword>Dispose</languageKeyword> method. </para>
        /// </summary>
        /// <remarks>
        /// <para>To override <see cref="System.Fabric.Transaction.OnDispose" />, be sure to call <languageKeyword>OnDispose</languageKeyword> on the base class.</para>
        /// </remarks>
        protected internal override void OnDispose()
        {
            base.OnDispose();

            if (this.nativeTransaction != null)
            {
                Marshal.FinalReleaseComObject(this.nativeTransaction);
                this.nativeTransaction = null;
            }
        }

        /// <summary>
        /// <para>Throws an exception if the <see cref="System.Fabric.Transaction" /> object is disposed.</para>
        /// </summary>
        protected void ThrowIfDisposed()
        {
            if (this.IsDisposed())
            {
                throw new ObjectDisposedException("FabricTransaction");
            }
        }

        private Task<long> InternalCommitAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowIfDisposed();

            var timeoutInMilliSeconds = Utility.ToMilliseconds(timeout, "timeout");

            return Utility.WrapNativeAsyncInvoke<long>(
                (callback) => this.BeginCommitHelper(timeoutInMilliSeconds, callback),
                this.EndCommitHelper,
                cancellationToken,
                "Transaction.Commit");
        }

        private void RollbackHelper()
        {
            this.nativeTransaction.Rollback();
        }

        private NativeCommon.IFabricAsyncOperationContext BeginCommitHelper(uint timeoutMilliSeconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeTransaction.BeginCommit(timeoutMilliSeconds, callback);
        }

        private long EndCommitHelper(NativeCommon.IFabricAsyncOperationContext context)
        {
            long commitSequenceNumber;
            this.nativeTransaction.EndCommit(context, out commitSequenceNumber);
            return commitSequenceNumber;
        }
    }
}