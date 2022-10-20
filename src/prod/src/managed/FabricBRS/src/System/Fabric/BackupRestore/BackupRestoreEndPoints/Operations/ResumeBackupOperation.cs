// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Fabric.BackupRestore.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Text;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Net;
    using System.Net.Http;

    internal class ResumeBackupOperation : BackupMappingOperation<HttpResponseMessage>
    {
        private readonly string _fabricRequestHeader;

        public ResumeBackupOperation(string fabricRequestHeader,string apiVersion , StatefulService statefulService) : base(apiVersion, statefulService)
        {
            this._fabricRequestHeader = fabricRequestHeader;
        }

        internal override async  Task<HttpResponseMessage> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationNameUri;
            string serviceNameUri;
            string partitionId;
            var fabricUri = await UtilityHelper.GetFabricUriFromRequstHeader(this._fabricRequestHeader, timeout, cancellationToken);
            FabricBackupResourceType fabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationNameUri, out serviceNameUri, out partitionId);
            var suspendStore = await SuspendStore.CreateOrGetSuspendStatusStore(this.StatefulService);
            var backupMappingList = await this.BackupMappingStore.GetAllProtectionForApplicationAndItsServiceAndPartition(applicationNameUri, timeout, cancellationToken);

            if (backupMappingList.Count == 0)
            {
                throw new FabricPeriodicBackupNotEnabledException();
            }

            WorkItem workItem = null;
            switch (fabricBackupResourceType)
            {
                case FabricBackupResourceType.ApplicationUri:
                    workItem = new ResolveToPartitionWorkItem(FabricBackupResourceType.ApplicationUri, applicationNameUri, new WorkItemInfo { WorkItemType = WorkItemPropogationType.ResumePartition });
                    break;
                case FabricBackupResourceType.ServiceUri:
                    workItem = new ResolveToPartitionWorkItem(FabricBackupResourceType.ServiceUri, serviceNameUri, new WorkItemInfo { WorkItemType = WorkItemPropogationType.ResumePartition });
                    break;
                case FabricBackupResourceType.PartitionUri:
                    workItem = new SendToServiceNodeWorkItem(serviceNameUri, partitionId, new WorkItemInfo { WorkItemType = WorkItemPropogationType.ResumePartition });
                    break;
            }

            if (fabricBackupResourceType == FabricBackupResourceType.Error || workItem == null)
            {
                throw new ArgumentException("Invalid argument");
            }

            try
            {
                using (var transaction = this.StatefulService.StateManager.CreateTransaction())
                {
                    await suspendStore.DeleteValueAsync(fabricUri, timeout, cancellationToken, transaction);
                    await
                        this.WorkItemQueue.AddWorkItem(workItem, timeout, cancellationToken,
                            transaction);
                    await transaction.CommitAsync();
                }
            }
            catch (FabricBackupKeyNotExisingException)
            {
                //Swallow it as the operation is idempotent
            }

            return new HttpResponseMessage(HttpStatusCode.Accepted); ;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Fabric Request Header {0}", this._fabricRequestHeader).AppendLine();
            return stringBuilder.ToString();
        }
    }
}