// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;

    internal static class ReplicationHelper
    {
        /// <summary>
        /// Utility routine that wraps the replication calls to avoid dealing with transient exceptions.
        /// </summary>
        public static async Task ReplicateOperationAsync(
            Transaction tx,
            long stateProviderId,
            string traceType,
            object operationContext,
            OperationData metaData,
            OperationData redo,
            OperationData undo,
            int retryStartDelayInMilliseconds = TimeoutHelper.DefaultRetryStartDelayInMilliseconds,
            int exponentialBackoffFactor = TimeoutHelper.DefaultExponentialBackoffFactor,
            int maxSingleDelayInMilliseconds = TimeoutHelper.DefaultMaxSingleDelayInMilliseconds,
            CancellationToken cancellationToken = default(CancellationToken),
            TimeSpan? timeout = null)
        {
            TimeSpan maxRetry = timeout ?? TimeoutHelper.DefaultReplicationTimeout;
            RetryHelper retryHelper = new RetryHelper(
                retryStartDelayInMilliseconds,
                exponentialBackoffFactor,
                maxSingleDelayInMilliseconds,
                (int)maxRetry.TotalMilliseconds);

            int retryCount = 0;
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                Exception ex;
                try
                {
                    // Issue replication operation.
                    tx.AddOperation(metaData, undo, redo, operationContext, stateProviderId);
                    return;
                }
                catch (Exception e)
                {
                    // Unwrap if necessary
                    ex = e is AggregateException ? e.InnerException : e;

                    if (!IsRetryableReplicationException(ex))
                    {
                        // Trace the failure and re-throw the exception.
                        // There might be serialization exceptions.
                        // All non retryable exception should be thrown to the user.
                        AppTrace.TraceSource.WriteExceptionAsWarning(
                            string.Format("ReplicationHelper.ReplicateOperationAsync@{0}", traceType),
                            ex,
                            "non-retriable replication exception {0}",
                            ex.GetType());

                        throw ex;
                    }
                }

                Trace.Assert(ex != null, "exception cannot be null");

                // Trace if a few retries have failed.
                retryCount++;
                if (4 == retryCount)
                {
                    AppTrace.TraceSource.WriteWarning(
                        string.Format("ReplicationHelper.ReplicateOperationAsync@{0}", traceType),
                        "retry transaction={0}, ex={1}",
                        tx.TransactionId,
                        ex);
                    retryCount = 0;
                }

                // Backoff and retry
                await retryHelper.ExponentialBackoffAsync(ex, cancellationToken).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Checks if a replication exception is retriable.
        /// </summary>
        /// <param name="e">Exception to be tested.</param>
        /// <returns></returns>
        private static bool IsRetryableReplicationException(Exception e)
        {
            // TODO: once 5052175 is fixed and FabricNotReadable is split into transient/non-transient cases, don't retry on it
            return e is FabricTransientException || e is FabricNotReadableException;
        }
    }
}