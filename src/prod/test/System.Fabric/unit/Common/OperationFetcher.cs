// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// The OperationFetcher is a helper class used to make operation processing easy
    /// It will handle all the work associated with draining queues/switching queues etc
    /// </summary>
    public sealed class OperationFetcher
    {
        private readonly IOperationQueue copyOperationQueue;
        private readonly IOperationQueue replicationOperationQueue;
        private readonly bool drainQueuesInParallel;

        private ReplicaRole role;

        public OperationFetcher(IStateReplicator stateReplicator, OperationProcessorInfo copyOperationProcessor, OperationProcessorInfo replicationOperationProcessor, bool drainQueuesInParallel) 
        {
            Requires.Argument("partition", stateReplicator).NotNull();
            Requires.Argument("copyOperationProcessor", copyOperationProcessor).NotNull();
            Requires.Argument("replicationOperationProcessor", replicationOperationProcessor).NotNull();

            if (copyOperationProcessor.Callback == null)
            {
                throw new ArgumentException("copyOperationProcessor.Callback cannot be null");
            }

            if (replicationOperationProcessor.Callback == null)
            {
                throw new ArgumentException("replicationOperationProcessor.Callback cannot be null");
            }

            this.copyOperationQueue = new OperationQueue(() => stateReplicator.GetCopyStream(), copyOperationProcessor) { Name = "CopyQueue" };
            this.replicationOperationQueue = new OperationQueue(() => stateReplicator.GetReplicationStream(), replicationOperationProcessor) { Name = "ReplicationQueue" };
            this.drainQueuesInParallel = drainQueuesInParallel;
            this.role = ReplicaRole.None;
        }

        internal OperationFetcher(IOperationQueue copyOperationQueue, IOperationQueue replicationOperationQueue, bool drainQueuesInParallel)
        {
            this.copyOperationQueue = copyOperationQueue;
            this.replicationOperationQueue = replicationOperationQueue;
            this.drainQueuesInParallel = drainQueuesInParallel;
            this.role = ReplicaRole.None;
        }

        internal interface IOperationQueue
        {
            string Name { get; }

            Task DrainAsync();
        }

        // For unit tests
        internal ReplicaRole Role
        {
            get { return this.role; }
        }

        public void ChangeRole(ReplicaRole newRole)
        {
            //// AppTrace.TraceMsg(TraceLogEventType.Information, "OperationFetcher.ChangeRole", "ChangeRole from {0} to {1}", this.role, newRole);
            //// what are the state transistions possible?

            ReplicaRole oldRole = this.role;
            this.role = newRole;

            if (oldRole == ReplicaRole.None && newRole == ReplicaRole.IdleSecondary)
            {
                if (this.drainQueuesInParallel)
                {
                    OperationFetcher.DrainQueueAsyncHelper(this.copyOperationQueue);
                    OperationFetcher.DrainQueueAsyncHelper(this.replicationOperationQueue);
                }
                else
                {
                    OperationFetcher.DrainQueueAsyncHelper(this.copyOperationQueue).ContinueWith(copyQueueDrainTask =>
                        {
                            //// AppTrace.TraceMsg(TraceLogEventType.Verbose, "OperationFetcher.QueueDrain", "Starting replication queue");
                            if (this.role == ReplicaRole.None)
                            {
                                //// AppTrace.TraceMsg(TraceLogEventType.Information, "OperationFetcher.QueueDrain", "ReplicaRole is None so not draining replication queue");
                                return;
                            }

                            OperationFetcher.DrainQueueAsyncHelper(this.replicationOperationQueue);
                        });
                }
            }
            else if (oldRole == ReplicaRole.IdleSecondary && newRole == ReplicaRole.ActiveSecondary)
            {
                // No-Op
            }
            else if (oldRole == ReplicaRole.ActiveSecondary && newRole == ReplicaRole.Primary)
            {
                // No-Op
            }
            else if (oldRole == ReplicaRole.Primary && newRole == ReplicaRole.ActiveSecondary)
            {
                OperationFetcher.DrainQueueAsyncHelper(this.replicationOperationQueue);                
            }
            else if (oldRole == ReplicaRole.None && newRole == ReplicaRole.Primary)
            {
                // No-op
            }
            else if (newRole == ReplicaRole.None)
            {
                // No-Op
            }
            else
            {
                // AppTrace.TraceMsg(TraceLogEventType.Error, "OperationFetcher.ChangeRole", "Invalid state change");
                ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "Invalid State Change from {0} to {1}", oldRole, newRole));
            }
        }

        public void Close()
        {
            // TODO: do we do anything with the queue drain tasks (wait for them?)
            this.role = ReplicaRole.None;
        }

        private static Task DrainQueueAsyncHelper(IOperationQueue queue)
        {
            return queue.DrainAsync().ContinueWith(t => OperationFetcher.LogQueueDrainCompletion(t, queue.Name), TaskContinuationOptions.ExecuteSynchronously);
        }

        private static void LogQueueDrainCompletion(Task t, string queueName)
        {
            // AppTrace.TraceMsg(TraceLogEventType.Information, "OperationFetcher.QueueDrainTask", "Queue drain complete {0}", queueName);
            if (t.IsFaulted)
            {
                AppTrace.TraceSource.WriteExceptionAsError("OperationFetcher.QueueDrainTask", t.Exception.InnerException, "Drain Task error {0}", queueName);
            }
        }

        internal class OperationQueue : IOperationQueue
        {
            private readonly OperationProcessorInfo info;
            private readonly Func<IOperationStream> operationStreamGetter;

            public OperationQueue(Func<IOperationStream> operationStreamGetter, OperationProcessorInfo info)
            {
                Requires.Argument("operationGetterFunc", operationStreamGetter).NotNull();
                Requires.Argument("info", info).NotNull();

                this.info = info;
                this.operationStreamGetter = operationStreamGetter;
            }

            public string Name { get; set; }

            public Task DrainAsync()
            {
                //// AppTrace.TraceMsg(TraceLogEventType.Information, "OperationQueue.DrainAsync", "Starting drain on queue: {0}", this.Name);

                TaskCompletionSource<object> tcs = new TaskCompletionSource<object>();

                IOperationStream stream = null;
                try
                {
                    stream = this.operationStreamGetter();
                }
                catch (Exception ex)
                {
                    // AppTrace.TraceException(ex, "OperationQueue.DrainAsync", "Exception was thrown while getting stream for {0}", this.Name);
                    tcs.SetException(ex);
                    return tcs.Task;
                }

                this.DrainTaskLoop(tcs, stream);

                return tcs.Task;
            }

            private void DrainTaskLoop(TaskCompletionSource<object> tcs, IOperationStream stream)
            {
                Task<IOperation> operationTask;

                try
                {
                    operationTask = stream.GetOperationAsync(new CancellationToken());
                }
                catch (Exception ex)
                {
                    // AppTrace.TraceException(ex, "OperationQueue.DrainAsync", "Exception was thrown while calling operation getter for {0}", this.Name);
                    tcs.SetException(ex);
                    return;
                }

                operationTask.ContinueWith(
                    t =>
                    {
                        if (t.IsFaulted)
                        {
                            // AppTrace.TraceException(t.Exception.InnerException, "OperationQueue.DrainAsync", "Exception from getter for queue {0}", this.Name);
                            tcs.SetException(t.Exception.InnerException);
                            return;
                        }

                        if (t.Result == null)
                        {
                            // AppTrace.TraceMsg(TraceLogEventType.Information, "OperationQueue.DrainAsync", "Queue is complete: {0}", this.Name);
                            tcs.SetResult(null);
                            return;
                        }

                        //// AppTrace.TraceMsg(TraceLogEventType.Verbose, "OperationQueue.DrainAsync", "Queue {0} Received operation: {1}", this.Name, t.Result.SequenceNumber);

                        Task callbackTask;
                        try
                        {
                            callbackTask = this.info.Callback(t.Result);
                        }
                        catch (Exception ex)
                        {
                            // crash 
                            AppTrace.TraceSource.WriteExceptionAsError("OperationQueue.DrainAsync", ex, "Queue {0} - callback task function invoke threw", this.Name);
                            throw;
                        }

                        // create a continuation on the callback task
                        callbackTask.ContinueWith(
                            (continuation) =>
                            {
                                if (continuation.IsFaulted)
                                {
                                    // crash
                                    // AppTrace.TraceException(continuation.Exception.InnerException, "OperationQueue.DrainAsync", "Queue {0} - callback failed", this.Name);
                                    throw continuation.Exception.InnerException;
                                }

                                // in single operation processing mode the outer task has not asked the fetcher for another operation
                                // ask for it in the continuation
                                if (!this.info.SupportsConcurrentProcessing)
                                {
                                    this.DrainTaskLoop(tcs, stream);
                                }
                            },
                            TaskContinuationOptions.ExecuteSynchronously);

                        // multiple operations can be processed at the same - continue asking for operations
                        if (this.info.SupportsConcurrentProcessing)
                        {
                            this.DrainTaskLoop(tcs, stream);
                        }
                    }, 
                    TaskContinuationOptions.ExecuteSynchronously);
            }
        }
    }
}