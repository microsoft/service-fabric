// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;

    using Microsoft.ServiceFabric.Replicator;

    internal class OperationLockContext<T> : LockContext
    {
        private readonly ReliableConcurrentQueue<T> queue;

        public readonly IListElement<T> ListElement;

        public readonly UnlockSource UnlockSource;

        public readonly ITransaction Transaction;

        public DateTime OperationStartTime;

        public OperationLockContext(ReliableConcurrentQueue<T> queue, IListElement<T> listElement, UnlockSource unlockSource, ITransaction tx, DateTime startTime)
        {
            this.queue = queue;
            this.ListElement = listElement;
            this.UnlockSource = unlockSource;
            this.Transaction = tx;
            this.OperationStartTime = startTime;
        }

        public override void Unlock(LockContextMode mode)
        {
            this.queue.Unlock(this);
        }

        protected override bool IsEqual(object stateOrKey)
        {
            // Should never be called.
            TestableAssertHelper.FailInvalidOperation("OperationLockContext", "IsEqual", "should never be called");
            return false;
        }
    }
}