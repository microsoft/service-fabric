// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System.Threading;
    using System.Threading.Tasks;
    using Collections.Generic;
    using Diagnostics;

    /// <summary>
    /// Interface used to go over the workflow and simulate interaction of various components involved in the 
    /// Infrastructure Service (IS) state machine. The components are Cluster Manager (CM) and Management Role (MR).
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
        private static readonly TraceType traceType = new TraceType(typeof(BaseWorkflowExecutor).Name);

        private readonly Stopwatch stopwatch = Stopwatch.StartNew();

        protected BaseWorkflowExecutor(IDictionary<string, string> configSettings = null, MockQueryClient queryClient = null)
        {
            ManagementClient = new MockManagementClient();
            QueryClient = queryClient;
            InfrastructureAgent = new MockInfrastructureAgentWrapper();
            Coordinator = new MockInfrastructureCoordinatorFactory(ManagementClient, configSettings, queryClient, InfrastructureAgent).Create() as WindowsAzureInfrastructureCoordinator;
            NotificationContext = new MockManagementNotificationContext();            
            CompletedEvent = new ManualResetEvent(false);

            TestExecutionTimeInSeconds = 300;
        }

        protected WindowsAzureInfrastructureCoordinator Coordinator { get; private set; }

        protected MockManagementClient ManagementClient { get; private set; }

        protected IQueryClient QueryClient { get; set; }

        protected MockInfrastructureAgentWrapper InfrastructureAgent { get; private set; }

        protected ManualResetEvent CompletedEvent { get; private set; }

        protected MockManagementNotificationContext NotificationContext { get; private set; }

        protected int TestExecutionTimeInSeconds { get; set; }

        public virtual void Execute()
        {
            var cts = new CancellationTokenSource();

            Coordinator.RunAsync(0, cts.Token);

            CompletedEvent.WaitOne(TimeSpan.FromSeconds(TestExecutionTimeInSeconds));

            cts.Cancel();

            // Wait for the IS to return gracefully
            Task.Delay(TimeSpan.FromSeconds(2)).Wait();

            traceType.ConsoleWriteLine("Completed executing workflow. Time taken: {0}", stopwatch.Elapsed);
        }
    }
}