// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System.Collections.Generic;
    using System.Fabric.Common;

    public class ConfigUpdateHandler : IConfigStoreUpdateHandler
    {
        /// <summary>
        /// Trace type string used by traces of this service.
        /// </summary>
        private const string TraceType = "BRSConfigUpdateHandler";

        private readonly HashSet<string> propertiesRequiringRecycle;

        public bool ProcessRecycleRequired
        {
            get;
            set;
        }

        public ConfigUpdateHandler()
        {
            this.ProcessRecycleRequired = false;
            this.propertiesRequiringRecycle = new HashSet<string>
            {
                BackupRestoreContants.ServerAuthX509StoreName,
                BackupRestoreContants.ServerAuthX509FindType,
                BackupRestoreContants.ServerAuthX509FindValue,
                BackupRestoreContants.ServerAuthX509FindValueSecondary
            };
        }

        bool IConfigStoreUpdateHandler.CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        bool IConfigStoreUpdateHandler.OnUpdate(string sectionName, string keyName)
        {
            if((BackupRestoreContants.FabricNodeConfigSectionName.Equals(sectionName,StringComparison.InvariantCultureIgnoreCase)) && this.propertiesRequiringRecycle.Contains(keyName))
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Recycling process, change in server cert config");
                this.ProcessRecycleRequired = true;
            }

            return true;
        }
    }
}