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
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;


    internal class BackupPartitionResponseOperation : BackupRestorePartitionOperation<BackupProgress>
    {
        private readonly string fabricRequestHeader;

        public BackupPartitionResponseOperation(string fabricRequestHeader,string apiVersion , StatefulService statefulService) : base(apiVersion, statefulService)
        {
            this.fabricRequestHeader = fabricRequestHeader;
        }

        internal override async  Task<BackupProgress> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            string fabricUri = await UtilityHelper.GetFabricUriFromRequstHeaderForPartitions(this.fabricRequestHeader, timeout, cancellationToken);
            BackupPartitionStatus backupPartitionStatus = await this.BackupPartitionStore.GetValueAsync(fabricUri, timeout, cancellationToken);
            if (backupPartitionStatus == null)
            {
                return null;
            }
            return backupPartitionStatus.ToBackupPartitionResponse();
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("BackupPartitionResponseOperation for fabricRequestHeader : {0}", this.fabricRequestHeader).AppendLine();
            return stringBuilder.ToString();
        }
    }
}