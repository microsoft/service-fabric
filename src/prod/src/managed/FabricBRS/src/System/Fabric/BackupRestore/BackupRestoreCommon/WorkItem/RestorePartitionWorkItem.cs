// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;

    [DataContract]
    internal  class RestorePartitionWorkItem : WorkItem
    {
        [DataMember]
        internal string ServiceUri { get; private set; }

        [DataMember]
        internal string PartitionId { get; private set; }

        [DataMember]
        private RestoreStatusStateModel RestoreStatusStateModel { get; set; }

        [DataMember]
        internal DateTime RequestDateTime { get; private set; }

        [DataMember]
        internal Guid DataLossGuid { get; private set; }

        [DataMember]
        internal bool IsDataLossRequestNeeded { get; private set; }

        [DataMember]
        internal TimeSpan RestoreTimeout { get; private set; }

        [DataMember]
        internal Guid RestoreRequestGuid { get; private set; }

        internal RestorePartitionWorkItem(string serviceUri, string partitionId , Guid restoreRequestGuid, Guid dataLossGuid , int restoreTimeoutinMinutes) 
        {
            this.ServiceUri = serviceUri;
            this.PartitionId = partitionId;
            this.RequestDateTime = DateTime.UtcNow;
            this.RestoreStatusStateModel = RestoreStatusStateModel.TriggerDataLoss;
            this.DataLossGuid = dataLossGuid;
            this.IsDataLossRequestNeeded = true;
            this.RestoreRequestGuid = restoreRequestGuid;
            this.RestoreTimeout = TimeSpan.FromMinutes(restoreTimeoutinMinutes);
        }

        public override async Task ProcessFailureHandle(StatefulService statefulService,TimeSpan timeout,CancellationToken cancellationToken , string processQueueTypeTrace)
        {
            WorkItemInProcessStore workItemInProcessStore = await
                WorkItemInProcessStore.CreateOrGetWorkItemInProcessStore(statefulService);
            WorkItemQueue1Min workItemQueue1Min = await WorkItemQueue1Min.CreateOrGetWorkItemQueue(statefulService);
            WorkItemProcessInfo workItemProcessInfo = await workItemInProcessStore.GetValueAsync(this.WorkItemGuid);
            using (ITransaction transaction = statefulService.StateManager.CreateTransaction())
            {
                await workItemInProcessStore.DeleteValueAsync(this.WorkItemGuid,timeout,cancellationToken,transaction);
                await workItemQueue1Min.AddWorkItem(workItemProcessInfo,timeout,cancellationToken, transaction);
                await transaction.CommitAsync();
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Process Failure Handle WorkItemInProcessStore Delete WorkItem {0} And Enqueue in {1}  ", this, workItemQueue1Min.GetType());
            }
        }

        public override async Task<bool> Process(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken, string processQueueTypeTrace)
        {
            DateTime timeoutDateTime = this.RequestDateTime.AddSeconds(this.RestoreTimeout.TotalSeconds);
            string fabricUri = UtilityHelper.GetBackupMappingKey(this.ServiceUri, this.PartitionId);
            RestoreStore restoreStore = await RestoreStore.CreateOrGetRestoreStatusStore(statefulService);
            if (DateTime.Compare(DateTime.UtcNow, timeoutDateTime) > 0)
            {
                await UtilityHelper.InvokeWithRetry(() => this.UpdateRestoreStatusAsTimeout(statefulService, fabricUri,timeout,cancellationToken,
                    processQueueTypeTrace));
                return true;
            }
            TimeSpan timeoutSpan = timeoutDateTime.Subtract(DateTime.UtcNow);
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "TimeoutSpan for Backup Partition WorkItem {0}", timeoutSpan);

            CancellationTokenSource cancellationTokenSource = new CancellationTokenSource(timeoutSpan);
            try
            {
                await this.ProcessRestore(statefulService, cancellationTokenSource.Token, processQueueTypeTrace);
            }
            catch (OperationCanceledException operationCanceledException)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                    "Operation Canceled exception Details {0}",
                    operationCanceledException.ToString());

                await UtilityHelper.InvokeWithRetry(() => this.UpdateRestoreStatusAsTimeout(statefulService, fabricUri, timeout,  cancellationToken, processQueueTypeTrace));
            }
            return true;
        }

        private async Task UpdateRestoreStatusAsTimeout(StatefulService statefulService , string fabricUri, TimeSpan timeout, CancellationToken cancellationToken,
            string processQueueTypeTrace)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,"Updating the Restore status as timeout for request Service Uri :  {0} , PartitionId {1}",this.ServiceUri ,this.PartitionId);
            RestoreStore restoreStore = await RestoreStore.CreateOrGetRestoreStatusStore(statefulService);
            WorkItemQueue workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(statefulService);
            WorkItem workItem = new SendToServiceNodeWorkItem(this.ServiceUri,this.PartitionId,new WorkItemInfo()
            {
                WorkItemType =  WorkItemPropogationType.UpdatePolicyAfterRestore
            });
            using (ITransaction transaction = statefulService.StateManager.CreateTransaction())
            {
                var currentRestoreStatus = await restoreStore.GetValueWithUpdateLockModeAsync(fabricUri,timeout,cancellationToken,transaction);
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "RestoreStatus {0} as timeout for request Service Uri :  {1} , PartitionId {2}", currentRestoreStatus, this.ServiceUri, this.PartitionId);
                if (currentRestoreStatus != null &&
                    currentRestoreStatus.RestoreRequestGuid.Equals(this.RestoreRequestGuid) &&
                    (currentRestoreStatus.RestoreStatusState == RestoreState.Accepted ||
                     currentRestoreStatus.RestoreStatusState == RestoreState.RestoreInProgress))
                {
                    var restoreStatus = currentRestoreStatus.ToBuilder()
                        .WithState(RestoreState.Timeout)
                        .WithErrorCode(unchecked((int)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_TIMEOUT))
                        .WithMessage(StringResources.RestoreTimeout)
                        .Build();

                    await restoreStore.UpdateValueAsync(fabricUri, restoreStatus, timeout,cancellationToken,transaction);
                    await workItemQueue.AddWorkItem(workItem, timeout, cancellationToken, transaction);
                    await transaction.CommitAsync();
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Updated Restore Status as Timeout for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
                }
                else
                {
                    transaction.Abort();
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Updated Restore Status as Timeout Failed for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
                }
                
            }
        }

        public async Task ProcessRestore(StatefulService statefulService,CancellationToken cancellationToken,string processQueueTypeTrace)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Processing the Restore for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
            WorkItemStore workItemStore = await WorkItemStore.CreateOrGetWorkItemStore(statefulService);
            RestoreStore restoreStore = await RestoreStore.CreateOrGetRestoreStatusStore(statefulService);
            
            string fabricUri = UtilityHelper.GetBackupMappingKey(this.ServiceUri, this.PartitionId);
            PartitionDataLossProgress partitionDataLossProgress = await this.GetPartitionDataLossProgress(processQueueTypeTrace);
            if (this.RestoreStatusStateModel == RestoreStatusStateModel.TriggerDataLoss)
            {
                if (partitionDataLossProgress == null)
                {
                    bool isInitiateDataLossSuccess =   await this.InitiateDataLoss(processQueueTypeTrace);
                    if (isInitiateDataLossSuccess)
                    {
                        this.RestoreStatusStateModel = RestoreStatusStateModel.DataLossTriggered;
                    }
                }
                await workItemStore.UpdateValueAsync(this.WorkItemGuid, this);
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "RestoreStatusStateModel as DataLossTriggered for request Service Uri :  {0} , PartitionId {1}", this.ServiceUri, this.PartitionId);
            }
            if (this.RestoreStatusStateModel == RestoreStatusStateModel.DataLossTriggered)
            {
                RestoreStatus restoreStatus = null;
                do
                {
                    partitionDataLossProgress = await this.GetPartitionDataLossProgress(processQueueTypeTrace);
                    switch (partitionDataLossProgress.State)
                    {
                        case TestCommandProgressState.Running:
                        case TestCommandProgressState.RollingBack:
                        case TestCommandProgressState.Completed:
                            break;
                        case TestCommandProgressState.Cancelled:
                        case TestCommandProgressState.Faulted:
                        case TestCommandProgressState.ForceCancelled:
                            this.DataLossGuid = Guid.NewGuid();
                            await workItemStore.UpdateValueAsync(this.WorkItemGuid, this);
                            await this.InitiateDataLoss(processQueueTypeTrace);
                            break;
                    }
                    await Task.Delay(Constants.CheckRestoreTimeSpan,cancellationToken);
                    restoreStatus = await restoreStore.GetValueAsync(fabricUri);
                    if (restoreStatus.RestoreStatusState == RestoreState.Timeout && restoreStatus.RestoreRequestGuid.Equals(this.RestoreRequestGuid))
                    {
                        partitionDataLossProgress = await this.GetPartitionDataLossProgress(processQueueTypeTrace);
                        switch (partitionDataLossProgress.State)
                        {
                            case TestCommandProgressState.Running:
                            case TestCommandProgressState.RollingBack:
                                await this.CancelDataLoss(processQueueTypeTrace);
                                break;
                            case TestCommandProgressState.Cancelled:
                            case TestCommandProgressState.Faulted:
                            case TestCommandProgressState.ForceCancelled:
                            case TestCommandProgressState.Completed:
                                break;
                        }
                    }
                } while (( restoreStatus.RestoreStatusState == RestoreState.RestoreInProgress ||
                         restoreStatus.RestoreStatusState == RestoreState.Accepted ) && 
                         this.RestoreRequestGuid.Equals(restoreStatus.RestoreRequestGuid) &&
                         !cancellationToken.IsCancellationRequested) ;
                cancellationToken.ThrowIfCancellationRequested();
            }
        }

        private async Task<PartitionDataLossProgress> GetPartitionDataLossProgress(string processQueueTypeTrace)
        {
            PartitionDataLossProgress partitionDataLossProgress = null;
            try
            {
                partitionDataLossProgress = await FabricClientHelper.GetPartitionDataLossProgress(this.DataLossGuid, Constants.GetDataLossProgressTimeout);
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                    "Get GetPartitionDataLossProgress Resulted as {0} with State as {1}",
                    partitionDataLossProgress.Result, partitionDataLossProgress.State.ToString());
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                    "Exception in GetPartitionDataLossProgress {0}",
                    exception.ToString());
            }
            return partitionDataLossProgress;
        }

        private async Task<bool> InitiateDataLoss(string processQueueTypeTrace)
        {
            bool isInitiateDataLossSuccess = false;
            try
            {
                await FabricClientHelper.InitiatePartitionDataLoss(this.DataLossGuid, this.ServiceUri, this.PartitionId,Constants.RequestDataLossTimeout);
                isInitiateDataLossSuccess = true;
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Successfully initiated the Data Loss");
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                    "Exception in GetPartitionDataLossProgress {0}",
                    exception.ToString());
            }
            return isInitiateDataLossSuccess;
        }

        private async Task<bool> CancelDataLoss(string processQueueTypeTrace)
        {
            bool isCancelDataLossSuccess = false;
            try
            {
                await FabricClientHelper.CancelPartitionDataLoss(this.DataLossGuid,  Constants.RequestDataLossTimeout);
                isCancelDataLossSuccess = true;
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Canceled the Data Loss Successfully");
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                    "Exception in GetPartitionDataLossProgress {0}",
                    exception.ToString());
            }
            return isCancelDataLossSuccess;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.Append(base.ToString()).AppendLine();
            stringBuilder.AppendFormat("Service Uri {0},Partition Id{1}",this.ServiceUri,this.PartitionId).AppendLine();
            stringBuilder.AppendFormat("at RequestDateTime {0} ,with RequestTimeout as {1}", this.RequestDateTime, this.RestoreTimeout).AppendLine();
            stringBuilder.AppendFormat("with DataLossGuid{0} ,IsDataLossRequestNeeded{1}, with RestoreStateModel as {2} ",
                this.DataLossGuid, this.IsDataLossRequestNeeded, this.RestoreStatusStateModel);
            return stringBuilder.ToString();
        }
    }

    enum RestoreStatusStateModel
    {
        TriggerDataLoss,
        DataLossTriggered,
    }

}
