// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class StatefulServiceReplicaBrokerTests
    {
        private const int TimeoutMilliseconds = 20000;

        //[TestMethod]
        //[TestProperty("ThreadingModel", "MTA")]
        //public void DataLoss()
        //{
        //    TestStatefulServiceReplica service = new TestStatefulServiceReplica();
        //    StatefulServiceReplicaBroker broker = new StatefulServiceReplicaBroker(service);
        //    ManualResetEvent waitEvent = new ManualResetEvent(false);
        //    NativeCommon.IFabricAsyncOperationContext context = ((NativeRuntime.IFabricStateProvider)broker).BeginOnDataLoss(new AsyncOperationCallback(() =>
        //        {
        //            waitEvent.Set();
        //        }));
            
        //    Assert.IsTrue(waitEvent.WaitOne(StatefulServiceReplicaBrokerTests.TimeoutMilliseconds), "BeginOnDataLoss timed out");
        //    Assert.IsFalse(((AsyncResult<bool>)((AsyncCallInAdapter)context).InnerResult).Result, "Unexpected value");
        //}

        //private class TestStatefulServiceReplica : StatefulServiceReplica
        //{
        //    public TestStatefulServiceReplica()
        //    {
        //        this.Name = "TestStatefulServiceReplica";
        //    }
        //}

        //private class AsyncOperationCallback : NativeCommon.IFabricAsyncOperationCallback
        //{
        //    private Action action;

        //    public AsyncOperationCallback(Action action)
        //    {
        //        this.action = action;
        //    }

        //    public void Invoke(NativeCommon.IFabricAsyncOperationContext context)
        //    {
        //        this.action();
        //    }
        //}
    }
}