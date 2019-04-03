// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Globalization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading;

    [TestClass]
    public class AzureParallelInfrastructureCoordinatorTest
    {
        private sealed class DocRawResponse
        {
            public string DocumentBase64 { get; set; }
        }

        [TestMethod]
        public void GetDocRawTest()
        {
            string result = new TestCommandExecutor().RunCommand(false, "GetDocRaw");

            // MockPolicyAgentService.GetDocumentRawAsync just returns { 1, 2, 3, 4, 5 } for now
            Assert.AreEqual("AQIDBAU=", result.FromJson<DocRawResponse>().DocumentBase64);
        }

        [TestMethod]
        public void RunAsyncTest1()
        {
            try
            {
                new WorkflowRunAsyncTest1().Execute();
                Constants.TraceType.ConsoleWriteLine("Cancellation of coordinator.RunAsync successful");
            }
            catch (Exception ex)
            {
                Assert.Fail(
                    string.Format(CultureInfo.InvariantCulture, "Exception unexpected during normal flow of execution till cancellation. Exception: {0}", ex));
            }
        }

        [TestMethod]
        public void RunAsyncTest2()
        {
            new WorkflowRunAsyncTest2().Execute();
        }

        [TestMethod]
        public void RunAsyncTest3()
        {
            new WorkflowRunAsyncTest3().Execute();
        }

        [TestMethod]
        public void RunAsyncTest4()
        {
            new WorkflowRunAsyncTest4().Execute();
        }

        [TestMethod]
        public void RunAsyncTest5()
        {
            new WorkflowRunAsyncTest5().Execute();
        }

        [TestMethod]
        public void RunAsyncRetryLogicTest()
        {
            new WorkflowRunAsyncRetryLogicTest().Execute();
        }

        [TestMethod]
        public void HIDetection1Test()
        {
            new WorkflowHIDetection1Test().Execute();
        }

        [TestMethod]
        public void HIDetection2Test()
        {
            new WorkflowHIDetection2Test().Execute();
        }

        [TestMethod]
        public void HIDetection3Test()
        {
            new WorkflowHIDetection3Test().Execute();
        }

        [TestMethod]
        public void ExternalRepairTaskTest()
        {
            new WorkflowExternalRepairTaskTest().Execute();
        }

        /// <summary>
        /// Only for manual debugging. <see cref="WorkflowExternalRepairTaskWithFCNotRespondingTest"/>
        /// </summary>
        public void ExternalRepairTaskWithFCNotRespondingTest()
        {
            new WorkflowExternalRepairTaskWithFCNotRespondingTest().Execute();
        }

        private class WorkflowRunAsyncTest1 : BaseWorkflowExecutor
        {
        }

        private class TestCommandExecutor : BaseWorkflowExecutor
        {
            public string RunCommand(bool isAdmin, string commandText)
            {
                return this.Coordinator.RunCommandAsync(isAdmin, commandText, FabricClient.DefaultTimeout, CancellationToken.None).GetAwaiter().GetResult();
            }
        }
    }
}