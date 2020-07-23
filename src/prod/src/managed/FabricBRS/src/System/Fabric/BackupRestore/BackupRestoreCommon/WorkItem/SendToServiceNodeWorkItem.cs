// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.BackupRestore.Service;
    using System.Fabric.Common;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using BackupPolicy = Model.BackupPolicy;
    using BackupPolicyDataStructure = DataStructures.BackupPolicy;

    [DataContract]
    internal  class SendToServiceNodeWorkItem : BackupPolicyWorkItem
    {
        [DataMember]
        internal string ServiceUri { get; private set; }

        [DataMember]
        internal string PartitionId { get; private set; }

        internal SendToServiceNodeWorkItem(string serviceUri, string partitionId,WorkItemInfo workItemInfo) :base(workItemInfo)
        {
            this.ServiceUri = serviceUri;
            this.PartitionId = partitionId;
        }

        #region Public override methods

        public override async Task<bool> Process(StatefulService statefulService, TimeSpan timeout, CancellationToken cancellationToken, string processQueueTypeTrace)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                "Send the Data for  ApplicationServiceUri = {0}, PartitionID = {1} WorkItemInfo {2} at UTC Time : {3} at Now Time {4} ",
                this.ServiceUri, this.PartitionId, this.WorkItemInfo, DateTime.UtcNow, DateTime.Now);

            var protectionPartitionTuple = await this.GetEffectiveBackupPartitionAndPolicyForPartition(statefulService, timeout, cancellationToken, processQueueTypeTrace);

            switch (this.WorkItemInfo.WorkItemType)
            {
                case WorkItemPropogationType.Disable:
                    
                    // Just send disable message but before sending disable check is there any application or service level backup policy exisintg
                    if (protectionPartitionTuple.Item1 == null)
                    {
                        
                        await this.ServiceExistingCheckUtility( () =>  this.DisablePartitionBackup(cancellationToken),
                            timeout, cancellationToken, processQueueTypeTrace);
                    }
                    else if(protectionPartitionTuple.Item2 != null)
                    {
                        if (!await this.IsPartitionBackupSuspended(statefulService, processQueueTypeTrace))
                        {
                            await
                                this.ServiceExistingCheckUtility(
                                    () => this.EnablePartitionBackup(protectionPartitionTuple.Item2, cancellationToken),timeout, cancellationToken, processQueueTypeTrace);
                        }
                    }
                    break;

                case WorkItemPropogationType.UpdateProtection:
                case WorkItemPropogationType.Enable:
                    if (protectionPartitionTuple.Item1 == null || protectionPartitionTuple.Item2 == null)
                    {
                        return true; //The enable request (protection) or backup policy is not existing hence no need to send to service node
                    }
                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                        "For Enable backupMapping.ProtectionId as {0} == {1} this.WorkItemInfo.ProctectionGuid && " +
                        "backupPolicy.UniqueId {2} == {3} this.WorkItemInfo.BackupPolicyUpdateGuid ",
                        protectionPartitionTuple.Item1.ProtectionId, this.WorkItemInfo.ProctectionGuid,
                        protectionPartitionTuple.Item2.UniqueId , this.WorkItemInfo.BackupPolicyUpdateGuid);

                    if (protectionPartitionTuple.Item1.ProtectionId.Equals(this.WorkItemInfo.ProctectionGuid) 
                        && protectionPartitionTuple.Item2.UniqueId.Equals(this.WorkItemInfo.BackupPolicyUpdateGuid) 
                        && (!await this.IsPartitionBackupSuspended(statefulService, processQueueTypeTrace)))
                    {
                        // Send to service
                        await
                              this.ServiceExistingCheckUtility(
                                  () =>
                                      this.EnablePartitionBackup(protectionPartitionTuple.Item2, cancellationToken),timeout, cancellationToken, processQueueTypeTrace);

                    }
                    else
                    {
                        // An updated request is existing in the work item queue hence discarding this request
                        return true;
                    }

                    break;

                case WorkItemPropogationType.UpdateBackupPolicy:
                    if (protectionPartitionTuple.Item1 == null || protectionPartitionTuple.Item2 == null)
                    {
                        return true; //The enable request or backup policy is not existing hence no need to send to servcie node OR Backup policy is not existing hence no need to send to servcie node
                    }

                    BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "BackupPolicy Unique Id {0} == {1} WorkItemInfo.BackupPolicyUpdateGuid", 
                        protectionPartitionTuple.Item2,this.WorkItemInfo.BackupPolicyUpdateGuid );
                    if (protectionPartitionTuple.Item2.UniqueId.Equals(this.WorkItemInfo.BackupPolicyUpdateGuid) && 
                        (!await this.IsPartitionBackupSuspended(statefulService, processQueueTypeTrace)))
                    {
                        // send the update to service node
                        await this.ServiceExistingCheckUtility(
                                () =>
                                    this.EnablePartitionBackup(protectionPartitionTuple.Item2, cancellationToken),
                                timeout, cancellationToken, processQueueTypeTrace);
                    }

                    break;

                case WorkItemPropogationType.SuspendPartition:
                    if (await this.IsPartitionBackupSuspended(statefulService, processQueueTypeTrace))
                    {
                        await this.ServiceExistingCheckUtility(
                                () => this.DisablePartitionBackup(cancellationToken),
                                timeout, cancellationToken, processQueueTypeTrace);
                    }

                    break;

                case WorkItemPropogationType.ResumePartition:

                    if (protectionPartitionTuple.Item1 == null || protectionPartitionTuple.Item2 == null)
                    {
                        return true; //The enable request or backup policy is not existing hence no need to send to servcie node OR Backup policy is not existing hence no need to send to servcie node
                    }

                    if (! await this.IsPartitionBackupSuspended(statefulService, processQueueTypeTrace))
                    {
                        await this.ServiceExistingCheckUtility(
                               () =>
                                   this.EnablePartitionBackup(protectionPartitionTuple.Item2,
                                       cancellationToken),
                               timeout, cancellationToken, processQueueTypeTrace);
                    }

                    break;

                case WorkItemPropogationType.UpdatePolicyAfterRestore:
                    if (protectionPartitionTuple.Item1 == null || protectionPartitionTuple.Item2 == null)
                    {
                        return true; //The enable request or backup policy is not existing hence no need to send to servcie node OR Backup policy is not existing hence no need to send to servcie node
                    }
                    if (!await this.IsPartitionBackupSuspended(statefulService, processQueueTypeTrace))
                    {
                        await this.ServiceExistingCheckUtility(
                                () =>
                                    this.EnablePartitionBackup(protectionPartitionTuple.Item2,
                                        cancellationToken),
                                timeout, cancellationToken, processQueueTypeTrace);
                    }

                    break;
            }
            
            return true;
        }

        #endregion

        #region Private Send To Service Node Utility Methods

        private BackupPolicyDataStructure GetBackupPolicyDataStructure(BackupPolicy backupPolicyModel, string serviceName, string partitionId)
        {
            BackupPolicyDataStructure backupPolicy;
            switch (backupPolicyModel.BackupSchedule.BackupScheduleType)
            {
                case BackupScheduleType.FrequencyBased:
                    backupPolicy = new FrequencyBasedBackupPolicy();
                    var frequencyBasedSchedulePolicy = (FrequencyBasedBackupSchedule)backupPolicyModel.BackupSchedule;
                    backupPolicy.PolicyType = BackupPolicyType.FrequencyBased;
                    var freqBackupPolicy = (FrequencyBasedBackupPolicy)backupPolicy;
                    freqBackupPolicy.RunFrequency = (ushort)frequencyBasedSchedulePolicy.Interval;

                    if (frequencyBasedSchedulePolicy.IntervalType == BackupScheduleInterval.Hours)
                    {
                        freqBackupPolicy.RunFrequencyType = BackupPolicyRunFrequency.Hours;
                    }

                    if (frequencyBasedSchedulePolicy.IntervalType == BackupScheduleInterval.Minutes)
                    {
                        freqBackupPolicy.RunFrequencyType = BackupPolicyRunFrequency.Minutes;
                    }

                    break;

                case BackupScheduleType.TimeBased:
                    backupPolicy = new ScheduleBasedBackupPolicy();
                    var scheduleBasedSchedulePolicy = (TimeBasedBackupSchedule)backupPolicyModel.BackupSchedule;
                    backupPolicy.PolicyType = BackupPolicyType.ScheduleBased;

                    var schdBackupPolicy = (ScheduleBasedBackupPolicy)backupPolicy;
                    if (scheduleBasedSchedulePolicy.ScheduleFrequencyType == BackupScheduleFrequency.Daily)
                    {
                        schdBackupPolicy.RunSchedule = BackupPolicyRunSchedule.Daily;
                    }

                    if (scheduleBasedSchedulePolicy.ScheduleFrequencyType == BackupScheduleFrequency.Weekly)
                    {
                        schdBackupPolicy.RunSchedule = BackupPolicyRunSchedule.Weekly;
                    }

                    schdBackupPolicy.RunDays = new List<DayOfWeek>(scheduleBasedSchedulePolicy.RunDays);
                    schdBackupPolicy.RunTimes = new List<TimeSpan>(scheduleBasedSchedulePolicy.RunTimes);

                    break;

                default:
                    throw new FabricPeriodicBackupNotEnabledException();
            }

            backupPolicy.MaxIncrementalBackups = Convert.ToByte(backupPolicyModel.MaxIncrementalBackup);
            backupPolicy.Name = backupPolicyModel.Name;
            backupPolicy.PolicyId = backupPolicyModel.UniqueId;

            switch (backupPolicyModel.Storage.BackupStorageType)
            {
                case BackupStorageType.FileShare:
                    var fileShareStorage = (FileShareBackupStorageInfo)backupPolicyModel.Storage;
                    backupPolicy.StoreInformation = new FileShareBackupStore
                    {
                        AccessType = String.IsNullOrEmpty(fileShareStorage.PrimaryUserName) ? FileShareAccessType.None : FileShareAccessType.DomainUser,
                        FileSharePath = Path.Combine(fileShareStorage.Path, UtilityHelper.GetBaseDirectoryPathForPartition(serviceName, partitionId)),
                        PrimaryUserName = fileShareStorage.PrimaryUserName,
                        PrimaryPassword = fileShareStorage.PrimaryPassword,
                        SecondaryUserName = fileShareStorage.SecondaryUserName,
                        SecondaryPassword = fileShareStorage.SecondaryPassword,
                        IsPasswordEncrypted = fileShareStorage.IsPasswordEncrypted,
                    };
                    break;

                case BackupStorageType.AzureBlobStore:
                    var azureStorage = (AzureBlobBackupStorageInfo)backupPolicyModel.Storage;
                    backupPolicy.StoreInformation = new AzureBlobBackupStore
                    {
                        ConnectionString = azureStorage.ConnectionString,
                        ContainerName = azureStorage.ContainerName,
                        FolderPath = UtilityHelper.GetBaseDirectoryPathForPartition(serviceName, partitionId),                    // TODO: This should be constructed
                        IsAccountKeyEncrypted = azureStorage.IsConnectionStringEncrypted,
                    };
                    break;

                case BackupStorageType.DsmsAzureBlobStore:
                    var dsmsAzureStorage = (DsmsAzureBlobBackupStorageInfo)backupPolicyModel.Storage;
                    backupPolicy.StoreInformation = new DsmsAzureBlobBackupStore
                    {
                        StorageCredentialsSourceLocation = dsmsAzureStorage.StorageCredentialsSourceLocation,
                        ContainerName = dsmsAzureStorage.ContainerName,
                        FolderPath = UtilityHelper.GetBaseDirectoryPathForPartition(serviceName, partitionId),                    // TODO: This should be constructed
                    };
                    break;
            }

            return backupPolicy;
        }

        private async Task<bool> IsPartitionBackupSuspended(StatefulService statefulService,string processQueueTypeTrace)
        {
            var suspendStore = await SuspendStore.CreateOrGetSuspendStatusStore(statefulService);
            var fabricKey = UtilityHelper.GetBackupMappingKey(this.ServiceUri, this.PartitionId);
            var containsSuspension = await suspendStore.GetValueAsync(fabricKey) ?? await suspendStore.GetValueAsync(ServiceUri) ?? await suspendStore.GetValueAsync(UtilityHelper.GetApplicationUriFromServiceUri(ServiceUri));
            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "IsPartition Suspended {0}", containsSuspension != null);
            return containsSuspension != null;
        }

        private async Task ServiceExistingCheckUtility(Func<Task> sendToServiceNodeFunc,TimeSpan timeout, CancellationToken cancellationToken , string processQueueTypeTrace)
        {
            try
            {
                await sendToServiceNodeFunc();
                BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Sent successfully");
            }
            catch (Exception exception)
            {
                var fabricException = exception as FabricException;
                if (fabricException?.ErrorCode == FabricErrorCode.ServiceOffline ||
                    fabricException?.ErrorCode ==  FabricErrorCode.PartitionNotFound)
                {
                    try
                    {
                        await FabricClientHelper.ValidatePartition(this.ServiceUri, this.PartitionId, timeout,
                                cancellationToken);
                    }
                    catch (Exception validateException)
                    {
                        var validateFabricException = validateException as FabricException;
                        if (validateFabricException?.ErrorCode == FabricErrorCode.PartitionNotFound)
                        {
                            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace, "Request {0} is Deleted as Partition Doesn't exist",this);
                            return;
                        }
                    }
                }

                throw;
            }
        }

        private async Task EnablePartitionBackup(BackupPolicy backupPolicy, CancellationToken cancellationToken)
        {
            await Program.ServiceAgent.EnableProtectionAsync(new Uri(UtilityHelper.GetUriFromCustomUri(this.ServiceUri)),
                               Guid.Parse(this.PartitionId),
                               this.GetBackupPolicyDataStructure(backupPolicy, this.ServiceUri, this.PartitionId),
                               BackupRestoreServiceConfig.ApiTimeout, cancellationToken);
        }

        private async Task DisablePartitionBackup(CancellationToken cancellationToken)
        {
            await Program.ServiceAgent.DisableProtectionAsync(new Uri(UtilityHelper.GetUriFromCustomUri(this.ServiceUri)),
                             Guid.Parse(this.PartitionId),
                             BackupRestoreServiceConfig.ApiTimeout, cancellationToken);
        }

        private async Task<Tuple<BackupMapping, BackupPolicy>>  GetEffectiveBackupPartitionAndPolicyForPartition(
            StatefulService statefulService,TimeSpan timeout,CancellationToken cancellationToken, string processQueueTypeTrace)
        {
            BackupPolicy backupPolicy = null;
            var backupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(statefulService);
            var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(statefulService);
            var backupMappingKey = UtilityHelper.GetBackupMappingKey(this.ServiceUri, this.PartitionId);

            var backupMapping = (await backupMappingStore.GetValueAsync(backupMappingKey, timeout, cancellationToken) ??
                                                 await backupMappingStore.GetValueAsync(this.ServiceUri, timeout, cancellationToken)) ??
                                                 await backupMappingStore.GetValueAsync(UtilityHelper.GetApplicationNameFromService(this.ServiceUri), timeout, cancellationToken);
            if (backupMapping != null)
            {
                backupPolicy = await backupPolicyStore.GetValueAsync(backupMapping.BackupPolicyName,timeout,cancellationToken);
            }

            BackupRestoreTrace.TraceSource.WriteInfo(processQueueTypeTrace,
                "GetEffectiveBackupPartitionAndPolicyForPartition resulted as BackupMapping {0} ,BackupPolicy {1} ", backupMapping,backupPolicy);

            return Tuple.Create(backupMapping, backupPolicy);
        }

        #endregion

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendLine(base.ToString());
            stringBuilder.AppendFormat("Partition Id : {0}",this.PartitionId).AppendLine();
            stringBuilder.AppendFormat("Service Uri : {0}",this.ServiceUri).AppendLine();
            stringBuilder.AppendLine(base.ToString());
            return stringBuilder.ToString();
        }
    }
}