// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;
using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.Common;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore.Service
{
    internal interface IBackupRestoreService
    {
        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<BackupPolicy> GetBackupSchedulingPolicyAsync(
            BackupPartitionInfo backupPartitionInfo,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task<RestorePointDetails> GetRestorePointDetailsAsync(
            BackupPartitionInfo backupPartitionInfo,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task RestoreOperationResultAsync(
            RestoreOperationResult operationResult,
            TimeSpan timeout,
            CancellationToken cancellationToken);

        [SuppressMessage(FxCop.Category.Design, FxCop.Rule.DoNotNestGenericTypesInMemberSignatures, Justification = "Approved public API.")]
        Task BackupOperationResultAsync(
            BackupOperationResult operationResult,
            TimeSpan timeout,
            CancellationToken cancellationToken);
    }
}