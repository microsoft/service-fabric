// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Fabric.Common;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Fabric.Description;

namespace System.Fabric
{
    interface IUpgradeOrchestrationService
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task StartClusterConfigurationUpgrade(
            ConfigurationUpgradeDescription startUpgradeDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken);
        Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatus(
            TimeSpan timeout,
            CancellationToken cancellationToken);
        Task<String> GetClusterConfiguration(
            string apiVersion,
            TimeSpan timeout,
            CancellationToken cancellationToken);
        Task GetUpgradesPendingApproval(
            TimeSpan timeout,
            CancellationToken cancellationToken);
        Task StartApprovedUpgrades(
            TimeSpan timeout,
            CancellationToken cancellationToken);
        Task<String> CallSystemService(
            string action,
            string inpubBlob,
            TimeSpan timeout,
            CancellationToken cancellationToken);
    }
}