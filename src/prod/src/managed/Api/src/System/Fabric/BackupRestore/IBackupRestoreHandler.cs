// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;
using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.Common;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Interface which handles backup restore related callbacks from native
    /// </summary>
    internal interface IBackupRestoreHandler
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task UpdateBackupSchedulingPolicyAsync(
            BackupPolicy policy,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task BackupPartitionAsync(
            Guid operationId,
            BackupNowConfiguration configuration,
            TimeSpan timeout,
            CancellationToken cancellationToken);
    }
}