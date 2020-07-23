// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using BackupMappingModel = System.Fabric.BackupRestore.Common.Model.BackupMapping;
    using BackupStorageModel = System.Fabric.BackupRestore.Common.Model.BackupStorage;
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Linq;
    using System.Threading;
    using System.Text;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;

    internal abstract class RestorePointOperation<TResult> : BaseOperation<TResult>
    {
        protected internal BackupPolicyStore BackupPolicyStore;
        protected internal BackupMappingStore BackupMappingStore;
        protected internal StatefulService StatefulService;
        protected internal string FabricUri = null;
        protected internal string ApplicationName = null;
        protected internal string ServiceName = null;
        protected internal string PartitionId = null;
        protected internal FabricBackupResourceType FabricBackupResourceType;
        protected internal string FabricRequestHeader = null;

        protected internal RestorePointOperation(string fabricRequestHeader, string apiVersion, StatefulService statefulService) : base(apiVersion)
        {
            this.StatefulService = statefulService;
            this.FabricRequestHeader = fabricRequestHeader;
        }

        internal async override Task InitializeAsync()
        {
            this.BackupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService).ConfigureAwait(false);
            this.BackupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.StatefulService).ConfigureAwait(false);
        }

        protected internal RestorePointOperation( string fabricRequestHeader,string serviceName, string apiVersion, StatefulService statefulService) : base(apiVersion)
        {
            this.StatefulService = statefulService;
            this.FabricRequestHeader = fabricRequestHeader;
        }

        protected internal async Task PopulateApplicationServiceAndPartition(TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.FabricUri = await UtilityHelper.GetFabricUriFromRequstHeader(this.FabricRequestHeader, timeout, cancellationToken);
            await this.ValidateTheRequestFabricUri(this.FabricUri, timeout, cancellationToken);
            this.FabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionName(this.FabricUri, out this.ApplicationName,
                out this.ServiceName, out this.PartitionId);

        }

        protected internal void PopulateApplicationServiceAndPartitionWithBackupEntity(BackupEntity backupEntity,TimeSpan timeout, CancellationToken cancellationToken)
        {
            switch (backupEntity.EntityKind)
            {
                case BackupEntityKind.Application:
                    ApplicationBackupEntity applicationBackupEntity = (ApplicationBackupEntity) backupEntity;
                    this.FabricUri = applicationBackupEntity.ApplicationName;
                    break;
                case BackupEntityKind.Service:
                    ServiceBackupEntity serviceBackupEntity = (ServiceBackupEntity)backupEntity;
                    this.FabricUri = serviceBackupEntity.ServiceName;
                    break;
                case BackupEntityKind.Partition:
                    PartitionBackupEntity partitionBackupEntity = (PartitionBackupEntity)backupEntity;
                    this.FabricUri = string.Format("{0}/{1}", partitionBackupEntity.ServiceName,
                        partitionBackupEntity.PartitionId);
                    break;
             
            }
            this.FabricBackupResourceType = UtilityHelper.GetApplicationAndServicePartitionName(this.FabricUri, out this.ApplicationName,
                out this.ServiceName, out this.PartitionId);

        }

        protected internal async Task ValidateTheRequestFabricUri(string fabricUri,TimeSpan timeout , CancellationToken cancellationToken)
        {
            string applicationNameUri = null;
            string serviceNameUri = null;
            string partitionId = null;
            FabricBackupResourceType fabricResourceType =
                UtilityHelper.GetApplicationAndServicePartitionUri(fabricUri, out applicationNameUri, out serviceNameUri, out partitionId);
            bool isInputValid = false;


            switch (fabricResourceType)
            {
                case FabricBackupResourceType.ApplicationUri:
                    isInputValid = await FabricClientHelper.ValidateApplicationUri(applicationNameUri, timeout, cancellationToken);
                    break;

                case FabricBackupResourceType.ServiceUri:
                    isInputValid = await FabricClientHelper.ValidateServiceUri(serviceNameUri, timeout, cancellationToken);
                    break;

                case FabricBackupResourceType.PartitionUri:
                    isInputValid = await FabricClientHelper.ValidateServiceUri(serviceNameUri, timeout, cancellationToken);
                    if (isInputValid)
                    {
                        isInputValid = await FabricClientHelper.ValidatePartition(serviceNameUri, partitionId, timeout, cancellationToken);
                    }
                    break;
                case FabricBackupResourceType.Error:
                    throw new ArgumentException("Invalid argument");
            }

            if (!isInputValid)
            {
                throw new ArgumentException("Invalid Arguments for Application / Service / Partitions Details.");
            }
        }

        protected internal async Task<List<BackupMappingModel>> GetBackupMappingsDetails(TimeSpan timeout, CancellationToken cancellationToken)
        {
            Task<List<BackupMappingModel>> serviceProtectionTask = null;
            Task<List<BackupMappingModel>> applicationProtectionTask = null;
            await this.PopulateApplicationServiceAndPartition(timeout, cancellationToken);

            if (this.FabricBackupResourceType  == FabricBackupResourceType.PartitionUri)
            {
                BackupMappingModel partitionBackupMapping = await this.BackupMappingStore.GetValueAsync(this.FabricUri,timeout ,cancellationToken);
                if (partitionBackupMapping != null)
                {
                    return  new List<BackupMappingModel> { partitionBackupMapping};
                }
                serviceProtectionTask = BackupMappingStore.GetProtectionForApplicationOrService(this.ApplicationName, this.ServiceName, timeout, cancellationToken);
                applicationProtectionTask = BackupMappingStore.GetProtectionForApplicationOrService(this.ApplicationName, null, timeout,cancellationToken);

                return await serviceProtectionTask ?? await applicationProtectionTask;
            }
            else if (this.FabricBackupResourceType  == FabricBackupResourceType.ServiceUri)
            {
                serviceProtectionTask =
                    BackupMappingStore.GetAllProtectionForApplicationOrService(this.ApplicationName, this.ServiceName,timeout,cancellationToken);
                applicationProtectionTask = BackupMappingStore.GetProtectionForApplicationOrService(this.ApplicationName, null , timeout,cancellationToken);
                await Task.WhenAll(serviceProtectionTask, applicationProtectionTask);
                if (serviceProtectionTask.Result != null && applicationProtectionTask.Result != null)
                {
                    return serviceProtectionTask.Result.Concat(applicationProtectionTask.Result).ToList();
                }
                else
                {
                    return applicationProtectionTask.Result ?? serviceProtectionTask.Result;
                }
            }
            else if (this.ApplicationName != null)
            {
                return
                    await
                        BackupMappingStore.GetAllProtectionForApplicationOrService(this.ApplicationName, null, timeout,
                            cancellationToken);
            }
            else
            {
                throw new ArgumentException("Invalid Arguments.");
            }
        }

        protected internal async Task<List<BackupMappingModel>> GetBackupMappings(TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            List<BackupMappingModel> backupMappings = await this.GetBackupMappingsDetails(timeout, cancellationToken);
            if (backupMappings == null || backupMappings.Count == 0)
            {
                throw new FabricPeriodicBackupNotEnabledException();
            }
            return backupMappings;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Fabric Request Header : {0}", this.FabricRequestHeader).AppendLine();
            stringBuilder.AppendFormat("ApiVersion : {0}", this.ApiVersion).AppendLine();
            return base.ToString();
        }
    }
}