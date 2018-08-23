// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Threading.Tasks;
    using System.Threading;

    interface IPackageRetriever
    {
        Task<ClusterUpgradeCommandParameter> GetUpgradeCommandParamterAsync(
            FabricUpgradeProgress currentUpgradeProgress,
            TimeSpan timeout,
            CancellationToken token);
    }
}