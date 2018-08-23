// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Common
{
    using System.Fabric.FaultAnalysis.Service.Actions;
    using System.Fabric.FaultAnalysis.Service.Actions.Steps;
    using System.Fabric.Result;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Collections.Generic;
    using Fabric.Common;
    using Interop;
    using Linq;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.Collections;
    using Query;

    internal enum SuccessRetryOrFail
    {
        Invalid,
        Success,
        RetryStep,
        RollbackAndRetryCommand,
        Fail
    }

    internal static class FaultAnalysisServiceUtility
    {
        private const string TraceType = "FaultAnalysisService";

        public static void ThrowTransientExceptionIfRetryable(Exception e)
        {
            if (e is OperationCanceledException
                || e is TransactionFaultedException
                || e is FabricNotReadableException
                || e is FabricNotPrimaryException)
            {
                throw FaultAnalysisServiceUtility.CreateException(TraceType, Interop.NativeTypes.FABRIC_ERROR_CODE.E_ABORT, "Operation cancelled");
            }
        }

        public static COMException CreateException(string traceType, NativeTypes.FABRIC_ERROR_CODE error, string format, params object[] args)
        {            
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            var exception = new COMException(message, (int)error);

            TestabilityTrace.TraceSource.WriteWarning(traceType, exception.ToString());

            return exception;
        }

        public static Exception ThrowEngineRetryableException(string message)
        {
            throw new FabricTransientException(message, FabricErrorCode.NotReady);
        }

        public static Task RunAndReportFaultOnRepeatedFailure(Guid operationId, Func<Task> action, IStatefulServicePartition partition, string caller, int numRetries, CancellationToken cancellationToken)
        {
            return FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                operationId,
                async () =>
                {
                    await action.Invoke();
                    return new object();
                },
                partition,
                caller,
                numRetries,
                cancellationToken);
        }

        public static async Task<TResult> RunAndReportFaultOnRepeatedFailure<TResult>(Guid operationId, Func<Task<TResult>> action, IStatefulServicePartition partition, string caller, int numRetries, CancellationToken cancellationToken)
        {
            bool wasSuccessful = false;
            int maxRetries = numRetries;
            TResult result = default(TResult);

            do
            {
                try
                {
                    result = await action().ConfigureAwait(false);
                    wasSuccessful = true;
                }
                catch (TimeoutException timeoutException)
                {
                    // catch and retry timeout, throw everything to caller to handle
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} timed out - {1}", operationId, timeoutException.ToString());
                }
                catch (TransactionFaultedException)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} TransactionFaultedException - retrying", operationId);
                }
                catch (FabricNotReadableException)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "{0} FabricNotReadableException - retrying", operationId);
                }
                catch (Exception exception)
                {
                    TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - {1}", operationId, exception.ToString());
                    throw;
                }

                if (!wasSuccessful)
                {
                    numRetries--;
                    await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken).ConfigureAwait(false);
                }
            }
            while ((!wasSuccessful) && (numRetries >= 0));

            if (!wasSuccessful)
            {
                TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - {1} did not succeed after {2} retries, reporting fault", operationId, caller, maxRetries);
                partition.ReportFault(FaultType.Transient);
            }

            return result;
        }

        public static async Task RetryOnTimeout(Guid operationId, Func<Task> action, TimeSpan timeout, CancellationToken cancellationToken)
        {
            System.Fabric.Common.TimeoutHelper timeoutHelper = new System.Fabric.Common.TimeoutHelper(timeout);

            bool wasSuccessful = false;

            do
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    await action().ConfigureAwait(false);
                    wasSuccessful = true;
                }
                catch (TimeoutException)
                {
                    TestabilityTrace.TraceSource.WriteWarning(
                        TraceType,
                        "{0} - ProcessGetProgressAsync - RD access timed out, retrying.  Total remaining time {1}",
                        operationId,
                        timeoutHelper.GetRemainingTime());
                }

                if (!wasSuccessful)
                {
                    await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken).ConfigureAwait(false);
                }
            }
            while (!wasSuccessful && timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

            if (!wasSuccessful)
            {
                throw new FabricException(FabricErrorCode.OperationTimedOut);
            }
        }

        public static async Task<TResult> RetryOnTimeout<TResult>(Guid operationId, Func<Task<TResult>> action, TimeSpan timeout, CancellationToken cancellationToken)
        {
            System.Fabric.Common.TimeoutHelper timeoutHelper = new System.Fabric.Common.TimeoutHelper(timeout);

            TResult result = default(TResult);
            bool wasSuccessful = false;

            do
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    result = await action().ConfigureAwait(false);
                    wasSuccessful = true;
                }
                catch (TimeoutException)
                {
                    TestabilityTrace.TraceSource.WriteWarning(
                        TraceType,
                        "{0} - ProcessGetProgressAsync - RD access timed out, retrying.  Total remaining time {1}",
                        operationId,
                        timeoutHelper.GetRemainingTime());
                }

                if (!wasSuccessful)
                {
                    await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken).ConfigureAwait(false);
                }
            }
            while (!wasSuccessful && timeoutHelper.GetRemainingTime() > TimeSpan.Zero);

            if (!wasSuccessful)
            {
                throw new FabricException(FabricErrorCode.OperationTimedOut);
            }

            return result;
        }

        public static Task<bool> ReadStoppedNodeStateAsync(Guid operationId, IStatefulServicePartition partition, IReliableStateManager stateManager, IReliableDictionary<string, bool> stoppedNodeTable, string nodeName, CancellationToken cancellationToken)
        {
            return FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                operationId,
                () => ReadStoppedNodeStateInnerAsync(stateManager, stoppedNodeTable, nodeName, cancellationToken),
                partition,
                "FaultAnalysisServiceUtility.ReadStoppedNodeStateAsync",
                3,
                cancellationToken);
        }

        public static async Task<bool> ReadStoppedNodeStateInnerAsync(IReliableStateManager stateManager, IReliableDictionary<string, bool> stoppedNodeTable, string nodeName, CancellationToken cancellationToken)
        {
            using (var tx = stateManager.CreateTransaction())
            {
                var data = await stoppedNodeTable.TryGetValueAsync(tx, nodeName, TimeSpan.FromSeconds(4), cancellationToken).ConfigureAwait(false);
                if (data.HasValue)
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "ReadStoppedNodeState:  table has value");
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        public static Task SetStoppedNodeStateAsync(Guid operationId, IStatefulServicePartition partition, IReliableStateManager stateManager, IReliableDictionary<string, bool> stoppedNodeTable, string nodeName, bool setStopped, CancellationToken cancellationToken)
        {
            return FaultAnalysisServiceUtility.RunAndReportFaultOnRepeatedFailure(
                operationId,
                () => SetStoppedNodeStateInnerAsync(operationId, stateManager, stoppedNodeTable, nodeName, setStopped, cancellationToken),
                partition,
                "FaultAnalysisServiceUtility.SetStoppedNodeStateAsync",
                3,
                cancellationToken);
        }

        // This should be wrapper in retry helper
        public static async Task SetStoppedNodeStateInnerAsync(
            Guid operationId,
            IReliableStateManager stateManager,
            IReliableDictionary<string, bool> stoppedNodeTable,
            string nodeName,
            bool setStopped,
            CancellationToken cancellationToken)
        {
            using (var tx = stateManager.CreateTransaction())
            {
                if (setStopped)
                {
                    await stoppedNodeTable.AddOrUpdateAsync(tx, nodeName, true, (k, v) => true, TimeSpan.FromSeconds(4), cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    await stoppedNodeTable.TryRemoveAsync(tx, nodeName, TimeSpan.FromSeconds(4), cancellationToken).ConfigureAwait(false);
                }

                await tx.CommitAsync().ConfigureAwait(false);
            }
        }

        public static async Task<Node> GetNodeInfoAsync(
            Guid operationId,
            FabricClient fc,
            string nodeName,
            IStatefulServicePartition partition,
            IReliableStateManager stateManager,
            IReliableDictionary<string, bool> stoppedNodeTable,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            // validate
            var nodeList = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => fc.TestManager.GetNodeListInternalAsync(
                    nodeName,
                    NodeStatusFilter.All,
                    null,
                    true,
                    requestTimeout,
                    cancellationToken),
                operationTimeout,
                cancellationToken).ConfigureAwait(false);

            if (nodeList.Count == 0 ||
               (nodeList[0].NodeStatus == NodeStatus.Invalid ||
               nodeList[0].NodeStatus == NodeStatus.Unknown ||
               nodeList[0].NodeStatus == NodeStatus.Removed))
            {
                await FaultAnalysisServiceUtility.SetStoppedNodeStateAsync(
                    operationId,
                    partition,
                    stateManager,
                    stoppedNodeTable,
                    nodeName,
                    false,
                    cancellationToken).ConfigureAwait(false);

                // this is fatal, fail the command
                Exception nodeNotFound = FaultAnalysisServiceUtility.CreateException(
                    TraceType,
                    NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_NODE_NOT_FOUND,
                    string.Format(CultureInfo.InvariantCulture, "Node {0} not found", nodeName),
                    FabricErrorCode.NodeNotFound);
                throw new FatalException("fatal", nodeNotFound);
            }

            return nodeList[0];
        }

        public static bool IsNodeRunning(Node node)
        {
            if (node == null)
            {
                throw new ArgumentNullException("IsNodeRunning: node is null");
            }

            if (node.NodeStatus == NodeStatus.Down ||
                node.NodeStatus == NodeStatus.Invalid ||
                node.NodeStatus == NodeStatus.Removed ||
                node.NodeStatus == NodeStatus.Unknown)
            {
                return false;
            }

            return true;
        }

        public static async Task<SelectedPartition> GetSelectedPartitionStateAsync(
            FabricClient fabricClient,
            PartitionSelector partitionSelector,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(partitionSelector, "PartitionSelector");

            Guid partitionId = Guid.Empty;
            Uri serviceName;

            if (!partitionSelector.TryGetPartitionIdIfNotGetServiceName(out partitionId, out serviceName))
            {            
                // Intentionally do not use FabricClientRetryErrors.GetPartitionListFabricErrors.  We do not want to retry "service not found".                
                ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ServicePartitionList>(
                    () => fabricClient.QueryManager.GetPartitionListAsync(
                        serviceName,
                        null,
                        default(string),
                        requestTimeout,
                        cancellationToken),
                    operationTimeout,
                    cancellationToken).ConfigureAwait(false);

                Partition partitionResult = partitionSelector.GetSelectedPartition(partitionsResult.ToArray(), new Random());
                partitionId = partitionResult.PartitionInformation.Id;
            }
            else
            {
                // Validate the partition specified is actually from the service specified.
                // Intentionally do not use FabricClientRetryErrors.GetPartitionListFabricErrors.  We do not want to retry "service not found".                
                ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ServicePartitionList>(
                    () => fabricClient.QueryManager.GetPartitionListAsync(
                        serviceName,
                        null,
                        default(string),
                        requestTimeout,
                        cancellationToken),
                    operationTimeout,
                    cancellationToken).ConfigureAwait(false);

                var guids = partitionsResult.Select(p => p.PartitionId()).ToList();
                if (!guids.Contains(partitionId))
                {
                    // There is no partition in the service specified that has a partition with the id specified.
                    throw new FabricException("Partition not found in this service", FabricErrorCode.PartitionNotFound);
                }
            }

            return new SelectedPartition(serviceName, partitionId);
        }

        public static async Task<Tuple<SelectedReplica, Replica>> GetSelectedReplicaAsync(
            FabricClient fabricClient,
            ReplicaSelector replicaSelector,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            ThrowIf.Null(replicaSelector, "ReplicaSelector");

            SelectedPartition selectedPartition = await FaultAnalysisServiceUtility.GetSelectedPartitionStateAsync(
                fabricClient,
                replicaSelector.PartitionSelector,
                requestTimeout,
                operationTimeout,
                cancellationToken).ConfigureAwait(false);
            Guid partitionId = selectedPartition.PartitionId;

            ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => fabricClient.QueryManager.GetReplicaListAsync(
                    partitionId,
                    0,
                    requestTimeout,
                    cancellationToken),
                operationTimeout,
                cancellationToken).ConfigureAwait(false);

            Replica replicaResult = replicaSelector.GetSelectedReplica(replicasResult.ToArray(), new Random(), true/*skip invalid replicas*/);
            var replicaSelectorResult = new SelectedReplica(replicaResult.Id, selectedPartition);
            return new Tuple<SelectedReplica, Replica>(replicaSelectorResult, replicaResult);
        }

        public static async Task<RestartReplicaResult> RestartReplicaAsync(
            FabricClient fabricClient,
            ReplicaSelector replicaSelector,
            CompletionMode completionMode,
            TimeSpan requestTimeout,
            TimeSpan operationTimeout,
            CancellationToken cancellationToken)
        {
            System.Fabric.Common.TimeoutHelper helper = new System.Fabric.Common.TimeoutHelper(operationTimeout);

            string nodeName = null;
            Guid partitionId = Guid.Empty;
            long replicaId = 0;
            SelectedReplica replicaSelectorResult = SelectedReplica.None;

            System.Fabric.Common.ThrowIf.Null(replicaSelector, "ReplicaSelector");

            Tuple<SelectedReplica, Replica> replicaStateActionResult = await FaultAnalysisServiceUtility.GetSelectedReplicaAsync(
                fabricClient,
                replicaSelector,
                requestTimeout,
                operationTimeout,
                cancellationToken).ConfigureAwait(false);

            replicaSelectorResult = replicaStateActionResult.Item1;
            if (replicaSelectorResult == null)
            {
                throw new InvalidOperationException("replicaStateActionResult cannot be null");
            }

            partitionId = replicaStateActionResult.Item1.SelectedPartition.PartitionId;

            Replica replicaStateResult = replicaStateActionResult.Item2;
            if (replicaStateResult == null)
            {
                throw new InvalidOperationException("replicaStateResult cannot be null");
            }

            nodeName = replicaStateResult.NodeName;
            replicaId = replicaStateResult.Id;

            ThrowIf.IsTrue(partitionId == Guid.Empty, "PartitionID");
            ThrowIf.IsTrue(replicaId == 0, "ReplicaID");

            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                () => fabricClient.ServiceManager.RestartReplicaAsync(
                    nodeName,
                    partitionId,
                    replicaId,
                    requestTimeout,
                    cancellationToken),
                FabricClientRetryErrors.RestartReplicaErrors.Value,
                operationTimeout,
                cancellationToken).ConfigureAwait(false);

            return new RestartReplicaResult(replicaSelectorResult);
        }

        internal static void TraceFabricObjectClosed(Guid operationId, Exception e)
        {
            // Change role has been called
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught FabricObjectClosed - '{1}'", operationId, e);
        }

        internal static void TraceFabricNotPrimary(Guid operationId, Exception e)
        {
            // Change role has been called
            TestabilityTrace.TraceSource.WriteWarning(TraceType, "{0} - Caught FabricNotPrimary - '{1}", operationId, e);
        }

        // Translate the current "state" into the state the user sees when calling a query or progress api, from a combination of StateStateNames and RollbackState.
        internal static TestCommandProgressState ConvertState(ActionStateBase action, string callerTraceType)
        {
            StepStateNames state = action.StateProgress.Peek();
            RollbackState rollbackState = action.RollbackState;

            TestabilityTrace.TraceSource.WriteInfo(callerTraceType, "ConvertState - state is={0}, rollbackState={1}", state.ToString(), rollbackState.ToString());
            if (state == StepStateNames.CompletedSuccessfully)
            {
                // It's possible to reach CompletedSuccessfully even though the cancel api was called because when
                // a command is running it only checks cancel status in between steps.  Since from the user's point of view cancel means 
                // "stop executing", and this has been achieved in this situation, it's ok to return Cancelled or ForceCancelled in the cases below.
                // This will also make the return value match what they expect.
                if (rollbackState == RollbackState.RollingBackForce)
                {
                    return TestCommandProgressState.ForceCancelled;
                }

                if (rollbackState == RollbackState.RollingBackDueToUserCancel)
                {
                    return TestCommandProgressState.Cancelled;
                }

                return TestCommandProgressState.Completed;
            }

            if ((rollbackState == RollbackState.RollingBackAndWillFailAction
                || rollbackState == RollbackState.RollingBackForce
                || rollbackState == RollbackState.RollingBackDueToUserCancel)
                &&
                state != StepStateNames.Failed)
            {
                return TestCommandProgressState.RollingBack;
            }

            if (state == StepStateNames.Failed)
            {
                if (rollbackState == RollbackState.RollingBackForce)
                {
                    return TestCommandProgressState.ForceCancelled;
                }

                if (rollbackState == RollbackState.RollingBackDueToUserCancel)
                {
                    return TestCommandProgressState.Cancelled;
                }
            }

            switch (state)
            {
                case StepStateNames.IntentSaved:
                    return TestCommandProgressState.Running;
                case StepStateNames.LookingUpState:
                    return TestCommandProgressState.Running;
                case StepStateNames.PerformingActions:
                    return TestCommandProgressState.Running;
                case StepStateNames.CompletedSuccessfully:
                    return TestCommandProgressState.Completed;
                case StepStateNames.Failed:
                    return TestCommandProgressState.Faulted;
                default:
                    return TestCommandProgressState.Invalid;
            }
        }

        internal static bool IsPrimaryOrSecondary(StatefulServiceReplica replica)
        {
            return replica.ReplicaRole == ReplicaRole.Primary || replica.ReplicaRole == ReplicaRole.ActiveSecondary;
        }

        internal static bool IsReplicaUp(StatefulServiceReplica replica)
        {
            return replica.ReplicaStatus == ServiceReplicaStatus.InBuild ||
                replica.ReplicaStatus == ServiceReplicaStatus.Ready ||
                replica.ReplicaStatus == ServiceReplicaStatus.Standby;
        }

        // Select a quorum of P and S that are not Down or Dropped
        internal static List<StatefulServiceReplica> GetReplicasForPartialLoss(Guid operationId, List<StatefulServiceReplica> replicaList)
        {
            List<StatefulServiceReplica> tempReplicas = new List<StatefulServiceReplica>();
            foreach (StatefulServiceReplica replica in replicaList)
            {
                if (FaultAnalysisServiceUtility.IsPrimaryOrSecondary(replica) && FaultAnalysisServiceUtility.IsReplicaUp(replica))
                {
                    TestabilityTrace.TraceSource.WriteInfo(TraceType, "DEBUG {0} temp adding {1},{2},{3}", operationId, replica.Id, replica.ReplicaRole, replica.ReplicaStatus);
                    tempReplicas.Add(replica);
                }
            }

            int replicasToRestartWithoutPrimary = tempReplicas.Count / 2;
            StatefulServiceReplica primary = tempReplicas.Where(r => r.ReplicaRole == ReplicaRole.Primary).FirstOrDefault();
            if (primary == null)
            {
                return null;
            }

            List<StatefulServiceReplica> targetReplicas = new List<StatefulServiceReplica>(replicasToRestartWithoutPrimary + 1);
            TestabilityTrace.TraceSource.WriteInfo(TraceType, "DEBUG {0} target adding primary {1},{2},{3}", operationId, primary.Id, primary.ReplicaRole, primary.ReplicaStatus);
            targetReplicas.Add(primary);
            tempReplicas.Remove(primary);

            for (int i = 0; i < replicasToRestartWithoutPrimary; i++)
            {
                TestabilityTrace.TraceSource.WriteInfo(TraceType, "DEBUG {0} target adding {1},{2},{3}", operationId, tempReplicas[i].Id, tempReplicas[i].ReplicaRole, tempReplicas[i].ReplicaStatus);
                targetReplicas.Add(tempReplicas[i]);
            }

            return targetReplicas;
        }
    }
}