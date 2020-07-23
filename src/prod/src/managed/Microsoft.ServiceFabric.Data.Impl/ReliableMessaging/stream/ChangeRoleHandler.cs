// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Fabric.Common.Tracing;
    using System.Fabric.ReliableMessaging;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator;

    #endregion

    /// <summary>
    /// Class that implements change role(replicator) sync capabilities. This provides the capability to quickly
    /// return the replication call back while enabling the role change process to be processed async.
    /// </summary>
    internal class ChangeRoleSynchronizer
    {
        #region fields

        private readonly StreamManager streamManager;
        private readonly object syncLock = new object();

        #endregion

        #region cstor

        /// <summary>
        /// Construct and Init. 
        /// </summary>
        /// <param name="streamManager"></param>
        internal ChangeRoleSynchronizer(StreamManager streamManager)
        {
            this.streamManager = streamManager;
        }

        #endregion

        #region properties

        /// <summary>
        /// Return TaskCompletionSource handle. The handle is null if this is a non primary role.
        /// </summary>
        internal TaskCompletionSource<object> WaitHandle { get; private set; }

        #endregion

        # region methods

        /// <summary>
        /// Create a handle to TaskCompletionSource, when replication role callback signals a change to the primary.
        /// </summary>
        internal void ChangeToPrimary()
        {
            lock (this.syncLock)
            {
                // Handle should be null (as this is assumed to be a secondary), and now changing into a primary.
                Diagnostics.Assert(
                    this.WaitHandle == null,
                    "{0} ChangeRoleSynchronizer.ChangeToPrimary found non null handle",
                    this.streamManager.Tracer);
                this.WaitHandle = new TaskCompletionSource<object>();
            }
        }

        /// <summary>
        /// Close the handle, when the role changes to a non-primary.
        /// </summary>
        internal void ChangeToNotPrimary()
        {
            lock (this.syncLock)
            {
                // Handle is not expected to be null, as this was assumed to be a primary earlier
                Diagnostics.Assert(
                    this.WaitHandle != null,
                    "{0} ChangeRoleSynchronizer.ChangeToNotPrimary found null handle",
                    this.streamManager.Tracer);

                // Set exception in case this occurred before ChangeToPrimary recovery completed and there are threads waiting
                var result = this.WaitHandle.TrySetException(new FabricNotPrimaryException());


                if (!result)
                {
                    FabricEvents.Events.ChangeRoleSetEventFailure(
                        this.streamManager.TraceType,
                        "ChangeToNotPrimary",
                        this.streamManager.Era.ToString());
                }

                // Set handle to null to indicate non primary status.
                this.WaitHandle = null;
            }
        }

        /// <summary>
        /// Set the task result to success, to indicate the successful transition of the role(and stream manager) to primary status.
        /// </summary>
        /// <returns></returns>
        internal bool SetSuccess()
        {
            var result = false;

            lock (this.syncLock)
            {
                if (this.WaitHandle != null)
                {
                    result = this.WaitHandle.TrySetResult(null);

                    if (!result)
                    {
                        FabricEvents.Events.ChangeRoleSetEventFailure(
                            this.streamManager.TraceType,
                            "SetSuccess",
                            this.streamManager.Era.ToString());
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Set the task result to failure, to indicate the unsuccessful transition of the role(and stream manager) to primary status.
        /// </summary>
        /// <returns></returns>
        internal bool SetFailure(Exception e)
        {
            var result = false;

            lock (this.syncLock)
            {
                if (this.WaitHandle != null)
                {
                    // Raise exception.
                    result = this.WaitHandle.TrySetException(e);

                    if (!result)
                    {
                        FabricEvents.Events.ChangeRoleSetEventFailure(
                            this.streamManager.TraceType,
                            "SetFailure",
                            this.streamManager.Era.ToString());
                    }
                }
            }

            return result;
        }

        /// <summary>
        /// Wait for role transition to complete.
        /// </summary>
        /// <returns></returns>
        /// <exception cref="FabricNotPrimaryException">FabricNotPrimaryException</exception>
        internal async Task WaitForRecovery()
        {
            var localHandle = this.WaitHandle;

            if (this.WaitHandle == null)
            {
                throw new FabricNotPrimaryException();
            }

            await localHandle.Task;
        }

        #endregion
    }

    /// <summary>
    /// This mechanism is used to discharge waiting async completions -- for instance for Open and Close protocols
    /// Uses the replicator's read-only transaction mechanism(LockContext) to get a role change notification via Unlock of a LockContext
    /// The pattern is to create and dispose a CloseWhenNotPrimary object in a using block; the Start method sets up the Unlock context.
    /// See Stream.FinishOpenAsync for an example
    /// TODO:With the retry added to the FinishOpen process we need to revisit how this is used there.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    internal class ChangeRoleLockContext<T> : LockContext
    {
        #region fields

        /// <summary>
        /// The completion task
        /// </summary>
        private readonly TaskCompletionSource<T> completion;

        #endregion

        #region cstor

        /// <summary>
        /// Create a Change Role Lock Context object and Track the completion task.
        /// </summary>
        /// <param name="completion"></param>
        internal ChangeRoleLockContext(TaskCompletionSource<T> completion)
        {
            this.completion = completion;
        }

        #endregion

        /// <summary>
        /// Not Supported.
        /// </summary>
        /// <param name="stateOrKey"></param>
        /// <exception cref="NotImplementedException"></exception>
        /// <returns></returns>
        protected override bool IsEqual(object stateOrKey)
        {
            Diagnostics.Assert(false, "ChangeRoleLockContext::IsEqual not supported");
            throw new NotImplementedException();
        }

        /// <summary>
        /// Throw an exception, when role unlock event is signaled.
        /// </summary>
        public override void Unlock(LockContextMode mode)
        {
            this.completion.TrySetException(new FabricNotPrimaryException());
        }
    }

    /// <summary>
    /// The class is to create and dispose a CloseWhenNotPrimary object in a using block; the Start method sets up the ChangeRole Unlock context.
    /// See Stream.FinishOpenAsync for an example
    /// </summary>
    /// <typeparam name="T"></typeparam>
    internal class CloseWhenNotPrimary<T> : IDisposable
    {
        private readonly TaskCompletionSource<T> completion;
        private readonly StreamManager streamManager;

        /// <summary>
        /// Provides a role change callback signal via unlockContext 
        /// </summary>
        private Transaction readOnlyTransaction;

        /// <summary>
        /// Close the task when the role is not primary.
        /// </summary>
        /// <param name="completion"></param>
        /// <param name="streamManager"></param>
        internal CloseWhenNotPrimary(TaskCompletionSource<T> completion, StreamManager streamManager)
        {
            this.completion = completion;
            this.streamManager = streamManager;
        }

        /// <summary>
        /// Sets up replication lock context to provide role change call backs.
        /// </summary>
        /// <param name="timeout"></param>
        /// <returns></returns>
        internal async Task Start(TimeSpan timeout)
        {
            this.readOnlyTransaction = await this.streamManager.CreateReplicatorTransactionAsync(timeout);
            this.readOnlyTransaction.AddLockContext(new ChangeRoleLockContext<T>(this.completion));
        }

        /// <summary>
        /// Assert as the role changed prior to dispose.
        /// </summary>
        /// <param name="disposedTask"></param>
        private void DisposeContinuation(Task disposedTask)
        {
            if (disposedTask.Exception != null)
            {
                Diagnostics.Assert(
                    disposedTask.Exception.InnerException is FabricObjectClosedException ||
                    disposedTask.Exception.InnerException is FabricNotPrimaryException,
                    "{0} Unexpected exception in enclosed completion when committing readOnlyTransaction in CloseWhenNotPrimary.Dispose",
                    this.streamManager.Tracer);
            }
        }

        /// <summary>
        /// Once the commit completes, check for non primary status and dispose 
        /// </summary>
        public void Dispose()
        {
            if (this.readOnlyTransaction != null)
            {
                this.readOnlyTransaction.CommitAsync().ContinueWith(this.DisposeContinuation);
            }
        }
    }
}