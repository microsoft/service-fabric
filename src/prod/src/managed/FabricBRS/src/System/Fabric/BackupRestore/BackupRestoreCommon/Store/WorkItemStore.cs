// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;

    internal class WorkItemStore : BaseStore<Guid , WorkItem>
    {
        private static WorkItemStore Store;
        private const string WorkItemStoreName = Constants.WorkItemStoreName;
        private const string TraceWorkItemStoreType = "WorkItemStore";

        private WorkItemStore(IReliableDictionary<Guid, WorkItem> reliableDictionary,
            StatefulService statefulService)
            : base(reliableDictionary, statefulService , TraceWorkItemStoreType)
        {
            
        }
        
        internal static async Task<WorkItemStore> CreateOrGetWorkItemStore(StatefulService statefulService )
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteNoise(TraceWorkItemStoreType, "Creating a WorkItem Store");
                IReliableDictionary<Guid, WorkItem> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<Guid, WorkItem>>(WorkItemStoreName);
                Store = new WorkItemStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteNoise(TraceWorkItemStoreType, "Created a WorkItem Store successfully");
            }
            return Store;
        }
    }
}