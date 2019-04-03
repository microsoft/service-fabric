// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using BackupPolicyModel = Common.Model.BackupPolicy;
    using System.Fabric.BackupRestore.Common;
    using System.Threading;
    using System.Text;
    using System.Net.Http;
    using System.Net;
    using System.Linq;
    using System.Fabric.BackupRestore.Service;

    internal class UpdateBackupPolicyOperation : BackupPolicyOperation<HttpResponseMessage>
    {
        private readonly BackupPolicy backupPolicy;
        private readonly string backupPolicyName;

        internal UpdateBackupPolicyOperation(string backupPolicyName, BackupPolicy backupPolicy,string apiVersion, StatefulService statefulService) :base(apiVersion, statefulService)
        {
            this.backupPolicy = backupPolicy;
            this.backupPolicyName = backupPolicyName;
        }
        
        internal override async Task<HttpResponseMessage> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowInvalidArgumentIfNull(this.backupPolicy);
            if (string.IsNullOrEmpty(this.backupPolicyName))
            {
                throw new ArgumentException();
            }
            
            var workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(this.StatefulService);

            var modelBackupPolicy =  BackupPolicyModel.FromBackupPolicyView(this.backupPolicy);
            var policyBuilder = modelBackupPolicy.ToBuilder()
                                        .WithName(this.backupPolicyName);
            RetentionManager retentionManager = await RetentionManager.CreateOrGetRetentionManager(this.StatefulService);

            using (var transaction = this.StatefulService.StateManager.CreateTransaction())
            {
                var existingBackupPolicy = await this.BackupPolicyStore.GetValueWithUpdateLockModeAsync(this.backupPolicyName, timeout, cancellationToken,transaction);
                if (existingBackupPolicy == null)
                {
                    throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
                }

                var existingEnablementSet = existingBackupPolicy.BackupEnabledSet;
                policyBuilder.WithBackupEnabledSet(existingEnablementSet);

                var updatedBackupPolicy = policyBuilder.Build();

                await this.BackupPolicyStore.UpdateValueAsync(this.backupPolicyName, existingBackupPolicy, updatedBackupPolicy, timeout,cancellationToken,transaction);
                if (existingEnablementSet.Count > 0)
                {
                    await
                        workItemQueue.AddWorkItem(
                            new UpdateEnablementWorkItem(existingEnablementSet.ToList(),
                                new WorkItemInfo { WorkItemType = WorkItemPropogationType.UpdateBackupPolicy ,
                                    BackupPolicyUpdateGuid = updatedBackupPolicy.UniqueId } ),timeout, cancellationToken, transaction);
                }

                await retentionManager.UpdatePolicyAsync(this.backupPolicyName, timeout, cancellationToken, transaction);
                await transaction.CommitAsync();
            }


            return new HttpResponseMessage(HttpStatusCode.OK);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat(" BackupPolicyName: {0}", this.backupPolicyName).AppendLine();
            stringBuilder.AppendFormat("Updated BackupPolicy {0}", this.backupPolicy).AppendLine();
            stringBuilder.AppendFormat(" apiVersion: {0}", this.ApiVersion).AppendLine();
            return stringBuilder.ToString();
        }
    }
}