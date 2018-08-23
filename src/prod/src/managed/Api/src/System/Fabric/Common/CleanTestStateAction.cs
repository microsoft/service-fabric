// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Threading;
    using System.Threading.Tasks;

    internal class CleanTestStateAction : FabricTestAction
    {
        internal override Type ActionHandlerType
        {
            get { return typeof(CleanTestStateActionHandler); }
        }

        private class CleanTestStateActionHandler : FabricTestActionHandler<CleanTestStateAction>
        {
            private TimeoutHelper helper;

            protected override async Task ExecuteActionAsync(
                FabricTestContext testContext,
                CleanTestStateAction action,
                CancellationToken cancellationToken)
            {
                this.helper = new TimeoutHelper(action.ActionTimeout);

                var nodes = await testContext.FabricCluster.GetLatestNodeInfoAsync(action.RequestTimeout, this.helper.GetRemainingTime(), cancellationToken).ConfigureAwait(false);
                foreach (var nodeInfo in nodes)
                {
                    if (nodeInfo.IsNodeUp)
                    {
                        var info = nodeInfo;
                        await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                             () => testContext.FabricClient.TestManager.RemoveUnreliableTransportBehaviorAsync(
                                 info.NodeName,
                                 "*",
                                 action.RequestTimeout,
                                 cancellationToken),
                             this.helper.GetRemainingTime(),
                             cancellationToken).ConfigureAwait(false);

                        // TODO: Wait for some time so that the removal of this unreliable transport behavior can be read from the files.
                        // Bug#2271465 - Unreliable transport through API should return only once the behavior has been successully applied
                        await Task.Delay(TimeSpan.FromSeconds(5.0), cancellationToken);

                        ActionTraceSource.WriteInfo(TraceType, "Test state cleaned for node:{0}", nodeInfo.NodeName);
                    }
                    else
                    {
                        ActionTraceSource.WriteInfo(TraceType, "Test clean failed to start node {0}", nodeInfo.NodeName);
                    }
                }

                ResultTraceString = StringHelper.Format("CleanTestStateAction succeeded");
            }
        }
    }
}