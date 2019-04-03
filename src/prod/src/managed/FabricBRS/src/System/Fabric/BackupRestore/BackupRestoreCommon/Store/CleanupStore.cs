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
    using System.Fabric.BackupRestore.Service;
    using System.Collections.Generic;

    internal class CleanupStore : BaseStore<string, List<string>>
    {
        private static CleanupStore Store;
        private const string CleanUpStoreName = Constants.CleanUpStoreName;
        private const string TraceRestoreStoreType = "CleanupStoreTrace";

        private CleanupStore(IReliableDictionary<string, List<string>> reliableDictionary,
            StatefulService statefulService) 
            : base(reliableDictionary, statefulService , TraceRestoreStoreType)
        {
            
        }

        internal static async Task<CleanupStore> CreateOrGetCleanupStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Creating a cleanup Store");
                IReliableDictionary<string, List<string>> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, List<string>>>(CleanUpStoreName);
                Store = new CleanupStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Created a Cleanup Store successfully");
            }
            return Store;
        }
    }
}