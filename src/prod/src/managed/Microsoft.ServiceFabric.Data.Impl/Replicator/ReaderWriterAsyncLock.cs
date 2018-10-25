// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Asynchronous reader writer lock.
    /// </summary>
    internal class ReaderWriterAsyncLock
    {
        /// <summary>
        /// Lock object used for mutual exclusion when readers and writers update the local state.
        /// </summary>
        private readonly object readerWriterLock = new object();

        /// <summary>
        /// Readers currently in the critical section.
        /// </summary>
        private int activeReaderCount;

        /// <summary>
        /// Indicates if there is an active writer executing the critical section or not.
        /// </summary>
        private TaskCompletionSource<bool> activeWriterTcs;

        /// <summary>
        /// Last TaskCompletionSource used the signal the waiting readers.
        /// </summary>
        private TaskCompletionSource<bool> lastSignalledReaderTcs;

        /// <summary>
        /// Indicates if the replica is closing.
        /// </summary>
        public bool IsClosed { get; private set; }

        /// <summary>
        /// Readers waiting to enter critical section.
        /// </summary>
        private int waitingReaderCount = 0;

        /// <summary>
        /// Readers wait on this tcs when there are active writers.
        /// </summary>
        private TaskCompletionSource<bool> waitingReadersTcs;

        /// <summary>
        /// Writers queue where writers wait for other writers/readers to finish.
        /// </summary>
        private List<TaskCompletionSource<bool>> waitingWritersList;

        /// <summary>
        /// Adds ability to induce failures/slow downs during hand off.
        /// </summary>
        private readonly Action signalOwnersTestAction;

        /// <summary>
        /// Initializes a new instance of the <see cref="ReaderWriterAsyncLock"/> class.
        /// </summary>
        public ReaderWriterAsyncLock()
        {
            this.activeReaderCount = 0;

            this.lastSignalledReaderTcs = null;
            this.activeWriterTcs = null;
            this.waitingReadersTcs = null;
            this.waitingWritersList = new List<TaskCompletionSource<bool>>();
            this.IsClosed = false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ReaderWriterAsyncLock"/> class.
        /// TEST ONLY CONSTRUCTOR - There is a reason why it is an internal constructor!
        /// </summary>
        /// <param name="signalOwnersTestAction">Used to inject failures and delays during handoff.</param>
        internal ReaderWriterAsyncLock(Action signalOwnersTestAction) : this()
        {
            Utility.Assert(signalOwnersTestAction != null, "Passed in null");
            this.signalOwnersTestAction = signalOwnersTestAction;
        }

        /// <summary>
        /// Gets the number of active readers
        /// </summary>
        /// <devnote> It is exposed for testing only.</devnote>
        public int ActiveReaderCount
        {
            get { return this.activeReaderCount; }
        }

        /// <summary>
        /// Gets the value that indicates if there is an active writer or not.
        /// </summary>
        /// <devnote> It is exposed for testing only.</devnote>
        public bool IsActiveWriter
        {
            get { return this.activeWriterTcs != null; }
        }

        /// <summary>
        /// Gets the number of waiting readers.
        /// </summary>
        /// <devnote> It is exposed for testing only.</devnote>
        public int WaitingReaderCount
        {
            get { return this.waitingReaderCount; }
        }

        /// <summary>
        /// Gets the number of waiting writers.
        /// </summary>
        /// <devnote> It is exposed for testing only.</devnote>
        public int WaitingWriterCount
        {
            get { return this.waitingWritersList.Count; }
        }

        /// <summary>
        /// Acquires read lock.
        /// </summary>
        /// <returns></returns>
        public async Task<bool> AcquireReadLockAsync(int timeoutInMilliSeconds, CancellationToken cancellationToken)
        {
            var waitForReadLock = false;
            var isLockAcquired = false;
            TaskCompletionSource<bool> tempWaitingReadersTcs = null;

            lock (this.readerWriterLock)
            {
                // Check if the replica is closing.
                if (this.IsClosed)
                {
                    throw new FabricObjectClosedException();
                }

                if (this.activeWriterTcs == null && this.waitingWritersList.Count == 0)
                {
                    this.activeReaderCount++;
                    isLockAcquired = true;
                }
                else
                {
                    this.waitingReaderCount++;

                    if (this.waitingReadersTcs == null)
                    {
                        this.waitingReadersTcs = new TaskCompletionSource<bool>();
                    }

                    // Make a copy because await is done outside the lock.
                    tempWaitingReadersTcs = this.waitingReadersTcs;

                    // Respect timeout only if the reader needs to wait.
                    waitForReadLock = true;
                }
            }

            if (!waitForReadLock)
            {
                return isLockAcquired;
            }
            else
            {
                isLockAcquired = await this.WaitForLock(tempWaitingReadersTcs, false, timeoutInMilliSeconds).ConfigureAwait(false);
            }

            return isLockAcquired;
        }

        /// <summary>
        /// Acquires write lock.
        /// </summary>
        /// <returns></returns>
        public async Task<bool> AcquireWriteLockAsync(int timeoutInMilliSeconds, CancellationToken cancellationToken)
        {
            TaskCompletionSource<bool> waitingWriterTcs = new TaskCompletionSource<bool>();
            var waitForWriteLock = false;
            var isLockAcquired = false;

            lock (this.readerWriterLock)
            {
                // check if the replica is closing.
                if (this.IsClosed)
                {
                    throw new FabricObjectClosedException();
                }

                if (this.activeWriterTcs == null && this.activeReaderCount == 0)
                {
                    this.activeWriterTcs = waitingWriterTcs;
                    isLockAcquired = true;
                }
                else
                {
                    // Queue a waiting writer task.
                    this.waitingWritersList.Add(waitingWriterTcs);
                    waitForWriteLock = true;
                }
            }

            if (!waitForWriteLock)
            {
                return isLockAcquired;
            }
            else
            {
                isLockAcquired = await this.WaitForLock(waitingWriterTcs, true, timeoutInMilliSeconds).ConfigureAwait(false);
            }

            return isLockAcquired;
        }

        /// <summary>
        /// Does clean up on close.
        /// </summary>
        public void Close()
        {
            var waiters = new List<TaskCompletionSource<bool>>();
            lock (this.readerWriterLock)
            {
                this.IsClosed = true;

                // Set exception for all waiting writers.
                waiters.AddRange(this.waitingWritersList);

                // Set exception for the waiting reader task.
                if (this.waitingReadersTcs != null)
                {
                    waiters.Add(this.waitingReadersTcs);
                }

                // Clear local state.
                this.waitingWritersList.Clear();
                this.waitingReadersTcs = null;
            }

            // Set exception for all waiters outside the lock.
            for (var index = 0; index < waiters.Count; index++)
            {
                var completedWaiter = waiters[index];
                this.ExecuteTaskWithPreferFairness(
                    () =>
                    {
                        completedWaiter.Task.IgnoreExceptionVoid();
                        completedWaiter.TrySetException(new FabricObjectClosedException());
                    });
            }

            // Clear the collection.
            waiters.Clear();
        }

        /// <summary>
        /// Releases read lock.
        /// </summary>
        public void ReleaseReadLock()
        {
            TaskCompletionSource<bool> tcsToBeSignalled = null;
            lock (this.readerWriterLock)
            {
                this.activeReaderCount--;
                Utility.Assert(
                    this.activeReaderCount >= 0,
                    "user is trying to realease a lock that has not been acquired");

                // If this is the last reader, then signal a waiting writer if one is present.
                if (this.activeReaderCount == 0 && this.waitingWritersList.Count > 0)
                {
                    Utility.Assert(this.activeWriterTcs == null, "Current active writer cannot be null");
                    tcsToBeSignalled = this.waitingWritersList[0];
                    Utility.Assert(tcsToBeSignalled != null, "Writer to be signaled cannot be null");

                    this.activeWriterTcs = tcsToBeSignalled;
                    this.waitingWritersList.Remove(tcsToBeSignalled);
                }
            }

            // Set result outside the lock.
            if (tcsToBeSignalled != null)
            {
                this.ExecuteTaskWithPreferFairness(
                    () =>
                    {
                        this.InvokeSignalOwnersActionIfNecessary();
                        tcsToBeSignalled.SetResult(true);
                    });
            }
        }

        /// <summary>
        /// Realeases write lock.
        /// </summary>
        public void ReleaseWriteLock()
        {
            Utility.Assert(this.IsActiveWriter, "Cannot release a lock that was not acquired.");

            TaskCompletionSource<bool> tcsToBeSignalled = null;
            lock (this.readerWriterLock)
            {
                // Check for waiting writers first so that writer gets preference.
                if (this.waitingWritersList.Count > 0)
                {
                    // Active writer is already true so do not have to set it, just signal the waiting writer.
                    tcsToBeSignalled = this.waitingWritersList[0];
                    this.activeWriterTcs = tcsToBeSignalled;
                    this.waitingWritersList.Remove(tcsToBeSignalled);
                }
                else
                {
                    // Active writer has finished and it is the reader's turn.
                    Utility.Assert(this.activeWriterTcs != null, "active writer tcs should not be null");
                    this.activeWriterTcs = null;
                    if (this.waitingReaderCount > 0)
                    {
                        // Allow waiting readers
                        tcsToBeSignalled = this.waitingReadersTcs;

                        this.lastSignalledReaderTcs = this.waitingReadersTcs;
                        this.activeReaderCount = this.waitingReaderCount;
                        this.waitingReaderCount = 0;

                        this.waitingReadersTcs = null;
                    }
                }
            }

            // Hand off the lock the new writer owner or read owners.
            if (tcsToBeSignalled != null)
            {
                this.ExecuteTaskWithPreferFairness(
                    () =>
                        {
                            this.InvokeSignalOwnersActionIfNecessary();
                            tcsToBeSignalled.SetResult(true);
                        });
            }
        }

        /// <summary>
        /// Waits for the given TCS to finish within the timeout specified.
        /// </summary>
        private async Task<bool> WaitForLock(
            TaskCompletionSource<bool> waitingTcs,
            bool writer,
            int timeoutInMilliSeconds)
        {
            // Respect timeout.
            var shouldTimeoutTaskBeCancelled = false;
            var isLockAcquired = false;
            Exception exception = null;

            using (var cancellationTokenSource = new CancellationTokenSource())
            {
                var timeoutTask = Task.Delay(timeoutInMilliSeconds, cancellationTokenSource.Token);

                await Task.WhenAny(timeoutTask, waitingTcs.Task).ConfigureAwait(false);

                lock (this.readerWriterLock)
                {
                    if (this.IsClosed)
                    {
                        throw new FabricObjectClosedException();
                    }

                    if (waitingTcs.Task.IsCompleted)
                    {
                        shouldTimeoutTaskBeCancelled = true;
                        if (waitingTcs.Task.Status == TaskStatus.RanToCompletion)
                        {
                            // Successful completion.
                            isLockAcquired = true;
                        }
                        else if (waitingTcs.Task.IsFaulted)
                        {
                            // Set lock acquired status to false.
                            isLockAcquired = false;
                            exception = waitingTcs.Task.Exception.InnerException ?? waitingTcs.Task.Exception;
                        }
                        else if (waitingTcs.Task.IsCanceled)
                        {
                            // Set lock acquired status to false.
                            isLockAcquired = false;
                            exception = new OperationCanceledException();
                        }
                    }
                    else
                    {
                        if (writer)
                        {
                            // Timeout raced with signaling the waiting tcs (setresult).
                            isLockAcquired = (waitingTcs == this.activeWriterTcs);
                        }
                        else
                        {
                            // Timeout raced with signaling the waiting tcs (setresult).
                            isLockAcquired = (waitingTcs == this.lastSignalledReaderTcs);
                            Utility.Assert(!isLockAcquired || this.activeReaderCount > 0, "If lock was acquired through race, active reader count must be greater than 0.");
                        }
                    }

                    if (false == isLockAcquired)
                    {
                        if (writer)
                        {
                            // Dequeue waiting writer.
                            var wasRemoved = this.waitingWritersList.Remove(waitingTcs);

                            // Assert will fire if there was a delay in signaling ths tcs on release writer or release reader.
                            Utility.Assert(wasRemoved, "failed to remove writer tcs");
                        }
                        else
                        {
                            // Clean up the state that was set now that lock-acquisition has failed.
                            this.waitingReaderCount--;

                            // Assert will fire if there was a delay in signaling ths tcs on release writer until bug 6720586 is fixed.
                            Utility.Assert(this.waitingReaderCount >= 0, "waiting reader count {0} cannot be lesser than zero", this.waitingReaderCount);
                        }
                    }
                }

                // This should be done outside the lock.
                if (shouldTimeoutTaskBeCancelled)
                {
                    // Cancel the tiemout task.
                    cancellationTokenSource.Cancel();

                    try
                    {
                        // Await the task and swallow the exception explicitly if the task has not yet completed since the task has been cancelled,
                        // else await would return right away.
                        await timeoutTask.ConfigureAwait(false);
                    }
                    catch (TaskCanceledException)
                    {
                        // Swallow TaskCanceledException.
                    }
                }

                if (null != exception)
                {
                    Utility.Assert(false == isLockAcquired, "lock acquisition status shoud be false");
                    throw exception;
                }
            }

            return isLockAcquired;
        }

        /// <summary>
        /// Executes the given action with prefer fairness option.
        /// </summary>
        /// <param name="action">action to be exceuted.</param>
        /// <devnote>This needs to move into utility and needs more overloads.</devnote>
        private void ExecuteTaskWithPreferFairness(Action action)
        {
            Task.Factory.StartNew(action, TaskCreationOptions.PreferFairness).IgnoreExceptionVoid();
        }

        private void InvokeSignalOwnersActionIfNecessary()
        {
            Action snapHandOffTestAction = this.signalOwnersTestAction;
            if (snapHandOffTestAction == null)
            {
                return;
            }

            // TODO: convert to structured tracing and add the correct trace type
            AppTrace.TraceSource.WriteWarning(string.Format(CultureInfo.InvariantCulture, "RWLock"), "invoking test hook");
            snapHandOffTestAction.Invoke();
        }
    }
}