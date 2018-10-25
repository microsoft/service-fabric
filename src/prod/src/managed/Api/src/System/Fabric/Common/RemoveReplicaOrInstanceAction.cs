// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RemoveReplicaAction : FabricTestAction<RemoveReplicaResult>
    {
        private const string TraceSource = "RemoveReplicaAction";

        public RemoveReplicaAction()
        {
            this.CompletionMode = CompletionMode.Verify;
            this.ForceRemove = false;
        }

        public RemoveReplicaAction(ReplicaSelector replicaSelector)
        {
            this.ReplicaSelector = replicaSelector;
            this.CompletionMode = CompletionMode.Verify;
            this.ForceRemove = false;
        }

        public bool ForceRemove { get; set; }

        public ReplicaSelector ReplicaSelector { get; set; }

        public CompletionMode CompletionMode { get; set; }

        public string NodeName { get; set; }

        public Guid? PartitionId { get; set; }

        public long? ReplicaId { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(RemoveReplicaActionHandler); }
        }

        private class RemoveReplicaActionHandler : FabricTestActionHandler<RemoveReplicaAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, RemoveReplicaAction action, CancellationToken cancellationToken)
            {
                TimeoutHelper helper = new TimeoutHelper(action.ActionTimeout);
                
                string nodeName = action.NodeName;
                Guid? partitionId = action.PartitionId;
                long? replicaId = action.ReplicaId;
                SelectedReplica replicaSelectorResult = SelectedReplica.None;

                if (string.IsNullOrEmpty(nodeName) ||
                    !partitionId.HasValue ||
                    !replicaId.HasValue)
                {
                    ThrowIf.Null(action.ReplicaSelector, "ReplicaSelector");

                    var getReplicaStateAction = new GetSelectedReplicaStateAction(action.ReplicaSelector)
                    {
                        RequestTimeout = action.RequestTimeout,
                        ActionTimeout = helper.GetRemainingTime()
                    };

                    await testContext.ActionExecutor.RunAsync(getReplicaStateAction, cancellationToken).ConfigureAwait(false);
                    var replicaStateActionResult = getReplicaStateAction.Result;
                    ReleaseAssert.AssertIf(replicaStateActionResult == null, "replicaStateActionResult cannot be null");
                    replicaSelectorResult = replicaStateActionResult.Item1;

                    partitionId = replicaStateActionResult.Item1.SelectedPartition.PartitionId;

                    Replica replicaStateResult = replicaStateActionResult.Item2;
                    ReleaseAssert.AssertIf(replicaStateResult == null, "replicaStateResult cannot be null");

                    nodeName = replicaStateResult.NodeName;
                    replicaId = replicaStateResult.Id;
                }

                ThrowIf.IsFalse(partitionId.HasValue, "PartitionID");
                ThrowIf.IsFalse(replicaId.HasValue, "ReplicaID");

                bool forceRemove = action.ForceRemove;

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.ServiceManager.RemoveReplicaAsync(
                        nodeName,
                        partitionId.Value,
                        replicaId.Value,
                        forceRemove,
                        action.RequestTimeout,
                        cancellationToken),
                    FabricClientRetryErrors.RemoveReplicaErrors.Value,
                    helper.GetRemainingTime(),
                    cancellationToken);

                if (action.CompletionMode == CompletionMode.Verify)
                {
                    // Check that replica on selected node has been removed i.e. the replica id does not exist anymore. 
                    bool success = false;
                    while (helper.GetRemainingTime() > TimeSpan.Zero)
                    {
                        var replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                                partitionId.Value, 
                                replicaId.Value, 
                                action.RequestTimeout, 
                                cancellationToken),
                            helper.GetRemainingTime(),
                            cancellationToken).ConfigureAwait(false);

                        bool dropped = replicasResult.Count == 0;
                        if (!dropped)
                        {
                            // Since we added a replica filter the result should contain the replica or none
                            ReleaseAssert.AssertIf(replicasResult.Count > 1, "More than 1 replica returned with replica filter {0}:{1}", partitionId.Value, replicaId.Value);
                            ReleaseAssert.AssertIf(replicasResult[0].Id != replicaId, "Incorrect replica Id {0} returned by query instead of {1}", replicasResult[0].Id, replicaId);
                            dropped = replicasResult[0].ReplicaStatus == ServiceReplicaStatus.Dropped;
                        }

                        if (dropped)
                        {
                            success = true;
                            break;
                        }

                        ActionTraceSource.WriteInfo(TraceSource, "Replica = {0}:{1} not yet completely removed. Retrying...", partitionId.Value, replicaId.Value);
                        await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken);
                    }

                    if (!success)
                    {
                        throw new TimeoutException(StringHelper.Format(StringResources.Error_TestabilityActionTimeout,
                            "RemoveReplica",
                            StringHelper.Format("{0}:{1}", partitionId.Value, replicaId.Value)));
                    }
                }

                action.Result = new RemoveReplicaResult(replicaSelectorResult);
                ResultTraceString = StringHelper.Format(
                    "RemoveReplicaOrInstance succeeded by removing replica {0}:{1} on node {2} with CompletionMode {3}",
                    partitionId.Value,
                    replicaId.Value,
                    nodeName,
                    action.CompletionMode);
            }
        }
    }
}