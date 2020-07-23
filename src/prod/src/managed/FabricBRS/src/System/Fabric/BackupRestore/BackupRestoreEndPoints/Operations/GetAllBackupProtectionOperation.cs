// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using BackupProtectionModel =  System.Fabric.BackupRestore.Common.Model.BackupProtection;
    using System.Collections.Generic;
    using System.Text;
    using System.Threading;

    internal class GetAllBackupProtectionOperation : BackupProtectionOperation<IList<BackupProtection>>
    {
        internal GetAllBackupProtectionOperation(string apiVersion , StatefulService statefulService ):base(apiVersion,statefulService)
        {
        }

        internal override async Task<IList<BackupProtection>> RunAsync(TimeSpan timeout , CancellationToken  cancellationToken)
        {
            IList<BackupProtectionModel> backupProtectionsModelList=  await this.BackupProtectionStore.GetAllFromStore(timeout , cancellationToken);
            IList<BackupProtection> backupProtectionsList = new List<BackupProtection>();
            foreach (BackupProtectionModel backupProtection in backupProtectionsModelList)
            {
                backupProtectionsList.Add(BackupProtectionModel.ToBackupProtectionView(backupProtection));
            }
            return backupProtectionsList;
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Get All backup protection").AppendLine();
            return stringBuilder.ToString();
        }
    }
}