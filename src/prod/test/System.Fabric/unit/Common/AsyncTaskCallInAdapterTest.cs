// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AsyncTaskCallInAdapterTest
    {
        private AsyncTaskCallInAdapter.ReleaseComObjectWrapper initial;

        private ReleaseComObjectWrapperStub releaseComObjectStub;
        private NativeStub stub;
        private Task baseTask;
        private ManualResetEventSlim asyncEvent;

        private void WaitUntilHelper(Func<bool> func)
        {
            Assert.IsTrue(MiscUtility.WaitUntil(func, TimeSpan.FromSeconds(15), 50), "WaitUntilHelper");
        }

        [TestInitialize]
        public void Initialize()
        {
            this.initial = AsyncTaskCallInAdapter.ReleaseComObjectWrapperInstance;
            Assert.IsInstanceOfType(this.initial, typeof(AsyncTaskCallInAdapter.ReleaseComObjectWrapper), "Release Com Object Wrapper Instance on the call in adapter is not set");

            this.releaseComObjectStub = new ReleaseComObjectWrapperStub();
            AsyncTaskCallInAdapter.ReleaseComObjectWrapperInstance = this.releaseComObjectStub;

            this.stub = new NativeStub();
            this.asyncEvent = new ManualResetEventSlim();
            this.asyncEvent.Reset();
            this.baseTask = Task.Factory.StartNew(() => this.asyncEvent.Wait());
        }

        [TestCleanup]
        public void Cleanup()
        {
            AsyncTaskCallInAdapter.ReleaseComObjectWrapperInstance = this.initial;
            this.initial = null;
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void IsCompletedFlagIsSetCorrectly()
        {
            // This test asserts that the IsCompleted flag on the implementation of IAsyncOperationContext
            // on the AsyncTaskCallInAdapter is set to true when the task that the user has supplied is completed
            // regardless of whether the callback is compelte or not
            var adapter = this.CreateAdapter(this.stub, this.baseTask);

            // setup a callback which blocks
            ManualResetEventSlim ev = new ManualResetEventSlim(false);
            this.stub.OnInvokeAction = (c) =>
                {
                    ev.Wait();
                };

            // the basetask is waiting for the event to be set so it is not complete
            Assert.IsFalse(NativeTypes.FromBOOLEAN(adapter.IsCompleted()), "IsCompleted was set to true without the task being completed");

            // set the event - this should complete the base task (user task)
            LogHelper.Log("Completing task");
            this.asyncEvent.Set();

            // This is a boolean wait to work around a CLR issue where an infinite basetask wait happens even though the task is complete
            bool waitCompleted = this.baseTask.Wait(10000);
            LogHelper.Log("Task.IsCompleted: {0}. Task Status: {1}. WaitCompletion: {2}", this.baseTask.IsCompleted, this.baseTask.Status, waitCompleted);

            // wait for a little bit -> the adapter.IsCompleted should become true
            // the callback right now is blocked on the event "ev"
            this.WaitUntilHelper(() => NativeTypes.FromBOOLEAN(adapter.IsCompleted()));

            // complete the callback
            ev.Set();
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void PassingInACompletedTaskCallsCallbackWithCompletedSynchronouslyAsTrue()
        {
            this.asyncEvent.Set();
            this.baseTask.Wait();

            // completed synchronously should be true in the callback
            bool completedSynchronouslyFromCallback = false;
            this.stub.OnInvokeAction = (c) => completedSynchronouslyFromCallback = NativeTypes.FromBOOLEAN(c.CompletedSynchronously());

            var adapter = this.CreateAdapter(this.stub, this.baseTask);

            this.WaitUntilHelper(() => completedSynchronouslyFromCallback);

            // completed synchronously should be true in the adapter as well
            Assert.IsTrue(NativeTypes.FromBOOLEAN(adapter.CompletedSynchronously()));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void PassingInACompletedTaskSetsCompletedSynchronouslyToTrue()
        {
            this.asyncEvent.Set();
            this.baseTask.Wait();

            var adapter = this.CreateAdapter(null, this.baseTask);

            Assert.IsTrue(NativeTypes.FromBOOLEAN(adapter.CompletedSynchronously()));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void PassingInANonCompletedTaskSetsCompletedSynchronouslyToFalse()
        {
            var adapter = this.CreateAdapter(null, this.baseTask);

            Assert.IsFalse(NativeTypes.FromBOOLEAN(adapter.CompletedSynchronously()));

            this.asyncEvent.Set();
            this.baseTask.Wait();

            Assert.IsFalse(NativeTypes.FromBOOLEAN(adapter.CompletedSynchronously()));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CallbackStateIsCorrect()
        {
            var adapter = this.CreateAdapter(null, this.baseTask);
            Assert.IsNull(adapter.get_Callback());

            var adapter2 = this.CreateAdapter(this.stub, this.baseTask);
            Assert.AreSame(this.stub, adapter2.get_Callback());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CallbackIsInvokedWhenTaskCompletes()
        {
            var adapter = this.CreateAdapter(this.stub, this.baseTask);

            NativeCommon.IFabricAsyncOperationContext context = null;
            this.stub.OnInvokeAction = (c) => context = c;

            // complete the task
            this.asyncEvent.Set();

            // wait for a little bit -> the callback should be invoked
            this.WaitUntilHelper(() => context != null);

            Assert.AreSame(adapter, context);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void CallbackIsInvokedWhenACompletedTaskIsPassedIn()
        {
            // complete the task
            this.asyncEvent.Set();

            // setup the cb
            bool wasInvoked = false;
            this.stub.OnInvokeAction = (c) =>
                {
                    // make sure that the continuation is accessible                    
                    wasInvoked = true;
                };

            var adapter = this.CreateAdapter(this.stub, this.baseTask);

            this.WaitUntilHelper(() => wasInvoked != false);
        }

        private void CallbackIsReleasedTestHelper(bool completeSynchronously)
        {
            if (completeSynchronously)
            {
                this.asyncEvent.Set();
            }

            AsyncTaskCallInAdapter adapter = null;

            this.stub.OnInvokeAction = (c) =>
            {
                // during execution of the callback the NativeCallback object cannot be released
                // assert that that is true
                Assert.AreEqual(0, this.releaseComObjectStub.ReleaseCalls.Count);
            };

            adapter = this.CreateAdapter(this.stub, this.baseTask);

            // complete asynchrnously
            this.asyncEvent.Set();

            // wait until the task completes
            this.WaitUntilHelper(() => NativeTypes.FromBOOLEAN(adapter.IsCompleted()));

            // at this time, the callback should have completed and released
            Assert.AreEqual(1, this.releaseComObjectStub.ReleaseCalls.Count);
            Assert.AreSame(this.stub, this.releaseComObjectStub.ReleaseCalls[0]);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CallbackIsReleasedTest_CompletedSynchronously()
        {
            this.CallbackIsReleasedTestHelper(true);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void CallbackIsReleasedTest_CompletedAsynchronously()
        {
            this.CallbackIsReleasedTestHelper(false);
        }


        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndTResultReturnsResult()
        {
            int result = 5;

            var task = this.baseTask.ContinueWith<int>((t) => result);

            var adapter = this.CreateAdapter(null, task);

            this.asyncEvent.Set();

            Assert.AreEqual<int>(result, AsyncTaskCallInAdapter.End<int>(adapter));
        }

        private void EndCanBeCalledFromOnCompleteHelper(Task t, Action<AsyncTaskCallInAdapter> endInvoker)
        {
            var adapter = this.CreateAdapter(this.stub, t);

            bool callbackCompleted = false;
            this.stub.OnInvokeAction = (c) =>
            {
                endInvoker(adapter);
                callbackCompleted = true;
            };

            this.asyncEvent.Set();

            this.WaitUntilHelper(() => callbackCompleted);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndCanBeCalledFromOnComplete()
        {
            this.EndCanBeCalledFromOnCompleteHelper(this.baseTask, (ad) => AsyncTaskCallInAdapter.End(ad));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndTResultCanBeCalledFromOnComplete()
        {
            var task = this.baseTask.ContinueWith<int>(t => 3);

            this.EndCanBeCalledFromOnCompleteHelper(task, (ad) => AsyncTaskCallInAdapter.End<int>(ad));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void EndTranslatesExceptions()
        {
            var adapter = this.CreateAdapter(null, this.baseTask.ContinueWith((t) => { throw new ArgumentNullException(); }));

            this.asyncEvent.Set();

            try
            {
                AsyncTaskCallInAdapter.End(adapter);
                Assert.Fail("Expected ex");
            }
            catch (ArgumentNullException)
            {
            }
        }

        private AsyncTaskCallInAdapter CreateAdapter(NativeCommon.IFabricAsyncOperationCallback callback, Task userTask)
        {
            return new AsyncTaskCallInAdapter(callback, userTask, InteropApi.Default);
        }

        class NativeStub : NativeCommon.IFabricAsyncOperationCallback
        {
            public Action<NativeCommon.IFabricAsyncOperationContext> OnInvokeAction { get; set; }

            public void Invoke(NativeCommon.IFabricAsyncOperationContext context)
            {
                if (this.OnInvokeAction != null)
                {
                    this.OnInvokeAction(context);
                }
            }
        }

        class ReleaseComObjectWrapperStub : AsyncTaskCallInAdapter.ReleaseComObjectWrapper
        {
            public ReleaseComObjectWrapperStub()
            {
                this.ReleaseCalls = new List<NativeCommon.IFabricAsyncOperationCallback>();
            }

            public IList<NativeCommon.IFabricAsyncOperationCallback> ReleaseCalls { get; private set; }

            public override void ReleaseComObject(NativeCommon.IFabricAsyncOperationCallback comObject)
            {
                this.ReleaseCalls.Add(comObject);
            }
        }
    }
}