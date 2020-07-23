// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Diagnostics;
using System.Fabric.BackupRestore.DataStructures;
using System.Fabric.BackupRestore.Interop;
using System.Fabric.Common;
using System.Fabric.Health;
using System.Fabric.Interop;
using System.IO;
using System.IO.Compression;
using System.Threading;
using System.Threading.Tasks;
using System.Linq;
using System.Fabric.Common.Tracing;
using System.Globalization;

namespace System.Fabric.BackupRestore
{
    internal class BackupRestoreManager : IBackupRestoreManager, IBackupRestoreHandler
    {
        #region Private Fields 

        private NativeBackupRestoreRuntime.IFabricBackupRestoreAgent nativeAgent;
        private readonly IBackupRestoreReplica replica;
        private BackupMetadata backupMetadataObj;
        private BackupScheduler backupScheduler;
        private CancellationTokenSource backupCts;
        private IStatefulServicePartition partition;
        private StatefulServiceInitializationParameters initializationParameters;
        private bool replicaRegisteredWithBap;
        private ReplicaRole replicaRole;

        private const string LocalBackupFolderName = "B";
        private const string LocalRestoreFolderName = "R";
        private const string BackupRestoreRootFolderPrefix = "brs_";

        private const string ZipExtension = ".zip";
        private const string MetadataFileNameExtension = ".bkmetadata";
        private const string BackupFolderNameFormat = "yyyy-MM-dd HH.mm.ss";

        private const int PrimaryStatusCheckRetryIntervalInMillis = 512;

        private TimeSpan apiTimeout;
        private TimeSpan storeApiTimeout;
        
        private CancellationTokenSource initializeAsyncCts;
        private Task initializeAsyncTask;
        private bool zipBackups;
        private const string TraceType = "BackupRestoreManager";

        private readonly SemaphoreSlim backupSemaphore = new SemaphoreSlim(1, 1);

        private readonly object backupNowRequestLock = new object();
        private Guid backupNowOperationId;
        private bool backupNowInProgress;
        private BackupNowConfiguration backupNowConfiguration;
        private CancellationTokenSource backupNowCts;
        private Task backupNowTask;
        private RecoveryPointMetadataProperties backupPropertiesForBackupNow;
        private bool? isBackupRestoreServiceConfigured;
        private bool isSystemService;
        private const int initialRetryIntervalMs = 1000;
        private const int maxRetryCount = 5;
        private const int maxRetryIntervalMs = 5000;


        private Stopwatch telemetryStopWatch;

        /// <summary>
        /// Used to synchronize access/ update to backup metadata object and save to DB.
        /// Update can happen from backup callback and from UpdateBackupPolicy from BRS.
        /// </summary>
        private readonly SemaphoreSlim metadataSyncSemaphore = new SemaphoreSlim(1, 1);

        internal static TimeSpan InitialRetryInterval { get; private set; }
        internal static TimeSpan MaxRetryInterval { get; private set; }
        internal static int MaxRetryCount { get; private set; }

        #endregion

        #region Constructor

        public BackupRestoreManager(IBackupRestoreReplica replica)
        {
            this.replica = replica;
            this.replicaRole = ReplicaRole.Unknown;
            BackupRestoreUtility.TraceSource = AppTrace.TraceSource;

            if (!this.IsSystemService() && this.IsBackupRestoreServiceConfigured())
            {
                Utility.WrapNativeSyncInvokeInMTA(this.CreateNativeAgent, "BackupRestoreWorker.CreateNativeAgent");

                // Initialize constants
                this.PopulateConstants();
            }

            this.telemetryStopWatch = new Stopwatch();
        }

        #endregion

        #region IBackupRestoreManager methods

        public void Initialize(StatefulServiceInitializationParameters initializationParametersParam)
        {
            if (!this.IsBackupRestoreServiceConfigured()) return;
            
            this.initializationParameters = initializationParametersParam;
            this.isSystemService = this.initializationParameters.CodePackageActivationContext.ApplicationName.Equals("fabric:/System");
        }

        public Task OpenAsync(ReplicaOpenMode openMode, IStatefulServicePartition partitionParam, CancellationToken cancellationToken)
        {
            if (!this.IsBackupRestoreServiceConfigured()) return Task.FromResult(0);

            this.partition = partitionParam;

            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "{0} OpenAsync invoked",
                this.GetTraceId());

            return Task.FromResult(0);
        }

        public async Task ChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            // Do nothing for system services or if Backup Restore service is not configured
            if (this.IsSystemService() || !this.IsBackupRestoreServiceConfigured())
            {
                this.replicaRole = newRole;
                return;
            }

            AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} ChangeRoleAsync. new role {1}",
                        this.GetTraceId(),
                        newRole);

            if (newRole == ReplicaRole.Primary)
            {
                this.RegisterBackupRestoreReplica(this.partition.PartitionInfo.Id, this.initializationParameters.ReplicaId, this);
                this.replicaRegisteredWithBap = true;
                this.initializeAsyncCts = new CancellationTokenSource();
                this.initializeAsyncTask = Task.Run(() => this.InitializeAsync(this.initializeAsyncCts.Token), CancellationToken.None);
            }
            else
            {
                await this.CancelInitializeAsync();
                await this.CancelBackupNowTaskAsync();
                if (null != this.backupCts)
                {
                    this.backupCts.Cancel();
                }

                this.Cleanup();
            }

            this.replicaRole = newRole;
        }

        public async Task CloseAsync(CancellationToken cancellationToken)
        {
            // Do nothing for system services or if Backup Restore service is not configured
            if (this.IsSystemService() || !this.IsBackupRestoreServiceConfigured()) return;

            AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} CloseAsync invoked",
                        this.GetTraceId());

            await this.CancelInitializeAsync();
            await this.CancelBackupNowTaskAsync();
            if (null != this.backupCts)
            {
                this.backupCts.Cancel();
            }

            this.Cleanup();
        }

        public void Abort()
        {
            // Do nothing for system services or if Backup Restore service is not configured
            if (this.IsSystemService() || !this.IsBackupRestoreServiceConfigured()) return;

            AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} Abort invoked",
                        this.GetTraceId());

            this.CancelInitializeAsync().ContinueWith(t => t.Exception, TaskContinuationOptions.OnlyOnFaulted);
            this.CancelBackupNowTaskAsync().ContinueWith(t => t.Exception, TaskContinuationOptions.OnlyOnFaulted);
            if (null != this.backupCts)
            {
                this.backupCts.Cancel();
            }

            this.Cleanup();
        }

        public async Task<bool> OnDataLossAsync(CancellationToken cancellationToken)
        {
            // Do nothing for system services or if Backup Restore service is not configured
            if (this.IsSystemService() || !this.IsBackupRestoreServiceConfigured()) return false;

            this.telemetryStopWatch.Restart();
            

            AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} On data loss invoked.",
                        this.GetTraceId());

            // If Backup Restore service is configured, we follow the below logic:
            // -> We first try to get the restore context from FabricBRS
            // -> If we get back the restore context from FabricBRS, we go ahead with the restore/ recovery
            // -> Or else, if we get the context details as empty but the partition is currently protection enabled with FabricBRS, we wait and keep on querying FabricBRS for a context
            // -> Otherwise, if we dont get a context and the partition is also not enabled for protection, we return back with false allowing user's OnDataLossAsyc to be invoked (if implemented)
            // -> In case, FabricBRS is not reachable, we keep on w

            RestorePointDetails recoveryPointDetails = null;
            var continueLoop = true;
            do
            {
                try
                {
                    // Now contact FabricBRS to see if user has triggered a restore for this partition
                    recoveryPointDetails = await this.GetRecoveryPointDetailsAsynHelper(cancellationToken);
                    continueLoop = false;
                    AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} Successfully fetched recoverypoint details from BRS",
                        this.GetTraceId());
                }
                catch (FabricPeriodicBackupNotEnabledException)
                {
                    // Restore context not found and protection is also not enabled, return from here.
                    AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} Recoverypoint details not found with BRS and protection is also not enabled",
                        this.GetTraceId());

                    return false;
                }
                catch (Exception ex)
                {
                    // Log the exception
                    AppTrace.TraceSource.WriteExceptionAsError(
                        TraceType,
                        ex,
                        "{0} Exception encountered in OnDataLossAsync",
                        this.GetTraceId());

                    await Task.Delay(InitialRetryInterval, cancellationToken);
                    // TODO: We need to add a mitigation here in case FabricBRS is not reachable.
                }
            } while (continueLoop);

            var destDir = String.Empty;

#if !DotNetCoreClr
            bool isAutoRestored = false;
            int totalBackupCounts = 0;
            bool isRestoreSuccessfull = false;
#endif

            try
            {
                destDir = this.GetRootBrsTempFolderPath(false);
                var downloadBackupInfo = new DownloadBackupInfo(recoveryPointDetails.BackupLocations, destDir);

                // Download the recovery points
                await this.DownloadBackupAsyncHelper(downloadBackupInfo, recoveryPointDetails.StoreInformation, cancellationToken);

                // Unzip the files if they are zipped
                foreach (var backup in recoveryPointDetails.BackupLocations)
                {
                    if (backup.EndsWith(ZipExtension))
                    {
                        var folderName = Path.GetFileNameWithoutExtension(backup);
                        var fileName = Path.GetFileName(backup);
                        var destinationDir = Path.Combine(destDir, folderName);
                        var destinationZipFileName = Path.Combine(destDir, fileName);
                        BackupRestoreUtility.PerformIOWithRetries(() => {
                            ZipFile.ExtractToDirectory(destinationZipFileName, destinationDir);
                        }, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs
                            );
                        
                        // Delete the zip file
                        BackupRestoreUtility.PerformIOWithRetries(() => {
                            File.Delete(destinationZipFileName);
                        }, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);

                    }
                }

                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} Successfully downloaded all backups",
                    this.GetTraceId());

#if !DotNetCoreClr
                isAutoRestored = !recoveryPointDetails.UserInitiatedOperation;
                totalBackupCounts = recoveryPointDetails.BackupLocations.Count;
#endif

                // All backups downloaded, invoke restore
                await BackupRestoreUtility.PerformIOWithRetriesAsync((destDirectory, token) => {
                    return this.replica.RestoreAsync(destDirectory, recoveryPointDetails.UserInitiatedOperation, token);
                }, destDir, cancellationToken, initialRetryIntervalMs, maxRetryCount, maxRetryIntervalMs);

                AppTrace.TraceSource.WriteInfo(
                    TraceType,
                    "{0} Succefully restored partition",
                    this.GetTraceId());

#if !DotNetCoreClr
                isRestoreSuccessfull = true;
#endif

                // Post a health event
                this.ReportRestoreSuccessHealth(recoveryPointDetails.UserInitiatedOperation);
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteExceptionAsError(
                    TraceType,
                    ex,
                    "{0} Error encountered while restore partition, user initiated operation - {1}",
                    this.GetTraceId(), recoveryPointDetails.UserInitiatedOperation);

                // Report health only if restore operation was not user initiated
                if (!recoveryPointDetails.UserInitiatedOperation)
                {
                    // Post a health event
                    this.ReportRestoreFailureHealth(ex);
                }
                else    // Else report back to BRS
                {
                    // Notify back BRS about restore failure
                    var operationResult = new RestoreOperationResult
                    {
                        ErrorCode = ex.HResult,
                        Message = ex.Message,
                        ServiceName = this.initializationParameters.ServiceName.ToString(),
                        PartitionId = this.partition.PartitionInfo.Id,
                        OperationId = recoveryPointDetails.OperationId,
                        TimeStampUtc = DateTime.UtcNow,
                    };

                    await BackupRestoreUtility.PerformFabricOperationWithRetriesAsync(
                            this.ReportRestoreOperationResultAsyncHelper,
                            operationResult, cancellationToken);
                }

                if (recoveryPointDetails.UserInitiatedOperation) return true;     // If its user initiated, let's return back from OnDataLoss with value is true
                // else, we are returning back exception which should trigger the data loss again.
                throw;
            }
            finally
            {
                if (!String.IsNullOrEmpty(destDir))
                {
                    try
                    {
                        Directory.Delete(destDir, true);

                    }
                    catch (Exception)
                    {
                        // Ignore any exception
                    }
                }

#if !DotNetCoreClr
                FabricEvents.Events.BackupRestoreDetails(isAutoRestored ? 1:0 , isRestoreSuccessfull ? 1:0 , this.telemetryStopWatch.Elapsed.TotalSeconds, 
                    totalBackupCounts,this.partition.PartitionInfo.Id.ToString());
#endif

            }

            // Report back to BRS only if the request is user initiated
            if (recoveryPointDetails.UserInitiatedOperation)
            {
                try
                {
                    var operationResult = new RestoreOperationResult
                    {
                        ErrorCode = 0,          // Success
                        ServiceName = this.initializationParameters.ServiceName.ToString(),
                        PartitionId = this.partition.PartitionInfo.Id,
                        OperationId = recoveryPointDetails.OperationId,
                        TimeStampUtc = DateTime.UtcNow,
                        Message = "Partition has been successfully restored."
                    };

                    // Notify back BRS about the restore completion, with retry
                    await BackupRestoreUtility.PerformFabricOperationWithRetriesAsync(
                            this.ReportRestoreOperationResultAsyncHelper,
                            operationResult, cancellationToken);

                    AppTrace.TraceSource.WriteInfo(
                        TraceType,
                        "{0} On data loss completed, posted report to FabricBRS, returning true",
                        this.GetTraceId());

                    return true;
                }
                catch (Exception ex)
                {
                    AppTrace.TraceSource.WriteExceptionAsWarning(
                        TraceType,
                        ex,
                        "{0} Error encountered while posting restore success to BRS",
                        this.GetTraceId());

                    return false;
                }
            }

            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "{0} On data loss completed, returning true",
                this.GetTraceId());

            return true;
            // TODO: Handle BRS not reachable
        }





#endregion

#region IBackupRestoreHandler Methods

        public async Task UpdateBackupSchedulingPolicyAsync(BackupPolicy policy, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var stopWatch = new Stopwatch();
            stopWatch.Start();

            if (null == policy)
            {
                AppTrace.TraceSource.WriteInfo(TraceType, "{0} UpdateBackupSchedulingPolicyAsync: Protection Disabled for partition", this.GetTraceId());

                // Protection disabled, stop backup

                if (null != this.backupMetadataObj)
                {
                    var tempMetadata = new BackupMetadata
                    {
                        PartitionId = this.partition.PartitionInfo.Id,
                        Policy = null,                  // Indicates dummy policy
                        PolicyUpdateTime = DateTime.UtcNow
                    };

                    await metadataSyncSemaphore.WaitAsync();

                    try
                    {
                        await this.replica.SaveBackupMetadataAsync(tempMetadata.Serialize(), timeout - stopWatch.Elapsed, cancellationToken);
                        this.backupMetadataObj = null;
                    }
                    finally
                    {
                        metadataSyncSemaphore.Release();
                    }
                }
                else
                {
                    await this.CancelInitializeAsync();
                }

                // Stop the timer
                if (null != this.backupScheduler)
                {
                    this.backupScheduler.Stop();
                    this.backupScheduler = null;
                }

                this.ClearBackupHealth();
            }
            else
            {
                // Protection getting enabled/ updated
                AppTrace.TraceSource.WriteInfo(TraceType, "{0} UpdateBackupSchedulingPolicyAsync: Protection Enabled/ Updated for partition, new Policy Id {1}",
                    this.GetTraceId(), policy.PolicyId);

                // First check if the policy is same as the existing one.
                if (this.backupMetadataObj != null && this.backupMetadataObj.Policy.PolicyId == policy.PolicyId) return;

                if (this.backupMetadataObj == null)
                {
                    await this.CancelInitializeAsync();
                }

                // Stop the timer
                if (null != this.backupScheduler)
                {
                    this.backupScheduler.Stop();
                    this.backupScheduler = null;
                }

                var tempObj = new BackupMetadata
                {
                    PartitionId = this.partition.PartitionInfo.Id,
                    Policy = policy,
                    PolicyUpdateTime = DateTime.UtcNow,
                };

                await metadataSyncSemaphore.WaitAsync();

                try
                {
                    await this.replica.SaveBackupMetadataAsync(tempObj.Serialize(), timeout - stopWatch.Elapsed, cancellationToken);
                    this.backupMetadataObj = tempObj;
                }
                finally
                {
                    metadataSyncSemaphore.Release();
                }

                this.backupScheduler = new BackupScheduler(this.backupMetadataObj, this.TimerCallback);
                this.backupScheduler.ArmTimer(false);
            }

            stopWatch.Stop();
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} UpdateBackupSchedulingPolicyAsync completed", this.GetTraceId());
        }

        public Task BackupPartitionAsync(Guid operationId, BackupNowConfiguration configuration, TimeSpan timeout, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(
                TraceType,
                "{0} BackupPartitionAsync invoked for operation ID {1}",
                this.GetTraceId(), operationId);
            
            lock (this.backupNowRequestLock)
            {
                if (this.backupNowInProgress)
                {
                    if (this.backupNowOperationId != operationId)
                    {
                        AppTrace.TraceSource.WriteWarning(
                            TraceType,
                            "{0} BackupPartitionAsync: Backup now in progress. Existing operation ID {1}, New operation ID {2}",
                            this.GetTraceId(), this.backupNowOperationId, operationId);

                        throw new FabricBackupInProgressException(
                            String.Format("{0}: Backup request already queued with operation ID {1}", this.GetTraceId(), this.backupNowOperationId));
                    }
                }
                else
                {
                    if (this.backupNowOperationId == operationId)
                    {
                        AppTrace.TraceSource.WriteInfo(
                            TraceType,
                            "{0} BackupPartitionAsync: Backup now in progress. Duplicate request operation ID {1}, returning",
                            this.GetTraceId(), this.backupNowOperationId);

                        return Task.FromResult(0);
                    }

                    this.backupNowInProgress = true;
                    this.backupNowOperationId = operationId;
                    this.backupNowConfiguration = configuration;

                    // Initiate backup now async task
                    this.backupNowCts = new CancellationTokenSource(configuration.OperationTimeoutMilliseconds);
                    this.backupNowTask = Task.Run(() => this.BackupNowAsync(this.backupNowCts.Token), CancellationToken.None);
                }
            }

            return Task.FromResult(0);
        }

#endregion

#region Private methods

        private void PopulateConstants()
        {
            var configStore = NativeConfigStore.FabricGetConfigStore();

            var apiTimeoutString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.ApiTimeoutKeyName);
            this.apiTimeout = String.IsNullOrEmpty(apiTimeoutString) ? BackupRestoreContants.ApiTimeoutInSecondsDefault : TimeSpan.FromSeconds(int.Parse(apiTimeoutString, CultureInfo.InvariantCulture));

            var storeApiTimeoutString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.StoreApiTimeoutKeyName);
            this.storeApiTimeout = String.IsNullOrEmpty(storeApiTimeoutString) ? BackupRestoreContants.StoreApiTimeoutInSecondsDefault : TimeSpan.FromSeconds(int.Parse(storeApiTimeoutString, CultureInfo.InvariantCulture));
            
            var retryIntervalSeconds = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.ApiRetryIntervalKeyName);
            InitialRetryInterval = String.IsNullOrEmpty(retryIntervalSeconds) ? BackupRestoreContants.ApiRetryIntervalInSecondsDefault : TimeSpan.FromSeconds(int.Parse(retryIntervalSeconds, CultureInfo.InvariantCulture));

            var maxRetryIntervalSeconds = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.MaxApiRetryIntervalKeyName);
            MaxRetryInterval = String.IsNullOrEmpty(maxRetryIntervalSeconds) ? BackupRestoreContants.MaxApiRetryIntervalInSecondsDefault : TimeSpan.FromSeconds(int.Parse(maxRetryIntervalSeconds, CultureInfo.InvariantCulture));

            var maxRetryCountString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.MaxApiRetryCountKeyName);
            MaxRetryCount = String.IsNullOrEmpty(maxRetryCountString) ? BackupRestoreContants.MaxRetryCountDefault : int.Parse(maxRetryCountString, CultureInfo.InvariantCulture);

            var zipBackupsString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.EnableCompressionKeyName);
            this.zipBackups = String.IsNullOrEmpty(retryIntervalSeconds) || bool.Parse(zipBackupsString);
        }

        private string GetRootBrsTempFolderPath(bool backup)
        {
            return Path.Combine(
                Path.GetTempPath(),
                BackupRestoreRootFolderPrefix + this.partition.PartitionInfo.Id,
                backup ? LocalBackupFolderName : LocalRestoreFolderName);
        }

        private bool IsSystemService()
        {
            return this.isSystemService;
        }

        private string GetTraceId()
        {
            return this.partition.PartitionInfo.Id.ToString();
        }

        private string GetBackupRelativePath()
        {
            // NOTE: The construction of relative path logic currently is assumed both in this library and FabricBRS. 
            // Though for backup case, FabricBRS passes the path which includes the relative path, while saving in the metadata file,
            // we are invoking this method to calculate the same. This can be fixed in future.
            var serviceUriName = this.initializationParameters.ServiceName.ToString();
            var applicationUriName = this.initializationParameters.CodePackageActivationContext.ApplicationName;
            var brsApplicationName = string.Join("/", applicationUriName.Split('/').Skip(1)).Replace('/','$');
            var brsserviceName = serviceUriName.Replace(applicationUriName, "").Trim('/').Replace('/','$');
            return Path.Combine(brsApplicationName, brsserviceName, this.partition.PartitionInfo.Id.ToString());
        }

        private async Task BackupNowAsync(CancellationToken cancellationToken)
        {
            try
            {
                BackupOperationResult backupResult;
                try
                {
                    // Let's first wait for the backup semaphore
                    await this.backupSemaphore.WaitAsync(cancellationToken);
                    this.telemetryStopWatch.Restart();
                    try
                    {
                        AppTrace.TraceSource.WriteInfo(TraceType, "{0} Taking backup now for operation id: {1}",
                            this.GetTraceId(), this.backupNowOperationId);

                        // We got the semaphore, let's trigger a backup now
                        await this.replica.BackupAsync(BackupOption.Full, this.BackupNowCallback, cancellationToken);
                    }
                    finally
                    {
                        // Release the semaphore
                        this.backupSemaphore.Release();
                    }

                    backupResult = new BackupOperationResult
                    {
                        ErrorCode = 0,
                        Message = "Partition successfully backed up.",
                        OperationId = this.backupNowOperationId,
                        PartitionId = this.partition.PartitionInfo.Id,
                        ServiceName = this.initializationParameters.ServiceName.ToString(),
                        TimeStampUtc = this.backupPropertiesForBackupNow.BackupTime,
                        BackupLocation = this.backupPropertiesForBackupNow.BackupLocation,
                        BackupId = this.backupPropertiesForBackupNow.BackupId,
                        EpochOfLastBackupRecord = this.backupPropertiesForBackupNow.EpochOfLastBackupRecord,
                        LsnOfLastBackupRecord = this.backupPropertiesForBackupNow.LsnOfLastBackupRecord,
                    };
                }
                catch (OperationCanceledException ex)
                {
                    AppTrace.TraceSource.WriteExceptionAsWarning(TraceType, ex, "{0} Got operation cancelled exception when trying to take backup now for operation ID {1}",
                        this.GetTraceId(), this.backupNowOperationId);

                    // Throwing back exception, as this operation is cancelled / timed out.
                    throw;
                }
                catch (Exception ex)
                {
                    AppTrace.TraceSource.WriteExceptionAsWarning(TraceType, ex, "{0} Got exception when trying to take backup now for operation ID {1}",
                        this.GetTraceId(), this.backupNowOperationId);
                    
                    backupResult = new BackupOperationResult
                    {
                        ErrorCode = ex.HResult,
                        Message = ex.Message,
                        OperationId = this.backupNowOperationId,
                        PartitionId = this.partition.PartitionInfo.Id,
                        ServiceName = this.initializationParameters.ServiceName.ToString(),
                        TimeStampUtc = DateTime.UtcNow,
                    };
                }

                // Backup is now completed, let's send a response to BRS
                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Reporting backup status to FabricBRS for backup now opertation: {1}",
                        this.GetTraceId(), this.backupNowOperationId);

                do
                {
                    try
                    {
                        await this.ReportBackupOperationResultAsyncHelper(backupResult, cancellationToken);
                        break;
                    }
                    catch (Exception ex)
                    {
                        AppTrace.TraceSource.WriteExceptionAsWarning(TraceType, ex, "{0} Got exception when trying to report backup now result to FabricBRS for operation ID {1}",
                            this.GetTraceId(), this.backupNowOperationId);

                        await Task.Delay(InitialRetryInterval, cancellationToken);
                    }
                } while (true);
                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup now operation completed for operation ID: {1}",
                        this.GetTraceId(), this.backupNowOperationId);
            }
            finally
            {
                lock (this.backupNowRequestLock)
                {
                    this.backupNowInProgress = false;
                    this.backupNowOperationId = Guid.Empty;
                    this.backupNowConfiguration = null;
                    this.backupNowTask = null;
                    this.backupNowCts = null;
                }
            }
        }

        private async Task CancelInitializeAsync()
        {
            if (this.initializeAsyncCts != null &&
                this.initializeAsyncCts.IsCancellationRequested == false)
            {
                this.initializeAsyncCts.Cancel();
                try
                {
                    if (this.initializeAsyncTask != null)
                    {
                        await this.initializeAsyncTask;
                    }
                }
                catch (OperationCanceledException)
                {
                    // Do nothing
                }
                catch (Exception ex)
                {
                    AppTrace.TraceSource.WriteError(
                        TraceType,
                        "{0} CancelInitializeAsync Error encountered {1}",
                        this.GetTraceId(),
                        ex);
                }
                finally
                {
                    this.initializeAsyncCts = null;
                    this.initializeAsyncTask = null;
                }
            }
        }

        private async Task CancelBackupNowTaskAsync()
        {
            if (this.backupNowCts != null &&
                this.backupNowCts.IsCancellationRequested == false)
            {
                this.backupNowCts.Cancel();
                try
                {
                    if (this.backupNowTask != null)
                    {
                        await this.backupNowTask;
                    }
                }
                catch (OperationCanceledException)
                {
                    // Do nothing
                }
                catch (Exception ex)
                {
                    AppTrace.TraceSource.WriteError(
                        TraceType,
                        "{0} CancelBackupNowTaskAsync Error encountered {1}",
                        this.GetTraceId(),
                        ex);
                }
                finally
                {
                    this.backupNowCts = null;
                    this.backupNowTask = null;
                }
            }
        }

        private async Task InitializeAsync(CancellationToken cancellationToken)
        {
            // Check if FabricBRS is configured or not. If not, return directly
            if (!this.IsBackupRestoreServiceConfigured()) { return; }

            var policyFoundLocally = false;

            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    AppTrace.TraceSource.WriteInfo(TraceType, "{0} Waiting for write status", this.GetTraceId());
                    var writeStatusGranted = await this.WaitForWriteStatusAsync(cancellationToken);

                    if (!writeStatusGranted)
                    {
                        AppTrace.TraceSource.WriteWarning(TraceType, "{0} Unable to acquire write status.", this.GetTraceId());
                        return;
                    }

                    byte[] backupMetadata;

                    try
                    {
                        // Check if policy exists or not
                        backupMetadata = await this.replica.GetBackupMetadataAsync(this.apiTimeout, cancellationToken);
                    }
                    catch (Exception ex)
                    {
                        AppTrace.TraceSource.WriteInfo(TraceType,
                            "{0} Fetching policy from local store returned the following error {1}", this.GetTraceId(), ex.ToString());
                        backupMetadata = null;
                    }

                    if (null != backupMetadata)
                    {
                        AppTrace.TraceSource.WriteInfo(TraceType, "{0} Policy found in local store", this.GetTraceId());
                        var tempMetadata = BackupMetadata.Deserialize(backupMetadata);

                        // Check if the policy is for this partition ID or not if not, ignore it
                        if (tempMetadata.PartitionId.Equals(this.partition.PartitionInfo.Id))
                        {
                            if (tempMetadata.Policy == null)
                            {
                                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Null policy found. Backup not enabled for this partition.", this.GetTraceId());
                                this.backupMetadataObj = null;
                                break;
                            }
                            else
                            {
                                policyFoundLocally = true;
                                this.backupMetadataObj = tempMetadata;
                            }
                        }
                        else
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Policy found not applicable for current partition, clearing it.", this.GetTraceId());
                            await this.replica.ClearBackupMetadataAsync(this.apiTimeout);
                            this.backupMetadataObj = null;
                        }
                    }

                    if (null == this.backupMetadataObj)
                    {
                        BackupMetadata tempMetadata;
                        try
                        {
                            // fetch the policy from Backup Service here. 
                            var policy = await this.GetBackupSchedulingPolicyAsyncHelper(this.apiTimeout, cancellationToken).ConfigureAwait(false);

                            tempMetadata = new BackupMetadata
                            {
                                PartitionId = this.partition.PartitionInfo.Id,
                                Policy = policy,
                                PolicyUpdateTime = DateTime.UtcNow
                            };
                        }
                        catch (FabricPeriodicBackupNotEnabledException)
                        {
                            // Backup protection is not enabled for this partition, return
                            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup protection not enabled for this partition", this.GetTraceId());

                            tempMetadata = new BackupMetadata
                            {
                                PartitionId = this.partition.PartitionInfo.Id,
                                Policy = null,                  // Indicates dummy policy
                                PolicyUpdateTime = DateTime.UtcNow
                            };
                        }

                        // Save the metadata here
                        await this.replica.SaveBackupMetadataAsync(tempMetadata.Serialize(), this.apiTimeout, cancellationToken);

                        if (tempMetadata.Policy != null)
                        {
                            this.backupMetadataObj = tempMetadata;
                        }
                        else
                        {
                            break;
                        }
                    }

                    this.backupScheduler = new BackupScheduler(this.backupMetadataObj, this.TimerCallback);
                    this.backupScheduler.ArmTimer(policyFoundLocally);
                    break;
                }
                catch (OperationCanceledException ex)
                {
                    if (cancellationToken.IsCancellationRequested)
                    {
                        break;
                    }

                    AppTrace.TraceSource.WriteError(TraceType, "{0} Error encountered in BackupRestoreWorker InitializeAsync {1}", this.GetTraceId(), ex.ToString());

                }
                catch (Exception ex)
                {
                    AppTrace.TraceSource.WriteError(TraceType, "{0} Error encountered in BackupRestoreWorker InitializeAsync {1}", this.GetTraceId(), ex.ToString());
                }

                await Task.Delay(InitialRetryInterval, cancellationToken);
            }
        }

        private void RegisterBackupRestoreReplica(Guid partitionId, long replicaId, IBackupRestoreHandler handler)
        {
            Requires.Argument("partitionId", partitionId).NotNullOrEmpty();
            Requires.Argument("replicaId", replicaId).NotNullOrEmpty();
            Requires.Argument("handler", handler).NotNull();

            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Registering replica {1} with BAP", this.GetTraceId(), this.initializationParameters.ReplicaId);
            Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterBackupRestoreReplicaHelper(partitionId, replicaId, handler), "BackupRestoreWorker.RegisterBackupRestoreReplica");
        }

        private void UnregisterBackupRestoreReplica(Guid partitionId, long replicaId)
        {
            Requires.Argument("partitionId", partitionId).NotNullOrEmpty();
            Requires.Argument("replicaId", replicaId).NotNullOrEmpty();

            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Unregistering replica {1} with BAP", this.GetTraceId(), this.initializationParameters.ReplicaId);
            Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterBackupRestoreReplicaHelper(partitionId, replicaId), "BackupRestoreWorker.UnregisterBackupRestoreReplica");
        }

        private Task<RestorePointDetails> GetRecoveryPointDetailsAsynHelper(CancellationToken cancellationToken)
        {
            var timeout = this.apiTimeout;

            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Checking with FabricBRS if user triggered a restore for partition and fetching the recovery point details", this.GetTraceId(), this.partition.PartitionInfo.Id);
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.GetRecoveryPointDetailsBeginWrapper(timeout, callback),
                this.GetRecoveryPointDetailsEndWrapper,
                cancellationToken,
                "BackupRestoreWorker.GetRecoveryPointDetailsAsync");
        }

        private Task<BackupPolicy> GetBackupSchedulingPolicyAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Fetch backup schedule policy from backup service", this.GetTraceId());
            
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.GetBackupSchedulingPolicyBeginWrapper(timeout, callback),
                this.GetBackupSchedulingPolicyEndWrapper,
                InteropExceptionTracePolicy.Info,
                cancellationToken,
                "BackupRestoreWorker.GetBackupSchedulingPolicyAsync");
        }

        private Task ReportBackupOperationResultAsyncHelper(BackupOperationResult result, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Report backup operation result, operation id {1}", this.GetTraceId(), result.OperationId);

            var timeout = this.apiTimeout;
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.ReportBackupOperationResultBeginWrapper(result, timeout, callback),
                this.ReportBackupOperationResultEndWrapper,
                cancellationToken,
                "BackupRestoreWorker.ReportBackupOperationResultAsync");
        }

        private Task ReportRestoreOperationResultAsyncHelper(RestoreOperationResult result, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Report restore operation result, operation id {1}", this.GetTraceId(), result.OperationId);

            var timeout = this.apiTimeout;
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.ReportRestoreOperationResultBeginWrapper(result, timeout, callback),
                this.ReportRestoreOperationResultEndWrapper,
                cancellationToken,
                "BackupRestoreWorker.ReportRestoreOperationResultAsync");
        }

        private Task UploadBackupAsyncHelper(UploadBackupInfo uploadInfo, BackupStoreInformation storeInformation,
            CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Upload backup", this.GetTraceId());

            var timeout = this.storeApiTimeout;
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.UploadBackupBeginWrapper(uploadInfo, storeInformation, timeout, callback),
                this.UploadBackupEndWrapper,
                cancellationToken,
                "BackupRestoreWorker.UploadBackupAsync");
        }

        private Task DownloadBackupAsyncHelper(DownloadBackupInfo downloadInfo, BackupStoreInformation storeInformation,
            CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Download backup", this.GetTraceId());

            var timeout = this.storeApiTimeout;
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.DownloadBackupBeginWrapper(downloadInfo, storeInformation, timeout, callback),
                this.DownloadBackupEndWrapper,
                cancellationToken,
                "BackupRestoreWorker.DownloadBackupAsync");
        }

        private async Task<bool> WaitForWriteStatusAsync(CancellationToken cancellationToken)
        {
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();

                PartitionAccessStatus result;
                try
                {
                    result = this.partition.WriteStatus;
                }
                catch (FabricObjectClosedException)
                {
                    // Normal case, replica is closing, so no need to do anything further
                    return false;
                }
                catch (Exception e)
                {
                    AppTrace.TraceSource.WriteError(
                        TraceType,
                        "{0} ServicePartition.WriteStatus threw an unexpected exception: {1}",
                        this.GetTraceId(),
                        e.ToString());
                    this.partition.ReportFault(FaultType.Transient);
                    return false;
                }

                if (result == PartitionAccessStatus.Granted)
                {
                    return true;
                }
                else if (result == PartitionAccessStatus.NotPrimary)
                {
                    return false;
                }

                await Task.Delay(PrimaryStatusCheckRetryIntervalInMillis, cancellationToken);
            }
        }

        private void EnsureReplicaIsPrimary()
        {
            if (this.replicaRole != ReplicaRole.Primary)
            {
                throw new FabricNotPrimaryException();
            }
        }

        private bool IsBackupRestoreServiceConfigured()
        {
            if (!this.isBackupRestoreServiceConfigured.HasValue)
            {
                var configStore = NativeConfigStore.FabricGetConfigStore();
                var targetReplicaSetSizeString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.TargetReplicaSetSizeKeyName);
                int targetReplicaSetSize;
                if (!int.TryParse(targetReplicaSetSizeString, out targetReplicaSetSize))
                {
                    targetReplicaSetSize = 0;
                }

                this.isBackupRestoreServiceConfigured = targetReplicaSetSize != 0;
            }

            return this.isBackupRestoreServiceConfigured.Value;
        }

        private void Cleanup()
        {
            if (this.replicaRegisteredWithBap)
            {
                this.UnregisterBackupRestoreReplica(this.partition.PartitionInfo.Id, this.initializationParameters.ReplicaId);
                this.replicaRegisteredWithBap = false;
            }

            this.backupMetadataObj = null;
            
            // Stop the timer
            if (null != this.backupScheduler)
            {
                this.backupScheduler.Stop();
                this.backupScheduler = null;
            }

            lock (this.backupNowRequestLock)
            {
                this.backupNowConfiguration = null;
                this.backupPropertiesForBackupNow = null;
                this.backupNowInProgress = false;
                this.backupNowOperationId = Guid.Empty;
            }
        }

#region Health Reporting Functions

        private void ClearBackupHealth()
        {
            var msg = SR.Clear_Backup_Health;

            var healthInfo = new HealthInformation(BackupRestoreContants.HealthSourceId, BackupRestoreContants.HealthBackupProperty, HealthState.Ok)
            {
                TimeToLive = TimeSpan.FromSeconds(1),
                RemoveWhenExpired = true,
                Description = msg,
            };

            this.partition.ReportPartitionHealth(healthInfo);
        }

        private void ReportBackupSuccessHealth()
        {
            var msg = String.Format(SR.Success_Backup_HealthMsg, DateTime.UtcNow);

            var healthInfo = new HealthInformation(BackupRestoreContants.HealthSourceId, BackupRestoreContants.HealthBackupProperty, HealthState.Ok)
            {
                TimeToLive = TimeSpan.MaxValue,
                Description = msg,
            };

            this.partition.ReportPartitionHealth(healthInfo);
        }

        private void ReportBackupFailureHealth(Exception ex)
        {
            var msg = String.Format(SR.Error_BackupFailure_HealthMsg, ex);

            var healthInfo = new HealthInformation(BackupRestoreContants.HealthSourceId, BackupRestoreContants.HealthBackupProperty, HealthState.Warning)
            {
                TimeToLive = TimeSpan.MaxValue,
                Description = BackupRestoreUtility.TrimToLength(msg, BackupRestoreContants.MaxHealthDescriptionLength),
            };

            this.partition.ReportPartitionHealth(healthInfo);
        }

        private void ReportRestoreSuccessHealth(bool userInitiatedOperation)
        {
            var msg = String.Format(userInitiatedOperation ? SR.Success_Restore_UserInitiated_HealthMsg : SR.Success_Restore_DataLoss_HealthMsg, DateTime.UtcNow);

            var healthInfo = new HealthInformation(BackupRestoreContants.HealthSourceId, BackupRestoreContants.HealthRestoreProperty, HealthState.Ok)
            {
                TimeToLive = userInitiatedOperation ? BackupRestoreContants.RestoreHealthInformationTimeToLive : TimeSpan.MaxValue,
                RemoveWhenExpired = true,
                Description = msg,
            };

            this.partition.ReportPartitionHealth(healthInfo);
        }

        private void ReportRestoreFailureHealth(Exception ex)
        {
            var msg = String.Format(SR.Error_RestoreFailure_HealthMsg, ex);

            var healthInfo = new HealthInformation(BackupRestoreContants.HealthSourceId, BackupRestoreContants.HealthRestoreProperty, HealthState.Error)
            {
                TimeToLive = BackupRestoreContants.HealthInformationDeltaTimeToLive,
                Description = BackupRestoreUtility.TrimToLength(msg, BackupRestoreContants.MaxHealthDescriptionLength),
            };

            this.partition.ReportPartitionHealth(healthInfo);
        }

#endregion

#region Backup Timer Operations

        private void TimerCallback()
        {
            this.EnsureReplicaIsPrimary();

            var tempMetadataObj = this.backupMetadataObj;
            if (null == tempMetadataObj) return;            // Backup disabled

            var backupOption = (tempMetadataObj.NumberOfBackupsInChain > tempMetadataObj.Policy.MaxIncrementalBackups) || tempMetadataObj.NumberOfBackupsInChain == 0
                ? BackupOption.Full
                : BackupOption.Incremental;

            // Ensure no parallel backup operation is ongoing
            this.backupSemaphore.Wait();

            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup timer fired: Triggering {1} backup", this.GetTraceId(), backupOption);
            this.telemetryStopWatch.Restart();
            try
            {
                this.backupCts = new CancellationTokenSource();
                this.replica.BackupAsync(backupOption, this.BackupCallback, this.backupCts.Token).ContinueWith(this.PostBackupContinuationTask);

            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteExceptionAsError(TraceType, ex, "{0} BackupAsync returned error", this.GetTraceId());

                // Rearm the timer
                this.backupSemaphore.Release();
                this.backupScheduler.RearmTimer(true);
            }
        }

        private void PostBackupContinuationTask(Task previousTask)
        {
            bool? reschedule = null;

            // Release the semaphore
            this.backupSemaphore.Release();

            if (previousTask.IsFaulted && previousTask.Exception != null)
            {
                if (previousTask.Exception.InnerException is FabricMissingFullBackupException ||
                    previousTask.Exception.InnerException is FabricException && ((FabricException) previousTask.Exception.InnerException).ErrorCode == FabricErrorCode.InvalidBackupSetting)
                {
                    AppTrace.TraceSource.WriteInfo(TraceType, "{0} Received FabricMissingFullBackupException or FabricException with InvalidBackupSetting while backing up partition. Triggering a full backup now", this.GetTraceId());

                    // Acquire backup lock
                    this.backupSemaphore.Wait();
                    this.backupCts = new CancellationTokenSource();
                    this.replica.BackupAsync(BackupOption.Full, this.BackupCallback, this.backupCts.Token).ContinueWith(this.PostBackupContinuationTask);
                    return;
                }
                else if (previousTask.Exception.InnerException is FabricNotPrimaryException)
                {
                    // This replica is no longer the primary. Stop processing
                    AppTrace.TraceSource.WriteWarning(TraceType, "{0}, Replica no longer primary, received FabricNotPrimaryException, cleaning up", this.GetTraceId());
                    this.Cleanup();
                }
                else
                {
                    AppTrace.TraceSource.WriteWarning(TraceType, "{0} Got exception when trying to schedule backup: {1}", this.GetTraceId(), previousTask.Exception.ToString());
                    reschedule = true;

                    // Just retry in case we receive FabricNotReadableException and dont report health for the same
                    if (!(previousTask.Exception.InnerException is FabricNotReadableException))
                    {
                        this.ReportBackupFailureHealth(previousTask.Exception.InnerException);
                    }
                }
            }
            else
            {
                this.ReportBackupSuccessHealth();
                reschedule = false;
            }

            if (reschedule.HasValue)
            {
                this.backupScheduler.RearmTimer(reschedule.Value);
            }
        }

        private async Task<RecoveryPointMetadataProperties> BackupCallbackCommon(BackupInfo backupInfo, BackupStoreInformation storeInformation, string parentBackupLocation, Guid parentBackupId, Guid backupChainId, CancellationToken cancellationToken)
        {
            var backupTime = DateTime.UtcNow;
            var folderName = backupTime.ToString(BackupFolderNameFormat);
            var newBackupLocation = Path.Combine(this.GetBackupRelativePath(), this.zipBackups ? folderName + ZipExtension : folderName);
            var tempDirectory = this.GetRootBrsTempFolderPath(true);
            var recoveryMetadataFile = Path.Combine(tempDirectory, folderName + MetadataFileNameExtension);
            TimeSpan localBackupTime = this.telemetryStopWatch.Elapsed;
            
            try
            {
                if (!Directory.Exists(tempDirectory))
                {
                    Directory.CreateDirectory(tempDirectory);
                }

                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Creating recovery point metadata file {1}",
                    this.GetTraceId(), recoveryMetadataFile);

                string serviceManifestVersion = this.initializationParameters.CodePackageActivationContext.GetServiceManifestVersion();

                // Write the recovery point metadata file
                var file = await RecoveryPointMetadataFile.CreateAsync(
                    recoveryMetadataFile,
                    backupTime,
                    backupInfo.BackupId,
                    parentBackupId,
                    backupChainId,
                    backupInfo.LastBackupVersion.Epoch,
                    backupInfo.LastBackupVersion.Lsn,
                    newBackupLocation,
                    parentBackupLocation,
                    this.partition.PartitionInfo,
                    serviceManifestVersion,
                    cancellationToken);

                cancellationToken.ThrowIfCancellationRequested();

                UploadBackupInfo uploadInfo;

                long backupSize = (Directory.GetFiles(
                    backupInfo.Directory, "*", SearchOption.AllDirectories).Sum(t => (new FileInfo(t).Length)))/1000;

                long zipSize = 0L;
                if (this.zipBackups)
                {
                    var zipFileName = Path.Combine(tempDirectory, folderName + ZipExtension);
                    ZipFile.CreateFromDirectory(backupInfo.Directory, zipFileName, CompressionLevel.Fastest, false);

                    uploadInfo = new UploadBackupInfo(recoveryMetadataFile, zipFileName);

                    zipSize = (new FileInfo(zipFileName).Length)/1000;
                }
                else
                {
                    uploadInfo = new UploadBackupInfo(recoveryMetadataFile, backupInfo.Directory, folderName);
                }
                
                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Uploading backup to store", this.GetTraceId());
                await this.UploadBackupAsyncHelper(uploadInfo, storeInformation, cancellationToken);
                TimeSpan uploadBackupTime = this.telemetryStopWatch.Elapsed.Subtract(localBackupTime);

                var recoveryPointMetadataProps = new RecoveryPointMetadataProperties
                {
                    BackupTime = backupTime,
                    BackupId = file.BackupId,
                    BackupLocation = file.BackupLocation,
                    EpochOfLastBackupRecord = file.EpochOfLastBackupRecord,
                    LsnOfLastBackupRecord = file.LsnOfLastBackupRecord,
                };

#if !DotNetCoreClr
                TimeSpan totalTime = this.telemetryStopWatch.Elapsed;
                if (backupInfo.Option == BackupOption.Full)
                {
                    FabricEvents.Events.FullBackupDetails(backupSize, zipSize, localBackupTime.TotalSeconds,
                        uploadBackupTime.TotalSeconds, totalTime.TotalSeconds, this.partition.PartitionInfo.Id.ToString());
                }else
                {
                    FabricEvents.Events.IncrementalBackupDetails(backupSize, zipSize, localBackupTime.TotalSeconds,
                        uploadBackupTime.TotalSeconds, totalTime.TotalSeconds, this.partition.PartitionInfo.Id.ToString());
                }
#endif

                return recoveryPointMetadataProps;

            }
            finally
            {
                if (Directory.Exists(tempDirectory))
                {
                    Directory.Delete(tempDirectory, true);
                }
            }
        }

        private async Task<bool> BackupCallback(BackupInfo backupInfo, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup callback fired", this.GetTraceId());

            this.EnsureReplicaIsPrimary();

            await metadataSyncSemaphore.WaitAsync();
            var tempMetadataObj = (BackupMetadata)this.backupMetadataObj?.Clone();
            metadataSyncSemaphore.Release();

            // If tempMetadataObj is null, backup is no longer configured, let's return from here
            if (null == tempMetadataObj)
            {
                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Returning from callback, backup disabled", this.GetTraceId());
                return false;
            }

            try
            {
                string parentBackupLocation = null;
                var parentBackupId = Guid.Empty;
                var backupChainId = backupInfo.BackupId;

                if (backupInfo.Option == BackupOption.Incremental)
                {
                    var backupChainBroken = false;
                    if (backupInfo.ParentBackupId != Guid.Empty)
                    {
                        if (tempMetadataObj.LastRecoveryPoint.BackupId != backupInfo.ParentBackupId)
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType,
                            "{0} Backup chain seems to be broken. Current Parent Backup ID {1}, Previous Backup ID {2}", 
                                this.GetTraceId(), 
                                backupInfo.ParentBackupId, 
                                tempMetadataObj.LastRecoveryPoint.BackupId);

                            backupChainBroken = true;
                        }
                    }
                    else
                    {
                        if (tempMetadataObj.LastRecoveryPoint.BackupChainId != backupInfo.BackupChainId ||
                            (tempMetadataObj.LastRecoveryPoint.BackupIndex + 1) != backupInfo.BackupIndex)
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup chain seems to be broken. Current Backup Chain {1}, Previous Backup Chain {2}, Current Backup Indes {3}, Previous Backup Index {4}",
                                this.GetTraceId(),
                                backupInfo.BackupChainId,
                                tempMetadataObj.LastRecoveryPoint.BackupChainId,
                                backupInfo.BackupIndex,
                                tempMetadataObj.LastRecoveryPoint.BackupIndex);

                            backupChainBroken = true;
                        }
                    }

                    if (backupChainBroken)
                    {
                        // This indicates, there is a miss somewhere in the chain, abort this backup and take again
                        // Throwing a fabric missing full backup exception will trigger a full backup
                        throw new FabricMissingFullBackupException();
                    }

                    parentBackupLocation = tempMetadataObj.LastRecoveryPoint.Location;
                    parentBackupId = tempMetadataObj.LastRecoveryPoint.BackupId;
                    backupChainId = tempMetadataObj.LastRecoveryPoint.BackupChainId;
                }

                var metadataProps =
                    await this.BackupCallbackCommon(backupInfo, tempMetadataObj.Policy.StoreInformation, parentBackupLocation,
                            parentBackupId, backupChainId, cancellationToken);

                if (backupInfo.Option == BackupOption.Incremental)
                {
                    tempMetadataObj.NumberOfBackupsInChain++;
                }
                else
                {
                    tempMetadataObj.NumberOfBackupsInChain = 1;
                }

                // Add the jitter time used to the backup time to ensure the next backup used is not the same one
                // In case the jitter is negative, the calculation of next backup time immediately could point to something 
                // again around the jitter time. Also adding a buffer of 1 second below
                var lastRecoveryPointTime = DateTime.UtcNow;                                // Assuming the current time as the time when backup upload and everything completed.
                if (tempMetadataObj.JitterInMillisUsedInLastBackup < 0)
                {
                    lastRecoveryPointTime =
                        lastRecoveryPointTime.AddMilliseconds(Math.Abs(tempMetadataObj.JitterInMillisUsedInLastBackup) +
                                                              1000);
                }

                tempMetadataObj.JitterInMillisUsedInLastBackup = 0;

                tempMetadataObj.LastRecoveryPoint = new RecoveryPointInformation
                {
                    BackupTime = lastRecoveryPointTime,
                    Location = metadataProps.BackupLocation,
                    BackupId = backupInfo.BackupId,
                    BackupChainId = backupChainId,
                    BackupIndex = backupInfo.BackupIndex,
                };

                AppTrace.TraceSource.WriteInfo(TraceType, "{0} Saving backup metadata", this.GetTraceId());

                await this.metadataSyncSemaphore.WaitAsync();

                try
                {
                    if (this.backupMetadataObj == null)
                    {
                        // Seems like backup is disabled, discard this backup.
                        return false;
                    }

                    if (this.backupMetadataObj.PolicyUpdateTime != tempMetadataObj.PolicyUpdateTime)
                    {
                        // Policy has been updated, discard this backup. Return false;
                        return false;
                    }
                    
                    await this.replica.SaveBackupMetadataAsync(tempMetadataObj.Serialize(), this.apiTimeout, cancellationToken);
                    this.backupMetadataObj.CopyFrom(tempMetadataObj);
                    // Never reassign the value directly as the reference of this variable is used inside the timer classes
                }
                finally
                {
                    this.metadataSyncSemaphore.Release();
                }

                return true;
            }
            catch (FabricMissingFullBackupException)
            {
                throw;
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteError(TraceType, "{0} Error encountered in backup callback {1}", this.GetTraceId(), ex.ToString());
                throw;
            }
        }

        private async Task<bool> BackupNowCallback(BackupInfo backupInfo, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteInfo(TraceType, "{0} Backup now callback fired", this.GetTraceId());

            this.EnsureReplicaIsPrimary();

            try
            {
                var metadataProps =
                    await this.BackupCallbackCommon(backupInfo, this.backupNowConfiguration.StoreInformation, null,
                            Guid.Empty, backupInfo.BackupId, cancellationToken);

                await this.metadataSyncSemaphore.WaitAsync();

                try
                {
                    if (null != this.backupMetadataObj)
                    {
                        // Reset number of backups in chain to ensure next backup taken is a full one
                        this.backupMetadataObj.NumberOfBackupsInChain = 0;

                        AppTrace.TraceSource.WriteInfo(TraceType, "{0} Saving backup metadata", this.GetTraceId());

                        await this.replica.SaveBackupMetadataAsync(this.backupMetadataObj.Serialize(), this.apiTimeout, cancellationToken);
                    }
                }
                finally
                {
                    this.metadataSyncSemaphore.Release();
                }

                this.backupPropertiesForBackupNow = metadataProps;
                return true;
            }
            catch (Exception ex)
            {
                AppTrace.TraceSource.WriteError(TraceType, "{0} Error encountered in backup now callback {1}", this.GetTraceId(), ex.ToString());
                throw;
            }
        }

#endregion

#endregion

#region Interop Helpers

        private void CreateNativeAgent()
        {
            var iid = typeof(NativeBackupRestoreRuntime.IFabricBackupRestoreAgent).GUID;
            this.nativeAgent = NativeBackupRestoreRuntime.FabricCreateBackupRestoreAgent(ref iid);
        }

        private void RegisterBackupRestoreReplicaHelper(Guid partitionId, long replicaId, IBackupRestoreHandler handler)
        {
            BackupRestoreBroker broker = new BackupRestoreBroker(handler);

            this.nativeAgent.RegisterBackupRestoreReplica(
                partitionId,
                replicaId,
                broker);
        }

        private void UnregisterBackupRestoreReplicaHelper(Guid partitionId, long replicaId)
        {
            this.nativeAgent.UnregisterBackupRestoreReplica(
                partitionId,
                replicaId);
        }

        private NativeCommon.IFabricAsyncOperationContext GetBackupSchedulingPolicyBeginWrapper(
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");
            var partitionInfo = new BackupPartitionInfo
            {
                PartitionId = this.partition.PartitionInfo.Id,
                ServiceName = this.initializationParameters.ServiceName.ToString(),
            };

            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginGetBackupSchedulingPolicy(partitionInfo.ToNative(pin), timeoutMilliseconds, callback);
            }
        }

        private BackupPolicy GetBackupSchedulingPolicyEndWrapper(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var backupResult = this.nativeAgent.EndGetBackupSchedulingPolicy(context);
            var backupPolicyPtr = backupResult.get_BackupSchedulePolicy();
            return BackupPolicy.FromNative(backupPolicyPtr);
        }

        private NativeCommon.IFabricAsyncOperationContext GetRecoveryPointDetailsBeginWrapper(
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");
            var partitionInfo = new BackupPartitionInfo
            {
                PartitionId = this.partition.PartitionInfo.Id,
                ServiceName = this.initializationParameters.ServiceName.ToString(),
            };

            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginGetRestorePointDetails(partitionInfo.ToNative(pin), timeoutMilliseconds, callback);
            }
        }

        private RestorePointDetails GetRecoveryPointDetailsEndWrapper(
            NativeCommon.IFabricAsyncOperationContext context)
        {
            var recoveryPointDetails = this.nativeAgent.EndGetRestorePointDetails(context);
            var recoveryPointDetailsPtr = recoveryPointDetails.get_RestorePointDetails();
            return RestorePointDetails.FromNative(recoveryPointDetailsPtr);
        }

        private NativeCommon.IFabricAsyncOperationContext ReportBackupOperationResultBeginWrapper(BackupOperationResult result, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");

            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginReportBackupOperationResult(result.ToNative(pin), timeoutMilliseconds, callback);
            }
        }

        private void ReportBackupOperationResultEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeAgent.EndReportBackupOperationResult(context);
        }

        private NativeCommon.IFabricAsyncOperationContext ReportRestoreOperationResultBeginWrapper(RestoreOperationResult result, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");
            
            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginReportRestoreOperationResult(result.ToNative(pin), timeoutMilliseconds, callback);
            }
        }

        private void ReportRestoreOperationResultEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeAgent.EndReportRestoreOperationResult(context);
        }

        private NativeCommon.IFabricAsyncOperationContext UploadBackupBeginWrapper(UploadBackupInfo uploadInfo, BackupStoreInformation storeInformation, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");

            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginUploadBackup(uploadInfo.ToNative(pin), storeInformation.ToNative(pin), timeoutMilliseconds, callback);
            }
        }

        private void UploadBackupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeAgent.EndUploadBackup(context);
        }

        private NativeCommon.IFabricAsyncOperationContext DownloadBackupBeginWrapper(DownloadBackupInfo downloadInfo, BackupStoreInformation storeInformation, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            var timeoutMilliseconds = Utility.ToMilliseconds(timeout, "timeout");

            using (var pin = new PinCollection())
            {
                return this.nativeAgent.BeginDownloadBackup(downloadInfo.ToNative(pin), storeInformation.ToNative(pin), timeoutMilliseconds, callback);
            }
        }

        private void DownloadBackupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeAgent.EndDownlaodBackup(context);
        }

#endregion
    }
}