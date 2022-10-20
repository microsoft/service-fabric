// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;
    using System.Threading;
    using System.Text;
    using System.Net;
    using System.Net.Http;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Service;

    internal class DisableBackupMappingOperation : BackupMappingOperation<HttpResponseMessage>
    {
        private readonly string fabricRequestHeader;
        private readonly bool cleanBackups;
        
        internal DisableBackupMappingOperation(string  fabricRequestHeader, bool cleanBackups, string apiVersion, StatefulService statefulService):base(apiVersion, statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
            this.cleanBackups = cleanBackups;
        }

        internal override async Task<HttpResponseMessage> RunAsync(TimeSpan timeout , CancellationToken cancellationToken)
        {
            string applicationNameUri;
            string serviceNameUri;
            string partitionId;
            var fabricUri = await UtilityHelper.GetFabricUriFromRequstHeader(this.fabricRequestHeader, timeout, cancellationToken);
            
            WorkItem workItem = null;
            FabricBackupResourceType fabricBackupResourceType =
                UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationNameUri, out serviceNameUri,
                    out partitionId);

            bool isApplicationOrServiceExisting;

            switch (fabricBackupResourceType)
            {
                case FabricBackupResourceType.ApplicationUri:
                    isApplicationOrServiceExisting =
                        await this.IsApplicationOrServiceExisting(fabricBackupResourceType, applicationNameUri,null, timeout, cancellationToken);
                    if (isApplicationOrServiceExisting)
                    {
                        workItem = new ResolveToPartitionWorkItem(FabricBackupResourceType.ApplicationUri,
                            applicationNameUri,
                            new WorkItemInfo
                            {
                                WorkItemType = WorkItemPropogationType.Disable,
                            });
                    }

                    break;

                case FabricBackupResourceType.ServiceUri:
                    isApplicationOrServiceExisting =
                        await this.IsApplicationOrServiceExisting(fabricBackupResourceType, serviceNameUri, null, timeout, cancellationToken);
                    if (isApplicationOrServiceExisting)
                    {
                        workItem = new ResolveToPartitionWorkItem(FabricBackupResourceType.ServiceUri, serviceNameUri,
                            new WorkItemInfo
                            {
                                WorkItemType = WorkItemPropogationType.Disable,
                            });
                    }

                    break;

                case FabricBackupResourceType.PartitionUri:
                    isApplicationOrServiceExisting =
                        await this.IsApplicationOrServiceExisting(fabricBackupResourceType, serviceNameUri, partitionId,  timeout, cancellationToken);
                    if (isApplicationOrServiceExisting)
                    {
                        serviceNameUri =
                            await
                                FabricClientHelper.GetFabricServiceUriFromPartitionId(partitionId, timeout,
                                    cancellationToken);
                        workItem = new SendToServiceNodeWorkItem(serviceNameUri, partitionId,
                            new WorkItemInfo
                            {
                                WorkItemType = WorkItemPropogationType.Disable,
                            });
                    }

                    break;

                case FabricBackupResourceType.Error:
                    throw new ArgumentException(StringResources.InvalidArguments);
            }
            
            if (workItem == null)
            {
                using (var transaction = this.StatefulService.StateManager.CreateTransaction())
                {
                    var backupMapping = await this.BackupMappingStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout, cancellationToken,transaction);
                    if (backupMapping == null)
                    {
                        throw new FabricPeriodicBackupNotEnabledException();
                    }

                    var assignedBackupPolicy =
                        await this.BackupPolicyStore.GetValueWithUpdateLockModeAsync(backupMapping.BackupPolicyName, timeout,
                            cancellationToken, transaction);

                    await this.BackupMappingStore.DeleteValueAsync(fabricUri,timeout,cancellationToken,transaction);
                    await this.BackupPolicyStore.RemoveProtectionEntity(assignedBackupPolicy, fabricUri,timeout,cancellationToken,transaction);
                    await transaction.CommitAsync();
                }
            }
            else
            {
                RetentionManager retentionManager = await RetentionManager.CreateOrGetRetentionManager(this.StatefulService);
                using (var transaction = this.StatefulService.StateManager.CreateTransaction())
                {
                    var backupMapping = await this.BackupMappingStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout, cancellationToken, transaction);
                    if (backupMapping == null)
                    {
                        throw new FabricPeriodicBackupNotEnabledException();
                    }

                    var assignedBackupPolicy = await this.BackupPolicyStore.GetValueWithUpdateLockModeAsync(backupMapping.BackupPolicyName, timeout,
                            cancellationToken, transaction);
                    await this.BackupMappingStore.DeleteValueAsync(fabricUri, timeout, cancellationToken, transaction);
                    await this.BackupPolicyStore.RemoveProtectionEntity(assignedBackupPolicy, fabricUri, timeout, cancellationToken, transaction);
                    await this.WorkItemQueue.AddWorkItem(workItem, timeout, cancellationToken, transaction);
                    if (cleanBackups)
                    {
                        await retentionManager.DisablePolicyAsync(fabricUri, backupMapping, timeout, cancellationToken, transaction);
                    }
                    await transaction.CommitAsync();
                }
            }
            return new HttpResponseMessage(HttpStatusCode.Accepted);
        }

        private async Task<bool> IsApplicationOrServiceExisting(FabricBackupResourceType fabricBackupResourceType,
            string applicationOrServiceUri, string partitionId ,TimeSpan timeout , CancellationToken cancellationToken)
        {
            bool isApplicationOrServiceExisting;
            try
            {
                if (fabricBackupResourceType == FabricBackupResourceType.ApplicationUri)
                {
                    isApplicationOrServiceExisting = await FabricClientHelper.ValidateApplicationUri(applicationOrServiceUri, timeout, cancellationToken);
                }
                else if (fabricBackupResourceType == FabricBackupResourceType.ServiceUri)
                {
                    isApplicationOrServiceExisting = await FabricClientHelper.ValidateServiceUri(applicationOrServiceUri, timeout, cancellationToken);
                }
                else if (fabricBackupResourceType == FabricBackupResourceType.PartitionUri)
                {
                    isApplicationOrServiceExisting = await FabricClientHelper.ValidatePartition(applicationOrServiceUri, partitionId, timeout, cancellationToken);
                }
                else
                {
                    throw new ArgumentException(StringResources.InvalidArguments);
                }
            }
            catch (FabricException)
            {
                //Since the Application Or Service is not existing we will return false
                isApplicationOrServiceExisting = false;
            }

            return isApplicationOrServiceExisting;
        }

        private string GetPartitionIdFromFabricRequestHeader()
        {
            string partitionId = null;
            try
            {
                var segments = this.fabricRequestHeader.Split('/');
                var type = segments[0];
                
                switch (type)
                {
                    case "Partitions":
                        if (segments.Length != 2 || string.IsNullOrEmpty(segments[1]))
                        {
                            throw new ArgumentException(StringResources.InvalidArguments);
                        }

                        partitionId = segments[1];

                        break;
                    default:
                        throw new ArgumentException(StringResources.ValidForPartitions);
                }

            }
            catch (Exception exception)
            {
                var fabricException = exception as FabricException;
                if (fabricException != null)
                {
                    throw;
                }

                throw new ArgumentException(StringResources.InvalidArguments);
            }
            return partitionId;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Disable Backup Protection Operation, fabricRequestHeader: {0}", this.fabricRequestHeader);
            return stringBuilder.ToString();
        }
    }
}