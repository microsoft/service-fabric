// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading;
    using System.Threading.Tasks;
    using BackupRestoreModel = System.Fabric.BackupRestore.Common.Model;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Text;
    using System.Net;
    using System.Net.Http;

    internal class RestorePartitionRequestOperation : BackupRestorePartitionOperation<HttpResponseMessage>
    {
        private readonly string fabricRequestHeader;
        private readonly RestoreRequest restoreRequest;
        private readonly int restoreTimeoutInMinutes;

        public RestorePartitionRequestOperation(string fabricRequestHeader, RestoreRequest restoreRequest ,string apiVersion ,int restoreTimeoutInMinutes,  StatefulService statefulService) : base( apiVersion , statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
            this.restoreRequest = restoreRequest;
            this.restoreTimeoutInMinutes = restoreTimeoutInMinutes;
        }

        internal override async  Task<HttpResponseMessage> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationNameUri;
            string serviceNameUri;
            string partitionId;
            var fabricUri = await UtilityHelper.GetFabricUriFromRequstHeaderForPartitions(this.fabricRequestHeader, timeout, cancellationToken);
            this.ThrowInvalidArgumentIfNull(this.restoreRequest);

            await FabricClientHelper.IsFaultServiceExisting(timeout, cancellationToken);

            UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationNameUri, out serviceNameUri,out partitionId);

            BackupRestoreModel.BackupStorage backupStorage;
            if (this.restoreRequest.BackupStorage == null)
            {
                var backupProtection =
                    await this.BackupMappingStore.GetValueAsync(fabricUri, timeout, cancellationToken) ??
                    await this.BackupMappingStore.GetValueAsync(serviceNameUri, timeout, cancellationToken) ??
                    await this.BackupMappingStore.GetValueAsync(applicationNameUri, timeout, cancellationToken);

                if (backupProtection == null)
                {
                    throw new FabricPeriodicBackupNotEnabledException();
                }

                var backupPolicy =
                    await this.BackupPolicyStore.GetValueAsync(backupProtection.BackupPolicyName, timeout, cancellationToken);
                backupStorage = backupPolicy.Storage;
            }
            else
            {
                backupStorage = BackupRestoreModel.BackupStorage.FromBackupStorageView(this.restoreRequest.BackupStorage);
            }

            var recoveryPointManager = RecoveryPointManagerFactory.GetRecoveryPointManager(backupStorage);
            var backupDetails = await recoveryPointManager.GetRecoveryPointDetailsAsync(this.restoreRequest.BackupLocation, cancellationToken);

            var retriveBackupDetails = backupDetails as RestorePoint;

            if (retriveBackupDetails == null)
            {
                throw new FabricException(StringResources. RestoreDetailsFetchFailed , FabricErrorCode.RestoreSafeCheckFailed);
            }

            if (!retriveBackupDetails.BackupId.Equals(this.restoreRequest.BackupId))
            {
                /*
                * Add validation here for the request
                */
                throw new FabricException(StringResources.BackupIdAndLocationMismatch,
                    FabricErrorCode.RestoreSourceTargetPartitionMismatch);
            }

            
            var backupServicePartitionInformation = retriveBackupDetails.PartitionInformation;
            var servicePartitionInformation = await FabricClientHelper.GetPartitionDetails(partitionId, timeout, cancellationToken);
            switch (servicePartitionInformation.Kind)
            {
                    case ServicePartitionKind.Int64Range:
                    var int64RangePartitionInformation = servicePartitionInformation as System.Fabric.Int64RangePartitionInformation;
                    var backupInt64RangePartitionInformation = backupServicePartitionInformation as BackupRestoreTypes.Int64RangePartitionInformation;

                    if (backupInt64RangePartitionInformation == null ||
                        backupInt64RangePartitionInformation.HighKey != int64RangePartitionInformation.HighKey ||
                        backupInt64RangePartitionInformation.LowKey != int64RangePartitionInformation.LowKey)
                    {
                        throw new FabricException(StringResources.InvalidInt64Keys,
                            FabricErrorCode.RestoreSourceTargetPartitionMismatch);
                    }

                    break;

                    case ServicePartitionKind.Named:
                    var namedPartitionInformation = servicePartitionInformation as System.Fabric.NamedPartitionInformation;
                    var backupNamedPartitionInformation = backupServicePartitionInformation as BackupRestoreTypes.NamedPartitionInformation;

                    if (backupNamedPartitionInformation == null ||
                        !namedPartitionInformation.Name.Equals(backupNamedPartitionInformation.Name))
                    {
                        throw new FabricException(StringResources.InvalidNameKey,
                            FabricErrorCode.RestoreSourceTargetPartitionMismatch);
                    }

                    break;

                    case ServicePartitionKind.Singleton:
                    var backupSingletonPartitionInformation = backupServicePartitionInformation as BackupRestoreTypes.SingletonPartitionInformation;
                    if (backupSingletonPartitionInformation == null)
                    {
                        throw new FabricException(StringResources.InvalidPartitionType,
                            FabricErrorCode.RestoreSourceTargetPartitionMismatch);
                    }

                    break;
            }
            

            var backupLocations =
                await recoveryPointManager.GetBackupLocationsInBackupChainAsync(this.restoreRequest.BackupLocation, cancellationToken);

            var dataLossGuid = Guid.NewGuid();
            var restoreRequestGuid = Guid.NewGuid();
            var restoreStatus = new RestoreStatus(fabricUri, backupStorage, restoreRequestGuid,dataLossGuid, backupLocations, retriveBackupDetails);
            var restoreWorkItem = new RestorePartitionWorkItem(serviceNameUri, partitionId, restoreRequestGuid,dataLossGuid, this.restoreTimeoutInMinutes);

            using (var transaction = this.StatefulService.StateManager.CreateTransaction())
            {
                await this.CheckForEitherBackupOrRestoreInProgress(fabricUri, timeout, cancellationToken,transaction);
                await this.WorkItemQueue.AddWorkItem(restoreWorkItem, timeout, cancellationToken, transaction);
                await this.RestoreStore.AddOrUpdateAsync(fabricUri, restoreStatus,
                        (fabricUri1, restoreStatus1) => restoreStatus , timeout, cancellationToken, transaction);
                await transaction.CommitAsync();
            } 

            return new HttpResponseMessage(HttpStatusCode.Accepted);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Fabric Request Header {0}", this.fabricRequestHeader).AppendLine();
            stringBuilder.AppendFormat("Restore Request  {0}", this.restoreRequest).AppendLine();
            return stringBuilder.ToString();
        }
    }
}