// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    //** Async Auto Reset Event Implementation
    internal class AutoResetEventAsync
    {
        private object _ThisLock = new object();
        private volatile bool _IsSet = true;
        private volatile bool _IsAborted = false;
        private List<Task> _Waiters = new List<Task>();

        public void Abort()
        {
            lock (this._ThisLock)
            {
                this._IsAborted = true;
                foreach (var t in this._Waiters)
                {
                    // Cause each waiter to run and throw AbandonedMutexException
                    t.Start();
                }

                this._Waiters.Clear();
            }
        }

        public Task WaitUntilSetAsync(CancellationToken cancellationToken)
        {
            Action ContinueAfterWait = () =>
            {
                if (this._IsAborted)
                    throw new AbandonedMutexException();
            };

            var waitTask = new Task(ContinueAfterWait, cancellationToken);

            lock (this._ThisLock)
            {
                if (this._IsAborted)
                    throw new AbandonedMutexException();

                if (this._IsSet)
                {
                    // No waiters and set - do auto reset and continue
                    System.Diagnostics.Debug.Assert(this._Waiters.Count == 0);
                    this._IsSet = false;

                    waitTask.RunSynchronously();
                }
                else
                {
                    // Suspend the current continuation until signaled
                    this._Waiters.Add(waitTask);
                }
            }

            return waitTask;
        }

        public void Set()
        {
            lock (this._ThisLock)
            {
                System.Diagnostics.Debug.Assert(this._IsSet == false);
                if (this._Waiters.Count > 0)
                {
                    // There are pending waiters - let the first one go
                    this._Waiters[0].Start();
                    this._Waiters.RemoveAt(0);
                    return;
                }

                this._IsSet = true;
            }
        }
    }
}