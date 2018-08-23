// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Result;
    using System.Fabric.Strings;
    using System.Linq;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;

    internal class StopNodeAction : FabricTestAction<StopNodeResult>
    {
        private const string TraceSource = "StopNodeAction";

        public StopNodeAction(string nodeName, BigInteger nodeInstance)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.CompletionMode = CompletionMode.Verify;
        }

        public string NodeName { get; set; }

        public BigInteger NodeInstance { get; set; }

        public CompletionMode CompletionMode { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(StopNodeActionHandler); }
        }

        private class StopNodeActionHandler : FabricTestActionHandler<StopNodeAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, StopNodeAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.NodeName, "NodeName");
                this.helper = new TimeoutHelper(action.ActionTimeout);

                string nodeName = action.NodeName;

                BigInteger nodeInstance = action.NodeInstance;
                if (nodeInstance == BigInteger.MinusOne)
                {
                    var nodeInfo = await GetCurrentNodeInfoAsync(testContext, nodeName, action, cancellationToken);
                    nodeInstance = nodeInfo.NodeInstanceId;
                }

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                        () => testContext.FabricClient.FaultManager.StopNodeUsingNodeNameAsync(
                            nodeName, 
                            nodeInstance, 
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
                        if (!nodeInfo.IsNodeUp)
                        {
                            success = true;
                            break;
                        }

                        ActionTraceSource.WriteInfo(TraceSource, "NodeName = {0} not yet stopped. Retrying...", action.NodeName);
                        await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken);
                    }

                    if (!success)
                    {
                        throw new TimeoutException(StringHelper.Format(StringResources.Error_TestabilityActionTimeout,
                            "StopNode",
                            nodeName));
                    }
                }

                action.Result = new StopNodeResult(nodeName, nodeInstance);
                ResultTraceString = StringHelper.Format("StopNodeAction succeeded for {0}:{1} with CompletionMode = {2}", nodeName, nodeInstance, action.CompletionMode);
            }

            private async Task<NodeInfo> GetCurrentNodeInfoAsync(FabricTestContext testContext, string nodeName, StopNodeAction action, CancellationToken cancellationToken)
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