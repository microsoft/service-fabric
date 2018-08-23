// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Test
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Client;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Engine;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Actions.Steps;
    using Diagnostics;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;

    internal class ActionTest
    {
        internal const string FatalRollbackExceptionMessage = "Intentional Test Exception";
        private const string TraceType = "ActionTest";

        private readonly MockClient mockClient;
        private readonly ActionStore actionStore;
        private readonly IReliableStateManager stateManager;
        private IReliableDictionary<string, string> internalTestsStarted;

        private ActionTest(FaultAnalysisServiceMessageProcessor messageProcessor, ActionStore actionStore, IReliableStateManager stateManager, IReliableDictionary<string, string> internalTestsStarted)
        {
            ThrowIf.Null(internalTestsStarted, "internalTestsStarted");

            this.actionStore = actionStore;
            this.stateManager = stateManager;
            this.internalTestsStarted = internalTestsStarted;

            this.mockClient = new MockClient(messageProcessor);
        }

        public static async Task<ActionTest> CreateAsync(FaultAnalysisServiceMessageProcessor messageProcessor, ActionStore actionStore, IReliableStateManager stateManager)
        {
            ThrowIf.Null(messageProcessor, "messageProcessor");
            ThrowIf.Null(actionStore, "actionStore");
            ThrowIf.Null(stateManager, "stateManager");

            IReliableDictionary<string, string> internalTestsStarted = await stateManager.GetOrAddAsync<IReliableDictionary<string, string>>("internalTests");
            return new ActionTest(messageProcessor, actionStore, stateManager, internalTestsStarted);
        }

        public static RollbackState PerformTestUserCancelIfRequested(Guid operationId, string location, ServiceInternalFaultInfo serviceInternalFaultInfo, ActionStateBase actionState)
        {
            if (serviceInternalFaultInfo != null)
            {
                TestabilityTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} location={1}, serviceInternalFaultInfo.Location={2}, internal test RollbackType={3}",
                    operationId,
                    location,
                    serviceInternalFaultInfo.Location,
                    serviceInternalFaultInfo.RollbackType);

                // "None" means "don't care"           
                if (serviceInternalFaultInfo.TargetCancelStepStateName == actionState.StateProgress.Peek()
                    || serviceInternalFaultInfo.TargetCancelStepStateName == StepStateNames.None)
                {
                    if (location == serviceInternalFaultInfo.Location)
                    {
                        if (serviceInternalFaultInfo.RollbackType == RollbackState.RollingBackDueToUserCancel)
                        {
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} Intentionally canceling command, force==false", operationId);
                            return RollbackState.RollingBackDueToUserCancel;
                        }
                        else if (serviceInternalFaultInfo.RollbackType == RollbackState.RollingBackForce)
                        {
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} Intentionally canceling command, force==true", operationId);
                            return RollbackState.RollingBackForce;
                        }
                    }
                }
            }

            return actionState.RollbackState;
        }

        public static void PerformInternalServiceFaultIfRequested(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo, ActionStateBase actionState, CancellationToken cancellationToken)
        {
            PerformInternalServiceFaultIfRequested(operationId, serviceInternalFaultInfo, actionState, cancellationToken, false);
        }

        public static void PerformInternalServiceFaultIfRequested(Guid operationId, ServiceInternalFaultInfo serviceInternalFaultInfo, ActionStateBase actionState, CancellationToken cancellationToken, bool isMidState)
        {
            TestabilityTrace.TraceSource.WriteInfo(StepBase.TraceType, "{0} - enter PerformInternalServiceFaultIfRequested", operationId);
            if (serviceInternalFaultInfo != null)
            {
                if (serviceInternalFaultInfo.TargetStepStateName == actionState.StateProgress.Peek() ||
                    (serviceInternalFaultInfo.TargetStepStateName == Actions.Steps.StepStateNames.MidPerformingActions && isMidState))
                {
                    if (serviceInternalFaultInfo.FaultType == ServiceInternalFaultType.KillProcess)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} Intentionally exiting process using Process.Kill", operationId);
                        Process.GetCurrentProcess().Kill();
                        Task.Delay(Timeout.Infinite, cancellationToken).Wait();
                    }
                    else if (serviceInternalFaultInfo.FaultType == ServiceInternalFaultType.ReportFaultPermanent)
                    {
                        // TODO - wire in the IStatefulServicePartition so ReportFault() can be called
                        throw new NotImplementedException();
                    }
                    else if (serviceInternalFaultInfo.FaultType == ServiceInternalFaultType.ReportFaultTransient)
                    {
                        // TODO - wire in the IStatefulServicePartition so ReportFault() can be called
                        throw new NotImplementedException();
                    }
                    else if (serviceInternalFaultInfo.FaultType == ServiceInternalFaultType.RollbackAction)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} {1}", operationId, ActionTest.FatalRollbackExceptionMessage);

                        // Intentionally using this "weird" error code so that it is noticeable.                         
                        throw new FabricException("Intentional test fault", FabricErrorCode.InvalidConfiguration);
                    }
                    else if (serviceInternalFaultInfo.FaultType == ServiceInternalFaultType.ThrowThreeTimes)
                    {
                        if (serviceInternalFaultInfo.CountForThrowThreeTimesFault == 0)
                        {
                            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - number of test retries reached, now NOT performing fault", operationId);
                        }
                        else
                        {
                            serviceInternalFaultInfo.CountForThrowThreeTimesFault--;
                            throw new InvalidOperationException("Intentional exception from ThrowThreeTimes");
                        }
                    }
                    else if (serviceInternalFaultInfo.FaultType == ServiceInternalFaultType.RollbackActionAndRetry)
                    {
                        TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} Intentionally faliing this action.  It should succeed on retry.", operationId);

                        serviceInternalFaultInfo.SetFaultTypeToNone();
                        throw new FabricTransientException("Intentional test fault", FabricErrorCode.NotReady);
                    }
                }
            }
            else
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} Inside PerformInternalServiceFaultIfRequested, no fault requested", actionState.OperationId);
            }
        }

        public void Test()
        {
            this.TestRetryStep();

            this.TestRestartPartitionAction();
            this.TestQuorumLossAction();
            this.TestDataLossAction();
            this.TestUserCancel();

            this.TestStartAndStopNode();
        }

        internal static Task<NodeList> GetNodeListAsync()
        {
            FabricClient fc = new FabricClient();
            return System.Fabric.Common.FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => fc.QueryManager.GetNodeListAsync(
                    null,
                    TimeSpan.FromSeconds(30),
                    CancellationToken.None),
                TimeSpan.FromMinutes(2));
        }

        internal static async Task<Node> GetNodeWithFASSecondary()
        {
            NodeList nodeList = ActionTest.GetNodeListAsync().Result;
            ServiceReplicaList list = null;
            FabricClient fc = new FabricClient();

            System.Fabric.Common.TimeoutHelper timeoutHelper = new System.Fabric.Common.TimeoutHelper(TimeSpan.FromMinutes(2));

            do
            {
                try
                {
                    list = await fc.QueryManager.GetReplicaListAsync(new Guid("00000000-0000-0000-0000-000000005000"));
                }
                catch (Exception)
                {
                    Task.Delay(TimeSpan.FromSeconds(1)).Wait();
                }
            }
            while (list == null && timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

            if (list == null)
            {
                throw new InvalidOperationException("Could not resolve FAS primary");
            }

            Replica replica = list.Where(r => ((StatefulServiceReplica)r).ReplicaRole == ReplicaRole.ActiveSecondary).FirstOrDefault();
            return nodeList.Where(n => n.NodeName == replica.NodeName).FirstOrDefault();
        }

        private void TestStartAndStopNode()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing start and stop node");

            this.StartActionIfItHasNotBeenStarted(Command.StopNodeWithExceptionAndSuccessAfterRetries);

            this.StartActionIfItHasNotBeenStarted(Command.StopNodeWithUnknownException);
        }

        private void TestRetryStep()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing retry step");

            // These are not in the corresponding script, TSInternal.test, because the ActionType this uses is not available from outside.
            // We will still know if they fail though because FAS will assert or the script will time out.
            this.StartActionIfItHasNotBeenStarted(Command.TestRetryStepWithForceCancel);
            this.StartActionIfItHasNotBeenStarted(Command.TestRetryStepWithSuccessAfterRetries);
        }

        // This doesn't run in automation, but it is being kept here so it can be run as a small test.
        // See FaultAnalysisServiceTruncate.test for a test on this code path.
        private void TestTruncate()
        {
            this.StartActionIfItHasNotBeenStarted(Command.StuckAction);
            this.StartActionIfItHasNotBeenStarted(Command.FailoverManagerDataLoss);
            this.StartActionIfItHasNotBeenStarted(Command.InvokeDataLossMidActionTestFatal);
            this.StartActionIfItHasNotBeenStarted(Command.InvokeDataLossMidActionTestTransient);

            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionMidActionTestFatal);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionMidActionTestTransient);

            this.WaitForActionCount(FASConstants.TestMaxStoredActionCountValue);

            // Confirm this action is still stuck - ie that an action not in terminal state is not removed
            this.mockClient.WaitForState(MockClient.MockClientCommandInfo[Command.StuckAction], Actions.Steps.StepStateNames.LookingUpState);
            this.mockClient.WaitForState(MockClient.MockClientCommandInfo[Command.RestartPartitionMidActionTestTransient], Actions.Steps.StepStateNames.CompletedSuccessfully);

            // At this point there should be 1 command in the actionTable, StuckAction, and somewhere between 2 (Constants.TestMaxStoredActionCountValue) and 5 (the total number of possible
            // completed commands) commands in the historyTable.  In steady state, after truncates have run, the historyTable should have 2 (Constants.TestMaxStoredActionCountValue) commands remaining,
            // and they should be the ones that completed last.  Since this test only allows 1 action at a time, this will always be the 2 that were started last -
            // the RestartPartition ones.
            bool conditionSatisfied = false;
            var timeoutHelper = new System.Fabric.Common.TimeoutHelper(TimeSpan.FromSeconds(3 * FASConstants.TestStoredActionCleanupIntervalInSeconds));
            do
            {
                TestCommandListDescription queryDescription = new TestCommandListDescription(Query.TestCommandStateFilter.CompletedSuccessfully, Query.TestCommandTypeFilter.PartitionRestart);
                TestCommandQueryResult queryResult = this.mockClient.GetTestCommandListAsync(queryDescription).GetAwaiter().GetResult();
                List<TestCommandStatus> result = queryResult.Items;

                if (result.Count < FASConstants.TestMaxStoredActionCountValue)
                {
                    string error = string.Format(
                        CultureInfo.InvariantCulture,
                        "Number of commands in the historyTable {0} is below TestMaxStoredActionCountValue (config 'DefaultMaxStoredActionCount')",
                        result.Count);
                    TestabilityTrace.TraceSource.WriteError(TraceType, error);
                    System.Fabric.Common.ReleaseAssert.Failfast(error);
                }

                if (result.Where(c => c.TestCommandType == TestCommandType.PartitionRestart).Count() != FASConstants.TestMaxStoredActionCountValue)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "Number of PartitionRestart results is {0}, expecting {1}, retrying", result.Count, FASConstants.TestMaxStoredActionCountValue);
                    continue;
                }

                conditionSatisfied = true;
            }
            while (!conditionSatisfied && timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

            System.Fabric.Common.ReleaseAssert.Failfast(string.Format(CultureInfo.InvariantCulture, "Did not reach expected target action, see traces above filtered by type~ActionTest'"));

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Exiting TestTruncate");
        }

        private void WaitForActionCount(long targetCount)
        {
            long count = 0;

            System.Fabric.Common.TimeoutHelper timeoutHelper = new System.Fabric.Common.TimeoutHelper(TimeSpan.FromMinutes(3));

            do
            {
                count = this.actionStore.GetActionCountAsync(false).GetAwaiter().GetResult();
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "Current action count='{0}', target action count='{1}'", count, targetCount);
                if (count == targetCount)
                {
                    break;
                }

                Task.Delay(TimeSpan.FromSeconds(5)).Wait();
            }
            while (count != targetCount && timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

            if (count != targetCount)
            {
                string error = string.Format(CultureInfo.InvariantCulture, "Did not reach expected target action count='{0}', current action count='{1}'", targetCount, count);
                TestabilityTrace.TraceSource.WriteError(TraceType, error);
                System.Fabric.Common.ReleaseAssert.Failfast(error);
            }
        }

        private void TestDataLossAction()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing data loss action");

            this.StartActionIfItHasNotBeenStarted(Command.InvokeDataLossMidActionTestFatal);
            this.StartActionIfItHasNotBeenStarted(Command.InvokeDataLossMidActionTestTransient);

            this.StartActionIfItHasNotBeenStarted(Command.FailoverManagerDataLossCauseActionRollbackFatal);
            this.StartActionIfItHasNotBeenStarted(Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetry);

            this.StartActionIfItHasNotBeenStarted(Command.InvokeDataLossMidActionTestFailover);

            this.StartActionIfItHasNotBeenStarted(Command.FailoverManagerDataLossCauseActionRollbackFatalBeforeActionStep);
            this.StartActionIfItHasNotBeenStarted(Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetryBeforeActionStep);

            this.StartActionIfItHasNotBeenStarted(Command.FailoverManagerDataLoss);

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Done testing data loss action");
        }

        private void TestQuorumLossAction()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing quorum loss action");

            this.StartActionIfItHasNotBeenStarted(Command.InvokeQuorumLossMidActionTestFatal);
            this.StartActionIfItHasNotBeenStarted(Command.InvokeQuorumLossMidActionTestTransient);

            this.StartActionIfItHasNotBeenStarted(Command.InvokeQuorumLossMidActionTestFailover);
        }

        private void TestRestartPartitionAction()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing restart partition action");

            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionMidActionTestFatal);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionMidActionTestFailover);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionMidActionTestTransient);
        }

        private void TestUserCancel()
        {
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing user cancel with force==false");

            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelOuterLoopNoForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelForwardNoForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelForwardExceptionNoForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelOuterCleanupNoForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelCleanupInnerNoForce);

            // Note: normally to cancel a command with force==true, cancel with force==false must have already been invoked.  Since
            // this is an internal test, we're just forcing it down the code path to cancel with force==true.
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "Testing user cancel with force==true");

            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelForwardForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelForwardExceptionForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelOuterCleanupForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelCleanupInnerForce);
            this.StartActionIfItHasNotBeenStarted(Command.RestartPartitionCancelOuterLoopForce);
        }

        // If there is a failover this path will get run again.  This method is to ensure the same action is not inadvertantly started 2x.
        private void StartActionIfItHasNotBeenStarted(Command command)
        {
            bool actionHasAlreadyBeenStarted = false;

            try
            {
                using (var tx = this.stateManager.CreateTransaction())
                {
                    actionHasAlreadyBeenStarted = this.internalTestsStarted.ContainsKeyAsync(tx, command.ToString()).GetAwaiter().GetResult();
                }
            }
            catch (Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, e.ToString());
                throw;
            }

            if (actionHasAlreadyBeenStarted)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "This action ({0}) has already been sent", command.ToString());
                Guid guid;
                bool keyFound = MockClient.MapOfTSFailoverActions.TryGetValue(command, out guid);
                if (keyFound)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "    waiting for {0} with action type {1} to complete", guid, command.ToString());
                    this.mockClient.WaitForState(guid, Actions.Steps.StepStateNames.CompletedSuccessfully);
                }
            }
            else
            {
                try
                {
                    using (var tx = this.stateManager.CreateTransaction())
                    {
                        this.internalTestsStarted.AddAsync(tx, command.ToString(), ".").Wait();
                        tx.CommitAsync().Wait();
                    }

                    Task task = null;
                    using (var tx = this.stateManager.CreateTransaction())
                    {
                        task = this.mockClient.InsertCommandAsync(command);
                        tx.CommitAsync().Wait();
                    }

                    task.Wait();
                }
                catch (Exception e)
                {
                    TestabilityTrace.TraceSource.WriteError(TraceType, e.ToString());
                    throw;
                }
            }
        }
    }
}