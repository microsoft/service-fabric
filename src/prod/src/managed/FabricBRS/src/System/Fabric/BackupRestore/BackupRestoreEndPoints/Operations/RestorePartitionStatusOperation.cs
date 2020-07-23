// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Common;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading;
    using System.Threading.Tasks;


    internal class RestorePartitionStatusOperation : BackupRestorePartitionOperation<RestoreProgress>
    {
        private readonly string fabricRequestHeader;

        public RestorePartitionStatusOperation(string fabricRequestHeader,string apiVersion , StatefulService statefulService) : base(apiVersion, statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
        }

        internal override async  Task<RestoreProgress> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string fabricUri = await UtilityHelper.GetFabricUriFromRequstHeaderForPartitions(this.fabricRequestHeader, timeout, cancellationToken);
            RestoreStatus restoreStatus = await this.RestoreStore.GetValueAsync(fabricUri,timeout,cancellationToken);
            if (restoreStatus == null)
            {
                return null;
            }
            return restoreStatus.ToRestoreResponse();
        }
    }
}