// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using Diagnostics;
    using InfrastructureService.Test;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading;
    using Threading.Tasks;

    /// <summary>
    /// Interface used to go over the workflow and simulate interaction of various components involved in the 
    /// Infrastructure Service (IS) state machine. The components are Repair Manager (RM) and Management Role (MR).
    /// </summary>
    internal interface IWorkflowExecutor
    {
        /// <summary>
        /// Executes the workflow.
        /// </summary>
        void Execute();
    }

    internal abstract class BaseWorkflowExecutor : IWorkflowExecutor
    {
        private static readonly TraceType traceType = new TraceType(Constants.TraceTypeName);

        private readonly Stopwatch stopwatch = Stopwatch.StartNew();

        protected BaseWorkflowExecutor(
            IDictionary<string, string> configSettings = null,
            MockPolicyAgentService policyAgentService = null,
            MockRepairManager repairManager = null)
        {
            PolicyAgentService = policyAgentService ?? new MockPolicyAgentService();

            var activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);

            var env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                new ConfigSection(Constants.TraceType, new MockConfigStore(), "ConfigSection"),
                string.Empty,
                new MockInfrastructureAgentWrapper());

            var policyAgentClient = new PolicyAgentClient(env, PolicyAgentService, activityLogger);

            RepairManager = repairManager ?? new MockRepairManagerFactory().Create() as MockRepairManager;

            Coordinator = new MockInfrastructureCoordinatorFactory(
                configSettings, 
                policyAgentClient, 
                RepairManager).Create() as AzureParallelInfrastructureCoordinator;            

            CompletedEvent = new ManualResetEvent(false);
            
            TestExecutionTimeInSeconds = 300;
        }

        protected MockRepairManager RepairManager { get; private set; }

        protected AzureParallelInfrastructureCoordinator Coordinator { get; private set; }
        
        protected MockPolicyAgentService PolicyAgentService { get; private set; }

        protected ManualResetEvent CompletedEvent { get; private set; }
        
        protected int TestExecutionTimeInSeconds { get; set; }

        /// <summary>
        /// Cannot use await calls here since this is used directly by TAEF test methods which don't support that
        /// </summary>
        public virtual void Execute()
        {
            var cts = new CancellationTokenSource();

            ManualResetEventSlim coordinatorStopped = new ManualResetEventSlim();

            Task<int> t1 = Task.Run(() =>
            {
                Coordinator.RunAsync(0, cts.Token).ContinueWith(_ => coordinatorStopped.Set());
                Run(cts.Token);                
                return 0;
            });

            Task<int> t2 = Task.Run(() =>
            {
                Thread.Sleep(TimeSpan.FromSeconds(TestExecutionTimeInSeconds));                
                return -1;
            });

            Task<int> t = Task.WhenAny(t1, t2).GetAwaiter().GetResult();
            int status = t.GetAwaiter().GetResult();
                                    
            cts.Cancel();

            // Wait for the IS to return gracefully
            Assert.IsTrue(coordinatorStopped.Wait(TimeSpan.FromSeconds(5)), "Verify if coordinator stopped gracefully");

            Assert.AreEqual(status, 0, "Verifying if test completed execution and didn't time out");

            traceType.ConsoleWriteLine("Completed executing workflow. Time taken: {0}", stopwatch.Elapsed);
        }

        /// <summary>
        /// Override in derived classes for functionality outside of the coordinator
        /// </summary>
        protected virtual void Run(CancellationToken cancellationToken)
        {
            CompletedEvent.Set();
        }
    }
}