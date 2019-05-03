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
    using System.Text;
    using System.Threading;
    using System.Linq;
    using BRSContinuationToken = System.Fabric.BackupRestore.Common.BRSContinuationToken;
    using BackupPolicyModel = System.Fabric.BackupRestore.Common.Model.BackupPolicy;

    internal class GetAllBackupPolicyOperation : BackupPolicyOperation<PagedBackupPolicyDescriptionList>
    {
        private readonly string continuationToken;
        private readonly int maxResults;

        internal GetAllBackupPolicyOperation(string apiVersion, StatefulService statefulService, string continuationToken = null, int maxResults = 0) : base(apiVersion, statefulService)
        {
            this.maxResults = maxResults;
            this.continuationToken = continuationToken;
        }

        internal override async Task<PagedBackupPolicyDescriptionList> RunAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            BRSContinuationToken brsContinuationToken = new BRSContinuationToken();
            brsContinuationToken.IncomingContinuationToken = continuationToken;

            List<BackupPolicy> listPolicies = new List<BackupPolicy>();

            var listBackupPolicies = await this.BackupPolicyStore.GetValuesAsync(timeout, cancellationToken, null, brsContinuationToken, maxResults);

            foreach (var backupPolicy in listBackupPolicies)
            {
                listPolicies.Add(backupPolicy.ToBackupPolicyView());
            }

            return new PagedBackupPolicyDescriptionList()
            {
                ContinuationToken = brsContinuationToken.OutgoingContinuationToken,
                Items = listPolicies
            };
        }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Get All BackupPolicy Operation");
            return stringBuilder.ToString();
        }
    }
}