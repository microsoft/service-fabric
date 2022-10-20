// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    internal static class KvsExtensions
    {
        internal static async Task<Transaction> CreateTransactionWithRetryAsync(this KeyValueStoreReplica kvsStore, IExceptionHandlingPolicy exceptionPolicy, CancellationToken token)
        {
            // TODO:
            // There's currently a bug in KVS where a service gets identified as a primary but doesn't actually have KVS write access.
            // Retrying solves the issue until the bug gets solved.
            while (!token.IsCancellationRequested)
            {
                try
                {
                    return kvsStore.CreateTransaction();
                }
                catch (Exception e)
                {
                    exceptionPolicy.ReportError(e, false);

                    await Task.Delay(Constants.KvsCreateTransactionRetryTime, token);
                }
            }

            return null;
        }
    }
}