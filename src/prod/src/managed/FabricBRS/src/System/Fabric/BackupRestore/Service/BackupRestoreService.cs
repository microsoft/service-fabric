// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using Microsoft.ServiceFabric.Data;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Communication.Runtime;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Security;
    using System.Fabric.Common.Tracing;
    using System.Linq;

    public class BackupRestoreService : StatefulService
    {
        /// <summary>
        /// Trace type string used by traces of this service.
        /// </summary>
        private const string TraceType = "Service";

        private BackupRestoreServiceImpl backupRestroRestoreServiceImpl;

        private readonly ClusterEndpointSecuritySettingsChangeNotifier clusterEndpointSecuritySettingsChangeNotifier;

        private Task updateSecretsTask;

        private CancellationTokenSource updateSecretsCancellationTokenSource;

        private NativeConfigStore configStore;

        private NativeConfigStore encryptionCertConfigStore;

        private RetentionManager retentionManager;

        /// <summary>
        /// An instance of this class is created for each service replica by the Service Fabric runtime.
        /// </summary>
        internal BackupRestoreService(StatefulServiceContext serviceContext, string endpoint)
            : base(
            serviceContext,
            new ReliableStateManager(
                serviceContext,
                new ReliableStateManagerConfiguration(
                    GetReplicatorSettings(endpoint))))
        {
            this.clusterEndpointSecuritySettingsChangeNotifier = new ClusterEndpointSecuritySettingsChangeNotifier(endpoint, this.UpdateReplicatorSettings);
        }

        /// <summary>
        /// Create communication listeners for the service
        /// </summary>
        /// <returns>A collection of listeners.</returns>
        protected override IEnumerable<ServiceReplicaListener> CreateServiceReplicaListeners()
        {
            return new[]
            {
              new ServiceReplicaListener(serviceContext => new OwinCommunicationListener(this, serviceContext) ,"RestEndpoint"),
              new ServiceReplicaListener(this.CreateNativeCommunicationListener)
            };
        }

        /// <summary>
        /// This is the main entry point for your service replica.
        /// This method executes when this replica of your service becomes primary and has write status.
        /// </summary>
        /// <param name="cancellationToken">Canceled when Service Fabric needs to shut down this service replica.</param>
        protected override async Task RunAsync(CancellationToken cancellationToken)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Enter RunAsync");
            FabricEvents.Events.BackupRestoreEnabled(this.Context.TraceId);

            // TODO: Ensure that the requests dont come in before we do the necessary initializations which are being done below
            await WorkItemHandler.StartAndScheduleWorkItemHandler(this);
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Registering config update handlers now");

            var configUpdateHandler = new ConfigUpdateHandler();
            configStore = NativeConfigStore.FabricGetConfigStore(configUpdateHandler);

            var certConfigHandler = new EncryptionCertConfigHandler(this.EncryptionCertUpdateCallbackAsync, this, cancellationToken);
            encryptionCertConfigStore = NativeConfigStore.FabricGetConfigStore(certConfigHandler);

            await certConfigHandler.InitializeAsync();

            Task<RetentionManager> retentionManagerTask = RetentionManager.CreateOrGetRetentionManager(this);
            
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Config update handlers registered");

            try
            {
                while (!configUpdateHandler.ProcessRecycleRequired)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    if(this.retentionManager == null && retentionManagerTask.IsCompleted )
                    {
                        this.retentionManager = retentionManagerTask.Result;
                    }
                    await Task.Delay(TimeSpan.FromSeconds(1), cancellationToken);

                }
                
                // Process recycle is required, throwing exception from RunAsync
                throw new Exception("Process recycle needed.");
            }
            finally
            {
                this.retentionManager?.StopRetentionManager();
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Exit RunAsync");
            }
        }

        protected override Task OnChangeRoleAsync(ReplicaRole newRole, CancellationToken cancellationToken)
        {
            if (newRole != ReplicaRole.Primary)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Stoping the WorkItem Handler");
                WorkItemHandler.StopWorkItemHandler();
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Stopped the WorkItem Handler");
            }
            return base.OnChangeRoleAsync(newRole, cancellationToken);
        }

        private async Task UpdateSecretsAsync(string thumbprintFromManifest, string x509StoreNameFromManifest, CancellationToken cancellationToken)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, String.Format("Updating secrets using the new thumbprint {0}", thumbprintFromManifest));
            using (var transaction = this.StateManager.CreateTransaction())
            {
                try
                {
                    var policyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this);
                    var workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(this);
                    var policies = await policyStore.GetValuesAsync(Common.Constants.StoreTimeOut, cancellationToken, transaction);

                    foreach (var policy in policies)
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                        var updatedStorage = this.UpdateSecretInStorage(policy.Storage, thumbprintFromManifest, x509StoreNameFromManifest, cancellationToken);
                        var newPolicy = policy.ToBuilder()
                                            .WithBackupStorage(updatedStorage)
                                            .UpdateUniqueId()
                                            .Build();

                        if (!await policyStore.UpdateValueAsync(policy.Name, policy, newPolicy, Common.Constants.StoreTimeOut, cancellationToken, transaction))
                        {
                            throw new InvalidOperationException(String.Format("Failed to update secret for policy with name {0}", newPolicy.Name));
                        }

                        if (newPolicy.BackupEnabledSet.Count > 0)
                        {
                            await workItemQueue.AddWorkItem(
                                    new UpdateEnablementWorkItem(newPolicy.BackupEnabledSet.ToList(),
                                        new WorkItemInfo
                                        {
                                            WorkItemType = WorkItemPropogationType.UpdateBackupPolicy,
                                            BackupPolicyUpdateGuid = newPolicy.UniqueId
                                        }), Common.Constants.StoreTimeOut, cancellationToken, transaction);
                        }
                    }

                    // Update the config store
                    var configStore = await ConfigStore.CreateOrGetConfigStore(this);
                    await configStore.UpdateValueAsync(Common.Constants.SecretUpdateInProgress, String.Empty, Common.Constants.StoreTimeOut, cancellationToken, transaction);
                    await configStore.UpdateValueAsync(Common.Constants.EncryptionCertThumbprintKey, thumbprintFromManifest, Common.Constants.StoreTimeOut, cancellationToken, transaction);
                    
                    await transaction.CommitAsync();
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "All secrets updated with manifest {0}", thumbprintFromManifest);
                    FabricEvents.Events.BRSSecretsUpdated(TraceType, thumbprintFromManifest);
                }
                catch (Exception ex)
                {
                    BackupRestoreTrace.TraceSource.WriteExceptionAsWarning(TraceType, ex, "Exception encountered while updating secrets");
                    throw;
                }
            }
        }

        private BackupStorage UpdateSecretInStorage(BackupStorage storage, string thumbprintFromManifest, string x509StoreNameFromManifest, CancellationToken cancellationToken)
        {
            if (storage.BackupStorageType == BackupStorageType.AzureBlobStore)
            {
                var azureStorage = (AzureBlobBackupStorageInfo) storage;
                if (!azureStorage.IsConnectionStringEncrypted && String.IsNullOrEmpty(thumbprintFromManifest)) return storage;
                if (azureStorage.IsConnectionStringEncrypted)
                {
                    return azureStorage.ToBuilder()
                        .WithConnectionString(UtilityHelper.ConvertToUnsecureString(EncryptionUtility.DecryptText(azureStorage.ConnectionString)))
                        .SetIsConnectionStringEncrypted(false)
                        .Build();
                }

                if (!String.IsNullOrEmpty(thumbprintFromManifest))
                {
                    return azureStorage.ToBuilder()
                        .WithConnectionString(EncryptionUtility.EncryptText(azureStorage.ConnectionString, thumbprintFromManifest, x509StoreNameFromManifest))
                        .SetIsConnectionStringEncrypted(true)
                        .Build();
                }

            }
            else if (storage.BackupStorageType == BackupStorageType.FileShare)
            {
                var fileShareStorage = (FileShareBackupStorageInfo)storage;
                if (!fileShareStorage.IsPasswordEncrypted && String.IsNullOrEmpty(thumbprintFromManifest)) return storage;
                if (fileShareStorage.IsPasswordEncrypted)
                {
                    var builder = fileShareStorage.ToBuilder()
                        .SetIsPasswordEncrypted(false);
                    if (!String.IsNullOrEmpty(fileShareStorage.PrimaryPassword))
                    { 
                        builder.WithPrimaryPassword(UtilityHelper.ConvertToUnsecureString(EncryptionUtility.DecryptText(fileShareStorage.PrimaryPassword)));
                    }

                    if (!String.IsNullOrEmpty(fileShareStorage.SecondaryPassword))
                    {
                        builder.WithSecondaryPassword(UtilityHelper.ConvertToUnsecureString(EncryptionUtility.DecryptText(fileShareStorage.SecondaryPassword)));
                    }

                    return builder.Build();
                }

                if (!String.IsNullOrEmpty(thumbprintFromManifest))
                {
                    var builder = fileShareStorage.ToBuilder()
                        .SetIsPasswordEncrypted(true);
                    if (!String.IsNullOrEmpty(fileShareStorage.PrimaryPassword))
                    { 
                        builder.WithPrimaryPassword(EncryptionUtility.EncryptText(fileShareStorage.PrimaryPassword, thumbprintFromManifest, x509StoreNameFromManifest));
                    }

                    if (!String.IsNullOrEmpty(fileShareStorage.SecondaryPassword))
                    {
                        builder.WithSecondaryPassword(EncryptionUtility.EncryptText(fileShareStorage.SecondaryPassword, thumbprintFromManifest, x509StoreNameFromManifest));
                    }

                    return builder.Build();
                }
            }
            else if (storage.BackupStorageType == BackupStorageType.DsmsAzureBlobStore)
            {
                // DsmsAzureBlobStorage doesn't contain any encrypted strings, return the original value.
                return storage;
            }

            throw new ArgumentException("Invalid storage type");
        }

        #region Private Methods

        private static ReliableStateManagerReplicatorSettings GetReplicatorSettings(string endpoint)
        {
            var securityCredentials = SecurityCredentials.LoadClusterSettings();
            var replicatorSettings = new ReliableStateManagerReplicatorSettings()
            {
                ReplicatorAddress = endpoint,
                SecurityCredentials = securityCredentials
            };

            return replicatorSettings;
        }

        private async void EncryptionCertUpdateCallbackAsync(string thumbprintFromManifest, string x509StoreFromManifest, CancellationToken cancellationToken)
        {
            if (this.updateSecretsTask != null && !this.updateSecretsTask.IsCompleted)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Previous update secret task is still running, waiting for it to cancel.");
                this.updateSecretsCancellationTokenSource.Cancel();
                while (!this.updateSecretsTask.IsCompleted)
                {
                    await Task.Delay(1000, cancellationToken);
                }
            }

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Starting a new udpate secret task.");

            // Start the secret update task
            this.updateSecretsCancellationTokenSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            this.updateSecretsTask = this.UpdateSecretsAsync(thumbprintFromManifest, x509StoreFromManifest, this.updateSecretsCancellationTokenSource.Token);
        }

        private void UpdateReplicatorSettings(string endpoint)
        {
            ReliableStateManagerReplicatorSettings replicatorSettings = GetReplicatorSettings(endpoint);

            var stateProviderReplica = ((ReliableStateManager)this.StateManager).Replica;
            var impl = (ReliableStateManagerImpl)stateProviderReplica;
            impl.Replicator.LoggingReplicator.UpdateReplicatorSettings(replicatorSettings);

            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Updated replicator settings");
        }

        /// <summary>
        /// Create Native communication listener. This is used for communication with Backup Agent
        /// </summary>
        /// <param name="serviceContext">Stateful service context instance</param>
        /// <returns>Returns native communication listener</returns>
        private ICommunicationListener CreateNativeCommunicationListener(ServiceContext serviceContext)
        {
            this.backupRestroRestoreServiceImpl = new BackupRestoreServiceImpl(this);
            var listener = new FabricBrsNativeCommunicationListener(serviceContext) { Impl = this.backupRestroRestoreServiceImpl };
            return listener;
        }

        #endregion
    }
}