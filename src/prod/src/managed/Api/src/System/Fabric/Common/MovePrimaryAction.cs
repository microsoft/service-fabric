// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class MovePrimaryAction : FabricTestAction<MovePrimaryResult>
    {
        public MovePrimaryAction(string nodeName, PartitionSelector partitionSelector, bool ignoreConstraints)
        {
            this.NodeName = nodeName;
            this.PartitionSelector = partitionSelector;
            this.IgnoreConstraints = ignoreConstraints;
        }

        public MovePrimaryAction(PartitionSelector paritionSelector, bool ignoreConstraints)
            : this(string.Empty, paritionSelector, ignoreConstraints)
        { }

        public bool IgnoreConstraints { get; set; }

        public string NodeName { get; set; }

        public PartitionSelector PartitionSelector { get; set; }

        public const string TraceSource = "MovePrimaryAction";

        internal override Type ActionHandlerType
        {
            get { return typeof(MovePrimaryActionHandler); }
        }

        private class MovePrimaryActionHandler : FabricTestActionHandler<MovePrimaryAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, MovePrimaryAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.PartitionSelector, "PartitionSelector");

                this.helper = new TimeoutHelper(action.ActionTimeout);

                string newPrimaryNodeName = action.NodeName;

                var getPartitionStateAction = new GetSelectedPartitionStateAction(action.PartitionSelector)
                {
                    RequestTimeout = action.RequestTimeout,
                    ActionTimeout = this.helper.GetRemainingTime()
                };

                await testContext.ActionExecutor.RunAsync(getPartitionStateAction, cancellationToken);
                Guid partitionId = getPartitionStateAction.Result.PartitionId;

                if (!action.IgnoreConstraints)
                {
                    // select random node where replica's primary not present
                    var nodesInfo = await testContext.FabricCluster.GetLatestNodeInfoAsync(action.RequestTimeout, this.helper.GetRemainingTime(), cancellationToken);

                    if ((nodesInfo == null || nodesInfo.Count() == 0))
                    {
                        throw new InvalidOperationException(StringHelper.Format(StringResources.Error_NotEnoughNodesForTestabilityAction, "MovePrimary"));
                    }

                    ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                            partitionId,
                            0,
                            action.RequestTimeout,
                            cancellationToken),
                        this.helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                    NodeInfo currentPrimaryNodeInfo = null;
                    string currentPrimaryNodeName = string.Empty;
                    foreach (var replica in replicasResult)
                    {
                        StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                        if(statefulReplica == null)
                        {
                            throw new InvalidOperationException(StringHelper.Format(StringResources.Error_InvalidServiceTypeTestability, "MovePrimary", "Stateful", action.PartitionSelector.ServiceName, "Stateless"));
                        }

                        if (statefulReplica.ReplicaRole == ReplicaRole.Primary)
                        {
                            currentPrimaryNodeInfo = nodesInfo.FirstOrDefault(n => n.NodeName == statefulReplica.NodeName);
                            if (!string.IsNullOrEmpty(newPrimaryNodeName) && newPrimaryNodeName == statefulReplica.NodeName)
                            {
                                throw new FabricException(
                                    StringHelper.Format(StringResources.Error_InvalidNodeNameProvided, newPrimaryNodeName, "MovePrimary", "Primary already exists on node"),
                                    FabricErrorCode.AlreadyPrimaryReplica);
                            }

                            break;
                        }
                    }

                    if (currentPrimaryNodeInfo == null)
                    {
                        throw new FabricException(StringHelper.Format(StringResources.Error_PartitionPrimaryNotReady, action.PartitionSelector + ":" + partitionId), FabricErrorCode.NotReady);
                    }

                    currentPrimaryNodeName = currentPrimaryNodeInfo.NodeName;

                    if (newPrimaryNodeName == currentPrimaryNodeName)
                    {
                        throw new FabricException(
                            StringHelper.Format(StringResources.Error_InvalidNodeNameProvided, newPrimaryNodeName, "MovePrimary", "Primary already exists on node"),
                            FabricErrorCode.AlreadyPrimaryReplica);
                    }
                }

                ActionTraceSource.WriteInfo(TraceSource, "Calling move primary with node {0}, partition {1}", string.IsNullOrEmpty(newPrimaryNodeName) ? "Random" : newPrimaryNodeName, partitionId);
                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.FaultManager.MovePrimaryUsingNodeNameAsync(
                        newPrimaryNodeName,
                        getPartitionStateAction.Result.ServiceName,
                        partitionId,
                        action.IgnoreConstraints,
                        action.RequestTimeout,
                        cancellationToken),
                    FabricClientRetryErrors.MovePrimaryFabricErrors.Value,
                    this.helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                action.Result = new MovePrimaryResult(newPrimaryNodeName, getPartitionStateAction.Result);

                ResultTraceString = StringHelper.Format("MovePrimaryAction succeeded for moving Primary for {0}  to node  {1}.", partitionId, newPrimaryNodeName);
            }
        }
    }
}