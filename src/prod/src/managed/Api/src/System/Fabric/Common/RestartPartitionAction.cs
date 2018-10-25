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

    internal class RestartPartitionAction : FabricTestAction<RestartPartitionResult>
    {
        public RestartPartitionAction(PartitionSelector partitionSelector, RestartPartitionMode restartPartitionMode)
        {
            this.PartitionSelector = partitionSelector;
            this.RestartPartitionMode = restartPartitionMode;
        }

        public PartitionSelector PartitionSelector { get; set; }

        public RestartPartitionMode RestartPartitionMode { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(RestartPartitionActionHandler); }
        }

        private class RestartPartitionActionHandler : FabricTestActionHandler<RestartPartitionAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, RestartPartitionAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.PartitionSelector, "partitionSelector");

                this.helper = new TimeoutHelper(action.ActionTimeout);

                // get service info so we can validate if the operation is valid
                ServiceDescription result = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.ServiceManager.GetServiceDescriptionAsync(
                        action.PartitionSelector.ServiceName, 
                        action.RequestTimeout, 
                        cancellationToken),
                    this.helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (result.Kind != ServiceDescriptionKind.Stateful && action.RestartPartitionMode == RestartPartitionMode.OnlyActiveSecondaries)
                {
                    throw new InvalidOperationException(StringHelper.Format(StringResources.Error_InvalidServiceTypeTestability, "RestartPartitionMode.OnlyActiveSecondaries", "Stateful", action.PartitionSelector.ServiceName, "Stateless"));
                }

                bool hasPersistedState = false;
                if (result.Kind == ServiceDescriptionKind.Stateful)
                {
                    StatefulServiceDescription statefulDescription = result as StatefulServiceDescription;
                    ReleaseAssert.AssertIf(statefulDescription == null, "Stateful service description is not WinFabricStatefulServiceDescription");
                    hasPersistedState = statefulDescription.HasPersistedState;
                }

                // now actually select a partition
                var getPartitionStateAction = new GetSelectedPartitionStateAction(action.PartitionSelector)
                {
                    RequestTimeout = action.RequestTimeout,
                    ActionTimeout = helper.GetRemainingTime()
                };

                await testContext.ActionExecutor.RunAsync(getPartitionStateAction, cancellationToken);
                Guid partitionId = getPartitionStateAction.Result.PartitionId;

                // get replicas for target
                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                        partitionId, 
                        0, 
                        action.RequestTimeout, 
                        cancellationToken),
                    this.helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                // get replicas for fm in order to get the primary
                ServiceReplicaList fmReplicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                        Constants.FmPartitionId, 
                        0, 
                        action.RequestTimeout, 
                        cancellationToken),
                    this.helper.GetRemainingTime(),
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

                ////------------------------------------------------------
                // target ut at the fm primary only
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
                   this.helper.GetRemainingTime(),
                   cancellationToken).ConfigureAwait(false);

                // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken).ConfigureAwait(false);

                bool triedToRemovedBehavior = false;

                // inspect the actual replicas to restart, only operate on stable ones
                try
                {
                    var stableReplicasToRestart = replicasResult.Where(r => r.ReplicaStatus == ServiceReplicaStatus.Ready).ToArray();

                    foreach (var replica in stableReplicasToRestart)
                    {
                        var currentReplica = replica;
                        if (action.RestartPartitionMode == RestartPartitionMode.OnlyActiveSecondaries)
                        {
                            StatefulServiceReplica statefulReplica = currentReplica as StatefulServiceReplica;
                            ReleaseAssert.AssertIf(statefulReplica == null, "Stateful service replica is not StatefulServiceReplica");
                            if (statefulReplica.ReplicaRole == ReplicaRole.Primary)
                            {
                                continue;
                            }
                        }

                        if (hasPersistedState)
                        {
                            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                               () => testContext.FabricClient.FaultManager.RestartReplicaAsync(
                                   currentReplica.NodeName, 
                                   partitionId, 
                                   currentReplica.Id, 
                                   CompletionMode.DoNotVerify, 
                                   action.RequestTimeout.TotalSeconds, 
                                   cancellationToken),
                               this.helper.GetRemainingTime(),
                               cancellationToken).ConfigureAwait(false);
                        }
                        else
                        {
                            await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                               () => testContext.FabricClient.FaultManager.RemoveReplicaAsync(
                                   currentReplica.NodeName, 
                                   partitionId, 
                                   currentReplica.Id,
                                   CompletionMode.DoNotVerify,
                                   false, /*force remove*/
                                   action.RequestTimeout.TotalSeconds, 
                                   cancellationToken),
                               this.helper.GetRemainingTime(),
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
                     this.helper.GetRemainingTime(),
                     cancellationToken).ConfigureAwait(false);

                    // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                    // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                    await Task.Delay(TimeSpan.FromSeconds(5.0)).ConfigureAwait(false);
                }
                finally
                {
                    // TODO: Provide a way to clear all behaviors just in case. 
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
                         this.helper.GetRemainingTime(),
                         cancellationToken).GetAwaiter().GetResult();

                        // TODO: Wait for some time so that the unreliable transport behavior can be read from the files.
                        // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successfully applied
                        Task.Delay(TimeSpan.FromSeconds(5.0)).GetAwaiter().GetResult();
                    }
                }

                // -- note there's no explict validation

                // action result 
                action.Result = new RestartPartitionResult(getPartitionStateAction.Result);
                ResultTraceString = StringHelper.Format("RestartPartitionAction succeeded for {0} with RestartPartitionMode = {1}", partitionId, action.RestartPartitionMode);
            }
        }
    }
}