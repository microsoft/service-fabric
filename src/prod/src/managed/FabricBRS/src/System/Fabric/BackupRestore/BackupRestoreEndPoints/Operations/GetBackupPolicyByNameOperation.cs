// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using BackupPolicyModel = System.Fabric.BackupRestore.Common.Model.BackupPolicy;
    using System.Fabric.BackupRestoreEndPoints;
    using System.Text;
    using System.Threading;

    internal class GetBackupPolicyByNameOperation : BackupPolicyOperation<BackupPolicy>
    {
        private readonly string backupPolicyName;

        public GetBackupPolicyByNameOperation(string backupPolicyName,string apiVersion, StatefulService statefulService) : base(apiVersion, statefulService)
        {
            this.backupPolicyName = backupPolicyName;
        }

        internal override async Task<BackupPolicy> RunAsync(TimeSpan timeout , CancellationToken cancellationToken)
        {
            if (string.IsNullOrEmpty(this.backupPolicyName))
            {
                throw new ArgumentException(StringResources.InvalidArguments);
            }

            var backupPolicyModel = await this.BackupPolicyStore.GetValueAsync(this.backupPolicyName,timeout , cancellationToken);
            if (backupPolicyModel != null)
            {
                return backupPolicyModel.ToBackupPolicyView();
            }

            throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
        }

        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Get BackupPolicy Operation, BackupPolicyName: {0}", this.backupPolicyName);
            return stringBuilder.ToString();
        }
    }
}