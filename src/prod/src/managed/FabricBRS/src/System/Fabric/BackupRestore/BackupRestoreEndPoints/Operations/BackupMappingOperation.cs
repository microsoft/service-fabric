// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading;
    using System.Fabric.BackupRestoreEndPoints;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;

    internal abstract class BackupMappingOperation<TResult> : BaseOperation<TResult>
    {
        protected internal BackupMappingStore BackupMappingStore;
        protected internal StatefulService StatefulService;
        protected internal WorkItemQueue WorkItemQueue;
        protected internal BackupPolicyStore BackupPolicyStore;

        protected internal BackupMappingOperation(string apiVersion , StatefulService statefulService) : base (apiVersion)
        {
            this.StatefulService = statefulService;
        }

        internal async override Task InitializeAsync()
        {
            this.BackupMappingStore = await BackupMappingStore.CreateOrGetBackupMappingStore(this.StatefulService).ConfigureAwait(false);
            this.BackupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService).ConfigureAwait(false);
            this.WorkItemQueue = await WorkItemQueue.CreateOrGetWorkItemQueue(this.StatefulService).ConfigureAwait(false); 
        }
    }
}