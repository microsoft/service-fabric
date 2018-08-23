// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Query;
    using System.Threading;
    using System.Threading.Tasks;

    internal class GetSelectedReplicaStateAction : FabricTestAction<Tuple<SelectedReplica, Replica>>
    {
        public GetSelectedReplicaStateAction(ReplicaSelector replicaSelector)
        {
            this.ReplicaSelector = replicaSelector;
        }

        public ReplicaSelector ReplicaSelector { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(GetSelectedReplicaStateActionHandler); }
        }

        private class GetSelectedReplicaStateActionHandler : FabricTestActionHandler<GetSelectedReplicaStateAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, GetSelectedReplicaStateAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.ReplicaSelector, "ReplicaSelector");

                TimeoutHelper helper = new TimeoutHelper(action.ActionTimeout);

                var getPartitionStateAction = new GetSelectedPartitionStateAction(action.ReplicaSelector.PartitionSelector)
                {
                    RequestTimeout = action.RequestTimeout,
                    ActionTimeout = helper.GetRemainingTime()
                };

                await testContext.ActionExecutor.RunAsync(getPartitionStateAction, cancellationToken);
                Guid partitionId = getPartitionStateAction.Result.PartitionId;

                // TODO: make these actions which store state locally as well. 
                ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync<ServiceReplicaList>(
                    () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                            partitionId, 
                            0, 
                            action.RequestTimeout, 
                            cancellationToken),
                    helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                Replica replicaResult = action.ReplicaSelector.GetSelectedReplica(replicasResult.ToArray(), testContext.Random, true/*skip invalid replicas*/);
                var replicaSelectorResult = new SelectedReplica(replicaResult.Id, getPartitionStateAction.Result); 
                action.Result = new Tuple<SelectedReplica, Replica>(
                    replicaSelectorResult,
                    replicaResult);

                ResultTraceString = StringHelper.Format("ReplicaSelector Selected  Replica {0}", replicaResult.Id);
            }
        }
    }
}