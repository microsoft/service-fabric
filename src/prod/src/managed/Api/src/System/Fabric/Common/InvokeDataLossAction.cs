// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Description;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class InvokeDataLossAction : FabricTestAction<InvokeDataLossResult>
    {
        public InvokeDataLossAction(PartitionSelector partitionSelector, DataLossMode dataLossMode)
        {
            this.PartitionSelector = partitionSelector;
            this.DataLossMode = dataLossMode;
        }

        public PartitionSelector PartitionSelector { get; set; }

        public DataLossMode DataLossMode { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(InvokeDataLossActionHandler); }
        }

        private class InvokeDataLossActionHandler : FabricTestActionHandler<InvokeDataLossAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, InvokeDataLossAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.PartitionSelector, "PartitionSelector");

                var helper = new TimeoutHelper(action.ActionTimeout);

                ServiceDescription result = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                        action.PartitionSelector.ServiceName, 
                        action.RequestTimeout, 
                        cancellationToken),
                    helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (result.Kind != ServiceDescriptionKind.Stateful)
                {
                    throw new InvalidOperationException(StringHelper.Format(StringResources.Error_InvalidServiceTypeTestability, "DataLoss", "Stateful", action.PartitionSelector.ServiceName, "Stateless"));
                }

                var getPartitionStateAction = new GetSelectedPartitionStateAction(action.PartitionSelector)
                {
                    RequestTimeout = action.RequestTimeout,
                    ActionTimeout = helper.GetRemainingTime()
                };

                await testContext.ActionExecutor.RunAsync(getPartitionStateAction, cancellationToken).ConfigureAwait(false);
                Guid partitionId = getPartitionStateAction.Result.PartitionId;
               
                long preDataLossNumber = 0;

                ServicePartitionList partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.QueryManager.GetPartitionListAsync(
                        action.PartitionSelector.ServiceName, 
                        null, 
                        action.RequestTimeout, 
                        cancellationToken),
                    helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                bool partitionFound = false;
                foreach (StatefulServicePartition partition in partitionsResult)
                {
                    if (partition.PartitionInformation.Id == partitionId)
                    {
                        preDataLossNumber = partition.PrimaryEpoch.DataLossNumber;
                        partitionFound = true;
                        break;
                    }
                }

                if (!partitionFound)
                {
                    throw new FabricException(StringHelper.Format(StringResources.Error_PartitionNotFound), FabricErrorCode.PartitionNotFound);
                }

                long postDataLossNumber = preDataLossNumber;

                do
                {
                    ActionTraceSource.WriteInfo(
                        TraceType,
                        "InvokeDataLossAction action pending time:{0}",
                        helper.GetRemainingTime());

                    if (helper.GetRemainingTime() <= TimeSpan.Zero)
                    {
                        throw new TimeoutException(StringHelper.Format(StringResources.Error_TestabilityActionTimeout, "InvokeDataLoss", partitionId));
                    }

                    ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                            partitionId, 
                            0, 
                            action.RequestTimeout, 
                            cancellationToken),
                        helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                    ServiceReplicaList fmReplicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                            Constants.FmPartitionId, 
                            0, 
                            action.RequestTimeout, 
                            cancellationToken),
                        helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                    string fmPrimaryNodeName = string.Empty;
                    var readyFMReplicas = fmReplicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();
                    foreach (var replica in readyFMReplicas)
                    {
                        StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                        ReleaseAssert.AssertIf(statefulReplica == null, "FM Replica is not a stateful replica");
                        if (statefulReplica.ReplicaRole == ReplicaRole.Primary)
                        {
                            fmPrimaryNodeName = replica.NodeName;
                        }
                    }

                    if (string.IsNullOrEmpty(fmPrimaryNodeName))
                    {
                        throw new FabricException(StringHelper.Format(StringResources.Error_PartitionPrimaryNotReady, "FailoverManager"), FabricErrorCode.NotReady);
                    }

                    UnreliableTransportBehavior behavior = new UnreliableTransportBehavior("*", "DoReconfiguration");
                    behavior.AddFilterForPartitionId(partitionId);
                    string behaviorName = "BlockDoReconfiguration";

                    await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.TestManager.AddUnreliableTransportBehaviorAsync(
                            fmPrimaryNodeName, 
                            behaviorName, 
                            behavior, 
                            action.RequestTimeout, 
                            cancellationToken),
                        helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                    // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                    // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                    await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);

                    bool triedToRemovedBehavior = false;

                    try
                    {
                        var stableReplicasToRemove = replicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();

                        ActionTraceSource.WriteInfo(TraceType, "Total number of replicas found {0}:{1}", replicasResult.Count(), stableReplicasToRemove.Count());

                        int replicasToRestartWithoutPrimary = 
                            action.DataLossMode == DataLossMode.FullDataLoss 
                                ? stableReplicasToRemove.Length - 1
                                : (stableReplicasToRemove.Length + 1) / 2 - 1;

                        foreach (var replica in stableReplicasToRemove)
                        {
                            var currentReplica = replica;
                            StatefulServiceReplica statefulReplica = currentReplica as StatefulServiceReplica;
                            ReleaseAssert.AssertIf(statefulReplica == null, "Service Replica is not of stateful type even though service is stateful");

                            ActionTraceSource.WriteInfo(
                                TraceType, 
                                "Inspecting replica {0}:{1} with role {2} and status {3} to induce data loss",
                                currentReplica.Id, 
                                partitionId, 
                                statefulReplica.ReplicaRole,
                                statefulReplica.ReplicaStatus);

                            if (statefulReplica.ReplicaRole != ReplicaRole.Primary)
                            {
                                replicasToRestartWithoutPrimary--;
                            }

                            if (replicasToRestartWithoutPrimary >= 0 || statefulReplica.ReplicaRole == ReplicaRole.Primary)
                            {
                                ActionTraceSource.WriteInfo(TraceType, "Removing replica {0}:{1} to induce data loss", currentReplica.Id, partitionId);

                                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                   () => testContext.FabricClient.FaultManager.RemoveReplicaAsync(
                                       currentReplica.NodeName,
                                       partitionId,
                                       currentReplica.Id,
                                       CompletionMode.DoNotVerify,
                                       false, /*force remove*/
                                       action.RequestTimeout.TotalSeconds,
                                       cancellationToken),
                                    helper.GetRemainingTime(),
                                    cancellationToken).ConfigureAwait(false);
                            }
                        }

                        triedToRemovedBehavior = true;
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                             () => testContext.FabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                                 fmPrimaryNodeName, 
                                 behaviorName, 
                                 action.RequestTimeout, 
                                 cancellationToken),
                             FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                             helper.GetRemainingTime(),
                             cancellationToken).ConfigureAwait(false);

                        // TODO: Wait for some time so that the removal of this unreliable transport behavior can be read from the files.
                        // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successully applied
                        await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);

                        // retry check for whether data loss number has increased 5 times else do the entire process again
                        const int maxRetryCount = 5;
                        int retryCount = 0;
                        do
                        {
                            partitionsResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                                () => testContext.FabricClient.QueryManager.GetPartitionListAsync(
                                    action.PartitionSelector.ServiceName, 
                                    null, 
                                    action.RequestTimeout, 
                                    cancellationToken),
                                FabricClientRetryErrors.GetPartitionListFabricErrors.Value,
                                helper.GetRemainingTime(),
                                cancellationToken).ConfigureAwait(false);

                            partitionFound = false;
                            foreach (StatefulServicePartition partition in partitionsResult)
                            {
                                if (partition.PartitionInformation.Id == partitionId)
                                {
                                    postDataLossNumber = partition.PrimaryEpoch.DataLossNumber;
                                    partitionFound = true;
                                    break;
                                }
                            }

                            if (!partitionFound)
                            {
                                throw new FabricException(StringHelper.Format(StringResources.Error_PartitionNotFound), FabricErrorCode.PartitionNotFound);
                            }

                            ActionTraceSource.WriteInfo(
                                TraceType,
                                "Checking data loss numbers for partition {0} with retryCount {1}. Current numbers {2}:{3}",
                                partitionId,
                                retryCount,
                                preDataLossNumber,
                                postDataLossNumber);

                            if (postDataLossNumber != preDataLossNumber)
                            {
                                break;
                            }

                            await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken);
                            ++retryCount;

                        } while (retryCount < maxRetryCount);

                    }
                    finally
                    {
                        if (!triedToRemovedBehavior)
                        {
                            ActionTraceSource.WriteWarning(TraceType, "Exception after adding behavior to block messages. Removing behavior synchronously");
                            FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                             () => testContext.FabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                                 fmPrimaryNodeName,
                                 behaviorName,
                                 action.RequestTimeout,
                                 cancellationToken),
                             FabricClientRetryErrors.RemoveUnreliableTransportBehaviorErrors.Value,
                             helper.GetRemainingTime(),
                             cancellationToken).GetAwaiter().GetResult();

                            // TODO: Wait for some time so that the removal of this unreliable transport behavior can be read from the files.
                            // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successully applied
                            Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).GetAwaiter().GetResult();
                        }
                    }
                }
                while (postDataLossNumber == preDataLossNumber);

                ActionTraceSource.WriteInfo(
                    TraceType,
                    "InvokeDataLossAction action completed postDataLossNumber:{0}, preDataLossNumber:{1}",
                     postDataLossNumber, preDataLossNumber);

                action.Result = new InvokeDataLossResult(getPartitionStateAction.Result);
                this.ResultTraceString = StringHelper.Format("InvokeDataLossAction succeeded for {0} with DatalossMode = {1}", partitionId, action.DataLossMode);
            }
        }
    }
}