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

    internal class RetentionStore : BaseStore<string , RetentionMetadata>
    {
        private static RetentionStore Store;
        private const string RetentionStoreName = Constants.RetentionStoreName;
        private const string TraceRestoreStoreType = "RetentionStoreTrace";

        private RetentionStore(IReliableDictionary<string, RetentionMetadata> reliableDictionary,
            StatefulService statefulService) 
            : base(reliableDictionary, statefulService , TraceRestoreStoreType)
        {
            
        }

        internal static async Task<RetentionStore> CreateOrGetRetentionStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Creating a Retention Store");
                IReliableDictionary<string, RetentionMetadata> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, RetentionMetadata>>(RetentionStoreName);
                Store = new RetentionStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Created a Retention Store successfully");
            }
            return Store;
        }
    }
}