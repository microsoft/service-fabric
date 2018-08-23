// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal class IsClusterUpgradingAction : FabricTestAction<bool>
    {
        internal override Type ActionHandlerType
        {
            get { return typeof(IsClusterUpgradingActionHandler); }
        }

        private class IsClusterUpgradingActionHandler : FabricTestActionHandler<IsClusterUpgradingAction>
        {
            protected override async Task ExecuteActionAsync(FabricTestContext testContext, IsClusterUpgradingAction action, CancellationToken cancellationToken)
            {
                var currentProgress = await FabricClientRetryHelper.ExecuteFabricActionWithRetryAsync(
                            () => testContext.FabricClient.ClusterManager.GetFabricUpgradeProgressAsync(
                                action.RequestTimeout,
                                cancellationToken),
                            FabricClientRetryErrors.UpgradeFabricErrors.Value,
                            action.ActionTimeout,
                            cancellationToken).ConfigureAwait(false);

                ReleaseAssert.AssertIfNull(currentProgress, "currentProgress");

                action.Result = currentProgress.UpgradeState != FabricUpgradeState.RollingBackCompleted
                    && currentProgress.UpgradeState != FabricUpgradeState.RollingForwardCompleted;
                ResultTraceString = "IsClusterUpgradingAction succeeded";
            }
        }
    }
}