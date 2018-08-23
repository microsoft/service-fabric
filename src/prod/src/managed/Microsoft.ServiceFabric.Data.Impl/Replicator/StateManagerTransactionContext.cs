// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric.Common;

    internal enum OperationType
    {
        None,

        Add,

        Remove,

        Read
    }

    /// <summary>
    /// Transaction information for state manager.
    /// </summary>
    internal class StateManagerTransactionContext : LockContext
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="StateManagerTransactionContext"/> class.
        /// </summary>
        internal StateManagerTransactionContext(
            long transactionId,
            StateManagerLockContext stateManagerLockContext,
            OperationType operationType)
        {
            Utility.Assert(stateManagerLockContext != null, "stateManagerLockContext cannot be empty.");

            this.LockContext = stateManagerLockContext;
            this.TransactionId = transactionId;
            this.OperationType = operationType;
        }

        /// <summary>
        /// Gets the state manager lock context.
        /// </summary>
        internal StateManagerLockContext LockContext { get; private set; }

        /// <summary>
        /// Gets the operation type.
        /// </summary>
        internal OperationType OperationType { get; private set; }

        /// <summary>
        /// Gets the transaction id.
        /// </summary>
        internal long TransactionId { get; private set; }

        /// <summary>
        /// Releases the lock and cleans up transient state.
        /// </summary>
        public override void Unlock(LockContextMode mode)
        {
            AppTrace.TraceSource.WriteInfo(
                "StateManager.StateManagerTransactionContext.Unlock",
                "unlock called for tansaction {0}",
                this.TransactionId);

            this.LockContext.Unlock(this);
        }

        /// <summary>
        /// Determines if the given key is equal to the object's key.
        /// </summary>
        protected override bool IsEqual(object stateOrKey)
        {
            // Should never be called.
            Utility.Assert(false, "should never be called");
            return false;
        }
    }
}