// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Threading;

namespace System.Fabric.AzureActiveDirectory.Server
{
    internal sealed class RwLock
    {
        readonly ReaderWriterLockSlim rwLock;

        public RwLock()
        {
            this.rwLock = new ReaderWriterLockSlim();
        }

        public ScopedWriteLock AcquireWriteLock()
        {
            return new ScopedWriteLock(this.rwLock);
        }

        public ScopedReadLock AcquireReadLock()
        {
            return new ScopedReadLock(this.rwLock);
        }

        public abstract class ScopedLockBase : IDisposable
        {
            private bool disposed = false;

            protected ReaderWriterLockSlim Lock { get; private set; }

            protected ScopedLockBase(ReaderWriterLockSlim rwLock)
            {
                this.Lock = rwLock;
            }

            public void Dispose()
            {
                if (this.disposed) { return; }

                this.disposed = true;

                this.OnDispose();

                GC.SuppressFinalize(this);
            }

            public abstract void OnDispose();
        }

        public sealed class ScopedWriteLock : ScopedLockBase
        {
            internal ScopedWriteLock(ReaderWriterLockSlim rwLock) : base(rwLock)
            {
                this.Lock.EnterWriteLock();
            }

            public override void OnDispose()
            {
                this.Lock.ExitWriteLock();
            }
        }

        public sealed class ScopedReadLock : ScopedLockBase
        {
            internal ScopedReadLock(ReaderWriterLockSlim rwLock) : base(rwLock)
            {
                this.Lock.EnterReadLock();
            }

            public override void OnDispose()
            {
                this.Lock.ExitReadLock();
            }
        }
    }
}