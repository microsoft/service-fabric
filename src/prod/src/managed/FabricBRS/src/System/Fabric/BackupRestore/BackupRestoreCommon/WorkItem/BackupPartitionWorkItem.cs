// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.IO;
    using System.Text;
    using System.Runtime.Serialization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.BackupRestore.Service;
    using System.Fabric.Interop;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;

    [DataContract]
    internal class BackupPartitionWorkItem : WorkItem
    {
        [DataMember]
        internal string ServiceUri { get; private set; }

        [DataMember]
        internal string PartitionId { get; private set; }

        [DataMember]
        internal DateTime RequestDateTime { get; private set; }

        [DataMember]
        internal Guid BackupRequestGuid { get; private set; }

        [DataMember]
        internal BackupStorage BackupStorage { get; private set; }

        [DataMember]
        internal TimeSpan BackupTimeout { get; private set; }

        internal BackupPartitionWorkItem(string serviceUri, string partitionId, BackupStorage backupStorage, Guid backupRequestGuid, int backupTimeoutinMinutes)
        {
            this.ServiceUri = serviceUri;
            this.PartitionId = partitionId;
            this.BackupRequestGuid = backupRequestGuid;
            this.BackupStorage = backupStorage;
            this.RequestDateTime = DateTime.UtcNow;
            this.BackupTimeout = TimeSpan.FromMinutes(backupTimeoutinMinutes);
        }

        public override async Task ProcessFailureHandle(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken,
            string processQueueTypeTrace)
        {
            var workItemInProcessStore = await
                WorkItemInProcessStore.CreateOrGetWorkItemInProcessStore(statefulService);
            var workItemQueue1Min = await WorkItemQueue1Min.CreateOrGetWorkItemQueue(statefulService);
            var workItemProcessInfo = await workItemInProcessStore.GetValueAsync(this.WorkItemGuid);
            using (var transaction = statefulService.StateManager.CreateTransaction())
            {
                await workItemInProcessStore.DeleteValueAsync(this.WorkItemGuid,timeout,cancellationToken, transaction);
                await workItemQueue1Min.AddWorkItem(workItemProcessInfo, timeout,cancellationToken ,transaction);
                await transaction.CommitAsync();
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Process Failure Handle WorkItemInProcessStore Delete WorkItem {0} And Enqueue in {1}  ", this, workItemQueue1Min.GetType());
            }
        }

        public override async Task<bool> Process(StatefulService statefulService,TimeSpan timeout, CancellationToken cancellationToken , string processQueueTypeTrace)
        {
            var timeoutDateTime = this.RequestDateTime.AddSeconds( this.BackupTimeout.TotalSeconds);
            var fabricUri = UtilityHelper.GetBackupMappingKey(this.ServiceUri, this.PartitionId);
            if (DateTime.Compare(DateTime.UtcNow, timeoutDateTime) > 0)
            {
                await UtilityHelper.InvokeWithRetry(() => this.UpdateBackupStatusAsTimeout(statefulService, fabricUri,timeout,cancellationToken, processQueueTypeTrace));
                return true;
            }
            var timeoutSpan = timeoutDateTime.Subtract(DateTime.UtcNow);
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "TimeoutSpan for Backup Partition WorkItem {0}",timeoutSpan);
            var cancellationTokenSource = new CancellationTokenSource(timeoutSpan);
            try
            {
                await this.ProcessBackup(statefulService,timeout ,cancellationTokenSource.Token, processQueueTypeTrace);
            }
            catch (OperationCanceledException operationCanceledException)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                    "Operation Cancellaed exception Details {0}",
                    operationCanceledException.ToString());
                await UtilityHelper.InvokeWithRetry(() => this.UpdateBackupStatusAsTimeout(statefulService, fabricUri,timeout,cancellationToken, processQueueTypeTrace));
            }
            return true;
        }

        private async Task UpdateBackupStatusAsTimeout(StatefulService statefulService, string fabricUri,TimeSpan timeout, CancellationToken cancellationToken,
            string processQueueTypeTrace)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Updating the Backup status as timeout for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
            BackupPartitionStore backupPartitionStore = await BackupPartitionStore.CreateOrGetBackupPartitionStore(statefulService);
            using (ITransaction transaction = statefulService.StateManager.CreateTransaction())
            {
                var currentBackupPartitionStatus = await backupPartitionStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout,cancellationToken, transaction);
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "BackupPartitionStatus {0} as timeout for request Service Uri :  {1} , PartitionId {2}", currentBackupPartitionStatus, this.ServiceUri, this.PartitionId);
                if ((currentBackupPartitionStatus?.BackupPartitionStatusState == BackupState.Accepted ||
                     currentBackupPartitionStatus?.BackupPartitionStatusState == BackupState.BackupInProgress) &&
                     this.BackupRequestGuid.Equals(currentBackupPartitionStatus.OperationId))
                {
                    var backupPartitionStatus = currentBackupPartitionStatus.ToBuilder()
                        .WithState(BackupState.Timeout)
                        .WithErrorCode(unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT))
                        .WithMessage(StringResources.BackupTimeout)
                        .Build();

                    await backupPartitionStore.UpdateValueAsync(fabricUri, backupPartitionStatus, timeout,cancellationToken, transaction);
                    await transaction.CommitAsync();
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Updated the Backup status as timeout failed for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
                }
                else
                {
                    transaction.Abort();
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Updating the Backup status as timeout failed for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
                }

            }
        }

        private async Task ProcessBackup(StatefulService statefulService,TimeSpan timeout, CancellationToken cancellationToken,string processQueueTypeTrace)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Processing the Backup for request Uri : {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
            var timeOutPersecCycle = 30000;
            var backupPartitionStore = await BackupPartitionStore.CreateOrGetBackupPartitionStore(statefulService);
            var fabricUri = UtilityHelper.GetBackupMappingKey(this.ServiceUri, this.PartitionId);
            BackupPartitionStatus backupPartitionStatus;
            do
            {
                var backupNowConfiguration =
                    new BackupNowConfiguration()
                    {
                        StoreInformation = this.GetBackupStoreInformationFromBackupStorage(this.BackupStorage),
                        OperationTimeoutMilliseconds = (int) this.BackupTimeout.TotalMilliseconds
                    };

                await UtilityHelper.InvokeWithRetry(async () =>
                {
                        await
                            Program.ServiceAgent.BackupPartitionAsync(new Uri(UtilityHelper.GetUriFromCustomUri(this.ServiceUri)),
                                Guid.Parse(this.PartitionId),
                                this.BackupRequestGuid, backupNowConfiguration, timeout, cancellationToken);
                });
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Requested the BackupPartitionAsync Successfully  for Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
                await UtilityHelper.InvokeWithRetry( async () => 
                { 
                    using (ITransaction transaction = statefulService.StateManager.CreateTransaction())
                    {
                        var currentBackupPartitionStatus = await backupPartitionStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout, cancellationToken, transaction);
                        if (currentBackupPartitionStatus.BackupPartitionStatusState == BackupState.Accepted)
                        {

                            var toUpdateBackupPartitionStatus = currentBackupPartitionStatus.ToBuilder().WithState(BackupState.BackupInProgress).Build();
                            await backupPartitionStore.UpdateValueAsync(fabricUri, toUpdateBackupPartitionStatus,
                                    timeout,
                                    cancellationToken,
                                    transaction);
                            await transaction.CommitAsync();
                            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Updated the Backup partition request to BackupInProcss for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
                        }
                        else
                        {
                            transaction.Abort();
                            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                                "Could not Update the  BackupPartition request to BackupInProcess for request Service Uri :  {0} , PartitionId {1} because currentBackupPartitionStatus value is {2}",
                                this.ServiceUri, this.PartitionId, currentBackupPartitionStatus);
                        }
                    }
                } 
                );
                await Task.Delay(timeOutPersecCycle,cancellationToken);
                backupPartitionStatus = await backupPartitionStore.GetValueAsync(fabricUri);
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "BackupPartitionstatusState {0},isCancellationRequested {1} for Service Uri :  {2} , PartitionId {3}", 
                    backupPartitionStatus.BackupPartitionStatusState, cancellationToken.IsCancellationRequested,this.ServiceUri,this.PartitionId);

            } while ( (backupPartitionStatus.BackupPartitionStatusState == BackupState.Accepted ||
                     backupPartitionStatus.BackupPartitionStatusState == BackupState.BackupInProgress ) &&
                     this.BackupRequestGuid.Equals(backupPartitionStatus.OperationId) &&
                     !cancellationToken.IsCancellationRequested);
            cancellationToken.ThrowIfCancellationRequested();
        }

        private BackupStoreInformation  GetBackupStoreInformationFromBackupStorage(BackupStorage backupStorage)
        {
            BackupStoreInformation storeInformation;
            switch(backupStorage.BackupStorageType)
            {
                case BackupStorageType.AzureBlobStore:
                    var azureStorage = (AzureBlobBackupStorageInfo)backupStorage;
                    storeInformation = new AzureBlobBackupStore
                    {
                        ConnectionString = azureStorage.ConnectionString,
                        ContainerName = azureStorage.ContainerName,
                        FolderPath = UtilityHelper.GetBaseDirectoryPathForPartition(this.ServiceUri, this.PartitionId),                    // TODO: This should be constructed
                        IsAccountKeyEncrypted = azureStorage.IsConnectionStringEncrypted,
                    };
                    break;
                case BackupStorageType.DsmsAzureBlobStore:
                    var dsmsAzureStorage = (DsmsAzureBlobBackupStorageInfo)backupStorage;
                    storeInformation = new DsmsAzureBlobBackupStore
                    {
                        StorageCredentialsSourceLocation = dsmsAzureStorage.StorageCredentialsSourceLocation,
                        ContainerName = dsmsAzureStorage.ContainerName,
                        FolderPath = UtilityHelper.GetBaseDirectoryPathForPartition(this.ServiceUri, this.PartitionId),
                    };
                    break;
                case BackupStorageType.FileShare:
                    var fileShareStorage = (FileShareBackupStorageInfo)backupStorage;
                    storeInformation = new FileShareBackupStore
                    {
                        AccessType = String.IsNullOrEmpty(fileShareStorage.PrimaryUserName) ? FileShareAccessType.None : FileShareAccessType.DomainUser,
                        FileSharePath = Path.Combine(fileShareStorage.Path, UtilityHelper.GetBaseDirectoryPathForPartition(this.ServiceUri, this.PartitionId)),
                        PrimaryUserName = fileShareStorage.PrimaryUserName,
                        PrimaryPassword = fileShareStorage.PrimaryPassword,
                        SecondaryUserName = fileShareStorage.SecondaryUserName,
                        SecondaryPassword = fileShareStorage.SecondaryPassword,
                        IsPasswordEncrypted = fileShareStorage.IsPasswordEncrypted,
                    };
                    break;
                default:
                    throw new ArgumentException(string.Format("{0} not supported.", backupStorage.BackupStorageType));
            }

            return storeInformation;
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.Append(base.ToString()).AppendLine();
            stringBuilder.AppendFormat("Service Uri {0},Partition Id : {1}", this.ServiceUri,this.PartitionId).AppendLine();
            stringBuilder.AppendFormat("At Backup Storage {0}",this.BackupStorage).AppendLine();
            stringBuilder.AppendFormat("with BackupRequest Guid {0},at Request DateTime {1} with BackupTimeout {2}",
                this.BackupRequestGuid,this.RequestDateTime,this.BackupTimeout).AppendLine();
            return stringBuilder.ToString();
        }
    }
}
