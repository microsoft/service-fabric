// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Threading;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal class BackupMappingStore : BaseStore<string , BackupMapping>
    {
        private static BackupMappingStore Store;
        private const string BackupMappingStoreName = Constants.BackupMappingStoreName;
        private static WorkItemQueue workItemQueue;

        private const string TraceTypeBackupMappingStoreType = "BackupPolicyStore";

        private BackupMappingStore(IReliableDictionary<string, BackupMapping> reliableDictionary,
            StatefulService statefulService)
            : base(reliableDictionary, statefulService , TraceTypeBackupMappingStoreType)
        {
            
        }
        
        internal static async Task<BackupMappingStore> CreateOrGetBackupMappingStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteNoise(TraceTypeBackupMappingStoreType, "Creating a Backup Protection Store");
                IReliableDictionary<string, BackupMapping> reliableDictionary = 
                    await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, BackupMapping>>(BackupMappingStoreName);
                Store = new BackupMappingStore(reliableDictionary, statefulService);
                workItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(statefulService);
                BackupRestoreTrace.TraceSource.WriteNoise(TraceTypeBackupMappingStoreType, "Created a Backup Protection Store successfully");
            }
            return Store;
        }

        internal async Task<List<BackupMapping>> GetAllProtectionForApplicationOrService(string applicationName, string serviceName, 
            TimeSpan timeout , CancellationToken cancellationToken)
        {

            string baseKey = serviceName != null
                ? string.Format("fabric:/{0}/{1}", applicationName, serviceName)
                : string.Format("fabric:/{0}", applicationName);

            List <BackupMapping> backupList = new List<BackupMapping>();

            List<BackupMapping> allBackupMappings =  await this.GetValuesAsync(timeout, cancellationToken);

            foreach (var BackupMapping in allBackupMappings)
            {
                if (BackupMapping.ApplicationOrServiceUri.StartsWith(baseKey))
                {
                    backupList.Add(BackupMapping);
                }
            }
            return backupList.Count == 0 ? null : backupList;
        }

        internal async Task<List<BackupMapping>> GetProtectionForApplicationOrService(string applicationName, string serviceName,
    TimeSpan timeout, CancellationToken cancellationToken)
        {

            string baseKey = serviceName != null
                ? string.Format("fabric:/{0}/{1}", applicationName, serviceName)
                : string.Format("fabric:/{0}", applicationName);

            List<BackupMapping> backupList = new List<BackupMapping>();

            BackupMapping backupMapping = await this.GetValueAsync(baseKey, timeout, cancellationToken);
            if(backupMapping != null)
            {
                backupList.Add(backupMapping);
            }
            
            return backupList.Count == 0 ? null : backupList;
        }

        internal async Task<List<BackupMapping>> GetAllProtectionForServiceAndItsPartition(string serviceNameUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetAllProtectionWithFabricUri(serviceNameUri, timeout, cancellationToken, null);
        }

        internal async Task<List<BackupMapping>> GetAllProtectionForApplicationAndItsServiceAndPartition(string applicationNameUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetAllProtectionWithFabricUri(applicationNameUri, timeout, cancellationToken, null);
        }

        internal async Task<List<BackupMapping>> GetAllProtectionWithFabricUri(string fabricUri, TimeSpan timeout,
            CancellationToken cancellationToken, string continuationToken)
        {
            List<BackupMapping> backupList = new List<BackupMapping>();
            List<BackupMapping> allBackupMappings = await this.GetValuesAsync(timeout, cancellationToken);

            foreach (var BackupMapping in allBackupMappings)
            {
                if (BackupMapping.ApplicationOrServiceUri.StartsWith(fabricUri)&& BackupMapping.ApplicationOrServiceUri.CompareTo(continuationToken) > 0)
                {
                    backupList.Add(BackupMapping);
                }
            }
            return backupList;
        }
    }
}