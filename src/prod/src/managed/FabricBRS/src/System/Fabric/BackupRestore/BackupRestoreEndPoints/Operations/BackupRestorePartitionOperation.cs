// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.Enums;
    using Microsoft.ServiceFabric.Data;

    internal abstract class BackupRestorePartitionOperation <TResult> : BackupMappingOperation<TResult>
    {
        protected internal RestoreStore RestoreStore;
        protected internal BackupPartitionStore BackupPartitionStore;
        
        protected internal BackupRestorePartitionOperation(string apiVersion , StatefulService statefulService) : base ( apiVersion, statefulService)
        {
            this.StatefulService = statefulService;
        }

        internal async override Task InitializeAsync()
        {
            await base.InitializeAsync();
            this.RestoreStore = await RestoreStore.CreateOrGetRestoreStatusStore(this.StatefulService).ConfigureAwait(false);
            this.BackupPartitionStore = await BackupPartitionStore.CreateOrGetBackupPartitionStore(this.StatefulService).ConfigureAwait(false);
        }

        internal async Task CheckForEitherBackupOrRestoreInProgress(string fabricUri, TimeSpan timeout,CancellationToken cancellationToken,ITransaction transaction)
        {
            var existingRestoreStatus = await this.RestoreStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout, cancellationToken,transaction);
            var existingBackupPartitionStatus = await this.BackupPartitionStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout,  cancellationToken,transaction);

            if (existingRestoreStatus != null && (existingRestoreStatus.RestoreStatusState == RestoreState.RestoreInProgress ||
                existingRestoreStatus.RestoreStatusState == RestoreState.Accepted))
            {
                throw new FabricException(StringResources.RestoreAlreadyInProgress, FabricErrorCode.RestoreAlreadyInProgress);
            }

            if (existingBackupPartitionStatus != null && (
                existingBackupPartitionStatus.BackupPartitionStatusState == BackupState.Accepted ||
                existingBackupPartitionStatus.BackupPartitionStatusState == BackupState.BackupInProgress))
            {
                throw new FabricBackupInProgressException(StringResources.BackupAlreadyInProgress);
            }
        }
    }
}