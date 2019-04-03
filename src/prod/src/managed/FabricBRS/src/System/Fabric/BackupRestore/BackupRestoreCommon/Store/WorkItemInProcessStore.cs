// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal class WorkItemInProcessStore : BaseStore<Guid , WorkItemProcessInfo>
    {
        private static WorkItemInProcessStore Store;
        private const string WorkItemStoreName = Constants.WorkItemInProcessStoreName;
        private const string TraceWorkItemInProcessStoreType = "WorkItemInProcessStore";

        private WorkItemInProcessStore(IReliableDictionary<Guid, WorkItemProcessInfo> reliableDictionary,
            StatefulService statefulService )
            : base(reliableDictionary, statefulService , TraceWorkItemInProcessStoreType)
        {
            
        }

        internal static async Task<WorkItemInProcessStore> CreateOrGetWorkItemInProcessStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceWorkItemInProcessStoreType, "Creating a WorkItem In Process  Store");
                IReliableDictionary<Guid, WorkItemProcessInfo> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<Guid, WorkItemProcessInfo>>(WorkItemStoreName);
                Store = new WorkItemInProcessStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceWorkItemInProcessStoreType, "Created a WorkItem In Process  successfully");
            }
            return Store;
        }

        
    }
}