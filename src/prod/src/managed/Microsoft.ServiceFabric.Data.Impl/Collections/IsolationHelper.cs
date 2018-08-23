// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.Fabric.Data.Common;
    using System.Fabric.Store;
    using Microsoft.ServiceFabric.Replicator;

    internal static class IsolationHelper
    {
        public enum OperationType : int
        {
            Invalid = 0,

            SingleEntity = 1,

            MultiEntity = 2,
        }

        /// <summary>
        /// Picks the isolation level.
        /// </summary>
        /// <param name="txn">Transaction object.</param>
        /// <param name="operationType">MultiEntity: Count, Enumeration.</param>
        public static StoreTransactionReadIsolationLevel GetIsolationLevel(Transaction txn, IsolationHelper.OperationType operationType)
        {
            Diagnostics.Assert(operationType != IsolationHelper.OperationType.Invalid, "Unexpected operationType");

            // All multi-entity read operations are SNAPSHOT.
            if (operationType == IsolationHelper.OperationType.MultiEntity)
            {
                return StoreTransactionReadIsolationLevel.Snapshot;
            }

            // Note that TStore does not take locks on secondary.
            // Hence the we must not request a isolation level that takes locks.
            // Furthermore, to increase the likelihood of straddling read transactions across ChangeRoles,
            // we must use IsPrimaryTransaction (role the transaction was created with) and not current role
            // todo: describe how Replicator and TStore do gatekeeping of operations/transactions when ChangeRole occurs
            return txn.IsPrimaryTransaction ? StoreTransactionReadIsolationLevel.ReadRepeatable : StoreTransactionReadIsolationLevel.Snapshot;
        }
    }
}