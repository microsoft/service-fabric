// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Service
{
    using System.Fabric.Common;
    using System.Globalization;

    internal class BackupRestoreServiceConfig
    {
        private const string TraceType = "BackupRestoreServiceConfig";

        internal static TimeSpan ApiTimeout { get; private set; }

        internal static TimeSpan WorkItemTimeout { get; private set; }

        static BackupRestoreServiceConfig()
        {
            // Read value from config store
            var configStore = NativeConfigStore.FabricGetConfigStore();
            
            var apiTimeoutString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.ApiTimeoutKeyName);
            ApiTimeout = String.IsNullOrEmpty(apiTimeoutString) ? BackupRestoreContants.ApiTimeoutInSecondsDefault : TimeSpan.FromSeconds(int.Parse(apiTimeoutString, CultureInfo.InvariantCulture));

            var workItemTimeoutString = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, WorkItemTimeoutKeyName);
            WorkItemTimeout = String.IsNullOrEmpty(workItemTimeoutString) ? DefaultWorkItemTimeout : TimeSpan.FromSeconds(int.Parse(workItemTimeoutString, CultureInfo.InvariantCulture));
        }

        private static readonly TimeSpan DefaultWorkItemTimeout = TimeSpan.FromSeconds(300);

        private const string WorkItemTimeoutKeyName = "WorkItemTimeoutInSeconds";

        internal const string SecretEncryptionCertStoreNameKey = "SecretEncryptionCertX509StoreName";
        internal const string SecretEncryptionCertThumbprintKey = "SecretEncryptionCertThumbprint";
    }
}