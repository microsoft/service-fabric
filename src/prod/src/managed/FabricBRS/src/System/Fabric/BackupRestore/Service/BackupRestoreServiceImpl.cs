// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric;
using System.Fabric.BackupRestore.BackupRestoreTypes;
using System.Fabric.BackupRestore.Common;
using System.Fabric.BackupRestore.Common.Model;
using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.BackupRestore.Enums;
using System.Fabric.BackupRestore.Service;
using System.Fabric.Common;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Data;
using AzureStorage = System.Fabric.BackupRestore.Common.Model.AzureBlobBackupStorageInfo;
using DsmsAzureStorage = System.Fabric.BackupRestore.Common.Model.DsmsAzureBlobBackupStorageInfo;
using BackupPolicy = System.Fabric.BackupRestore.DataStructures.BackupPolicy;
using BackupStorage = System.Fabric.BackupRestore.Common.Model.BackupStorage;
using FileShareStorage = System.Fabric.BackupRestore.Common.Model.FileShareBackupStorageInfo;
using StatefulService = Microsoft.ServiceFabric.Services.Runtime.StatefulService;

internal sealed class BackupRestoreServiceImpl : IBackupRestoreService
{
    private const string TraceType = "BackupRestoreServiceImpl";
    private readonly StatefulService statefulService;

    public BackupRestoreServiceImpl(StatefulService statefulService)
    {
        this.statefulService = statefulService;
    }

    public async Task<BackupPolicy> GetBackupSchedulingPolicyAsync(BackupPartitionInfo backupPartitionInfo,
        TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        BackupPolicy backupPolicy;

        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "{0} GetBackupPolicy invoked", backupPartitionInfo.PartitionId);

        var backupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.statefulService);
        var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.statefulService);
        var suspendStore = await SuspendStore.CreateOrGetSuspendStatusStore(this.statefulService);
        var serviceNameUri = await UtilityHelper.GetCustomServiceUri(backupPartitionInfo.ServiceName, timeout, cancellationToken);
        var backupMappingKey = UtilityHelper.GetBackupMappingKey(serviceNameUri, backupPartitionInfo.PartitionId.ToString());

        var backupMappingModel = (await backupMappingStore.GetValueAsync(backupMappingKey) ??
                                        await backupMappingStore.GetValueAsync(serviceNameUri)) ??
                                            await backupMappingStore.GetValueAsync(UtilityHelper.GetApplicationNameFromService(serviceNameUri));
        bool isPartitionSuspended = await suspendStore.GetValueAsync(backupMappingKey) != null;

        if (backupMappingModel == null || isPartitionSuspended)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "{0} Backup protection not enabled", backupPartitionInfo.PartitionId);
            throw new FabricPeriodicBackupNotEnabledException();
        }

        var backupPolicyName = backupMappingModel.BackupPolicyName;
        var backupPolicyModel = await  backupPolicyStore.GetValueAsync(backupPolicyName);
        
        switch (backupPolicyModel.BackupSchedule.BackupScheduleType)
        {
            case BackupScheduleType.FrequencyBased:
                backupPolicy = new FrequencyBasedBackupPolicy();
                var frequencyBasedSchedulePolicy = (System.Fabric.BackupRestore.Common.Model.FrequencyBasedBackupSchedule) backupPolicyModel.BackupSchedule;
                backupPolicy.PolicyType = BackupPolicyType.FrequencyBased;
                var freqBackupPolicy = (FrequencyBasedBackupPolicy) backupPolicy;

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
                var scheduleBasedSchedulePolicy = (System.Fabric.BackupRestore.Common.Model.TimeBasedBackupSchedule) backupPolicyModel.BackupSchedule;
                backupPolicy.PolicyType = BackupPolicyType.ScheduleBased;

                var schdBackupPolicy = (ScheduleBasedBackupPolicy) backupPolicy;
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

        switch(backupPolicyModel.Storage.BackupStorageType)
        {
            case BackupStorageType.FileShare:
                var fileShareStorage = (System.Fabric.BackupRestore.Common.Model.FileShareBackupStorageInfo)backupPolicyModel.Storage;
                backupPolicy.StoreInformation = new FileShareBackupStore
                {
                    AccessType = String.IsNullOrEmpty(fileShareStorage.PrimaryUserName) ? FileShareAccessType.None : FileShareAccessType.DomainUser,
                    FileSharePath = Path.Combine(fileShareStorage.Path, UtilityHelper.GetBaseDirectoryPathForPartition(serviceNameUri, backupPartitionInfo.PartitionId.ToString())),
                    PrimaryUserName = fileShareStorage.PrimaryUserName,
                    PrimaryPassword = fileShareStorage.PrimaryPassword,
                    SecondaryUserName = fileShareStorage.SecondaryUserName,
                    SecondaryPassword = fileShareStorage.SecondaryPassword,
                    IsPasswordEncrypted = fileShareStorage.IsPasswordEncrypted,
                };
                break;

            case BackupStorageType.AzureBlobStore:
                var azureStorage = (System.Fabric.BackupRestore.Common.Model.AzureBlobBackupStorageInfo)backupPolicyModel.Storage;
                backupPolicy.StoreInformation = new AzureBlobBackupStore
                {
                    ConnectionString = azureStorage.ConnectionString,
                    ContainerName = azureStorage.ContainerName,
                    FolderPath = UtilityHelper.GetBaseDirectoryPathForPartition(serviceNameUri, backupPartitionInfo.PartitionId.ToString()),
                    IsAccountKeyEncrypted = azureStorage.IsConnectionStringEncrypted,
                };
                break;

            case BackupStorageType.DsmsAzureBlobStore:
                var dsmsAzureStorage = (System.Fabric.BackupRestore.Common.Model.DsmsAzureBlobBackupStorageInfo)backupPolicyModel.Storage;
                backupPolicy.StoreInformation = new DsmsAzureBlobBackupStore
                {
                    StorageCredentialsSourceLocation = dsmsAzureStorage.StorageCredentialsSourceLocation,
                    ContainerName = dsmsAzureStorage.ContainerName,
                    FolderPath = UtilityHelper.GetBaseDirectoryPathForPartition(serviceNameUri, backupPartitionInfo.PartitionId.ToString()),
                };
                break;
        }

        return backupPolicy;
    }

    public async Task<RestorePointDetails> GetRestorePointDetailsAsync(BackupPartitionInfo backupPartitionInfo, TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "GetRestorePointDetailsAsync invoked for partition {0}", backupPartitionInfo.PartitionId);

        var restoreStore = await RestoreStore.CreateOrGetRestoreStatusStore(this.statefulService);
        var backupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.statefulService);
        var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.statefulService);
        bool userInitiatedOperation;
        BackupStorage backupStorage;
        BackupStoreInformation backupStoreInformation = null;
        List<string> backupLocations;
        string brsServiceUri = await UtilityHelper.GetCustomServiceUri(backupPartitionInfo.ServiceName, timeout, cancellationToken);
        var fabricUri =  UtilityHelper.GetBackupMappingKey(brsServiceUri, backupPartitionInfo.PartitionId.ToString());
        var restoreStatus = await restoreStore.GetValueAsync(fabricUri);
        if (restoreStatus != null && (
            restoreStatus.RestoreStatusState != RestoreState.Failure ||
            restoreStatus.RestoreStatusState != RestoreState.Invalid ||
            restoreStatus.RestoreStatusState != RestoreState.Success ||
            restoreStatus.RestoreStatusState != RestoreState.Timeout))
        {
            backupStorage = restoreStatus.BackupStorage;
            userInitiatedOperation = true;
            backupLocations = restoreStatus.BackupLocations.ToList();
        }
        else
        {
            var backupMappingKey = UtilityHelper.GetBackupMappingKey(brsServiceUri,
                backupPartitionInfo.PartitionId.ToString());
            var backupMappingModel = (await backupMappingStore.GetValueAsync(backupMappingKey) ??
                                         await backupMappingStore.GetValueAsync(brsServiceUri)) ??
                                        await backupMappingStore.GetValueAsync(UtilityHelper.GetApplicationNameFromService(brsServiceUri));

            if (backupMappingModel == null)
            {
                throw new FabricPeriodicBackupNotEnabledException();
            }

            var backupPolicy = await backupPolicyStore.GetValueAsync(backupMappingModel.BackupPolicyName);
            if (backupPolicy != null && backupPolicy.AutoRestore)
            {
                backupStorage = backupPolicy.Storage;
                userInitiatedOperation = false;

                string applicationName, serviceName;
                UtilityHelper.GetApplicationAndServiceNameFromServiceUri(brsServiceUri,
                    out applicationName, out serviceName);

                // Get the latest backup now
                var backups = await backupStorage.GetBackupEnumerationsTask(
                                    applicationName,
                                    serviceName,
                                    backupPartitionInfo.PartitionId.ToString(),
                                    DateTime.MinValue,
                                    DateTime.MaxValue,
                                    true,
                                    cancellationToken);

                if (backups == null || backups.Count == 0)
                {
                    throw new ArgumentException(); // TODO: Change this with appropriate exception
                }

                var backupDetail = backups.First();

                var recoveryPointManager = RecoveryPointManagerFactory.GetRecoveryPointManager(backupStorage);
                backupLocations = await recoveryPointManager.GetBackupLocationsInBackupChainAsync(backupDetail.BackupLocation, cancellationToken);
            }
            else
            {
                throw new ArgumentException(); // TODO: Change with the OTher exception
            }
        }

        switch(backupStorage.BackupStorageType)
        {
            case BackupStorageType.FileShare:
                var fileShareStorage = (FileShareStorage)backupStorage;
                backupStoreInformation = new FileShareBackupStore
                {
                    AccessType = String.IsNullOrEmpty(fileShareStorage.PrimaryUserName) ? FileShareAccessType.None : FileShareAccessType.DomainUser,
                    FileSharePath = fileShareStorage.Path,
                    PrimaryUserName = fileShareStorage.PrimaryUserName,
                    PrimaryPassword = fileShareStorage.PrimaryPassword,
                    SecondaryUserName = fileShareStorage.SecondaryUserName,
                    SecondaryPassword = fileShareStorage.SecondaryPassword,
                    IsPasswordEncrypted = fileShareStorage.IsPasswordEncrypted,
                };
                break;

            case BackupStorageType.AzureBlobStore:
                var azureStorage = (AzureStorage)backupStorage;
                backupStoreInformation = new AzureBlobBackupStore
                {
                    ConnectionString = azureStorage.ConnectionString,
                    ContainerName = azureStorage.ContainerName,
                    FolderPath = String.Empty,                    // TODO: This should be constructed
                    IsAccountKeyEncrypted = azureStorage.IsConnectionStringEncrypted,
                };
                break;

            case BackupStorageType.DsmsAzureBlobStore:
                var dsmsAzureStorage = (DsmsAzureStorage)backupStorage;
                backupStoreInformation = new DsmsAzureBlobBackupStore
                {
                    StorageCredentialsSourceLocation = dsmsAzureStorage.StorageCredentialsSourceLocation,
                    ContainerName = dsmsAzureStorage.ContainerName,
                    FolderPath = String.Empty,                    // TODO: This should be constructed
                };
                break;
        }

        Guid restoreOperationId = Guid.NewGuid();

        if (userInitiatedOperation)
        {
            
            await UtilityHelper.InvokeWithRetry( async () =>
            {
                using (ITransaction transaction = this.statefulService.StateManager.CreateTransaction())
                {
                    var currentRestoreStatus = await restoreStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout, cancellationToken, transaction);
                    restoreOperationId = currentRestoreStatus.RestoreRequestGuid;
                    if (currentRestoreStatus.RestoreStatusState == RestoreState.Accepted) //TODO Check for Guid
                    {
                        var toUpdateRestoreStatus = currentRestoreStatus.ToBuilder().WithState(RestoreState.RestoreInProgress).Build();
                        await
                            restoreStore.UpdateValueAsync(fabricUri, toUpdateRestoreStatus, timeout,
                                cancellationToken, transaction);
                        await transaction.CommitAsync();
                    }
                    else
                    {
                        transaction.Abort();
                    }
                }

            });
        }

        var restorePartitionDetails  = new RestorePointDetails()
        {
            BackupLocations = backupLocations,
            UserInitiatedOperation = userInitiatedOperation,
            StoreInformation = backupStoreInformation,
            OperationId = restoreOperationId
        };

        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "GetRestorePointDetailsAsync invoked for partition {0} with Return Values {1}", backupPartitionInfo.PartitionId,restorePartitionDetails);
        return restorePartitionDetails;
    }

    public async Task RestoreOperationResultAsync(RestoreOperationResult operationResult, TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "RestoreOperationResultAsync invoked for partition {0}, ErrorCode {1}, message {2}", 
            operationResult.PartitionId, operationResult.ErrorCode, operationResult.Message);

        var restoreStore = await RestoreStore.CreateOrGetRestoreStatusStore(this.statefulService);
        var workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(this.statefulService);
        string brsServiceUri = await UtilityHelper.GetCustomServiceUri(operationResult.ServiceName, timeout, cancellationToken);
        var fabricUri = UtilityHelper.GetBackupMappingKey(brsServiceUri, operationResult.PartitionId.ToString());
        
        WorkItem pushPolicyAfterRestore = new SendToServiceNodeWorkItem(brsServiceUri, operationResult.PartitionId.ToString(), new WorkItemInfo()
        {
            WorkItemType = WorkItemPropogationType.UpdatePolicyAfterRestore,
        });
        await UtilityHelper.InvokeWithRetry(async () =>
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Trying to Update the Restore request for partition {0}", operationResult.PartitionId);
            using (ITransaction transaction = this.statefulService.StateManager.CreateTransaction())
            {
                var restoreStatus = await restoreStore.GetValueWithUpdateLockModeAsync(fabricUri,timeout,cancellationToken,transaction);
                if ( (restoreStatus != null) &&
                    (restoreStatus.RestoreStatusState == RestoreState.RestoreInProgress ||
                    restoreStatus.RestoreStatusState == RestoreState.Accepted ) &&
                    (restoreStatus.RestoreRequestGuid.Equals(operationResult.OperationId) ) )
                                                                                                            //TODO: Add check for each request by Guid
                {
                    var updatedRestoreStatus = restoreStatus.ToBuilder()
                                                .WithState(operationResult.ErrorCode != 0 ? RestoreState.Failure : RestoreState.Success)
                                                .WithErrorCode(operationResult.ErrorCode)
                                                .WithMessage(operationResult.Message)
                                                .WithTimeStampUtc(operationResult.TimeStampUtc)
                                                .Build();

                    await restoreStore.UpdateValueAsync(fabricUri, updatedRestoreStatus, timeout,cancellationToken, transaction);
                    await workItemQueue.AddWorkItem(pushPolicyAfterRestore, timeout, cancellationToken, transaction);
                    await transaction.CommitAsync();
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Updated the Restore request for partition {0}",operationResult.PartitionId);
                }
                else
                {
                    transaction.Abort();
                }
            }
        });
    }

    public async Task BackupOperationResultAsync(BackupOperationResult operationResult, TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        BackupRestoreTrace.TraceSource.WriteInfo(TraceType,
            "BackupOperationResultAsync invoked for partition {0}, OperationId {1} ErrorCode {2}, message {3}",
            operationResult.PartitionId, operationResult.OperationId, operationResult.ErrorCode, operationResult.Message);

        var backupPartitionStore =
            await BackupPartitionStore.CreateOrGetBackupPartitionStore(this.statefulService);
        var fabricUri = UtilityHelper.GetBackupMappingKey(await UtilityHelper.GetCustomServiceUri(operationResult.ServiceName, timeout, cancellationToken),
            operationResult.PartitionId.ToString());

        await UtilityHelper.InvokeWithRetry(async () =>
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType,
                        "Trying to update the Backup Request for Partition Id {0} ", operationResult.PartitionId);
            using (ITransaction transaction = this.statefulService.StateManager.CreateTransaction())
            {
                var backupPartitionStatus = await backupPartitionStore.GetValueWithUpdateLockModeAsync(fabricUri, timeout, cancellationToken, transaction);
                if ( (backupPartitionStatus != null ) &&
                    (backupPartitionStatus.BackupPartitionStatusState == BackupState.BackupInProgress ||
                    backupPartitionStatus.BackupPartitionStatusState == BackupState.Accepted ) &&
                    backupPartitionStatus.OperationId.Equals(operationResult.OperationId)) // TODO : Also check for Guid for the request
                {
                    var backupPartitionStatusBuilder = backupPartitionStatus.ToBuilder()
                                                        .WithState(operationResult.ErrorCode != 0 ? BackupState.Failure : BackupState.Success)
                                                        .WithErrorCode(operationResult.ErrorCode)
                                                        .WithMessage(operationResult.Message)
                                                        .WithTimeStampUtc(operationResult.TimeStampUtc);

                    if (operationResult.ErrorCode == 0)
                    {
                        var updatedEpoch = new BackupEpoch
                        {
                            ConfigurationNumber = operationResult.EpochOfLastBackupRecord.ConfigurationNumber,
                            DataLossNumber = operationResult.EpochOfLastBackupRecord.DataLossNumber
                        };

                        backupPartitionStatusBuilder
                            .WithEpochOfLastBackupRecord(updatedEpoch)
                            .WithLsnOfLastBackupRecord(operationResult.LsnOfLastBackupRecord)
                            .WithBackupId(operationResult.BackupId)
                            .WithBackupLocation(operationResult.BackupLocation);
                    }

                    await backupPartitionStore.UpdateValueAsync(fabricUri, backupPartitionStatusBuilder.Build(), transaction);
                    await transaction.CommitAsync();
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType,
                        "Updated the Backup Request for Partition Id {0} ", operationResult.PartitionId);
                }
                else
                {
                    transaction.Abort();
                }
            }
        });
    }
}