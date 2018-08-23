// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;

    internal class PrimeLockThrows : ReaderWriterAsyncLock
    {
        public async Task AcquireReadAsync(
            TimeSpan? timeout,
            CancellationToken cancellationToken)
        {
            int timeoutMs = TimeoutHelper.GetTimeSpanInMilliseconds(timeout ?? TimeoutHelper.DefaultLockTimeout);
            var lockAcquired = await this.AcquireReadLockAsync(timeoutMs, cancellationToken).ConfigureAwait(false);
            if (!lockAcquired)
            {
                throw new TimeoutException(SR.PrimeLockThrows_LockTimeout);
            }
        }

        public async Task<bool> TryAcquireReadAsync(TimeSpan? timeout, CancellationToken cancellationToken)
        {
            int timeoutMs = TimeoutHelper.GetTimeSpanInMilliseconds(timeout ?? TimeoutHelper.DefaultLockTimeout);
            return await this.AcquireReadLockAsync(timeoutMs, cancellationToken).ConfigureAwait(false);
        }

        public void ReleaseRead()
        {
            this.ReleaseReadLock();
        }

        public async Task AcquireWriteAsync(
            TimeSpan? timeout,
            CancellationToken cancellationToken)
        {
            int timeoutMs = TimeoutHelper.GetTimeSpanInMilliseconds(timeout ?? TimeoutHelper.DefaultLockTimeout);
            var lockAcquired = await this.AcquireWriteLockAsync(timeoutMs, cancellationToken).ConfigureAwait(false);
            if (!lockAcquired)
            {
                throw new TimeoutException(SR.PrimeLockThrows_LockTimeout);
            }
        }

        public void ReleaseWrite()
        {
            this.ReleaseWriteLock();
        }
    }
}