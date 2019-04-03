// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Threading;
    using System.Text;
    using System.Net.Http;
    using System.Net;
    using Microsoft.ServiceFabric.Data;
    using System.Fabric.BackupRestore.Service;

    internal class DeleteBackupPolicyOperation : BackupPolicyOperation<HttpResponseMessage>
    {
        private readonly string backupPolicyName;

        internal DeleteBackupPolicyOperation(string backupPolicyName,string apiVersion , StatefulService statefulService) : base(apiVersion, statefulService)
        {
            this.backupPolicyName = backupPolicyName;
        }

        internal override async Task<HttpResponseMessage> RunAsync(TimeSpan timeout , CancellationToken cancellationToken)
        {
            if (string.IsNullOrEmpty(this.backupPolicyName))
            {
                throw new ArgumentException(StringResources.InvalidArguments);
            }
            var backupPolicy = await this.BackupPolicyStore.GetValueAsync(this.backupPolicyName);
            BackupMappingStore backupMappingStore = await 
                BackupMappingStore.CreateOrGetBackupMappingStore(this.StatefulService);
            SuspendStore suspendStore = await SuspendStore.CreateOrGetSuspendStatusStore(this.StatefulService);
            List<string> allSuspensions = await suspendStore.GetAllSuspensions(timeout, cancellationToken);

            if (backupPolicy != null && backupPolicy.BackupEnabledSet.Count > 0)
            {
                HashSet<string> backupEnables = new HashSet<string>(backupPolicy.BackupEnabledSet);
                foreach (var backupEnable in backupEnables)
                {
                    if (!await this.IsFabricEntityExisting(backupEnable, timeout, cancellationToken))
                    {
                        using (ITransaction transaction = this.StatefulService.StateManager.CreateTransaction())
                        {
                            var currentBackupPolicy =
                                await
                                    this.BackupPolicyStore.GetValueWithUpdateLockModeAsync(this.backupPolicyName, timeout,
                                        cancellationToken,
                                        transaction);

                            var updatedBackupPolicy = currentBackupPolicy.ToBuilder()
                                                        .RemoveFromBackupEnabledSet(backupEnable)
                                                        .Build();

                            await
                                this.BackupPolicyStore.RemoveProtectionEntity(updatedBackupPolicy, backupEnable,
                                    timeout,
                                    cancellationToken, transaction);
                            await backupMappingStore.DeleteValueAsync(backupEnable, timeout, cancellationToken);
                            await transaction.CommitAsync();
                        }
                    }
                }
            }

            RetentionManager retentionManager = await RetentionManager.CreateOrGetRetentionManager(this.StatefulService);
            using (var transaction = this.StatefulService.StateManager.CreateTransaction())
            {
                foreach (string suspension in allSuspensions)
                {
                    string applicationNameUri = null;
                    string serviceNameUri = null;
                    string partitionId = null;
                    FabricBackupResourceType fabricBackupResourceType =
                    UtilityHelper.GetApplicationAndServicePartitionUri(suspension, out applicationNameUri, out serviceNameUri, out partitionId);

                    BackupMapping backupMapping = null;
                    switch (fabricBackupResourceType)
                    {
                        case FabricBackupResourceType.PartitionUri:
                            backupMapping =
                            await backupMappingStore.GetValueAsync(UtilityHelper.GetBackupMappingKey(serviceNameUri, partitionId),
                            timeout, cancellationToken, transaction) ??
                        await backupMappingStore.GetValueAsync(serviceNameUri, timeout, cancellationToken, transaction) ??
                        await backupMappingStore.GetValueAsync(applicationNameUri, timeout, cancellationToken, transaction);
                            break;

                        case FabricBackupResourceType.ServiceUri:
                            backupMapping =
                            await backupMappingStore.GetValueAsync(serviceNameUri, timeout, cancellationToken, transaction) ??
                            await backupMappingStore.GetValueAsync(applicationNameUri, timeout, cancellationToken, transaction);
                            break;

                        case FabricBackupResourceType.ApplicationUri:
                            backupMapping =
                            await backupMappingStore.GetValueAsync(applicationNameUri, timeout, cancellationToken, transaction);
                            break;
                    }

                    if (backupMapping != null && this.backupPolicyName.Equals(backupMapping.BackupPolicyName))
                    {
                        await suspendStore.DeleteValueAsync(suspension, timeout, cancellationToken, transaction);
                    }
                }

                await this.BackupPolicyStore.DeleteValueAsync(this.backupPolicyName, timeout, cancellationToken, transaction);
                await retentionManager.DeletePolicyAsync(this.backupPolicyName, timeout, cancellationToken, transaction);
                await transaction.CommitAsync();
            }

            return new HttpResponseMessage(HttpStatusCode.OK);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Delete BackupPolicy Operation, BackupPolicy: {0}", this.backupPolicyName);
            return stringBuilder.ToString();
        }

        private async Task<bool> IsFabricEntityExisting(string fabricUri,TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationUri = null;
            string serviceUri = null;
            string partitionId = null;
            FabricBackupResourceType fabricBackupResourceType =
                UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationUri, out serviceUri,
                    out partitionId);
            switch (fabricBackupResourceType)
            {
                    case FabricBackupResourceType.ApplicationUri:
                    try
                    {
                        return
                            await FabricClientHelper.ValidateApplicationUri(applicationUri, timeout, cancellationToken);
                    }
                    catch (Exception exception)
                    {
                        FabricException fabricException = exception as FabricException;
                        if (fabricException!= null && fabricException.ErrorCode == FabricErrorCode.ApplicationNotFound)
                        {
                            return false;
                        }
                    }
                    break;

                    case FabricBackupResourceType.ServiceUri:
                    try
                    {
                        return
                            await FabricClientHelper.ValidateServiceUri(serviceUri, timeout, cancellationToken);
                    }
                    catch (Exception exception)
                    {
                        FabricException fabricException = exception as FabricException;
                        if (fabricException != null &&
                            ( fabricException.ErrorCode == FabricErrorCode.ServiceNotFound || fabricException.ErrorCode == FabricErrorCode.ApplicationNotFound))
                        {
                            return false;
                        }
                    }
                    break;

                case FabricBackupResourceType.PartitionUri:
                    try
                    {
                        return
                            await FabricClientHelper.ValidatePartition(serviceUri,partitionId, timeout, cancellationToken);
                    }
                    catch (Exception exception)
                    {
                        FabricException fabricException = exception as FabricException;
                        if (fabricException != null && fabricException.ErrorCode == FabricErrorCode.PartitionNotFound)
                        {
                            return false;
                        }
                    }
                    break;
            }
            return true;
        }
    }
}