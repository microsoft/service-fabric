// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using BackupMappingModel =  Common.Model.BackupMapping;
    using BackupPolicyModel = Common.Model.BackupPolicy;
    using System.Fabric.BackupRestore.Common;
    using System.Threading;
    using System.Text;
    using System.Net;
    using System.Net.Http;

    internal class EnableBackupMappingOperation : BackupMappingOperation<HttpResponseMessage>
    {
        private readonly BackupMapping backupMapping;
        private readonly string fabricRequestHeader;

        internal EnableBackupMappingOperation( string fabricRequestHeader , BackupMapping backupMapping,string apiVersion, StatefulService statefulService) :base(apiVersion, statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
            this.backupMapping = backupMapping;
        }

        internal override async Task<HttpResponseMessage> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            WorkItem workItem = null;
            string applicationNameUri = null;
            string serviceNameUri = null;
            string partitionId = null;

            this.ThrowInvalidArgumentIfNull(this.backupMapping);

            var appOrServiceUri = await UtilityHelper.GetFabricUriFromRequstHeader(this.fabricRequestHeader, timeout, cancellationToken);
            BackupMappingModel backupMappingModel = BackupMappingModel.FromBackupMappingView(this.backupMapping, appOrServiceUri);

            FabricBackupResourceType fabricResourceType = 
                UtilityHelper.GetApplicationAndServicePartitionUri(backupMappingModel.ApplicationOrServiceUri, out applicationNameUri, out serviceNameUri, out partitionId);
            bool isInputValid = false;
            
            string key = backupMappingModel.ApplicationOrServiceUri;
            BackupMappingModel existingBackupMapping = await this.BackupMappingStore.GetValueAsync(key,timeout,cancellationToken);
            
            BackupPolicyModel backupPolicy =  await this.BackupPolicyStore.GetValueAsync(backupMappingModel.BackupPolicyName,timeout,cancellationToken);
            if (backupPolicy == null)
            {
                throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
            }
            WorkItemPropogationType workItemPropogationType = existingBackupMapping == null? WorkItemPropogationType.Enable :
                WorkItemPropogationType.UpdateProtection;
            WorkItemInfo workItemInfo = new WorkItemInfo
            {
                WorkItemType =  workItemPropogationType ,
                ProctectionGuid = backupMappingModel.ProtectionId,
                BackupPolicyUpdateGuid = backupPolicy.UniqueId

            };

            switch (fabricResourceType)
            {
                case FabricBackupResourceType.ApplicationUri:
                isInputValid = await FabricClientHelper.ValidateApplicationUri(applicationNameUri, timeout,cancellationToken);
                workItem = new ResolveToPartitionWorkItem(fabricResourceType, backupMappingModel.ApplicationOrServiceUri, workItemInfo);
                break;

                case FabricBackupResourceType.ServiceUri:
                isInputValid = await FabricClientHelper.ValidateServiceUri(serviceNameUri, timeout, cancellationToken);
                workItem = new ResolveToPartitionWorkItem(fabricResourceType, backupMappingModel.ApplicationOrServiceUri, workItemInfo);
                break;

                case FabricBackupResourceType.PartitionUri:
                    isInputValid = await FabricClientHelper.ValidateServiceUri(serviceNameUri, timeout, cancellationToken);
                    if (isInputValid)
                    {
                        isInputValid = await FabricClientHelper.ValidatePartition(serviceNameUri, partitionId, timeout, cancellationToken);
                        workItem = new SendToServiceNodeWorkItem(serviceNameUri, partitionId, workItemInfo);
                    }

                    break;

                case FabricBackupResourceType.Error:
                    throw new ArgumentException("Invalid argument");
            }

            if (!isInputValid)
            {
                throw new ArgumentException("Invalid Arguments for Application / Service / Partitions Details.");
            }

            using (var transaction = this.StatefulService.StateManager.CreateTransaction())
            {
                await
                    this.BackupMappingStore.AddOrUpdateAsync(key, backupMappingModel,
                        (key1, BackupMappingModel1) => backupMappingModel, timeout, cancellationToken);
                await this.WorkItemQueue.AddWorkItem(workItem, timeout, cancellationToken, transaction);
                BackupPolicyModel existingBackupPolicy = await this.BackupPolicyStore.GetValueWithUpdateLockModeAsync(backupMappingModel.BackupPolicyName, timeout,
                        cancellationToken, transaction);
                if(existingBackupMapping != null)
                {
                    BackupPolicyModel previousBackupPolicy  =  await this.BackupPolicyStore.GetValueWithUpdateLockModeAsync(existingBackupMapping.BackupPolicyName, timeout,
                        cancellationToken, transaction);
                    await this.BackupPolicyStore.RemoveProtectionEntity(previousBackupPolicy, key, timeout, cancellationToken, transaction);
                }
                await this.BackupPolicyStore.AddProtectionEntity(existingBackupPolicy, key, timeout, cancellationToken, transaction);
                cancellationToken.ThrowIfCancellationRequested();
                await transaction.CommitAsync();
            }
            return new HttpResponseMessage(HttpStatusCode.Accepted);
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Enable Backup Mapping Operation, fabricRequestHeader : {0} , BackupPolicy: {1}", this.fabricRequestHeader,this.backupMapping);
            return stringBuilder.ToString();
        }
    }
}