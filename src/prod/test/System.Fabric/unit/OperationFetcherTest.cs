// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class OperationFetcherTest
    {

        #region Replica Role Test

        private void RoleChangeTest(ReplicaRole expectedRole, params object[] roleChangeArgs)
        {
            var inst = OperationFetcherTest.Create();

            inst.DoStateChangeOperations(roleChangeArgs);

            Assert.AreEqual<ReplicaRole>(expectedRole, inst.Fetcher.Role);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_NoneOnStartup()
        {
            this.RoleChangeTest(ReplicaRole.None);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_NoneToPrimary()
        {
            this.RoleChangeTest(ReplicaRole.Primary, ReplicaRole.Primary);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_NoneToIdleSec()
        {
            this.RoleChangeTest(ReplicaRole.IdleSecondary, ReplicaRole.IdleSecondary);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_IdleSecToActiveSec()
        {
            this.RoleChangeTest(ReplicaRole.ActiveSecondary, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_ActiveSecToPrimary()
        {
            this.RoleChangeTest(ReplicaRole.Primary, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary, ReplicaRole.Primary);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_PrimaryToActiveSec()
        {
            this.RoleChangeTest(ReplicaRole.ActiveSecondary, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary, ReplicaRole.Primary, ReplicaRole.ActiveSecondary);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_IdleSecToClose()
        {
            this.RoleChangeTest(ReplicaRole.None, ReplicaRole.IdleSecondary, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_ActiveSecToClose()
        {
            this.RoleChangeTest(ReplicaRole.None, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_PrimaryToClose()
        {
            this.RoleChangeTest(ReplicaRole.None, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary, ReplicaRole.Primary, true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_IdleSecToNone()
        {
            this.RoleChangeTest(ReplicaRole.None, ReplicaRole.IdleSecondary, ReplicaRole.None);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_ActiveSecToNone()
        {
            this.RoleChangeTest(ReplicaRole.None, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary, ReplicaRole.None);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_RoleChange_PrimaryToNone()
        {
            this.RoleChangeTest(ReplicaRole.None, ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary, ReplicaRole.Primary, ReplicaRole.None);
        }

        #endregion


        private static void AssertQueueTaskCount(int expectedCount, OperationQueueStub queue)
        {
#if DotNetCoreClr
            // asnegi : RDBug 10087224 : Need to find why we need this higher timeout.
            int timeout = 15;
#else
            int timeout = 1;
#endif
            Assert.IsTrue(MiscUtility.WaitUntil(() => queue.DrainTasks.Count == expectedCount, TimeSpan.FromSeconds(timeout), 50));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInSequentialStartsOnlyCopyQueueOnChangeRole()
        {
            var inst = OperationFetcherTest.Create();

            inst.Fetcher.ChangeRole(ReplicaRole.IdleSecondary);

            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(0, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInSequentialDrainsReplicationQueueAfterCopyQueueIsDone()
        {
            var inst = OperationFetcherTest.Create();

            inst.Fetcher.ChangeRole(ReplicaRole.IdleSecondary);

            // complete the draining of the copy queue
            OperationQueueStub.CompleteTaskAndWait(inst.CopyQueue.DrainTasks[0]);

            // it should start draining the replicaiton queue now
            OperationFetcherTest.AssertQueueTaskCount(1, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInSequentialDoesNotDrainReplicationQueueIfCloseIsCalled()
        {
            var inst = OperationFetcherTest.Create();

            // become idle sec -> start draining copy queue
            // call close
            inst.DoStateChangeOperations(ReplicaRole.IdleSecondary, true);

            // complete the draining of the copy queue
            OperationQueueStub.CompleteTaskAndWait(inst.CopyQueue.DrainTasks[0]);

            // the replication queue should not have been drained
            OperationFetcherTest.AssertQueueTaskCount(0, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInParallelStartsDrainingBothQueues()
        {
            var inst = OperationFetcherTest.Create(true);

            // become idle sec -> start draining copy queue and replication queue
            inst.Fetcher.ChangeRole(ReplicaRole.IdleSecondary);

            // both queues are being drained
            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(1, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInParallelDoesNotStartDrainingReplicationQueueTwice()
        {
            var inst = OperationFetcherTest.Create(true);

            // become idle sec -> start draining copy queue
            // and then go from idle sec -> active sec
            inst.DoStateChangeOperations(ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary);

            // both queues are being drained only once
            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(1, inst.ReplicationQueue);
        }

        private void PrimaryToActiveSecondaryTestHelper(bool drainInParallel)
        {
            var inst = OperationFetcherTest.Create(drainInParallel);

            // go to active sec
            inst.DoStateChangeOperations(ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary);

            // complete the copy queue
            if (!drainInParallel)
            {
                OperationQueueStub.CompleteTaskAndWait(inst.CopyQueue.DrainTasks[0]);
            }

            // now go to primary
            inst.DoStateChangeOperations(ReplicaRole.Primary);

            // both queues are being drained only once
            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(1, inst.ReplicationQueue);

            // now go to active secondary
            inst.Fetcher.ChangeRole(ReplicaRole.ActiveSecondary);

            // we have started draining again
            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(2, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInBothCases_FromPrimaryToSecondaryTransistionStartsDrainingOnceAgain()
        {
            this.PrimaryToActiveSecondaryTestHelper(false);
            this.PrimaryToActiveSecondaryTestHelper(true);
        }

        private void ActiveSecondaryToCloseTestHelper(bool drainInParallel)
        {
            var inst = OperationFetcherTest.Create(drainInParallel);

            // idle -> active -> close
            inst.DoStateChangeOperations(ReplicaRole.IdleSecondary, ReplicaRole.ActiveSecondary);

            if (!drainInParallel)
            {
                OperationQueueStub.CompleteTaskAndWait(inst.CopyQueue.DrainTasks[0]);
            }

            // the count of drains should be = 1
            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(1, inst.ReplicationQueue);

            // close
            inst.DoStateChangeOperations(true);

            // the count of drains should be = 1
            OperationFetcherTest.AssertQueueTaskCount(1, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(1, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInBothCases_ActiveSecToCloseDoesNotDrainAnyMore()
        {
            this.ActiveSecondaryToCloseTestHelper(false);
            this.ActiveSecondaryToCloseTestHelper(true);
        }

        private void IdleSecondaryToCloseTestHelper(bool drainInParallel)
        {
            var inst = OperationFetcherTest.Create(drainInParallel);

            // change to idle sec
            inst.Fetcher.ChangeRole(ReplicaRole.IdleSecondary);

            int copyQueueCount = inst.CopyQueue.DrainTasks.Count;
            int replicationQueueCount = inst.ReplicationQueue.DrainTasks.Count;

            // close
            inst.Fetcher.Close();

            // the counts should be the same
            OperationFetcherTest.AssertQueueTaskCount(copyQueueCount, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(replicationQueueCount, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInBothCases_IdleSecToCloseDoesNotDrainAnyMore()
        {
            this.IdleSecondaryToCloseTestHelper(true);
            this.IdleSecondaryToCloseTestHelper(false);
        }

        private void NoneToPrimaryDoesNotDrainAnyQueues(bool drainInParallel)
        {
            var inst = OperationFetcherTest.Create(drainInParallel);

            // primary
            inst.DoStateChangeOperations(ReplicaRole.Primary);

            // the count of drains should be = 0
            OperationFetcherTest.AssertQueueTaskCount(0, inst.CopyQueue);
            OperationFetcherTest.AssertQueueTaskCount(0, inst.ReplicationQueue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationFetcherTest_DrainInBothCases_NoneToPrimaryDOesNotCauseAnyDrain()
        {
            this.NoneToPrimaryDoesNotDrainAnyQueues(false);
            this.NoneToPrimaryDoesNotDrainAnyQueues(true);
        }

        private static Instance<OperationQueueStub> Create(bool drainInParallel = false)
        {
            return new Instance<OperationQueueStub>(drainInParallel);
        }

        class Instance<TQueue> where TQueue : OperationQueueStubBase, new()
        {
            public TQueue ReplicationQueue { get; set; }

            public TQueue CopyQueue { get; set; }

            public bool DrainInParallel { get; private set; }

            public OperationFetcher Fetcher { get; set; }

            public Instance(bool drainInParallel = false)
            {
                this.DrainInParallel = drainInParallel;
                this.ReplicationQueue = new TQueue() { Name = "Replication" };
                this.CopyQueue = new TQueue() { Name = "Copy" };
                this.Fetcher = new OperationFetcher(this.CopyQueue, this.ReplicationQueue, this.DrainInParallel);
            }

            /// <summary>
            /// if the bool is true then it is a close
            /// </summary>
            /// <param name="operations"></param>
            public void DoStateChangeOperations(params object[] operations)
            {
                for (int i = 0; i < operations.Length; i++)
                {
                    // represents a close
                    if (operations[i].GetType() == typeof(bool))
                    {
                        this.Fetcher.Close();
                    }
                    else if (operations[i].GetType() == typeof(ReplicaRole))
                    {
                        this.Fetcher.ChangeRole((ReplicaRole)operations[i]);
                    }
                    else
                    {
                        throw new ArgumentException("Invalid type");
                    }
                }
            }

        }

        class OperationQueueStubBase : OperationFetcher.IOperationQueue
        {
            public string Name { get; set; }

            public virtual Task DrainAsync()
            {
                throw new NotImplementedException();
            }
        }

        class OperationQueueStub : OperationQueueStubBase
        {
            public OperationQueueStub()
            {
                this.DrainTasks = new List<Task>();
            }

            public IList<Task> DrainTasks { get; private set; }

            public override Task DrainAsync()
            {
                var resetEvent = new ManualResetEventSlim(false);
                var task = Task.Factory.StartNew((obj) => resetEvent.Wait(), resetEvent);
                this.DrainTasks.Add(task);
                return task;
            }

            public static ManualResetEventSlim GetEventForTask(Task t)
            {
                return t.AsyncState as ManualResetEventSlim;
            }

            public static void CompleteTaskAndWait(Task t)
            {
                if (t.IsCompleted) return;

                OperationQueueStub.GetEventForTask(t).Set();
                t.Wait();
            }
        }
    }

    [TestClass]
    public class OperationQueueTest
    {

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationQueueTest_DrainTaskIsCompletedWhenNullOperationIsReceived()
        {
            var opList = new OperationList();
            opList.AddOperation(null);

            var processor = new TrackingOperationProcessor();

            var opQueue = new OperationFetcher.OperationQueue(() => opList, new OperationProcessorInfo
            {
                SupportsConcurrentProcessing = false,
                Callback = processor.Process
            });

            var task = opQueue.DrainAsync();
            opList.CompleteOperation();

            // the task should be complete
            task.AssertWaitCompletes(500);
        }

        private void ExceptionFromOperationFetchingIsThrownTestHelper(ThrowingOperationList list)
        {
            var ex = new InvalidOperationException();

            list.ExceptionToThrow = ex;

            var queue = new OperationFetcher.OperationQueue(() => list, new OperationProcessorInfo
            {
                Callback = new TrackingOperationProcessor().Process
            });

            var task = queue.DrainAsync();

            try
            {
                task.Wait();
                Assert.Fail("expceted ex");
            }
            catch (AggregateException aex)
            {
                Assert.AreSame(ex, aex.InnerException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationQueueTest_ExceptionFromOperationFetchingIsThrown()
        {
            this.ExceptionFromOperationFetchingIsThrownTestHelper(new ThrowingOperationList { ThrowSynchronously = false });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationQueueTest_ExceptionFromOperationFetchingIsThrown2()
        {
            this.ExceptionFromOperationFetchingIsThrownTestHelper(new ThrowingOperationList { ThrowSynchronously = true });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("apramyer")]
        public void OperationQueueTest_OperationStreamGetterIsInvokedOncePerDrain()
        {
            var list = new OperationList();
            list.AddOperation(null);

            int invokeCount = 0;
            Func<IOperationStream> streamGetter = () =>
            {
                invokeCount++;
                return list;
            };

            var queue = new OperationFetcher.OperationQueue(streamGetter, new OperationProcessorInfo
            {
                Callback = new TrackingOperationProcessor().Process
            });

            var task1 = queue.DrainAsync();
            Assert.AreEqual(1, invokeCount);

            var task2 = queue.DrainAsync();
            Assert.AreEqual(2, invokeCount);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("apramyer")]
        public void OperationQueueTest_FailingOperationStreamGetterReturnsException()
        {
            var ex = new InvalidOperationException();

            Func<IOperationStream> streamGetter = () => { throw ex; };

            var queue = new OperationFetcher.OperationQueue(streamGetter, new OperationProcessorInfo
            {
                Callback = new TrackingOperationProcessor().Process
            });

            var task = queue.DrainAsync();

            try
            {
                task.Wait();
                Assert.Fail("expceted ex");
            }
            catch (AggregateException aex)
            {
                Assert.AreSame(ex, aex.InnerException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationQueueTest_ParallelDrain()
        {
            var operations = new List<IOperation> { new OperationStub(), new OperationStub(), null };

            var opList = new OperationList();
            foreach (var item in operations)
            {
                opList.AddOperation(item);
            }

            var processor = new TrackingOperationProcessor();

            var opQueue = new OperationFetcher.OperationQueue(() => opList, new OperationProcessorInfo
            {
                SupportsConcurrentProcessing = true,
                Callback = processor.Process
            });

            var task = opQueue.DrainAsync();

            for (int i = 0; i < operations.Count; i++)
            {
                // we are processing asynchronously
                // tell the list to provide an operation to the queue
                opList.CompleteOperation();

                if (i == operations.Count - 1)
                {
                    // the last null is not received
                    continue;
                }

                // the operation has been sent to the callback
                // the callback has not completed the task
                // the queue will continue (this loop will run)
                Assert.IsTrue(MiscUtility.WaitUntil(() => processor.GetOperations().Count == i + 1, TimeSpan.FromSeconds(1), 50));
                Assert.AreSame(operations[i], processor.GetOperations()[i].Item2);
            }

            // the task should be complete
            task.AssertWaitCompletes(500);
        }


        class TrackingOperationProcessor
        {
            private readonly object syncLock = new object();
            private readonly List<Tuple<TaskCompletionSource<object>, IOperation>> operations = new List<Tuple<TaskCompletionSource<object>, IOperation>>();

            public Task Process(IOperation operation)
            {
                var tcs = new TaskCompletionSource<object>();

                lock (this.syncLock)
                {
                    this.operations.Add(Tuple.Create(tcs, operation));
                }

                return tcs.Task;
            }

            public IList<Tuple<TaskCompletionSource<object>, IOperation>> GetOperations()
            {
                lock (this.syncLock)
                {
                    return new List<Tuple<TaskCompletionSource<object>, IOperation>>(this.operations);
                }
            }
        }


        class OperationStub : Stubs.OperationStubBase
        {
            public override long SequenceNumber
            {
                get { return -1; }
            }
        }

        class ThrowingOperationList : Stubs.OperationStreamStubBase
        {
            public bool ThrowSynchronously { get; set; }
            public Exception ExceptionToThrow { get; set; }

            public override Task<IOperation> GetOperationAsync(CancellationToken cancellationToken)
            {
                if (this.ThrowSynchronously)
                {
                    throw this.ExceptionToThrow;
                }
                else
                {
                    var tcs = new TaskCompletionSource<IOperation>();
                    tcs.SetException(this.ExceptionToThrow);
                    return tcs.Task;
                }
            }
        }

        class OperationList : Stubs.OperationStreamStubBase
        {
            private Queue<Tuple<IOperation, TaskCompletionSource<IOperation>>> operations = new Queue<Tuple<IOperation, TaskCompletionSource<IOperation>>>();

            public void AddOperation(IOperation operation)
            {
                TaskCompletionSource<IOperation> producer = new TaskCompletionSource<IOperation>();
                this.operations.Enqueue(Tuple.Create(operation, producer));
            }

            public void CompleteOperation()
            {
                var item = this.operations.Dequeue();
                item.Item2.SetResult(item.Item1);
            }

            public override Task<IOperation> GetOperationAsync(CancellationToken cancellationToken)
            {
                return this.operations.Peek().Item2.Task;
            }
        }
    }
}