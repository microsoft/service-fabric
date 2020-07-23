// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Replication
{
    using System;
    using System.Diagnostics;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    // [TestClass]
    public class ReplicationWithLockingTests
    {
        private static readonly ServiceInitializationParameters DefaultInitializationParameters = new ServiceInitializationParameters
        {
            ServiceTypeName = "foo",
            ServiceName = new Uri("http://foo"),
        };

        private const long DefaultReplicaId = 123;


        private const int TimeoutMilliseconds = 20000;
        private static TestStateReplicator StateReplicator = null;
        private static StatefulServiceExecutionContext Context = null;
        private static ReplicableTypeInfo TypeInfo = null;
        private static StatefulProgramInstance ProgramInstance = null;
        private Action empty = () => { };

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            TestStatefulServiceReplica service = new TestStatefulServiceReplica();
            StatefulServiceReplicaBroker broker = new StatefulServiceReplicaBroker(service, ReplicationWithLockingTests.DefaultInitializationParameters, ReplicationWithLockingTests.DefaultReplicaId);
            TestNativePartition nativePartition = new TestNativePartition();
            StatefulServicePartition statefulServicePartition = new StatefulServicePartition(nativePartition, ServicePartitionInformation.CreateFromNative(nativePartition.PartitionInfo));
            //((IServiceCatalog)ServiceCatalog.Current).AddService(service.Name, statefulServicePartition.Id, service);
            ((IStatefulServiceReplica)service).OpenAsync(statefulServicePartition);
            ((IStatefulServiceReplica)service).ChangeRoleAsync(ReplicaRole.Primary).Wait();
            ReplicationWithLockingTests.Context = new StatefulServiceExecutionContext(
                service,
                new Dictionary<string, object> 
                { 
                    { System.Fabric.Common.Constants.PropertyNames.Service, service.Name } ,
                    { System.Fabric.Common.Constants.PropertyNames.PartitionId, service.Partition.Id }
                });
            ReplicationWithLockingTests.TypeInfo = ReplicableTypeInfo.GenerateReplicableTypeInfo(typeof(SampleService));
            ReplicationWithLockingTests.ProgramInstance = new StatefulProgramInstance(service, Guid.NewGuid(), TypeInfo, new SampleService());
            ReplicationWithLockingTests.StateReplicator = nativePartition.StateReplicator;
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SyncNoLock()
        {
            this.ExecuteTest("SyncNoLock", 0, 0, new object[0]);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SyncWriteLock()
        {
            this.ExecuteTest("SyncWriteLock", 1, 0, new object[0]);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SyncReadLock()
        {
            this.ExecuteTest("SyncReadLock", 0, 1, new object[0]);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SyncReadAndWriteLock()
        {
            this.ExecuteTest("SyncReadAndWriteLock", 2, 1, new object[0]);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void BeginNoLock()
        {
            AutoResetEvent waitEvent = new AutoResetEvent(false);
            AsyncCallback callback = (asyncResult) => { waitEvent.Set(); };
            this.ExecuteTest("BeginNoLock", 0, 0, new object[] { callback, new object() });
            Assert.IsTrue(waitEvent.WaitOne(ReplicationWithLockingTests.TimeoutMilliseconds), "Test timed out");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void BeginWriteLock()
        {
            AutoResetEvent waitEvent = new AutoResetEvent(false);
            AsyncCallback callback = (asyncResult) => { waitEvent.Set(); };
            this.ExecuteTest("BeginWriteLock", 1, 0, new object[] { callback, new object() });
            Assert.IsTrue(waitEvent.WaitOne(ReplicationWithLockingTests.TimeoutMilliseconds), "Test timed out");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void BeginReadAndWriteLock()
        {
            AutoResetEvent waitEvent = new AutoResetEvent(false);
            AsyncCallback callback = (asyncResult) => { waitEvent.Set(); };
            this.ExecuteTest("BeginReadAndWriteLock", 2, 1, new object[] { callback, new object() });
            Assert.IsTrue(waitEvent.WaitOne(ReplicationWithLockingTests.TimeoutMilliseconds), "Test timed out");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TaskNoLockAsync()
        {
            AutoResetEvent waitEvent = new AutoResetEvent(false);
            AsyncCallback callback = (asyncResult) => { waitEvent.Set(); };
            Task task = (Task)this.ExecuteTest("TaskNoLockAsync", 0, 0, new object[] { new object() });
            Assert.IsTrue(task.Wait(ReplicationWithLockingTests.TimeoutMilliseconds), "Test timed out");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TaskWriteLockAsync()
        {
            AutoResetEvent waitEvent = new AutoResetEvent(false);
            AsyncCallback callback = (asyncResult) => { waitEvent.Set(); };
            Task task = (Task)this.ExecuteTest("TaskWriteLockAsync", 1, 0, new object[] { new object() });
            Assert.IsTrue(task.Wait(ReplicationWithLockingTests.TimeoutMilliseconds), "Test timed out");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TaskReadAndWriteLockAsync()
        {
            AutoResetEvent waitEvent = new AutoResetEvent(false);
            AsyncCallback callback = (asyncResult) => { waitEvent.Set(); };
            Task task = (Task)this.ExecuteTest("TaskReadAndWriteLockAsync", 2, 1, new object[] { new object() });
            Assert.IsTrue(task.Wait(ReplicationWithLockingTests.TimeoutMilliseconds), "Test timed out");
        }

        private object ExecuteTest(string methodName, int numberOfExpectedWriteLocks, int numberOfExpectedReadLocks, object[] inParams)
        {
            object[] outParams = null;
            ReplicableMethodInfo replicableMethodInfo = ReplicationWithLockingTests.TypeInfo.ReplicableMethodInfos[methodName][0];
            ReplicationWithLockingTests.StateReplicator.ReplicableMethodInfo = replicableMethodInfo;
            ReplicationWithLockingTests.StateReplicator.NumberOfExpectedWriteLocks = numberOfExpectedWriteLocks;
            ReplicationWithLockingTests.StateReplicator.NumberOfExpectedReadLocks = numberOfExpectedReadLocks;
            return replicableMethodInfo.Invoke(ReplicationWithLockingTests.Context, this.empty, this.empty, ReplicationWithLockingTests.ProgramInstance, replicableMethodInfo.Methods[0], inParams, out outParams);
        }

        private class SampleService
        {
            private const string StateLock1 = "StateLock1";
            private const string StateLock2 = "StateLock2";
            private const string StateLock3 = "StateLock3";

            [Replicable]
            private int x = 0;

            [ReplicateOnExit]
            public void SyncNoLock()
            {
                this.x++;
            }

            [ReplicateOnExit]
            [StateLock(StateLock1, true)]
            public void SyncWriteLock()
            {
                this.x++;
            }

            [StateLock(StateLock1, false)]
            public void SyncReadLock()
            {
                this.x++;
            }

            [ReplicateOnExit]
            [StateLock(StateLock1, true)]
            [StateLock(StateLock2, false)]
            [StateLock(StateLock3, true)]
            public void SyncReadAndWriteLock()
            {
                this.x++;
            }

            [ReplicateOnExit(Async = true)]
            public IAsyncResult BeginNoLock(AsyncCallback callback, object state)
            {
                this.x++;
                ThreadPool.QueueUserWorkItem((returnState) => { callback(null); });
                return null;
            }

            public void EndNoLock(IAsyncResult result)
            {
            }

            [ReplicateOnExit(Async = true)]
            [StateLock(StateLock1, true)]
            public IAsyncResult BeginWriteLock(AsyncCallback callback, object state)
            {
                this.x++;
                ThreadPool.QueueUserWorkItem((returnState) => { callback(null); });
                return null;
            }

            public void EndWriteLock(IAsyncResult result)
            {
            }

            [ReplicateOnExit(Async = true)]
            [StateLock(StateLock1, true)]
            [StateLock(StateLock2, false)]
            [StateLock(StateLock3, true)]
            public IAsyncResult BeginReadAndWriteLock(AsyncCallback callback, object state)
            {
                this.x++;
                ThreadPool.QueueUserWorkItem((returnState) => { callback(null); });
                return null;
            }

            public void EndReadAndWriteLock(IAsyncResult result)
            {
            }

            [ReplicateOnExit(Async = true)]
            public Task TaskNoLockAsync(object state)
            {
                this.x++;
                return Task<object>.Factory.StartNew(() => { return new object(); });
            }

            [ReplicateOnExit(Async = true)]
            [StateLock(StateLock1, true)]
            public Task TaskWriteLockAsync(object state)
            {
                this.x++;
                return Task<object>.Factory.StartNew(() => { return new object(); });
            }

            [ReplicateOnExit(Async = true)]
            [StateLock(StateLock1, true)]
            [StateLock(StateLock2, false)]
            [StateLock(StateLock3, true)]
            public Task TaskReadAndWriteLockAsync(object state)
            {
                this.x++;
                return Task<object>.Factory.StartNew(() => { return new object(); });
            }
        }

        private class TestStatefulServiceReplica : StatefulServiceReplica
        {
            public TestStatefulServiceReplica()
            {
                this.Name = "TestStatefulServiceReplica";
            }
        }

        private class TestStateReplicator : StateReplicator
        {
            public ReplicableMethodInfo ReplicableMethodInfo
            {
                get;
                set;
            }

            public int NumberOfExpectedReadLocks
            {
                get;
                set;
            }

            public int NumberOfExpectedWriteLocks
            {
                get;
                set;
            }

            public override NativeCommon.IFabricAsyncOperationContext BeginReplicate(NativeRuntime.IFabricOperationData operationData, NativeCommon.IFabricAsyncOperationCallback callback, out Int64 sequenceNumber)
            {
                ValidateStateLocks();
                return base.BeginReplicate(operationData, callback, out sequenceNumber);
            }

            private void ValidateStateLocks()
            {
                IEnumerable<Tuple<StateLock, bool>> stateLocks = this.ReplicableMethodInfo.TestStateLocks;

                int numberOfReadLocks = 0;
                int numberOfWriteLocks = 0;

                if (stateLocks != null)
                {
                    foreach (Tuple<StateLock, bool> lockTuple in stateLocks)
                    {
                        StateLock stateLock = lockTuple.Item1;
                        bool isWriteLock = lockTuple.Item2;
                        if (isWriteLock == false)
                        {
                            Assert.IsTrue(stateLock.TestNumberOfReaders >= 1, "No readers found");
                            Assert.AreEqual(0, stateLock.TestNumberOfWaitingReaders, "Unexpected number of waiting readers");
                            numberOfReadLocks++;
                        }
                        else
                        {
                            Assert.IsTrue(stateLock.TestIsWriterLockHeld, "Writer lock not held");
                            Assert.AreEqual(0, stateLock.TestNumberOfWaitingWriters, "Unexpected number of waiting writers");
                            numberOfWriteLocks++;
                        }
                    }
                }

                Assert.AreEqual(this.NumberOfExpectedReadLocks, numberOfReadLocks, "Unexpected number of read locks");
                Assert.AreEqual(this.NumberOfExpectedWriteLocks, numberOfWriteLocks, "Unexpected number of write locks");
            }
        }

        private class TestNativePartition : StatefulNativePartition
        {
            public TestStateReplicator StateReplicator
            {
                get;
                set;
            }

            public override NativeRuntime.IFabricStateReplicator CreateReplicator(NativeRuntime.IFabricStateProvider stateProvider, IntPtr fabricReplicatorSettings, out NativeRuntime.IFabricReplicator replicator)
            {
                replicator = new NativeReplicator();
                this.StateReplicator = new TestStateReplicator();
                return this.StateReplicator;
            }
        }
    }
}