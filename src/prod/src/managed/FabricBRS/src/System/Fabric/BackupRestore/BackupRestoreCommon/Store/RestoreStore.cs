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

    internal class RestoreStore : BaseStore<string , RestoreStatus>
    {
        private static RestoreStore Store;
        private const string RestoreStoreName = Constants.RestoreStoreName;
        private const string TraceRestoreStoreType = "RestoreStore";

        private RestoreStore(IReliableDictionary<string, RestoreStatus> reliableDictionary,
            StatefulService statefulService) 
            : base(reliableDictionary, statefulService , TraceRestoreStoreType)
        {
            
        }
        
        internal static async Task<RestoreStore> CreateOrGetRestoreStatusStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Creating a Restore Store");
                IReliableDictionary <string, RestoreStatus> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, RestoreStatus>>(RestoreStoreName);
                Store = new RestoreStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Created a Restore Store successfully");
            }
            return Store;
        }
    }
}