// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Replicator;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Helpers around replicator transaction routines.
    /// </summary>
    internal class TransactionHelpers
    {
        /// <summary>
        /// Creates a replicator transaction and retries transient errors.
        /// </summary>
        /// <returns></returns>
        public static async Task<Transaction> SafeCreateReplicatorTransactionAsync(TransactionalReplicator replicator)
        {
            Transaction replicatorTransaction = null;

            // Create replicator transaction.
            while (true)
            {
                bool doneCreateTx = false;
                try
                {
                    replicatorTransaction = replicator.CreateTransaction();
                    doneCreateTx = true;
                }
                catch (FabricTransientException)
                {
                    // Retry.
                }
                catch (Exception e)
                {
                    // Be done.
                    throw e;
                }

                if (!doneCreateTx)
                {
                    // Sleep for a while.
                    await Task.Delay(1000);
                }
                else
                {
                    break;
                }
            }

            return replicatorTransaction;
        }

        /// <summary>
        /// Creates a replicator transaction and retries transient errors.
        /// </summary>
        /// <returns></returns>
        public static async Task SafeTerminateReplicatorTransactionAsync(Transaction replicatorTransaction, bool commit)
        {
            // Commit replicator transaction.
            while (true)
            {
                bool doneCompleteTx = false;
                try
                {
                    if (commit)
                    {
                        await replicatorTransaction.CommitAsync();
                    }
                    else
                    {
                        await replicatorTransaction.AbortAsync();
                    }

                    doneCompleteTx = true;
                }
                catch (FabricTransientException)
                {
                    // Retry.
                }
                catch (Exception e)
                {
                    // Be done.
                    throw e;
                }

                if (!doneCompleteTx)
                {
                    // Sleep for a while.
                    await Task.Delay(1000);
                }
                else
                {
                    break;
                }
            }
        }
    }
}