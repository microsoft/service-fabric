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

    internal class ConfigStore : BaseStore<string, string>
    {
        private static ConfigStore Store;
        private const string ConfigStoreName = Constants.ConfigStoreName;
        private const string TraceConfigStoreType = "ConfigStore";

        private ConfigStore(IReliableDictionary<string, string> reliableDictionary,
            StatefulService statefulService)
            : base(reliableDictionary, statefulService, TraceConfigStoreType)
        {

        }

        internal static async Task<ConfigStore> CreateOrGetConfigStore(StatefulService statefulService)
        {
            if (Store != null) return Store;

            BackupRestoreTrace.TraceSource.WriteNoise(TraceConfigStoreType, "Creating a Config Store");
            var reliableDictionary = await statefulService.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(ConfigStoreName);
            Store = new ConfigStore(reliableDictionary, statefulService);
            BackupRestoreTrace.TraceSource.WriteNoise(TraceConfigStoreType, "Created a Config Store successfully");
            return Store;
        }
    }
}