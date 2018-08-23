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
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;

    internal class RestartNodeAction : FabricTestAction<RestartNodeResult>
    {
        private const string TraceSource = "RestartNodeAction";

        public RestartNodeAction(string nodeName, BigInteger nodeInstance)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.CompletionMode = CompletionMode.Verify;
        }

        public RestartNodeAction(string nodeName, BigInteger nodeInstance, bool createFabricDump)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.CreateFabricDump = createFabricDump;
            this.CompletionMode = CompletionMode.Verify;
        }

        public RestartNodeAction(ReplicaSelector replicaSelector)
        {
            this.ReplicaSelector = replicaSelector;
            this.CompletionMode = CompletionMode.Verify;
        }

        public RestartNodeAction(ReplicaSelector replicaSelector, bool createFabricDump)
        {
            this.ReplicaSelector = replicaSelector;
            this.CreateFabricDump = createFabricDump;
            this.CompletionMode = CompletionMode.Verify;
        }

        public string NodeName { get; set; }

        public BigInteger NodeInstance { get; set; }

        public ReplicaSelector ReplicaSelector { get; set; }

        public CompletionMode CompletionMode { get; set; }

        public bool CreateFabricDump { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(RestartNodeActionHandler); }
        }

        private class RestartNodeActionHandler : FabricTestActionHandler<RestartNodeAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, RestartNodeAction action, CancellationToken cancellationToken)
            {
                ActionTraceSource.WriteInfo(TraceSource, "Enter RestartNodeAction/ExecuteActionAsync: operationTimeout='{0}', requestTimeout='{1}'", action.ActionTimeout, action.RequestTimeout);

                this.helper = new TimeoutHelper(action.ActionTimeout);
                SelectedReplica selectedReplica = SelectedReplica.None;
                string nodeName = action.NodeName;
                BigInteger nodeInstance = action.NodeInstance;
                bool createFabricDump = action.CreateFabricDump;

                if (string.IsNullOrEmpty(nodeName))
                {
                    ThrowIf.Null(action.ReplicaSelector, "ReplicaSelector");

                    var getReplicaStateAction = new GetSelectedReplicaStateAction(action.ReplicaSelector)
                    {
                        RequestTimeout = action.RequestTimeout,
                        ActionTimeout = helper.GetRemainingTime()
                    };

                    await testContext.ActionExecutor.RunAsync(getReplicaStateAction, cancellationToken).ConfigureAwait(false);
                    var replicaStateActionResult = getReplicaStateAction.Result;
                    ReleaseAssert.AssertIf(replicaStateActionResult == null , "replicaStateActionResult cannot be null");
                    selectedReplica = replicaStateActionResult.Item1;
                    Replica replicaStateResult = replicaStateActionResult.Item2;
                    ReleaseAssert.AssertIf(replicaStateResult == null, "replicaStateResult cannot be null");

                    nodeName = replicaStateResult.NodeName;
                    nodeInstance = BigInteger.MinusOne;
                }

                if (nodeInstance == BigInteger.MinusOne)
                {
                    var nodeInfo = await this.GetCurrentNodeInfoAsync(testContext, nodeName, action, cancellationToken).ConfigureAwait(false);
                    nodeInstance = nodeInfo.NodeInstanceId;
                }

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.FaultManager.RestartNodeUsingNodeNameAsync(
                        nodeName,
                        nodeInstance,
                        createFabricDump,
                        action.RequestTimeout,
                        cancellationToken),
                    this.helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (action.CompletionMode == CompletionMode.Verify)
                {
                    bool success = false;
                    while (this.helper.GetRemainingTime() > TimeSpan.Zero)
                    {
                        var nodeInfo = await this.GetCurrentNodeInfoAsync(testContext, nodeName, action, cancellationToken).ConfigureAwait(false);
                        if (nodeInfo.NodeInstanceId > nodeInstance && nodeInfo.IsNodeUp)
                        {
                            success = true;
                            break;
                        }

                        ActionTraceSource.WriteInfo(TraceSource, "NodeName = {0} not yet restarted. '{1}' seconds remain. Retrying...", nodeName, this.helper.GetRemainingTime().TotalSeconds);
                        await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken);
                    }

                    if (!success)
                    {
                        throw new TimeoutException(StringHelper.Format(StringResources.Error_TestabilityActionTimeout,
                            "RestartNode",
                            nodeName));
                    }
                }

                // create result
                action.Result = new RestartNodeResult(selectedReplica, new NodeResult(nodeName, nodeInstance));

                ResultTraceString = StringHelper.Format("RestartNodeAction succeeded for {0}:{1} with CompletionMode = {2}", nodeName, nodeInstance, action.CompletionMode);
            }

            private async Task<NodeInfo> GetCurrentNodeInfoAsync(FabricTestContext testContext, string nodeName, RestartNodeAction action, CancellationToken cancellationToken)
            {
                var nodesInfo = await testContext.FabricCluster.GetLatestNodeInfoAsync(action.RequestTimeout, this.helper.GetRemainingTime(), cancellationToken).ConfigureAwait(false);

                var nodeInfo = nodesInfo.FirstOrDefault(n => n.NodeName == nodeName);
                if (nodeInfo == null)
                {
                    throw new FabricException(StringResources.Error_NodeNotFound, FabricErrorCode.NodeNotFound);
                }

                return nodeInfo;
            }
        }
    }
}