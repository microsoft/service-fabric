// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Collections.Generic;
    using Globalization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;
    using Threading;
    using Threading.Tasks;
    using WEX.TestExecution;

    [TestClass]
    public class WindowsAzureInfrastructureCoordinatorTest
    {
        private const string ConfigAllowCompletedJobStepReAckKeyName = "WindowsAzure.AllowCompletedJobStepReAck";
        private const string ConfigRecentJobStateUpdateThresholdInSecondsKeyName = "WindowsAzure.RecentJobStateUpdateThresholdInSeconds";

        private static readonly TraceType TraceType = new TraceType(typeof(WindowsAzureInfrastructureCoordinatorTest).Name);

        #region unit test methods

        [TestMethod]
        public void ConstructorTest1()
        {
            var coordinator1 = new MockInfrastructureCoordinatorFactory(new MockManagementClient()).Create();

            Verify.IsNotNull(coordinator1, "Successfully created a real coordinator tuple passing in mock objects");            

            Verify.Throws<DeploymentManagementEndpointNotFoundException>(() =>
                {
                    var managementClient = new WindowsAzureManagementClient();
                    new MockInfrastructureCoordinatorFactory(managementClient).Create();
                },
                "Exception thrown as expected while creating management client since MR is not enabled in unit-test mode");
        }

        [TestMethod]
        public void RunAsyncTest1()
        {
            try
            {
                new WorkflowRunAsyncTest1().Execute();
                Trace.ConsoleWriteLine(TraceType, "Cancellation of coordinator.RunAsync successful");
            }
            catch (Exception ex)
            {
                Verify.Fail(
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

        /// <summary>
        /// Currently a simple sanity check for testing command handling.
        /// </summary>
        [TestMethod]
        public void HandleCommands1()
        {
            var coordinator1 = new MockInfrastructureCoordinatorFactory(new MockManagementClient()).Create();

            {
                var result =
                    coordinator1.RunCommandAsync(false, "GetRoleInstances", TimeSpan.MaxValue,
                        CancellationToken.None).Result;
                Verify.IsNotNull(result, "GetRoleInstances returned a valid string");
            }

            {
                var result = coordinator1.RunCommandAsync(false, "GetJobs", TimeSpan.MaxValue,
                        CancellationToken.None).Result;
                Verify.IsNotNull(result, "GetJobs returned a valid string");
            }

            {
                var result =
                    coordinator1.RunCommandAsync(false, "GetCurrentState", TimeSpan.MaxValue,
                        CancellationToken.None).Result;
                Verify.IsNotNull(result, "GetCurrentState returned a valid string");
            }

            {
                try
                {
                    // a common typo - GetCurrentStatus instead of GetCurrentState
                    var result =
                        coordinator1.RunCommandAsync(false, "GetCurrentStatus", TimeSpan.MaxValue,
                            CancellationToken.None).Result;
                    Verify.Fail("Exception should have been thrown due to an invalid command");
                }
                catch (AggregateException ae)
                {
                    ae.Flatten().Handle(ex =>                    
                    {
                        if (ex is ArgumentException)
                        {
                            Trace.ConsoleWriteLine(TraceType, "Handling expected exception: {0}", ex);
                            return true;
                        }

                        return false;
                    });
                }
            }
        }

        [TestMethod]
        public void VendorRepairBeginTest()
        {
            new WorkflowRunAsyncVendorRepairTest().Execute();
        }

        [TestMethod]
        public void FCMissesPostUDNotificationTest()
        {
            new WorkflowRunAsyncFCMissesPostUDNotification().Execute();
        }

        [TestMethod]
        public void FCMissesPostUDNotificationTestAndIsReAcked()
        {
            new WorkflowRunAsyncFCMissesPostUDNotificationAndIsReAcked().Execute();
        }

        #endregion unit test methods

        #region workflows for RunAsync tests

        private class WorkflowRunAsyncTest1 : BaseWorkflowExecutor
        {
            public WorkflowRunAsyncTest1()
            {
                CompletedEvent.Set();
            }            
        }

        /// <summary>
        /// Wraps a simple workflow by mocking cluster manager (CM) and MR. This tests the state machine changes inside of IS. 
        /// This is a happy path scenario. i.e.
        /// IS polls and gets a new MR job notification --> IS consults CM --> CM approves it --> IS sends SignalReady to MR 
        /// --> [after a while] MR comes back with job step complete --> IS checks with CM again to know if Azure's actions 
        /// are okay (reboot) etc --> CM says it was successful --> IS reports another SignalReady to MR
        /// </summary>
        private class WorkflowRunAsyncTest2 : BaseWorkflowExecutor
        {                        
            private State state;

            private int signalReadyHitCounter;

            /// <summary>
            /// Represents flow of control across the various components (CM, MR etc.)
            /// </summary>
            private enum State
            {
                Unknown,
                FromMRStartingJob,
                FromCMStartedJob,
                FromMRCompletedJob,
                FromCMAckedCompletedJob,
            }

            public WorkflowRunAsyncTest2()
            {
                ManagementClient.GetCurrentNotificationFunc = GetCurrentNotification;
                NotificationContext.SignalReadyAction = SignalReady;
            }

            private MockManagementNotificationContext GetCurrentNotification()
            {
                switch (state)
                {
                    case State.Unknown:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = StartJobStep to simulate start of a job");
                        NotificationContext.NotificationType = NotificationType.StartJobStep;
                        state = State.FromMRStartingJob;
                        break;

                    case State.FromMRStartingJob:
                        // called 2nd time we enter this method after the StartJobStep part above
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportStartTaskSuccessAsync to change job state from WaitingForApproval to Executing inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.StartJobStep;
                                Coordinator.ReportStartTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMStartedJob;
                        break;

                    case State.FromCMStartedJob:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job");
                        NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                        state = State.FromMRCompletedJob;
                        break;

                    case State.FromMRCompletedJob:
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportFinishTaskSuccessAsync to change job state from WaitingForHealthCheck to Idle inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                                Coordinator.ReportFinishTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMAckedCompletedJob;
                        break;
                }

                return NotificationContext;
            }

            private void SignalReady()
            {
                switch (signalReadyHitCounter)
                {
                    case 0:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for giving approval to FC's job.");
                        signalReadyHitCounter++;
                        break;

                    case 1:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady after job execution and completion of health check from CM.");
                        Verify.AreEqual(state, State.FromCMAckedCompletedJob,
                            string.Format(CultureInfo.InvariantCulture, "SignalReady invoked after after health checks from CM to ack completed job: {0}", state));

                        CompletedEvent.Set();

                        break;
                }
            }
        }

        /// <summary>
        /// Wraps a simple workflow by mocking cluster manager (CM) and MR. This tests the state machine changes inside of IS.        
        /// In this case, CM reports a task failure which results in SignalError being called from IS to MR.
        /// </summary>
        /// <seealso cref="WorkflowRunAsyncTest2"/>
        private class WorkflowRunAsyncTest3 : BaseWorkflowExecutor
        {
            private enum State
            {
                Unknown,
                FromMRStartingJob,
                FromCMStartedJob,
                FromMRCompletedJob,
                FromCMHealthCheckFailed,                
            }

            private State state;

            public WorkflowRunAsyncTest3()
            {
                ManagementClient.GetCurrentNotificationFunc = GetCurrentNotification;
                NotificationContext.SignalReadyAction = SignalReady;
                NotificationContext.SignalErrorAction = SignalError;
            }

            public MockManagementNotificationContext GetCurrentNotification()
            {                
                switch (state)
                {
                    case State.Unknown:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = StartJobStep to simulate start of a job");
                        NotificationContext.NotificationType = NotificationType.StartJobStep;
                        state = State.FromMRStartingJob;
                        break;

                    case State.FromMRStartingJob:
                        // called 2nd time we enter this method after the StartJobStep part above
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportStartTaskSuccessAsync to change job state from WaitingForApproval to Executing inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.StartJobStep;
                                Coordinator.ReportStartTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMStartedJob;
                        break;

                    case State.FromCMStartedJob:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job");
                        NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                        state = State.FromMRCompletedJob;
                        break;

                    case State.FromMRCompletedJob:
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportTaskFailureAsync to change job state from WaitingForHealthCheck to Idle inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                                Coordinator.ReportTaskFailureAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMHealthCheckFailed;
                        break;
                }

                return NotificationContext;
            }

            private static void SignalReady()
            {
                Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for giving approval to FC's job.");
            }

            private void SignalError(string errorDescription)
            {
                Trace.ConsoleWriteLine(TraceType,
                    "IS: Invoking SignalError after job execution and task failure from CM. Error description: {0}", errorDescription);
                Verify.AreEqual(state, State.FromCMHealthCheckFailed,
                    string.Format(CultureInfo.InvariantCulture, "SignalError invoked after health checks from CM failed. Workflow state: {0}", state));

                CompletedEvent.Set();
            }
        }

        /// <summary>
        /// Wraps a simple workflow by mocking cluster manager (CM) and MR. This tests the state machine changes inside of IS.        
        /// In this case, 
        /// a. MR initiates a vendor repair begin job that needs to be stalled so that an operator can intervene
        /// b. Then MR initiates a normal start job step that is approved via CM.
        /// c. Then MR sends a vendor repair end job that again needs manual intervention (via a SignalReady)
        /// d. Then MR sends a normal complete job step that again goes through CM which reports success
        /// </summary>        
        private class WorkflowRunAsyncVendorRepairTest : BaseWorkflowExecutor
        {
            private int vendorRepairNotificationCounter;

            private int signalReadyHitCounter;

            private enum State
            {
                FromMRInitialVendorRepairBegin,
                FromISSendSignalReady,
                ReadyForNextJob,
                FromMRStartingJob,
                FromMRVendorRepairEnd,
                FromISSendSignalReady2,
                FromMRNewCompletedJob,
                FromCMAckedCompletedJob,
            }

            private State state;

            public WorkflowRunAsyncVendorRepairTest()
            {
                // We are looping a few times to mock a manual intervention. So, set a larger timeout.
                TestExecutionTimeInSeconds = 120;

                ManagementClient.GetCurrentNotificationFunc = GetCurrentNotification;
                NotificationContext.SignalReadyAction = SignalReady;                
            }

            public MockManagementNotificationContext GetCurrentNotification()
            {
                switch (state)
                {
                    case State.FromMRInitialVendorRepairBegin:
                        Trace.ConsoleWriteLine(TraceType, "MR: Sending a job to IS with impact reason {0}", ImpactReason.VendorRepairBegin);
                        NotificationContext.NotificationType = NotificationType.StartJobStep;
                        NotificationContext.ImpactedInstances = new List<ImpactedInstance>
                        {
                            new ImpactedInstance("RoleA_IN_0", new List<ImpactReason> { ImpactReason.VendorRepairBegin })
                        };

                        vendorRepairNotificationCounter ++;

                        // At this point, the operator has to intervene with a signal ready.
                        // We are simulating this by looping a few times in the same state in this switch-case.
                        if (vendorRepairNotificationCounter >= 3)
                        {
                            state = State.FromISSendSignalReady;
                            vendorRepairNotificationCounter = 0;
                        }

                        break;

                    case State.FromISSendSignalReady:                        
                        Trace.ConsoleWriteLine(TraceType, "IS: Manually issuing a SignalReady to unblock manual approval job (vendor repair begin job)");

                        NotificationContext.SignalReady();

                        state = State.ReadyForNextJob;
                        break;

                    case State.ReadyForNextJob:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = StartJobStep to simulate start of a job with an impact reason not requiring manual approval");

                        NotificationContext.NotificationType = NotificationType.StartJobStep;
                        NotificationContext.ImpactedInstances = new List<ImpactedInstance>
                        {
                            new ImpactedInstance("RoleA_IN_1", new List<ImpactReason> { ImpactReason.Reboot })
                        };

                        state = State.FromMRStartingJob;
                        break;

                    case State.FromMRStartingJob:
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportStartTaskSuccessAsync to change job state from WaitingForApproval to Executing inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.StartJobStep;
                                Coordinator.ReportStartTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromMRVendorRepairEnd;
                        break;

                    case State.FromMRVendorRepairEnd:
                        Trace.ConsoleWriteLine(TraceType, "MR: Sending a new job to IS with impact reason {0}", ImpactReason.VendorRepairEnd);

                        NotificationContext.NotificationType = NotificationType.StartJobStep; // TODO or is it a CompleteJobStep
                        NotificationContext.ImpactedInstances = new List<ImpactedInstance>
                        {
                            new ImpactedInstance("RoleA_IN_0", new List<ImpactReason> { ImpactReason.VendorRepairEnd })
                        };

                        vendorRepairNotificationCounter++;

                        if (vendorRepairNotificationCounter >= 3)
                        {
                            state = State.FromISSendSignalReady2;
                            vendorRepairNotificationCounter = 0;
                        }

                        break;

                    case State.FromISSendSignalReady2:
                        Trace.ConsoleWriteLine(TraceType, "IS: Manually issuing a SignalReady again to unblock manual approval job (vendor repair end job)");

                        NotificationContext.SignalReady();

                        state = State.FromMRNewCompletedJob;
                        break;

                    case State.FromMRNewCompletedJob:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job with an impact reason not requiring manual approval");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                                Coordinator.ReportFinishTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMAckedCompletedJob;
                        break;
                }

                return NotificationContext;
            }

            private void SignalReady()
            {
                switch (signalReadyHitCounter)
                {
                    case 0:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for MANUAL VendorRepairBegin job.");
                        signalReadyHitCounter++;
                        break;

                    case 1:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for giving approval to a normal FC's job's StartJobStep.");
                        signalReadyHitCounter++;
                        break;

                    case 2:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for MANUAL VendorRepairEnd job.");
                        signalReadyHitCounter++;

                        NotificationContext.ImpactedInstances = new List<ImpactedInstance>
                        {
                            new ImpactedInstance("RoleA_IN_1", new List<ImpactReason> { ImpactReason.Reboot })
                        };

                        break;

                    case 3:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for giving approval to a normal FC's job's CompleteJobStep.");
                        signalReadyHitCounter++;

                        CompletedEvent.Set();
                        break;

                }
            }
        }

        /// <summary>
        /// Wraps a simple workflow by mocking cluster manager (CM) and MR. This tests the state machine changes inside of IS. 
        /// IS polls and gets a new MR job notification --> IS consults CM --> CM approves it --> IS sends SignalReady to MR 
        /// --> [after a while] MR comes back with job step complete --> IS checks with CM again to know if Azure's actions 
        /// are okay (reboot) etc --> CM says it was successful --> IS reports another SignalReady to MR (which goes to FC) -->
        /// FC misses this and sends one more CompleteJobStep --> IS doesn't expect this and expects manual operator approval
        /// to send another ack to FC.
        /// </summary>
        private class WorkflowRunAsyncFCMissesPostUDNotification : BaseWorkflowExecutor
        {
            private State state;

            private int signalReadyHitCounter;

            private int completeJobStepResendByFCCount;

            /// <summary>
            /// Represents flow of control across the various components (CM, MR etc.)
            /// </summary>
            private enum State
            {
                Unknown,
                FromMRStartingJob,
                FromCMStartedJob,
                FromMRCompletedJob,
                FromCMAckedCompletedJob,
                FromISSendSignalReady,
            }

            public WorkflowRunAsyncFCMissesPostUDNotification()
                : base(
                    new Dictionary<string, string>
                    {
                        // TODO, move all constants (in windowsazureinfrastructurecoordinator) to a constant class.                            
                        { ConfigAllowCompletedJobStepReAckKeyName, false.ToString() },
                        { ConfigRecentJobStateUpdateThresholdInSecondsKeyName, 3.ToString() }
                    })
            {
                ManagementClient.GetCurrentNotificationFunc = GetCurrentNotification;
                NotificationContext.SignalReadyAction = SignalReady;
            }

            private MockManagementNotificationContext GetCurrentNotification()
            {
                switch (state)
                {
                    case State.Unknown:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = StartJobStep to simulate start of a job");
                        NotificationContext.NotificationType = NotificationType.StartJobStep;
                        state = State.FromMRStartingJob;
                        break;

                    case State.FromMRStartingJob:
                        // called 2nd time we enter this method after the StartJobStep part above
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportStartTaskSuccessAsync to change job state from WaitingForApproval to Executing inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.StartJobStep;
                                Coordinator.ReportStartTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMStartedJob;
                        break;

                    case State.FromCMStartedJob:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job");
                        NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                        state = State.FromMRCompletedJob;
                        break;

                    case State.FromMRCompletedJob:
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportFinishTaskSuccessAsync to change job state from WaitingForHealthCheck to Idle inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                                Coordinator.ReportFinishTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMAckedCompletedJob;
                        break;

                    case State.FromCMAckedCompletedJob:
                        // MR/FC has missed the previous complete job step. So it sends it again
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job");
                        NotificationContext.NotificationType = NotificationType.CompleteJobStep;

                        completeJobStepResendByFCCount++;

                        if (completeJobStepResendByFCCount >= 3)
                        {
                            state = State.FromISSendSignalReady;
                        }
                        break;

                    case State.FromISSendSignalReady:
                        NotificationContext.SignalReady();
                        break;

                }

                return NotificationContext;
            }

            private void SignalReady()
            {
                switch (signalReadyHitCounter)
                {
                    case 0:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for giving approval to FC's job.");
                        signalReadyHitCounter++;
                        break;

                    case 1:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady after job execution and completion of health check from CM.");
                        Verify.AreEqual(state, State.FromCMAckedCompletedJob,
                            string.Format(CultureInfo.InvariantCulture, "SignalReady invoked after after health checks from CM to ack completed job: {0}", state));
                        signalReadyHitCounter++;
                        break;

                    case 2:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady 2nd time since MR/FC missed out the previous CompleteJobStep ack.");
                        Verify.AreEqual(state, State.FromISSendSignalReady,
                            string.Format(CultureInfo.InvariantCulture, "SignalReady invoked 2nd time to re-ack completed job: {0}", state));

                        CompletedEvent.Set();
                        break;
                }
            }
        }

        /// <summary>
        /// Wraps a simple workflow by mocking cluster manager (CM) and MR. This tests the state machine changes inside of IS. 
        /// This is a happy path scenario. i.e.
        /// IS polls and gets a new MR job notification --> IS consults CM --> CM approves it --> IS sends SignalReady to MR 
        /// --> [after a while] MR comes back with job step complete --> IS checks with CM again to know if Azure's actions 
        /// are okay (reboot) etc --> CM says it was successful --> IS reports another SignalReady to MR (which goes to FC) -->
        /// FC misses this and sends one more CompleteJobStep --> IS then sends another SignalReady from the re-ack logic --> 
        /// This time FC gets it.
        /// </summary>
        private class WorkflowRunAsyncFCMissesPostUDNotificationAndIsReAcked : BaseWorkflowExecutor
        {
            private State state;

            private int signalReadyHitCounter;

            /// <summary>
            /// Represents flow of control across the various components (CM, MR etc.)
            /// </summary>
            private enum State
            {
                Unknown,
                FromMRStartingJob,
                FromCMStartedJob,
                FromMRCompletedJob,
                FromCMAckedCompletedJob,
                FromMRCompletedJob2,
            }

            public WorkflowRunAsyncFCMissesPostUDNotificationAndIsReAcked()
                : base(                      
                    new Dictionary<string, string>
                    {
                        // TODO, move all constants (in windowsazureinfrastructurecoordinator) to a constant class.                            
                        { ConfigAllowCompletedJobStepReAckKeyName, true.ToString() },
                        { ConfigRecentJobStateUpdateThresholdInSecondsKeyName, 15.ToString() }

                    })
            {
                // We are waiting for a re-ack. So, set a larger timeout.
                TestExecutionTimeInSeconds = 120;

                ManagementClient.GetCurrentNotificationFunc = GetCurrentNotification;
                NotificationContext.SignalReadyAction = SignalReady;                                
            }

            private MockManagementNotificationContext GetCurrentNotification()
            {
                switch (state)
                {
                    case State.Unknown:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = StartJobStep to simulate start of a job");
                        NotificationContext.NotificationType = NotificationType.StartJobStep;
                        state = State.FromMRStartingJob;
                        break;

                    case State.FromMRStartingJob:
                        // called 2nd time we enter this method after the StartJobStep part above
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportStartTaskSuccessAsync to change job state from WaitingForApproval to Executing inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.StartJobStep;
                                Coordinator.ReportStartTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMStartedJob;
                        break;

                    case State.FromCMStartedJob:
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job");
                        NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                        state = State.FromMRCompletedJob;
                        break;

                    case State.FromMRCompletedJob:
                        Trace.ConsoleWriteLine(TraceType, "CM: ReportFinishTaskSuccessAsync to change job state from WaitingForHealthCheck to Idle inside state machine of WindowsAzureInfrastructureCoordinator");

                        Task.Factory.StartNew(
                            () =>
                            {
                                NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                                Coordinator.ReportFinishTaskSuccessAsync(Coordinator.TaskId, 0, TimeSpan.MaxValue,
                                    CancellationToken.None);
                            }).Wait();

                        state = State.FromCMAckedCompletedJob;
                        break;

                    case State.FromCMAckedCompletedJob:
                        // MR/FC has missed the previous complete job step. So it sends it again
                        Trace.ConsoleWriteLine(TraceType, "MR: Setting NotificationType = CompleteJobStep to simulate completion of a job");
                        NotificationContext.NotificationType = NotificationType.CompleteJobStep;
                        state = State.FromMRCompletedJob2;
                        break;
                }

                return NotificationContext;
            }

            private void SignalReady()
            {
                switch (signalReadyHitCounter)
                {
                    case 0:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady for giving approval to FC's job.");
                        signalReadyHitCounter++;
                        break;

                    case 1:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady after job execution and completion of health check from CM.");
                        Verify.AreEqual(state, State.FromCMAckedCompletedJob,
                            string.Format(CultureInfo.InvariantCulture, "SignalReady invoked after after health checks from CM to ack completed job: {0}", state));
                        signalReadyHitCounter++;
                        break;

                    case 2:
                        Trace.ConsoleWriteLine(TraceType, "IS: Invoking SignalReady 2nd time since MR/FC missed out the previous CompleteJobStep ack.");
                        Verify.AreEqual(state, State.FromMRCompletedJob2,
                            string.Format(CultureInfo.InvariantCulture, "SignalReady invoked 2nd time to re-ack completed job: {0}", state));
                        CompletedEvent.Set();
                        break;
                }
            }
        }

        #endregion workflows for RunAsync tests
    }
}