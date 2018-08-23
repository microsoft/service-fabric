// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    internal class MoveSecondaryAction : FabricTestAction<MoveSecondaryResult>
    {
        public MoveSecondaryAction(string currentSecondaryNodeName, string newSecondaryNodeName, PartitionSelector partitionSelector, bool ignoreConstraints)
        {
            this.CurrentSecondaryNodeName = currentSecondaryNodeName;
            this.NewSecondaryNodeName = newSecondaryNodeName;
            this.PartitionSelector = partitionSelector;
            this.IgnoreConstraints = ignoreConstraints;
        }

        public bool IgnoreConstraints { get; set; }

        public string CurrentSecondaryNodeName { get; set; }

        public const string TraceSource = "MoveSecondary";

        public string NewSecondaryNodeName { get; set; }

        public PartitionSelector PartitionSelector { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(MoveSecondaryActionHandler); }
        }

        private class MoveSecondaryActionHandler : FabricTestActionHandler<MoveSecondaryAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, MoveSecondaryAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.PartitionSelector, "PartitionSelector");

                this.helper = new TimeoutHelper(action.ActionTimeout);

                string newSecondaryNode = action.NewSecondaryNodeName;
                string currentSecondaryNode = action.CurrentSecondaryNodeName;

                var getPartitionStateAction = new GetSelectedPartitionStateAction(action.PartitionSelector)
                {
                    RequestTimeout = action.RequestTimeout,
                    ActionTimeout = this.helper.GetRemainingTime()
                };

                await testContext.ActionExecutor.RunAsync(getPartitionStateAction, cancellationToken).ConfigureAwait(false);
                Guid partitionId = getPartitionStateAction.Result.PartitionId;

                if (!action.IgnoreConstraints)
                {
                    // get current primary replica node name.
                    ServiceReplicaList replicasResult = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.QueryManager.GetReplicaListAsync(
                            partitionId,
                            0,
                            action.RequestTimeout,
                            cancellationToken),
                        this.helper.GetRemainingTime(),
                        cancellationToken).ConfigureAwait(false);

                    string currentPrimaryNodeInfo = string.Empty;
                    List<string> currentSecReplicaNodes = new List<string>();
                    foreach (var replica in replicasResult)
                    {
                        StatefulServiceReplica statefulReplica = replica as StatefulServiceReplica;
                        if (statefulReplica == null)
                        {
                            throw new InvalidOperationException(StringHelper.Format(StringResources.Error_InvalidServiceTypeTestability, "MoveSecondary", "Stateful", action.PartitionSelector.ServiceName, "Stateless"));
                        }

                        if (statefulReplica.ReplicaRole == ReplicaRole.Primary)
                        {
                            currentPrimaryNodeInfo = statefulReplica.NodeName;
                            if (!string.IsNullOrEmpty(newSecondaryNode) && newSecondaryNode == statefulReplica.NodeName)
                            {
                                throw new FabricException(
                                    StringHelper.Format(StringResources.Error_InvalidNodeNameProvided, newSecondaryNode, "MoveSecondary", "Primary exists on node"),
                                    FabricErrorCode.AlreadyPrimaryReplica);
                            }
                        }
                        else if (statefulReplica.ReplicaRole == ReplicaRole.ActiveSecondary)
                        {
                            currentSecReplicaNodes.Add(statefulReplica.NodeName);
                            if (!string.IsNullOrEmpty(newSecondaryNode) && newSecondaryNode == statefulReplica.NodeName)
                            {
                                throw new FabricException(
                                    StringHelper.Format(StringResources.Error_InvalidNodeNameProvided, newSecondaryNode, "MoveSecondary", "Secondary exists on node"),
                                    FabricErrorCode.AlreadySecondaryReplica);
                            }
                        }
                    }

                    if (currentSecReplicaNodes.Count == 0)
                    {
                        throw new InvalidOperationException(StringResources.Error_NoSecondariesInReplicaSet);
                    }

                    if (string.IsNullOrEmpty(currentSecondaryNode))
                    {
                        int num = testContext.Random.Next(currentSecReplicaNodes.Count);
                        currentSecondaryNode = currentSecReplicaNodes.ElementAt(num);
                    }

                    if (!currentSecReplicaNodes.Contains(currentSecondaryNode))
                    {
                        throw new FabricException(
                            StringHelper.Format(StringResources.Error_InvalidNodeNameProvided, newSecondaryNode, "MoveSecondary", "Current node does not have a secondary replica"),
                            FabricErrorCode.InvalidReplicaStateForReplicaOperation);
                    }
                }

                ReleaseAssert.AssertIf(string.IsNullOrEmpty(currentSecondaryNode), "Current node name cannot be null or empty.");
                ReleaseAssert.AssertIf(newSecondaryNode == currentSecondaryNode, "Current and New node names are same.");

                ActionTraceSource.WriteInfo(TraceSource, "Calling move secondary with current node {0}, new node {1}, partition {2}", currentSecondaryNode, string.IsNullOrEmpty(newSecondaryNode) ? "Random" : newSecondaryNode, partitionId);
                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.FaultManager.MoveSecondaryUsingNodeNameAsync(
                        currentSecondaryNode,
                        newSecondaryNode,
                        getPartitionStateAction.Result.ServiceName,
                        partitionId,
                        action.IgnoreConstraints,
                        action.RequestTimeout,
                        cancellationToken),
                    FabricClientRetryErrors.MoveSecondaryFabricErrors.Value,
                    this.helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                action.Result = new MoveSecondaryResult(currentSecondaryNode, newSecondaryNode, getPartitionStateAction.Result);
                this.ResultTraceString = StringHelper.Format(
                    "MoveSecondaryAction succeeded for moving Primary for {0} from {1} to {2}.",
                    partitionId,
                    currentSecondaryNode,
                    newSecondaryNode);
            }
        }
    }
}