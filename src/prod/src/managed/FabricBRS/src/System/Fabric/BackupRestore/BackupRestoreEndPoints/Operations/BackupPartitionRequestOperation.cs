// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{

    using BackupStorageView = System.Fabric.BackupRestore.BackupRestoreTypes.BackupStorage;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Text;
    using System.Net;
    using System.Net.Http;

    internal class BackupPartitionRequestOperation : BackupRestorePartitionOperation<HttpResponseMessage>
    {
        private readonly string fabricRequestHeader;
        private readonly BackupStorageView backupStorage;
        private readonly int backupTimeoutinMinutes;

        public BackupPartitionRequestOperation(string fabricRequestHeader, BackupStorageView backupStorage, int backupTimeoutinMinutes, string apiVersion, StatefulService statefulService) : base( apiVersion, statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
            this.backupStorage = backupStorage;
            this.backupTimeoutinMinutes = backupTimeoutinMinutes;
        }

        internal override async  Task<HttpResponseMessage> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationNameUri;
            string serviceNameUri;
            string partitionId;
            string fabricUri = await UtilityHelper.GetFabricUriFromRequstHeaderForPartitions(this.fabricRequestHeader, timeout, cancellationToken);

            UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationNameUri, out serviceNameUri, out partitionId);
            BackupStorage backupStorageModel;

            if (this.backupStorage == null)
            {
                var backupMapping = (await this.BackupMappingStore.GetValueAsync(fabricUri) ??
                                              await this.BackupMappingStore.GetValueAsync(serviceNameUri)) ??
                                                  await this.BackupMappingStore.GetValueAsync(UtilityHelper.GetApplicationNameFromService(applicationNameUri));
                if (backupMapping == null)
                {
                    throw new FabricPeriodicBackupNotEnabledException();
                }

                var backupPolicy = await this.BackupPolicyStore.GetValueAsync(backupMapping.BackupPolicyName, timeout, cancellationToken);
                backupStorageModel = backupPolicy.Storage;
            }
            else
            { 
                backupStorageModel  = BackupStorage.FromBackupStorageView(this.backupStorage);
            }

            var backupRequestGuid = Guid.NewGuid();
            var backupPartitionStatus = new BackupPartitionStatus(fabricUri, backupRequestGuid);
            var sendBackupPartitionToServiceNodeWorkItem = new BackupPartitionWorkItem(serviceNameUri,partitionId, backupStorageModel, backupRequestGuid, this.backupTimeoutinMinutes);

            using (var transaction = this.StatefulService.StateManager.CreateTransaction())
            {
                await this.CheckForEitherBackupOrRestoreInProgress(fabricUri, timeout, cancellationToken,transaction);
                await this.WorkItemQueue.AddWorkItem(sendBackupPartitionToServiceNodeWorkItem, timeout, cancellationToken, transaction);
                await this.BackupPartitionStore.AddOrUpdateAsync(fabricUri,backupPartitionStatus,(fabricUriKey,backupPartitionStatusUpdate)=> backupPartitionStatus, timeout,cancellationToken,transaction);
                await transaction.CommitAsync();
            }
            return new HttpResponseMessage(HttpStatusCode.Accepted);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Fabric Request Header {0}", this.fabricRequestHeader).AppendLine();
            stringBuilder.AppendFormat("Backup Storage: {0}", this.backupStorage).AppendLine();
            return stringBuilder.ToString();
        }
    }
}