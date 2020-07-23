// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{

    using System.Net;
    using System.Net.Http;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.Common.Tracing;
    using System.Threading;
    using BackupPolicyModel = System.Fabric.BackupRestore.Common.Model.BackupPolicy;
    using System.Text;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.BackupRestore.Service;

    internal class AddBackupPolicyOperation : BackupPolicyOperation<HttpResponseMessage>
    {
        private readonly BackupPolicy backupPolicy;

        internal AddBackupPolicyOperation(BackupPolicy backupPolicy,string apiVersion, StatefulService statefulService): base(apiVersion, statefulService)
        {
            this.backupPolicy = backupPolicy;
        }

        internal override async Task<HttpResponseMessage> RunAsync(TimeSpan timeout , CancellationToken cancellationToken)
        {
            this.ThrowInvalidArgumentIfNull(this.backupPolicy);
            RetentionManager retentionManager = await RetentionManager.CreateOrGetRetentionManager(this.StatefulService);
            try
            {
                using (var transaction = this.StatefulService.StateManager.CreateTransaction())
                {

                    await
                        this.BackupPolicyStore.AddAsync(this.backupPolicy.Name,
                            BackupPolicyModel.FromBackupPolicyView(this.backupPolicy)
                            , timeout, cancellationToken, transaction);
                    
                    await retentionManager.CreatePolicyAsync(BackupPolicyModel.FromBackupPolicyView(this.backupPolicy), timeout, cancellationToken, transaction);
                    await transaction.CommitAsync();
                }
            }
            catch (FabricBackupRestoreKeyAlreadyExisingException)
            {
                throw new FabricException(StringResources.BackupPolicyAlreayExists, FabricErrorCode.BackupPolicyAlreayExists);
            }
            FabricEvents.Events.BackupPolicyDetails(this.backupPolicy.Name, this.backupPolicy.MaxIncrementalBackups,
                this.backupPolicy.AutoRestoreOnDataLoss == true ? 1 : 0,
                this.backupPolicy.Schedule.ScheduleKind == Enums.BackupScheduleType.TimeBased ? 1 : 0,
                this.backupPolicy.Schedule.ScheduleKind == Enums.BackupScheduleType.FrequencyBased ? 1 : 0,
                this.backupPolicy.Storage.StorageKind == Enums.BackupStorageType.AzureBlobStore ? 1 : 0,
                this.backupPolicy.Storage.StorageKind == Enums.BackupStorageType.FileShare ? 1 : 0);

            return  new HttpResponseMessage(HttpStatusCode.Created);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("BackupPolicy : {0}", this.backupPolicy).AppendLine();
            stringBuilder.AppendFormat("ApiVersion : {0}", this.ApiVersion).AppendLine();
            return stringBuilder.ToString();
        }
    }
}