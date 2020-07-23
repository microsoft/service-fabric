// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Fabric.BackupRestore.Common.Model;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;
    using System.Threading;
    using Microsoft.ServiceFabric.Data;

    internal class BackupPolicyStore : BaseStore<string , BackupPolicy>
    {
        private static BackupPolicyStore _store;
        private const string BackupPolicyStoreName = Constants.BackupPolicyStoreName;
        private const string TraceBackupPolicStoreType = "BackupPolicyStore";

        private BackupPolicyStore(IReliableDictionary<string, BackupPolicy> reliableDictionary,
            StatefulService statefulService) 
            : base(reliableDictionary, statefulService , TraceBackupPolicStoreType)
        {
        }
        
        internal static async Task<BackupPolicyStore> CreateOrGetBackupPolicyStore(StatefulService statefulService)
        {
            if (_store == null)
            {
                BackupRestoreTrace.TraceSource.WriteNoise(TraceBackupPolicStoreType, "Creating a Backup Policy Store");
                var reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, BackupPolicy>>(BackupPolicyStoreName);
                _store = new BackupPolicyStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteNoise(TraceBackupPolicStoreType, "Created a Backup Policy Store successfully");
            }
            return _store;
        }

        internal async Task AddProtectionEntity(BackupPolicy backupPolicy, string enablementKey, TimeSpan timeout, CancellationToken cancellationToken,
            ITransaction transaction)
        {
            if (backupPolicy == null)
            {
                throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
            }

            var updatedPolicy = backupPolicy.ToBuilder()
                                .AddToBackupEnabledSet(enablementKey)
                                .Build();

            await this.StoreReliableDictionary.SetAsync(transaction, backupPolicy.Name, updatedPolicy, timeout, cancellationToken);
        }

        internal async Task RemoveProtectionEntity(BackupPolicy backupPolicy, string enablementKey, TimeSpan timeout, CancellationToken cancellationToken ,
            ITransaction transaction)
        {
            if (backupPolicy == null)
            {
                throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
            }

            var updatedPolicy = backupPolicy.ToBuilder()
                                .RemoveFromBackupEnabledSet(enablementKey)
                                .Build();

            await this.StoreReliableDictionary.SetAsync(transaction, backupPolicy.Name,
                updatedPolicy, timeout, cancellationToken);
        }

        internal override async Task DeleteValueAsync(string key, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction = null)
        {
            var backupPolicy =  await base.GetValueAsync(key,timeout,cancellationToken);
            if (backupPolicy == null)
            {
                throw new FabricException(StringResources.BackupPolicyDoesNotExist, FabricErrorCode.BackupPolicyDoesNotExist);
            }

            if (backupPolicy.BackupEnabledSet.Count > 0)
            {
                throw new FabricException(StringResources.MappingExist, FabricErrorCode.BackupEnabled);
            }

            await base.DeleteValueAsync(key, timeout,cancellationToken , transaction);
        }
    }
}