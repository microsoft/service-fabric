// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints.Operations
{
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Services.Runtime;
    using BackupPolicyModel = System.Fabric.BackupRestore.Common.Model.BackupPolicy;
    using System.Text;
    using System.Threading;
    using System.Linq;

    internal class GetBackupPolicyProtectedItemsOperation : BackupPolicyOperation<PagedBackupEntityList>
    {
        private readonly string backupPolicyName;
        private readonly string continuationToken;
        private readonly int maxResults;

        public GetBackupPolicyProtectedItemsOperation(string backupPolicyName, string apiVersion, StatefulService statefulService, string continuationToken = null, int maxResults = 0) : base(apiVersion, statefulService)
        {
            this.backupPolicyName = backupPolicyName;
            this.continuationToken = continuationToken;
            this.maxResults = maxResults;
        }
        internal override async Task<PagedBackupEntityList> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            this.ThrowInvalidArgumentIfNull(this.backupPolicyName);
            BackupPolicyModel backupPolicyModel = await this.BackupPolicyStore.GetValueAsync(this.backupPolicyName, timeout, cancellationToken);
            if (backupPolicyModel == null)
            {
                throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
            }
            List<BackupEntity> backupEntities = new List<BackupEntity>();

            List<string> orderedBackupUriList = backupPolicyModel.BackupEnabledSet.ToList();
            orderedBackupUriList.Sort();

            List<string> filteredOrderedBackupUriList = orderedBackupUriList.Where(key => key.CompareTo(continuationToken) > 0).ToList();
            string continuationTokenToSend = null;
            long counter = 0;
            foreach (var backupedUri in filteredOrderedBackupUriList)
            {
                backupEntities.Add(BackupEntity.GetBackupEntityFromFabricKey(backupedUri));
                counter++;
                if (maxResults != 0 && counter == maxResults)
                {
                    if (counter < filteredOrderedBackupUriList.Count)
                    {
                        continuationTokenToSend = backupedUri;
                    }
                    break;
                }
            }

            return new PagedBackupEntityList()
            {
                Items = backupEntities,
                ContinuationToken = continuationTokenToSend
            };
        }
        public override string ToString()
        {
            var stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Get BackupPolicy Operation By Name, BackupPolicyName: {0}", this.backupPolicyName);
            return stringBuilder.ToString();
        }
    }
}