// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.Common;

    internal abstract class BackupPolicyOperation<TResult> : BaseOperation<TResult>
    {
        protected internal BackupPolicyStore BackupPolicyStore;
        
        protected internal StatefulService StatefulService;

        protected internal BackupPolicyOperation(string apiVersion , StatefulService statefulService) : base(apiVersion)
        {
            this.StatefulService = statefulService;
        }

        internal async override Task InitializeAsync()
        {
            this.BackupPolicyStore = await BackupPolicyStore.CreateOrGetBackupPolicyStore(this.StatefulService).ConfigureAwait(false);
        }
    }
}