// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.Collections;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.Common;
    using System.Threading;
    using System.Collections.Generic;


    internal class SuspendStore : BaseStore<string , string>
    {
        private static SuspendStore Store;
        private const string SuspendBackupStoreName = Constants.SuspendBackupStoreName;
        private const string TraceRestoreStoreType = "SuspendStore";

        private SuspendStore(IReliableDictionary<string, string> reliableDictionary,
            StatefulService statefulService) 
            : base(reliableDictionary, statefulService , TraceRestoreStoreType)
        {
            
        }
        
        internal static async Task<SuspendStore> CreateOrGetSuspendStatusStore(StatefulService statefulService)
        {
            if (Store == null)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Creating a Suspend Store");
                IReliableDictionary <string, string> reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(SuspendBackupStoreName);
                Store = new SuspendStore(reliableDictionary, statefulService);
                BackupRestoreTrace.TraceSource.WriteInfo(TraceRestoreStoreType, "Created a Suspend Store successfully");
            }
            return Store;
        }

        internal async Task<List<string>> GetAllSuspensionWithFabricUri(string fabricUri, TimeSpan timeout,
            CancellationToken cancellationToken, string continuationToken = null)
        {
            List<string> suspendedList = new List<string>();
            List<string> allSusupendPartitons = await this.GetValuesAsync(timeout, cancellationToken);

            foreach (var suspendPartition in allSusupendPartitons)
            {
                if (suspendPartition.StartsWith(fabricUri) && fabricUri.CompareTo(continuationToken) > 0)
                {
                    suspendedList.Add(suspendPartition);
                }
            }
            return suspendedList;
        }

        internal async Task<bool> IsFabrcUriSuspended(string fabricUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return await this.GetValueAsync(fabricUri, timeout, cancellationToken) != null;
        }

        internal async Task<List<string>> GetAllSuspensions(TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            List<string> suspendedList = new List<string>();
            return await this.GetValuesAsync(timeout, cancellationToken);
        }

    }
}