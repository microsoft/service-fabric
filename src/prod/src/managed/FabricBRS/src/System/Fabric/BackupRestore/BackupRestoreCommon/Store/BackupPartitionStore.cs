// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;

    internal class BackupPartitionStore : BaseStore<string , BackupPartitionStatus>
    {
        private static BackupPartitionStore Store;
        private const string BackupPartitionStoreName = Constants.BackupPartitionStoreName;
        private const string TraceRestoreStoreType = "BackupPartitionStore";

        private BackupPartitionStore(IReliableDictionary<string, BackupPartitionStatus> reliableDictionary,
            StatefulService statefulService) 
            : base(reliableDictionary, statefulService , TraceRestoreStoreType)
        {
            
        }
        
        internal static async Task<BackupPartitionStore> CreateOrGetBackupPartitionStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Creating a BackupPartition Store");
                IReliableDictionary <string, BackupPartitionStatus> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, BackupPartitionStatus>>(BackupPartitionStoreName);
                Store = new BackupPartitionStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Created a BackupPartition Store successfully");
            }
            return Store;
        }
    }
}