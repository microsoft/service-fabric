// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System;
    using System.Fabric;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using System.Fabric.BackupRestore.Common.Model;
    using Microsoft.ServiceFabric.Data;
    using System.Linq;
    using System.Collections.Generic;
    using RestorePoint = System.Fabric.BackupRestore.BackupRestoreTypes.RestorePoint;
    using System.Fabric.BackupRestore.Enums;
    using System.Collections.Concurrent;
    using System.Threading;
    using System.Fabric.Common;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;

    internal class RetentionManager
    {
        private static RetentionManager RManager;
        private static readonly SemaphoreSlim RManagerSemaphore = new SemaphoreSlim(1, 1);

        private RetentionStore RetentionStore;
        // should name it BackupRetentionManager.
        internal const string TraceType = "RetentionManagerTrace";
        StatefulService StatefulService;
        private CleanupStore CleanupStore;
        private ConcurrentDictionary<string, RetentionScheduler> RetentionSchedulerDict;
        internal CancellationTokenSource tokenSource;
        private const int jitterWaitMinTimeInMilliSeconds = 20;
        private const int jitterWaitMaxTimeInMilliSeconds = 50;
        private const int minimumWaitTimeForRetryOperationInMs = 1000;
        private const int maximumWaitTimeForRetryOperationInMs = 5000;
        private const int maximumWaitTimeForCleanUpRetryOperationInMs = 30000;
        private const int maxRetryCountForCleanup = 10;
        private const int maxRetryCountForDisposingRetention = 30;
        private const int waitTimeInMilliSecondForDeleteOp = 100;
        private const int waitTimeInSecondForTasksToClose = 30;
        private TimeSpan timeout = TimeSpan.FromMinutes(1);
        private Random rand;
        // PartititonCleanUpList is required.

        internal static async Task<RetentionManager> CreateOrGetRetentionManager(StatefulService statefulService)
        {
            if (RManager == null)
            {
                await RManagerSemaphore.WaitAsync();
                if(RManager == null)
                {
                    try
                    {
                        RetentionManager  retentionManager = new RetentionManager(statefulService);
                        await retentionManager.InitialiseRetentionManager();
                        RManager = retentionManager;
                    }
                    finally
                    {
                        RManagerSemaphore.Release();
                    }
                }
            }
            return RManager;
        }

        private RetentionManager(StatefulService StatefulService)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "RetentionManager constructor called");
            this.RetentionSchedulerDict = new ConcurrentDictionary<string, RetentionScheduler>();
            this.StatefulService = StatefulService;
            this.tokenSource = new CancellationTokenSource();
            this.rand = new Random();
        }

        /// <summary>
        /// Gets Invoked When a policy is created.
        /// </summary>
        /// <returns></returns>
        internal async Task CreatePolicyAsync(BackupPolicy backupPolicy, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            // create a metadata and initialise the RetentionMetadata.
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Scheduling retention for backupPolicy : {0}", backupPolicy.Name);
            RetentionPolicy retentionPolicy = backupPolicy.RetentionPolicy;

            if (null == retentionPolicy)
            {
                // there is nothing to be done.
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "RetentionPolicy for backupPolicy : {0} is not defined.", backupPolicy.Name);
            }
            else
            {
                RetentionMetadata retentionMetadata = new RetentionMetadata(backupPolicy.RetentionPolicy);
                await this.RetentionStore.AddAsync(backupPolicy.Name, retentionMetadata, timeout, cancellationToken, transaction);
                
                RetentionScheduler retentionScheduler = new RetentionScheduler(backupPolicy.Name, backupPolicy.RetentionPolicy, timerCallback);
                
                if (!RetentionSchedulerDict.TryAdd(backupPolicy.Name, retentionScheduler))
                {
                    BackupRestoreTrace.TraceSource.WriteError(TraceType, "CreatePolicyAsync: Not able to add retention scheduler for backupPolicy : {0}", backupPolicy.Name);
                    throw new InvalidOperationException(string.Format("{0}: Key already exists ", backupPolicy.Name));
                }
                retentionScheduler.ArmTimer(retentionMetadata);
            }
        }

        internal async Task InitialiseRetentionManager()
        {
            this.RetentionStore = await RetentionStore.CreateOrGetRetentionStore(StatefulService);
            this.CleanupStore = await CleanupStore.CreateOrGetCleanupStore(StatefulService);

            // Initialise RetentionSchedulerDict using RetentionStore and retention schedulers
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Intialising Retention Manager");
            using (ITransaction transaction = this.StatefulService.StateManager.CreateTransaction())
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Recreating RetentionSchedulerDict.");
                List<Tuple<string, RetentionMetadata>> listOfTuple = await this.RetentionStore.GetTupleListAsyncUtils(transaction, this.tokenSource.Token);
                foreach (var tuple in listOfTuple)
                {
                    if(!this.RetentionSchedulerDict.TryAdd(tuple.Item1, 
                        new RetentionScheduler(tuple.Item1, tuple.Item2.CurrentRetentionPolicy, timerCallback)))
                    {
                        BackupRestoreTrace.TraceSource.WriteError(TraceType, "Not able to add retention scheduler for backupPolicy : {0}", tuple.Item1);
                        throw new InvalidOperationException(string.Format("{0}: Key already exists ", tuple.Item1));
                    }
                }

                // Start all the cleanUp Tasks again.
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Starting all the previous cleanup tasks.");
                List<Tuple<string, List<string>>> listOfTuple2 = await this.CleanupStore.GetTupleListAsyncUtils(transaction, this.tokenSource.Token);
                int counter = 0;
                foreach (var tuple in listOfTuple2)
                {
                    var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService);
                    var backupPolicy = await backupPolicyStore.GetValueAsync(tuple.Item1, transaction);
                    if (tuple.Item2 != null && tuple.Item2.Count > 0)
                    {
                        counter += tuple.Item2.Count;
                        tuple.Item2.ForEach((partition =>
                        {
                            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Starting cleanUp task for partition : {0} and backupPolicy : {1}", partition, backupPolicy.Name);

                            Task task = CleanUpTaskUtil(partition, backupPolicy);
                        }));
                    }
                }
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Number of cleanup Tasks started : {0}", counter);
            }
        }

        // It should be called when cancellationToken is set.
        internal void StopRetentionManager()
        {
            try
            {
                // It is better if all the tasks running retentionSchedulerDict are deleted.
                this.tokenSource.Cancel();

                // Wait for all the running tasks to cancel.
                Task.Delay(TimeSpan.FromSeconds(waitTimeInSecondForTasksToClose));

                // Stop the timers.
                foreach (var tuple in this.RetentionSchedulerDict)
                {
                    RetentionScheduler retentionScheduler = tuple.Value;
                    retentionScheduler.Stop();
                }

                // Delete all the key value pairs.
                this.RetentionSchedulerDict.Clear();
            }
            catch (Exception ex)
            {
                BackupRestoreTrace.TraceSource.WriteError(TraceType, "RetentionManager was not stopped properly. ExceptionThrown: {0}", ex);
            }
            finally
            {
                this.tokenSource.Dispose();
            }
        }

        /// <summary>
        /// Gets invoked when the RetentionScheduler's timer's time is elapsed
        /// </summary>
        internal void timerCallback(string backupPolicyName)
        {
            RetentionScheduler retentionScheduler;
            if (!RetentionSchedulerDict.TryGetValue(backupPolicyName, out retentionScheduler))
            {
                BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "RetentionScheduler object is not found in retentionScheduler dict for policy: {0}", backupPolicyName);
                return;
            }
            bool toRearm = true;
            RetentionMetadata retentionMetadata = this.RetentionStore.GetValueAsync(backupPolicyName, this.timeout, this.tokenSource.Token).GetAwaiter().GetResult();
            try
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " TimerCallback running for backupPolicy : {0}", backupPolicyName);

                var backupPolicyStore = BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService).GetAwaiter().GetResult();
                BackupPolicy backupPolicy = backupPolicyStore.GetValueAsync(backupPolicyName, this.timeout, this.tokenSource.Token).GetAwaiter().GetResult();
                if(backupPolicy == null)
                {
                    /**
                     * This is used to garbage collect the retention scheduler. It could happen while deleting\updating policy retention scheduler object was not 
                     * stopped properly. So, timer will call its callback again just to find that the backup policy does not exists.
                     * In this case, we should stop the retentionScheduler.
                     * **/
                    RetentionScheduler retentionSchedulerToDelete;
                    if(RetentionSchedulerDict.TryRemove(backupPolicyName, out retentionSchedulerToDelete))
                    {
                        retentionSchedulerToDelete.Stop();
                    }
                    else
                    {
                        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "TimerCallback TryRemove retentionSchedulerDict failed of policy {0}. It could happen" +
                            " when residual callback already scheduled on threadpool thread is getting invoked.", backupPolicyName);
                    }
                    toRearm = false;
                    return;
                }
                HashSet<string> partitionsEnabledByPolicy = FindParititionsEnabledByPolicy(backupPolicy).GetAwaiter().GetResult();

                retentionMetadata.UpdateLastRetentionStartTime();

                this.RetentionStore.UpdateValueAsync(backupPolicyName, retentionMetadata, this.timeout, this.tokenSource.Token, null).GetAwaiter().GetResult();

                var derivedRetentionPolicy = (BasicRetentionPolicy)retentionMetadata.CurrentRetentionPolicy;
                TimeSpan retentionDuration = derivedRetentionPolicy.RetentionDuration;
                int minimumNumberOfBackups = derivedRetentionPolicy.MinimumNumberOfBackups;
                
                foreach (var partitionId in partitionsEnabledByPolicy)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Deleting backups for partitionId : {0}", partitionId);
                    List<string> partitionList = this.CleanupStore.GetValueAsync(backupPolicyName, this.timeout, this.tokenSource.Token).GetAwaiter().GetResult();
                    if (!(partitionList == null || partitionList.Count == 0) && partitionList.Contains(partitionId))
                    {
                        // Cleanup is going on for this partition.
                        continue;
                    }
                    DateTime endDateFilter = DateTime.UtcNow - retentionDuration;
                    BackupStorage storage = backupPolicy.Storage;
                    DeleteFilesForPartition(partitionId, endDateFilter, minimumNumberOfBackups, storage, backupPolicyName).GetAwaiter().GetResult();
                    this.tokenSource.Token.ThrowIfCancellationRequested();
                }

                // --> Introduce some wait time here so that CompletionTime is never equal to StartTime.
                retentionMetadata.UpdateLastRetentionCompletionTime();
                this.RetentionStore.UpdateValueAsync(backupPolicyName, retentionMetadata).GetAwaiter().GetResult();

                retentionScheduler.ArmTimer(retentionMetadata);
                toRearm = false;
            }
            catch (Exception ex)
            {
                if (ex is OperationCanceledException)
                {
                    // Since, timercallback was cancelled, therefore, there is no need to rearm the timer.
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " TimerCallback for backupPolicy : {0} was cancelled ", backupPolicyName);
                    toRearm = false;
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " TimerCallback for backupPolicy : {0} has thrown an exception : {1}", backupPolicyName, ex);
                }
            }
            finally
            {
                if(toRearm)
                {
                    retentionScheduler.RearmTimer(true, retentionMetadata);
                }
            }
        }

        internal async Task<HashSet<string>> FindParititionsEnabledByPolicy(BackupPolicy backupPolicy)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Finding all the partitions enabled for the backupPolicy : {0}", backupPolicy.Name);

            HashSet<string> partitionsEnabledByPolicy = new HashSet<string>();
            var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService);
            var backupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.StatefulService);
            var backupEnabledSet = backupPolicy.BackupEnabledSet.ToList();

            foreach (var backupedUri in backupEnabledSet)
            {
                string applicationName = null;
                string serviceName = null;
                string partitionId = null;
                FabricBackupResourceType fabricBackupResourceType =
                    UtilityHelper.GetApplicationAndServicePartitionUri(backupedUri, out applicationName, out serviceName,
                        out partitionId);

                switch (fabricBackupResourceType)
                {
                    case FabricBackupResourceType.PartitionUri:
                        partitionsEnabledByPolicy.Add(partitionId);
                        break;
                    default:
                        var partitionIDList = await GetPartitionsForServiceOrApplication(backupedUri, fabricBackupResourceType, this.timeout, this.tokenSource.Token);
                        foreach (var partitionID in partitionIDList)
                        {
                            partitionsEnabledByPolicy.Add(partitionID);
                            this.tokenSource.Token.ThrowIfCancellationRequested();
                        }
                        break;
                }
            }

            List<string> partitionListToRemove = new List<string>();
            foreach (var partitionId in partitionsEnabledByPolicy)
            {
                this.tokenSource.Token.ThrowIfCancellationRequested();
                string serviceNameUri = await FabricClientHelper.GetFabricServiceUriFromPartitionId(partitionId, timeout, tokenSource.Token);
                string applicationNameUri = await FabricClientHelper.GetFabricApplicationUriFromServiceUri(serviceNameUri, timeout, tokenSource.Token);
                string partitionUri = await UtilityHelper.GetFabricUriFromPartitionId(partitionId, this.timeout, this.tokenSource.Token);

                var backupMapping = (await backupMappingStore.GetValueAsync(partitionUri, timeout, this.tokenSource.Token) ??
                                                await backupMappingStore.GetValueAsync(serviceNameUri, this.timeout, tokenSource.Token)) ??
                                                await backupMappingStore.GetValueAsync(applicationNameUri, this.timeout, tokenSource.Token);
                if (backupMapping.BackupPolicyName != backupPolicy.Name)
                {
                    partitionListToRemove.Add(partitionId);
                }
            }

            foreach (var partitionId in partitionListToRemove)
            {
                partitionsEnabledByPolicy.Remove(partitionId);
            }

            return partitionsEnabledByPolicy;
        }

        internal async Task<List<string>> GetPartitionsForServiceOrApplication(string fabricUri, FabricBackupResourceType fabricBackupResourceType, TimeSpan timeout, CancellationToken cancellationToken)
        {
            List<string> serviceUriList = new List<string>();
            if (fabricBackupResourceType == FabricBackupResourceType.ApplicationUri)
            {
                Query.ServiceList serviceList =
                await FabricClientHelper.GetServiceList(fabricUri);
                foreach (Query.Service service in serviceList)
                {
                    serviceUriList.Add(service.ServiceName.OriginalString);
                }
            }
            else
            {
                serviceUriList.Add(fabricUri);
            }

            this.tokenSource.Token.ThrowIfCancellationRequested();

            List<string> partitionIdList = new List<string>();

            foreach (var serviceUri in serviceUriList)
            {
                Query.ServicePartitionList servicePartitionList = await
                    FabricClientHelper.GetPartitionList(serviceUri);
                foreach (Query.Partition servicePartition in servicePartitionList)
                {
                    partitionIdList.Add(servicePartition.PartitionInformation.Id.ToString());
                }
                this.tokenSource.Token.ThrowIfCancellationRequested();
            }

            return partitionIdList;
        }

        /// <summary>
        /// Gets called when a policy is updated
        /// </summary>
        internal async Task UpdatePolicyAsync(string backupPolicyName, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            /**
             * Cases:
             * 1. UpdateRetention ----> Re-arm the timer.
             *    0. Add Retention.
             *    a. Remove retention --> dispose the timer.
             *    b. update the retention.
             * 2. UpdateStorage ----> 
             * 3. Update Backup Schedule. --> no need to do anything/
             * 
             */
            try
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " In UpdatePolicyAsync of Retention Manager for Backup policy: {0}", backupPolicyName);
                var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService);
                RetentionMetadata retentionMetadata = await this.RetentionStore.GetValueAsync(backupPolicyName, timeout, cancellationToken, transaction);

                BackupPolicy backupPolicy = await backupPolicyStore.GetValueAsync(backupPolicyName, timeout, cancellationToken, transaction);
                if (retentionMetadata == null)
                {
                    if(backupPolicy.RetentionPolicy != null)
                    {
                        retentionMetadata = new RetentionMetadata(backupPolicy.RetentionPolicy);
                        await this.RetentionStore.AddAsync(backupPolicyName, retentionMetadata, timeout, cancellationToken, transaction);
                        RetentionScheduler retentionScheduler = new RetentionScheduler(backupPolicy.Name, backupPolicy.RetentionPolicy, timerCallback);

                        // Here we have updated the policy to introduce retention.
                        retentionScheduler.ArmTimer(retentionMetadata, true);
                        if(!RetentionSchedulerDict.TryAdd(backupPolicyName, retentionScheduler))
                        {
                            // If we are not able to add retention in RetentionSchedulerDict then, stop timer in retention scheduler
                            retentionScheduler.Stop();
                            BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "UpdatePolicyAsync, Not able to add retentionScheduler to dict for policy : {0} ", backupPolicyName);
                            throw new InvalidOperationException(string.Format("{0}: Key already exists.", backupPolicyName));
                        }
                    }
                }
                else if (backupPolicy.RetentionPolicy == null)
                {
                    // need to remove the retentionScheduler from RetentionStore.
                    Func<bool> condition = () =>
                    {
                        if (retentionMetadata.LastRetentionCompletionTime > retentionMetadata.LastRetentionStartTime || retentionMetadata.LastRetentionStartTime == DateTime.MinValue)
                        {
                            return true;
                        }
                        return false;
                    };

                    bool disposeComplete = await BackupRestoreUtility.TryPerformOperationWithWaitAsync(
                        (policyName, token) =>
                        {
                            return DisposeRetentionScheduler(policyName, timeout, token, transaction);
                        },
                        condition,
                        backupPolicyName,  
                        cancellationToken,
                        minimumWaitTimeForRetryOperationInMs, 
                        maxRetryCountForDisposingRetention,
                        minimumWaitTimeForRetryOperationInMs);

                    if(!disposeComplete)
                    {
                        BackupRestoreTrace.TraceSource.WriteError(TraceType, "Dispose retention did not complete for {0}", backupPolicyName);
                        throw new FabricBackupRestoreLocalRetryException();
                    }

                }
                else
                {
                    if (retentionMetadata.UpdateRetentionPolicyMetadata(backupPolicy.RetentionPolicy))
                    {
                        // Update the retention Scheduler store and arm the timer.
                        RetentionScheduler retentionScheduler;
                        if(!RetentionSchedulerDict.TryGetValue(backupPolicyName, out retentionScheduler))
                        {
                            BackupRestoreTrace.TraceSource.WriteError(TraceType, "UpdateBackupPolicy: Not able to get retention scheduler for backupPolicy {0}", backupPolicyName);
                            throw new KeyNotFoundException();
                        }

                        retentionScheduler.RearmTimer(false, retentionMetadata);
                        await this.RetentionStore.UpdateValueAsync(backupPolicyName, retentionMetadata, timeout, cancellationToken, transaction);
                    }
                }
            }
            catch (Exception ex)
            {
                if (ex is OperationCanceledException)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " DisposeRetentionScheduler ended with Timeout as the operation was cencelled: {0}", backupPolicyName);
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " It could be because the retention was in progress and user tried to remove the retention policy during UpdatePolicy: {0}", backupPolicyName);
                }
                else
                {
                    // Check for the exceptions of DisposeRetentionScheduler Timeout Retries.
                    BackupRestoreTrace.TraceSource.WriteError(TraceType, " UpdateBackupPolicy resulted in the exception : {0}", backupPolicyName);
                }
                throw;
            }

        }

        private async Task DisposeRetentionScheduler(string backupPolicyName, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " In DisposeRetentionScheduler for backupPolicy {0}", backupPolicyName);
            cancellationToken.ThrowIfCancellationRequested();

            RetentionMetadata retentionMetadata = await this.RetentionStore.GetValueAsync(backupPolicyName, timeout, cancellationToken, transaction);
            await this.RetentionStore.DeleteValueAsync(backupPolicyName, timeout, cancellationToken, transaction);

            // It means timer callback is not running.
            RetentionScheduler retentionScheduler;
            if(!this.RetentionSchedulerDict.TryRemove(backupPolicyName, out retentionScheduler))
            {
                // This means key does not exists and we are just trying to remove the key.
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Key: {0} not found in retentionSchedulerDict.", backupPolicyName);
                return;
            }
            retentionScheduler.Stop();

        }

        private async Task DeleteFilesForPartition(string partitionId, DateTime endDateFilter, int minimumNumberOfBackups, BackupStorage storage, string backupPolicyName, bool isCleanupTask = false)
        {
            // Design:
            /**
            * Need out Two API's from Azure share and file share
            * 1. List of all backups before a certain date for the partition
            * 2. Start deleting the files one by one.(one file at a time with some random wait time.)
            *
            **/

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " In DeleteFilesForPartition for partitionId : {0} with endDateFilter {1}", partitionId, endDateFilter);
            string applicationName = null;
            string serviceName = null;
            string partitionID = null;
            string fabricUri = await UtilityHelper.GetFabricUriFromPartitionId(partitionId, this.timeout, this.tokenSource.Token);
            var FabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionName(fabricUri, out applicationName,
                out serviceName, out partitionID);
            Debug.Assert(partitionId == partitionID);
            var storeManager = RecoveryPointManagerFactory.GetRecoveryPointManager(storage);

            this.tokenSource.Token.ThrowIfCancellationRequested();
            BRSContinuationToken bRSContinuationToken = new BRSContinuationToken();
            List<string> allBackupFiles = storeManager.EnumerateRecoveryPointMetadataFiles(storage.GetRelativeStorePath(applicationName, serviceName, partitionId), 
                DateTime.MinValue, DateTime.MaxValue, false, bRSContinuationToken, 0);

            List<RestorePoint> restorePoints = new List<RestorePoint>();
            if ((allBackupFiles != null) && (allBackupFiles.Count > 0))
            {
                restorePoints = await storeManager.GetRecoveryPointDetailsAsync(allBackupFiles, this.tokenSource.Token);
            }

            this.tokenSource.Token.ThrowIfCancellationRequested();

            int backupCount = allBackupFiles.Count;
            if (backupCount <= minimumNumberOfBackups)
            {
                return;
            }

            restorePoints = restorePoints.OrderBy(restorePoint => restorePoint.CreationTimeUtc).ToList();
            restorePoints = restorePoints.Where(restorePoint => restorePoint.CreationTimeUtc < endDateFilter).ToList();

            restorePoints.Reverse();
            bool lastFullBackupFound = false;
            RetentionMetadata retentionMetadata = await this.RetentionStore.GetValueAsync(backupPolicyName, this.timeout, this.tokenSource.Token);
            if (isCleanupTask)
            {
                lastFullBackupFound = true;
            }
            if (!isCleanupTask && retentionMetadata.OnGoingRetentionFile.ContainsKey(partitionId) &&
                retentionMetadata.OnGoingRetentionFile[partitionId] != null)
            {
                storeManager.DeleteBackupFiles(retentionMetadata.OnGoingRetentionFile[partitionId]);
            }
            foreach (var restorePoint in restorePoints)
            {
                this.tokenSource.Token.ThrowIfCancellationRequested();
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Trying to Delete Restore point with backupLocation: {0}", restorePoint.BackupLocation);
                if(!isCleanupTask)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Updating OnGoing RetentionFile in the metadata for backupPolicy : {0} with partitionId : {1} and BackupLocation {2}"
                    , backupPolicyName, partitionId, restorePoint.BackupLocation);
                    retentionMetadata.UpdateOnGoingRetentionFile(partitionId, restorePoint.BackupLocation);
                    await this.RetentionStore.UpdateValueAsync(backupPolicyName, retentionMetadata, this.timeout, this.tokenSource.Token, null);
                }

                if (!lastFullBackupFound)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Skipping RestorePoint with location : {0} as full backup is not found", restorePoint.BackupLocation);
                    if (restorePoint.BackupType == BackupOptionType.Full)
                    {
                        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Full Backup found so, after restore points after :{0} will be deleted. ", restorePoint.BackupLocation);
                        lastFullBackupFound = true;
                    }
                }
                else
                {
                    if (backupCount <= minimumNumberOfBackups)
                    {
                        break;
                    }
                    
                    if(storeManager.DeleteBackupFiles(restorePoint.BackupLocation))
                    {
                        BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Deleted Restore point with backupLocation: {0}", restorePoint.BackupLocation);
                        backupCount--;
                    }
                    else
                    {
                        BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " Not able to delete Restore point with backupLocation: {0}", restorePoint.BackupLocation);
                    }

                }
                await IntroduceRandomDelay();
            }

            // Deletion for a partition is completed.
            if(!isCleanupTask)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Updating OnGoing RetentionFile in the metadata for backupPolicy : {0}", backupPolicyName);
                retentionMetadata.UpdateOnGoingRetentionFile(partitionId);
                await this.RetentionStore.UpdateValueAsync(backupPolicyName, retentionMetadata, this.timeout, this.tokenSource.Token, null);
            }
        }

        internal async Task DisablePolicyAsync(string fabricUri, BackupMapping currentBackupMapping, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " In DisablePolicyAsync for fabricUri : {0}", fabricUri);
            string applicationNameUri = null;
            string serviceNameUri = null;
            string partitionID = null;
            var fabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationNameUri, out serviceNameUri, out partitionID);
            List<string> filteredPartitionList = new List<string>();
            var backupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.StatefulService);
            var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService);
            var currentBackupPolicy = await backupPolicyStore.GetValueAsync(currentBackupMapping.BackupPolicyName, transaction);

            BackupMapping inheritedBackupMapping = null;
            switch (fabricBackupResourceType)
            {
                case FabricBackupResourceType.ApplicationUri:
                    filteredPartitionList = await FilterPartitionListToDisableForFabricURI(await GetPartitionsForServiceOrApplication(fabricUri,
                        FabricBackupResourceType.ApplicationUri, timeout, cancellationToken), timeout, cancellationToken, FabricBackupResourceType.ServiceUri);
                    break;
                case FabricBackupResourceType.ServiceUri:
                    filteredPartitionList = await FilterPartitionListToDisableForFabricURI(await GetPartitionsForServiceOrApplication(fabricUri,
                        FabricBackupResourceType.ServiceUri, timeout, cancellationToken), timeout, cancellationToken, FabricBackupResourceType.ServiceUri);
                    inheritedBackupMapping = await backupMappingStore.GetValueAsync(applicationNameUri, transaction);
                    break;
                case FabricBackupResourceType.PartitionUri:
                    inheritedBackupMapping = await backupMappingStore.GetValueAsync(serviceNameUri, transaction);
                    if (inheritedBackupMapping == null)
                    {
                        inheritedBackupMapping = await backupMappingStore.GetValueAsync(applicationNameUri, transaction);
                    }
                    filteredPartitionList.Add(partitionID);
                    break;
            }
            foreach (var partitionId in filteredPartitionList)
            {
                // there can be only one inherited backup policy for a single URI.
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Trying to start DisablePolicyTask for partitionId : {0}", partitionId);
                await DisablePolicyAsync(currentBackupPolicy, partitionId, inheritedBackupMapping, timeout, cancellationToken, transaction);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Succesfully started DisablePolicyTask for partitionId : {0}", partitionId);
            }
        }

        internal async Task<List<string>> FilterPartitionListToDisableForFabricURI(List<string> partitionList, TimeSpan timeout, CancellationToken cancellationToken, FabricBackupResourceType fabricBackupResourceType)
        {
            List<string> filteredPartitionedList = new List<string>();
            foreach (var partitionId in partitionList)
            {
                var backupMapping = await GetEffectiveBackupMappingFromPartitionId(partitionId, timeout, cancellationToken, fabricBackupResourceType);
                if (backupMapping == null)
                {
                    filteredPartitionedList.Add(partitionId);
                }
            }
            return filteredPartitionedList;
        }

        internal async Task<BackupMapping> GetEffectiveBackupMappingFromPartitionId(string partitionId, TimeSpan timeout, CancellationToken cancellationToken, FabricBackupResourceType fabricBackupResourceType)
        {

            string serviceNameUri = await FabricClientHelper.GetFabricServiceUriFromPartitionId(partitionId, timeout, tokenSource.Token);
            string partitionUri = await UtilityHelper.GetFabricUriFromPartitionId(partitionId, this.timeout, this.tokenSource.Token);
            var backupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.StatefulService);
            BackupMapping backupMapping = null;
            if (fabricBackupResourceType == FabricBackupResourceType.ApplicationUri)
            {
                backupMapping = (await backupMappingStore.GetValueAsync(partitionUri, timeout, this.tokenSource.Token) ??
                                  await backupMappingStore.GetValueAsync(serviceNameUri, this.timeout, tokenSource.Token));
            }
            else
            {
                backupMapping = await backupMappingStore.GetValueAsync(partitionUri, timeout, this.tokenSource.Token);
            }
            return backupMapping;
        }

        /// <summary>
        /// Gets called when a partition is disabled for a pariticular policy.
        /// </summary>
        internal async Task DisablePolicyAsync(BackupPolicy backupPolicy, string partitionId, BackupMapping inheritedBackupMapping, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            /**
             * Cases:
             * 1. Partition has different policy inherited and storage is different, cleanup wont impact the same storage.
             *    ---> Check whether the timercallback is finished or not.
             * 2. Partition has different inherited policy and storage and container are same.
             *    --> NO Cleanup
             * **/

            // Since, you have disabled the policy now look for the backup mapping on the partition.
            var backupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService);
            if (inheritedBackupMapping == null)
            {
                await AddPartitionToCleanUpStore(partitionId, backupPolicy, cancellationToken, transaction);
                Task cleanUpTask = CleanUpTaskUtil(partitionId, backupPolicy);
            }
            else
            {
                var inheritedPolicy = await backupPolicyStore.GetValueAsync(inheritedBackupMapping.BackupPolicyName);
                if (!backupPolicy.Storage.CompareStorage(inheritedPolicy.Storage))
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "Inherited Policy for partition:{0} is having same storage, so, cleanup is skipped.");
                }
                else
                {
                    await AddPartitionToCleanUpStore(partitionId, backupPolicy, cancellationToken, transaction);
                    Task cleanUpTask = CleanUpTaskUtil(partitionId, backupPolicy);
                }
            }
        }

        internal async Task CleanUpTaskUtil(string partitionId, BackupPolicy backupPolicy)
        {
            try
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Running CleanUp task for partitionId : {0} and backupPolicy: {1}", partitionId, backupPolicy.Name);
                await IntroduceRandomDelay();

                RetentionMetadata retentionMetadata = await this.RetentionStore.GetValueAsync(backupPolicy.Name, this.timeout, this.tokenSource.Token);
                Func<bool> condition = () =>
                {
                    if (retentionMetadata == null || retentionMetadata.LastRetentionStartTime == DateTime.MinValue || retentionMetadata.LastRetentionCompletionTime > retentionMetadata.LastRetentionStartTime ||
                    !retentionMetadata.OnGoingRetentionFile.ContainsKey(partitionId))
                    {
                        return true;
                    }
                    return false;
                };

                bool cleanUpComplete = await BackupRestoreUtility.TryPerformOperationWithWaitAsync(
                    (partition, token) =>
                    {
                        return CleanUpTask(partition, backupPolicy, token);
                    }, condition, partitionId, 
                    this.tokenSource.Token,
                    minimumWaitTimeForRetryOperationInMs, 
                    maxRetryCountForCleanup,
                    maximumWaitTimeForCleanUpRetryOperationInMs
                    );

                if(!cleanUpComplete)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " CleanUp task for partitionId : {0} and backupPolicy : {1} was not completed." +
                        "It could be because retention of the partition was going on from a long time which did not complete 40 seconds.", partitionId, backupPolicy.Name);
                }
            }
            catch (Exception ex)
            {
                if (ex is OperationCanceledException)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, " CleanUp task for partitionId : {0} and backupPolicy : {1} is canceled", partitionId, backupPolicy.Name);
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteError(TraceType, " CleanUp task for partitionId : {0} and backupPolicy : {1} has thrown an exception : {2}", partitionId, backupPolicy.Name, ex);
                }
            }
        }


        private async Task AddPartitionToCleanUpStore(string partitionId, BackupPolicy backupPolicy, CancellationToken cancellationToken, ITransaction transaction)
        {
            // There should only be one callers of this function(Only from disable).
            List<string> partitionList = await this.CleanupStore.GetValueAsync(backupPolicy.Name, this.timeout, cancellationToken, transaction);

            if (partitionList != null && partitionList.Contains(partitionId))
            {
                // Cleanup Task is already running.
                return;
            }

            if (partitionList == null)
            {
                partitionList = new List<string>();
            }
            partitionList.Add(partitionId);
            await this.CleanupStore.UpdateValueAsync(backupPolicy.Name, partitionList, this.timeout, cancellationToken, transaction);
        }

        internal async Task CleanUpTask(string partitionId, BackupPolicy backupPolicy, CancellationToken cancellationToken)
        {    
            // Update the cleanUpStore to add the partition in partition list.
            List<string> partitionList = await this.CleanupStore.GetValueAsync(backupPolicy.Name, this.timeout, cancellationToken);
            await DeleteFilesForPartition(partitionId, DateTime.UtcNow, 0, backupPolicy.Storage, backupPolicy.Name, true);

            // Since, the files are deleted, update the cleanup store.
            partitionList = await this.CleanupStore.GetValueAsync(backupPolicy.Name, this.timeout, cancellationToken);
            partitionList.Remove(partitionId);
            await this.CleanupStore.UpdateValueAsync(backupPolicy.Name, partitionList, this.timeout, cancellationToken, null);
        }

        private async Task IntroduceRandomDelay()
        {
            await Task.Delay(this.rand.Next(jitterWaitMinTimeInMilliSeconds, jitterWaitMaxTimeInMilliSeconds));
        }

        internal async Task DeletePolicyAsync(string backupPolicyName,TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            /**
             * cases:
             * 1. If cleanup going on, then stall deleting the policy.
             * 
             * need to change cleanup store.
             * Cleanup store change ---> {"backupPolicyName": "PartitionList"}--> this should have been there at the first place.
             * 
             * Calling function should call it again and again and timeout if it true is not returned.
             * * */

            try
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " In DeletePolicyAsync : {0} is getting deleted", backupPolicyName);
                while (true)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    List<string> partitionList = await this.CleanupStore.GetValueAsync(backupPolicyName, timeout, cancellationToken, transaction);
                    if (partitionList == null || partitionList.Count == 0)
                    {
                        // Finally,remove retentionMetadata from retentionStore for the backupPolicyName.
                        RetentionMetadata retentionMetadata = await this.RetentionStore.GetValueAsync(backupPolicyName,timeout, cancellationToken, transaction);
                        if(retentionMetadata != null)
                        {

                            await this.RetentionStore.DeleteValueAsync(backupPolicyName, timeout, cancellationToken, transaction);
                            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, " Stopping RetentionScheduler for backupPolicy: {0}", backupPolicyName);
                            RetentionScheduler retentionScheduler;
                            if(!this.RetentionSchedulerDict.TryRemove(backupPolicyName, out retentionScheduler))
                            {
                                // We wanted to delete the value, we did not find it, so moving on.
                                return;
                            }
                            retentionScheduler.Stop();
                        }
                        return;
                    }
                    await Task.Delay(waitTimeInMilliSecondForDeleteOp);
                }

            }
            catch (Exception ex)
            {
                if(ex is OperationCanceledException)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(TraceType, "{0} Policy cannot be deleted because the operation is cancelled because of timeout ", backupPolicyName);
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteError(TraceType, "{0} Policy cannot be deleted because of the exception: {1} ", backupPolicyName, ex);
                }
                throw ex;
            }
        }
    }
}