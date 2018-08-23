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

    internal class StartNodeAction : FabricTestAction<StartNodeResult>
    {
        private const string TraceSource = "StartNodeAction";

        public StartNodeAction(string nodeName, BigInteger nodeInstance)
        {
            this.NodeName = nodeName;
            this.NodeInstance = nodeInstance;
            this.IPAddressOrFQDN = string.Empty;
            this.ClusterConnectionPort = 0;
            this.CompletionMode = CompletionMode.Verify;
        }

        public string NodeName { get; set; }

        public BigInteger NodeInstance { get; set; }

        public CompletionMode CompletionMode { get; set; }

        public string IPAddressOrFQDN { get; set; }

        public int ClusterConnectionPort { get; set; }

        internal override Type ActionHandlerType
        {
            get { return typeof(StartNodeActionHandler); }
        }

        private class StartNodeActionHandler : FabricTestActionHandler<StartNodeAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(FabricTestContext testContext, StartNodeAction action, CancellationToken cancellationToken)
            {
                ThrowIf.Null(action.NodeName, "NodeName");
                this.helper = new TimeoutHelper(action.ActionTimeout);
                string nodeName = action.NodeName;
                BigInteger nodeInstance = action.NodeInstance;

                if (nodeInstance == BigInteger.MinusOne)
                {
                    var nodeInfo = await GetCurrentNodeInfoAsync(testContext, action, cancellationToken);
                    if (nodeInfo == null)
                    {
                        throw new FabricException(StringResources.Error_NodeNotFound, FabricErrorCode.NodeNotFound);
                    }

                    nodeInstance = nodeInfo.NodeInstanceId;
                }

                await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                    () => testContext.FabricClient.FaultManager.StartNodeUsingNodeNameAsync(
                        nodeName, 
                        nodeInstance, 
                        action.IPAddressOrFQDN, 
                        action.ClusterConnectionPort, 
                        action.RequestTimeout, cancellationToken),
                    this.helper.GetRemainingTime(),
                    cancellationToken).ConfigureAwait(false);

                if (action.CompletionMode == CompletionMode.Verify)
                {                    
                    bool success = false;
                    while (this.helper.GetRemainingTime() > TimeSpan.Zero)
                    {
                        var nodeInfo = await this.GetCurrentNodeInfoAsync(testContext, action, cancellationToken).ConfigureAwait(false);
                        
                        if (nodeInfo != null && nodeInfo.NodeInstanceId > nodeInstance && nodeInfo.IsNodeUp)
                        {
                            success = true;
                            break;
                        }

                        ActionTraceSource.WriteInfo(TraceSource, "NodeName = {0} not yet Started. Retrying...", action.NodeName);
                        await AsyncWaiter.WaitAsync(TimeSpan.FromSeconds(5), cancellationToken).ConfigureAwait(false);
                    }

                    if (!success)
                    {
                        throw new TimeoutException(StringHelper.Format(StringResources.Error_TestabilityActionTimeout,
                            "StartNode",
                            action.NodeName + ":" + action.NodeInstance));
                    }
                }

                action.Result = new StartNodeResult(action.NodeName, nodeInstance);
                this.ResultTraceString = StringHelper.Format("StartNodeAction succeeded for {0}:{1} with CompletionMode = {2}", action.NodeName, nodeInstance, action.CompletionMode);
            }

            private async Task<NodeInfo> GetCurrentNodeInfoAsync(FabricTestContext testContext, StartNodeAction action, CancellationToken cancellationToken)
            {
                var nodesInfo = await testContext.FabricCluster.GetLatestNodeInfoAsync(action.RequestTimeout, this.helper.GetRemainingTime(), cancellationToken).ConfigureAwait(false);
                var nodeInfo = nodesInfo.FirstOrDefault(n => n.NodeName == action.NodeName);
                return nodeInfo;
            }
        }
    }
}