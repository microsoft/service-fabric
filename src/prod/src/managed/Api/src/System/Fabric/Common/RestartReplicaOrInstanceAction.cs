// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RestartReplicaAction : FabricTestAction<RestartReplicaResult>
    {
        public RestartReplicaAction()
        {
            this.CompletionMode = CompletionMode.Verify;
        }

        public RestartReplicaAction(ReplicaSelector replicaSelector)
        {
            this.ReplicaSelector = replicaSelector;
            this.CompletionMode = CompletionMode.Verify;
        }

        public ReplicaSelector ReplicaSelector { get; set; }

        public CompletionMode CompletionMode { get; set; }

        public string NodeName { get; set; }

        public Guid? PartitionId { get; set; }

        public long? ReplicaId { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(RestartReplicaActionHandler); }
        }

        private class RestartReplicaActionHandler : FabricTestActionHandler<RestartReplicaAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, RestartReplicaAction action, CancellationToken cancellationToken)
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
                    replicaSelectorResult = replicaStateActionResult.Item1;
                    ReleaseAssert.AssertIf(replicaSelectorResult == null, "replicaSelectorResult cannot be null");

                    partitionId = replicaStateActionResult.Item1.SelectedPartition.PartitionId;

                    Replica replicaStateResult = replicaStateActionResult.Item2;
                    ReleaseAssert.AssertIf(replicaStateResult == null, "replicaStateResult cannot be null");

                    nodeName = replicaStateResult.NodeName;
                    replicaId = replicaStateResult.Id;
                }

                ThrowIf.IsFalse(partitionId.HasValue, "PartitionID");
                ThrowIf.IsFalse(replicaId.HasValue, "ReplicaID");

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.ServiceManager.RestartReplicaAsync(
                        nodeName,
                        partitionId.Value,
                        replicaId.Value,
                        action.RequestTimeout,
                        cancellationToken),
					FabricClientRetryErrors.RestartReplicaErrors.Value,
                    helper.GetRemainingTime(),
                    cancellationToken);

                if (action.CompletionMode == CompletionMode.Verify)
                {
                    // TODO: Check with failover team to see how to confirm that the replica actually restarted. We do not expose instance id for persisted replicas
                }

                action.Result = new RestartReplicaResult(replicaSelectorResult);
                this.ResultTraceString = StringHelper.Format(
                    "RestartReplicaOrInstance succeeded by restarting replica {0}:{1} node {2} with CompletionMode {3}",
                    partitionId.Value,
                    replicaId.Value,
                    nodeName,
                    action.CompletionMode);
            }
        }
    }
}