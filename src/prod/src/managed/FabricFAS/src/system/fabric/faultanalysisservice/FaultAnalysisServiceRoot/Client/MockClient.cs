// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Client
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.FaultAnalysis.Service.Common;
    using System.Fabric.FaultAnalysis.Service.Engine;
    using System.Fabric.FaultAnalysis.Service.Test;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using TestabilityTrace = System.Fabric.Common.TestabilityTrace;

    internal enum Command
    {
        FailoverManagerDataLoss,
        FailoverManagerDataLossCauseActionRollbackFatal,
        FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetry,
        FailoverManagerDataLossFailoverFaultAnalysisService,

        FailoverManagerDataLossCauseActionRollbackFatalBeforeActionStep,
        FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetryBeforeActionStep,

        InvokeDataLossMidActionTestFatal,
        InvokeDataLossMidActionTestTransient,
        InvokeDataLossMidActionTestFailover,

        QuorumLoss,

        InvokeQuorumLossMidActionTestFatal,
        InvokeQuorumLossMidActionTestTransient,
        InvokeQuorumLossMidActionTestFailover,

        RestartPartition,

        RestartPartitionMidActionTestFatal,
        RestartPartitionMidActionTestTransient,
        RestartPartitionMidActionTestFailover,

        // This will intentionally get stuck
        StuckAction,

        RestartPartitionCancelOuterLoopNoForce,
        RestartPartitionCancelOuterLoopForce,

        RestartPartitionCancelForwardNoForce,
        RestartPartitionCancelForwardForce,

        RestartPartitionCancelForwardExceptionNoForce,
        RestartPartitionCancelForwardExceptionForce,

        RestartPartitionCancelOuterCleanupNoForce,
        RestartPartitionCancelOuterCleanupForce,

        RestartPartitionCancelCleanupInnerNoForce,
        RestartPartitionCancelCleanupInnerForce,

        TestRetryStepWithSuccessAfterRetries,
        TestRetryStepWithForceCancel,

        StopNodeWithUnknownException,
        StopNodeWithExceptionAndSuccessAfterRetries,
    }

    // A local test client which feeds the service commands.
    internal class MockClient
    {       
        // These actions will perform an intentional test TS primary failover while executing.  After a failover, we want to wait for the current action to resume and complete 
        // before starting the next one.  This map is so we can look up the actions we need to wait for after failover.
        internal static readonly Dictionary<Command, Guid> MapOfTSFailoverActions = new Dictionary<Command, Guid>()
        {
            { Command.FailoverManagerDataLossFailoverFaultAnalysisService, new Guid("18dd72b4-14aa-49dc-8a84-73b116887c73") },
            { Command.InvokeDataLossMidActionTestFailover,               new Guid("6a068bb8-e99d-495a-ba17-6f457380a94b") },
            { Command.InvokeQuorumLossMidActionTestFailover,             new Guid("79f714a1-c2a4-4a8a-8250-5d85086dfdc1") },
            { Command.RestartPartitionMidActionTestFailover,             new Guid("4734c0fa-bf58-47f9-875a-d491d2d37a4b") }
        };

        internal static readonly Dictionary<Command, Guid> MockClientCommandInfo = new Dictionary<Command, Guid>()
        {
            { Command.FailoverManagerDataLoss,                                                      new Guid("414d31dd-d2b7-4e9e-a9ca-5beb239bd729") },
            { Command.InvokeDataLossMidActionTestFatal,                                             new Guid("cd2934dc-14d5-463e-9f3e-3f9cec82dc93") },
            { Command.InvokeDataLossMidActionTestTransient,                                         new Guid("88f02ee5-4f5b-4f1a-a6b2-62b18282fb0f") },
            { Command.InvokeDataLossMidActionTestFailover,                                          new Guid("6a068bb8-e99d-495a-ba17-6f457380a94b") },
            { Command.FailoverManagerDataLossCauseActionRollbackFatal,                              new Guid("3c85247b-0e72-4a08-96ea-8be60847932f") },
            { Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetry,                 new Guid("77a276bd-8ef2-49f9-82ad-d79407cb38a8") },
            { Command.FailoverManagerDataLossFailoverFaultAnalysisService,                          new Guid("18dd72b4-14aa-49dc-8a84-73b116887c73") },
            { Command.FailoverManagerDataLossCauseActionRollbackFatalBeforeActionStep,              new Guid("660851c6-81d2-42c9-aa27-3cd2d3abd72e") },
            { Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetryBeforeActionStep, new Guid("dc0f931a-0465-431c-97cc-246db0e1f42b") },
            { Command.InvokeQuorumLossMidActionTestFatal,                                           new Guid("8d3cea86-c962-4357-8e00-ce5389d1d568") },
            { Command.InvokeQuorumLossMidActionTestFailover,                                        new Guid("79f714a1-c2a4-4a8a-8250-5d85086dfdc1") },
            { Command.InvokeQuorumLossMidActionTestTransient,                                       new Guid("571200ce-1738-4c74-b38c-def524c9cf22") },
            { Command.RestartPartitionMidActionTestFatal,                                           new Guid("b379f75b-5091-4cd2-9337-681205e98b92") },
            { Command.RestartPartitionMidActionTestFailover,                                        new Guid("4734c0fa-bf58-47f9-875a-d491d2d37a4b") },
            { Command.RestartPartitionMidActionTestTransient,                                       new Guid("0a8712ad-1cfe-46f1-82c0-56da072f796f") },
            { Command.StuckAction,                                                                  new Guid("f7ab6eda-2ccd-4428-b108-4842abc2862c") },
            { Command.RestartPartitionCancelOuterLoopNoForce,                                       new Guid("00000000-0000-0000-0000-00000000aaaa") },
            { Command.RestartPartitionCancelOuterLoopForce,                                         new Guid("00000000-0000-0000-0000-00000000aaaf") },
            { Command.RestartPartitionCancelForwardNoForce,                                         new Guid("00000000-0000-0000-0000-00000000bbbb") },
            { Command.RestartPartitionCancelForwardForce,                                           new Guid("00000000-0000-0000-0000-00000000bbbf") },
            { Command.RestartPartitionCancelForwardExceptionNoForce,                                new Guid("00000000-0000-0000-0000-00000000cccc") },
            { Command.RestartPartitionCancelForwardExceptionForce,                                  new Guid("00000000-0000-0000-0000-00000000cccf") },
            { Command.RestartPartitionCancelOuterCleanupNoForce,                                    new Guid("00000000-0000-0000-0000-00000000dddd") },
            { Command.RestartPartitionCancelOuterCleanupForce,                                      new Guid("00000000-0000-0000-0000-00000000dddf") },
            { Command.RestartPartitionCancelCleanupInnerNoForce,                                    new Guid("00000000-0000-0000-0000-00000000eeee") },
            { Command.RestartPartitionCancelCleanupInnerForce,                                      new Guid("00000000-0000-0000-0000-00000000eeef") },
            { Command.TestRetryStepWithForceCancel,                                                 new Guid("00000000-0000-0000-0000-0000000ffff0") },
            { Command.TestRetryStepWithSuccessAfterRetries,                                         new Guid("00000000-0000-0000-0000-0000000ffff1") },
            { Command.StopNodeWithUnknownException,                                                 new Guid("00000000-0000-0000-0000-0000000ffff5") },
            { Command.StopNodeWithExceptionAndSuccessAfterRetries,                                  new Guid("00000000-0000-0000-0000-0000000ffff6") },
        };

        private const string TraceType = "MockClient";        
        private readonly FaultAnalysisServiceMessageProcessor messageProcessor;

        public MockClient(FaultAnalysisServiceMessageProcessor messageProcessor)
        {
            ThrowIf.Null(messageProcessor, "messageProcessor");

            this.messageProcessor = messageProcessor;
        }

        public Task InsertCommandAsync(Command command)
        {
            Uri failoverManagerUri = new Uri("fabric:/System/FailoverManagerService");
            TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "****Adding command: " + command);
            Task task = null;
            if (command == Command.FailoverManagerDataLoss)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                Guid id = MockClientCommandInfo[Command.FailoverManagerDataLoss];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, null);

                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.InvokeDataLossMidActionTestFatal)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.RollbackAction);
                Guid id = MockClientCommandInfo[Command.InvokeDataLossMidActionTestFatal];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed);
            }
            else if (command == Command.InvokeDataLossMidActionTestTransient)
            {
                // rollback and retry then success                
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.RollbackActionAndRetry);
                Guid id = MockClientCommandInfo[Command.InvokeDataLossMidActionTestTransient];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.InvokeDataLossMidActionTestFailover)
            {
                // failover then success                
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.KillProcess);
                Guid id = MockClientCommandInfo[Command.InvokeDataLossMidActionTestFailover];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.FailoverManagerDataLossCauseActionRollbackFatal)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.CompletedSuccessfully, ServiceInternalFaultType.RollbackAction);
                Guid id = MockClientCommandInfo[Command.FailoverManagerDataLossCauseActionRollbackFatal];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed);
            }
            else if (command == Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetry)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.CompletedSuccessfully, ServiceInternalFaultType.RollbackActionAndRetry);
                Guid id = MockClientCommandInfo[Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetry];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.FailoverManagerDataLossFailoverFaultAnalysisService)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.PerformingActions, ServiceInternalFaultType.KillProcess);
                Guid id = MockClientCommandInfo[Command.FailoverManagerDataLossFailoverFaultAnalysisService];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.FailoverManagerDataLossCauseActionRollbackFatalBeforeActionStep)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.PerformingActions, ServiceInternalFaultType.RollbackAction);
                Guid id = MockClientCommandInfo[Command.FailoverManagerDataLossCauseActionRollbackFatalBeforeActionStep];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed);
            }
            else if (command == Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetryBeforeActionStep)
            {
                PartitionSelector ps = PartitionSelector.SingletonOf(failoverManagerUri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.PerformingActions, ServiceInternalFaultType.RollbackActionAndRetry);
                Guid id = MockClientCommandInfo[Command.FailoverManagerDataLossCauseActionRollbackWithSuccessOnRetryBeforeActionStep];
                task = this.messageProcessor.ProcessDataLossCommandAsync(id, ps, DataLossMode.FullDataLoss, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.InvokeQuorumLossMidActionTestFatal)
            {
                Uri uri = new Uri("fabric:/System/NamingService");
                PartitionSelector ps = PartitionSelector.RandomOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.RollbackAction);
                Guid id = MockClientCommandInfo[Command.InvokeQuorumLossMidActionTestFatal];
                task = this.messageProcessor.ProcessQuorumLossCommandAsync(id, ps, QuorumLossMode.AllReplicas, TimeSpan.FromSeconds(10.0d), FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed);
            }
            else if (command == Command.InvokeQuorumLossMidActionTestFailover)
            {
                Uri uri = new Uri("fabric:/System/NamingService");
                PartitionSelector ps = PartitionSelector.RandomOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.KillProcess);
                Guid id = MockClientCommandInfo[Command.InvokeQuorumLossMidActionTestFailover];
                task = this.messageProcessor.ProcessQuorumLossCommandAsync(id, ps, QuorumLossMode.AllReplicas, TimeSpan.FromSeconds(10.0d), FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.InvokeQuorumLossMidActionTestTransient)
            {
                Uri uri = new Uri("fabric:/System/NamingService");
                PartitionSelector ps = PartitionSelector.RandomOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.RollbackActionAndRetry);
                Guid id = MockClientCommandInfo[Command.InvokeQuorumLossMidActionTestTransient];
                task = this.messageProcessor.ProcessQuorumLossCommandAsync(id, ps, QuorumLossMode.AllReplicas, TimeSpan.FromSeconds(10.0d), FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.RestartPartitionMidActionTestFatal)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.RollbackAction);
                Guid id = MockClientCommandInfo[Command.RestartPartitionMidActionTestFatal];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed);
            }
            else if (command == Command.RestartPartitionMidActionTestFailover)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.KillProcess);
                Guid id = MockClientCommandInfo[Command.RestartPartitionMidActionTestFailover];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.RestartPartitionMidActionTestTransient)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(StepStateNames.MidPerformingActions, ServiceInternalFaultType.RollbackActionAndRetry);
                Guid id = MockClientCommandInfo[Command.RestartPartitionMidActionTestTransient];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully);
            }
            else if (command == Command.StuckAction)
            {
                Guid id = MockClientCommandInfo[Command.StuckAction];
                task = this.messageProcessor.ProcessStuckCommandAsync(id, null);
            }
            else if (command == Command.RestartPartitionCancelOuterLoopNoForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.None, // for this case this value does not matter
                    ServiceInternalFaultType.None,
                    RollbackState.RollingBackDueToUserCancel,
                    FASConstants.OuterLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelOuterLoopNoForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackDueToUserCancel);
            }
            else if (command == Command.RestartPartitionCancelForwardNoForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.None,
                    ServiceInternalFaultType.None,
                    RollbackState.RollingBackDueToUserCancel,
                    FASConstants.ForwardLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelForwardNoForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackDueToUserCancel);
            }
            else if (command == Command.RestartPartitionCancelForwardExceptionNoForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.MidPerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackDueToUserCancel,
                    FASConstants.ForwardLoopExceptionBlock);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelForwardExceptionNoForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackDueToUserCancel);
            }
            else if (command == Command.RestartPartitionCancelOuterCleanupNoForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackDueToUserCancel,
                    FASConstants.OuterCleanupLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelOuterCleanupNoForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackDueToUserCancel);
            }
            else if (command == Command.RestartPartitionCancelCleanupInnerNoForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackDueToUserCancel,
                    FASConstants.InnerCleanupLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelCleanupInnerNoForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackDueToUserCancel);
            }
            else if (command == Command.RestartPartitionCancelOuterLoopForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions, // for this case, this value does not matter
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackForce,
                    FASConstants.ForwardLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelOuterLoopForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);
            }
            else if (command == Command.RestartPartitionCancelForwardForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackForce,
                    FASConstants.ForwardLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelForwardForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);
            }
            else if (command == Command.RestartPartitionCancelForwardExceptionForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackForce,
                    FASConstants.ForwardLoopExceptionBlock);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelForwardExceptionForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);
            }
            else if (command == Command.RestartPartitionCancelOuterCleanupForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackForce,
                    FASConstants.OuterCleanupLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelOuterCleanupForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);
            }
            else if (command == Command.RestartPartitionCancelCleanupInnerForce)
            {
                Uri uri = new Uri("fabric:/System/ClusterManagerService");
                PartitionSelector ps = PartitionSelector.SingletonOf(uri);
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.PerformingActions,
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackForce,
                    FASConstants.InnerCleanupLoop);
                Guid id = MockClientCommandInfo[Command.RestartPartitionCancelCleanupInnerForce];
                task = this.messageProcessor.ProcessRestartPartitionCommandAsync(id, ps, RestartPartitionMode.AllReplicasOrInstances, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);
            }
            else if (command == Command.TestRetryStepWithSuccessAfterRetries)
            {
                // Intentionally fail the step corresponding to StepStateNames.PerformingActions step a few times, then run it normally (pass).  It should succeed.
                Guid id = MockClientCommandInfo[Command.TestRetryStepWithSuccessAfterRetries];
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.MidPerformingActions,
                    ServiceInternalFaultType.ThrowThreeTimes);

                task = this.messageProcessor.ProcessRetryStepCommandAsync(id, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully, RollbackState.NotRollingBack);
            }
            else if (command == Command.TestRetryStepWithForceCancel)
            {
                // Force cancel a command with ActionStateBase.RetryStepWithoutRollingBackOnFailure set to true
                Guid id = MockClientCommandInfo[Command.TestRetryStepWithForceCancel];
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                    StepStateNames.CompletedSuccessfully, // this just has to be a late step so an earlier fault is not used before we reach the situation we want.
                    ServiceInternalFaultType.RollbackAction,
                    RollbackState.RollingBackForce, // note, the graceful one should not cause cancellation since for this type we only allow user cancellation when force is true
                    FASConstants.InnerForwardLoop,
                    StepStateNames.PerformingActions);
                task = this.messageProcessor.ProcessRetryStepCommandAsync(id, faultInfo);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);
            }
            else if (command == Command.StopNodeWithUnknownException)
            {
                Guid id = MockClientCommandInfo[Command.StopNodeWithUnknownException];
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                     StepStateNames.MidPerformingActions,
                     ServiceInternalFaultType.RollbackAction); // In this case, since start and stop node do not rollback like other commands, this exception should cause the step to retry.

                Node target = ActionTest.GetNodeWithFASSecondary().Result;
                TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} stopping {1}:{2}", id, target.NodeName, target.NodeInstanceId);
                task = this.messageProcessor.ProcessStopNodeCommandAsync(id, target.NodeName, target.NodeInstanceId, 999, FASConstants.DefaultTestTimeout, faultInfo);

                // Let the command make progress
                Task.Delay(TimeSpan.FromSeconds(30)).Wait();
                this.WaitForState(id, StepStateNames.PerformingActions, RollbackState.NotRollingBack);

                // This should not result in cancellation, since start and stop node have different rollback policies than the other commands.
                TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} - cancelling with force==false.  This should not cancel the command", id);
                this.messageProcessor.CancelTestCommandAsync(id, false);

                this.WaitForState(id, StepStateNames.PerformingActions, RollbackState.RollingBackDueToUserCancel);

                // Now force cancel.  This should cancel.
                TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} - cancelling with force==true.  This should cancel the command", id);
                this.messageProcessor.CancelTestCommandAsync(id, true);
                this.WaitForState(id, StepStateNames.Failed, RollbackState.RollingBackForce);

                NodeList nodes = ActionTest.GetNodeListAsync().Result;
                TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} - node info:", id);
                foreach (Node n in nodes)
                {
                    TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "    OperationId:{0} - NodeName{1}, NodeStatus:{2}, IsStopped:{3}", id, n.NodeName, n.NodeStatus, n.IsStopped);
                }

                Node targetNodeAfterTest = nodes.Where(n => n.NodeName == target.NodeName).FirstOrDefault();
                if (targetNodeAfterTest == null)
                {
                    throw new InvalidOperationException("target node was not found in query after test");
                }

                if (targetNodeAfterTest.IsStopped == false)
                {
                    throw new InvalidOperationException("target node should have IsStopped true, was false");
                }
            }
            else if (command == Command.StopNodeWithExceptionAndSuccessAfterRetries)
            {
                Guid id = MockClientCommandInfo[Command.StopNodeWithExceptionAndSuccessAfterRetries];

                // Inject a fault during the operation so that step "StepStateNames.MidPerformingActions" has to retry 3 times before succeeding
                ServiceInternalFaultInfo faultInfo = new ServiceInternalFaultInfo(
                     StepStateNames.MidPerformingActions,
                     ServiceInternalFaultType.ThrowThreeTimes);

                Node target = ActionTest.GetNodeWithFASSecondary().Result;
                TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} stopping {1}:{2}", id, target.NodeName, target.NodeInstanceId);
                task = this.messageProcessor.ProcessStopNodeCommandAsync(id, target.NodeName, target.NodeInstanceId, 999, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully, RollbackState.NotRollingBack);

                // Start the stopped node
                task = this.messageProcessor.ProcessStartNodeCommandAsync(Guid.NewGuid(), target.NodeName, target.NodeInstanceId, FASConstants.DefaultTestTimeout, faultInfo);
                this.WaitForState(id, StepStateNames.CompletedSuccessfully, RollbackState.NotRollingBack);
            }
            else
            {
                ReleaseAssert.Failfast("Unexpected command");
            }

            return task;
        }

        public Task CancelTestCommandAsync(Guid operationId, bool force)
        {
            return this.messageProcessor.CancelTestCommandAsync(operationId, force);
        }

        public Task<TestCommandQueryResult> GetTestCommandListAsync(TestCommandListDescription description)
        {
            return this.messageProcessor.ProcessGetTestCommandListAsync(description);
        }

        public void WaitForState(Guid operationId, StepStateNames targetState, RollbackState targetRollbackState = RollbackState.NotRollingBack)
        {
            StepStateNames currentState = StepStateNames.IntentSaved;
            RollbackState rollbackState = RollbackState.NotRollingBack;
            TimeoutHelper timeoutHelper = new TimeoutHelper(TimeSpan.FromMinutes(3));
            
            do
            {
                bool callSucceeded = false;
                TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} waiting for state '{1}'", operationId, targetState);
                ActionStateBase state = null;
                try
                {
                    state = this.messageProcessor.ProcessGetProgressAsync(operationId, FASConstants.DefaultTestTimeout, CancellationToken.None).GetAwaiter().GetResult();
                    callSucceeded = true;
                }               
                catch (OperationCanceledException oce)
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught {1}", operationId, oce.ToString());
                }
                catch (TransactionFaultedException tfe)
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught {1}", operationId, tfe.ToString());
                }
                catch (FabricTransientException fte)
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught {1}", operationId, fte.ToString());
                }

                if (callSucceeded)
                {
                    currentState = state.StateProgress.Peek();
                    rollbackState = state.RollbackState;
                    TestabilityTrace.TraceSource.WriteInfo(MockClient.TraceType, "{0} current state='{1}', rollbackState='{2}'", operationId, currentState, rollbackState);
                }

                Task.Delay(TimeSpan.FromSeconds(5)).Wait();
            }
            while ((currentState != targetState || rollbackState != targetRollbackState) &&
                timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

            if ((currentState != targetState) ||
                (rollbackState != targetRollbackState))
            {
                string error = string.Format(
                    CultureInfo.InvariantCulture,
                    "{0} Did not reach expected situation: current state='{1}', expected state='{2}', current rollbackState='{3}', expected rollbackState='{4}'",
                    operationId,
                    currentState,
                    targetState,
                    rollbackState,
                    targetRollbackState);
                TestabilityTrace.TraceSource.WriteError(TraceType, error);
                ReleaseAssert.Failfast(error);
            }

            TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} reached expected state '{1}' and rollbackState '{2}'", operationId, targetState, targetRollbackState);
        }

        // tba
        public object GetState()
        {
            throw new NotImplementedException();
        }

        // tba
        public object GetHistory()
        {
            throw new NotImplementedException();
        }
    }
}