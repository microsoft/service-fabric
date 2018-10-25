// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue;
    using Microsoft.ServiceFabric.Replicator;

    internal class PrepareForRemoveStateLockContext<T> : LockContext
    {
        private readonly ReaderWriterAsyncLock primeLock;

        public PrepareForRemoveStateLockContext(ReaderWriterAsyncLock primeLock)
        {
            this.primeLock = primeLock;
        }

        public override void Unlock(LockContextMode mode)
        {
            // todo: release this sooner (if possible) once #6433142 is fixed
            this.primeLock.ReleaseWriteLock();
        }

        // todo sangarg : Use IEquatable instead.
        protected override bool IsEqual(object stateOrKey)
        {
            // Should never be called.
            TestableAssertHelper.FailInvalidOperation("PrepareForRemoveStateLockContext", "IsEqual", "should never be called. comparand: {0}, this: {1}", stateOrKey, this);
            return false;
        }
    }
}