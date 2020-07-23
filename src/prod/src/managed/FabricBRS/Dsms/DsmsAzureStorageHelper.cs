// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Dsms
{
    using System.Fabric.BackupRestore;
    using System.Fabric.Common;
    using Microsoft.WindowsAzure.Security.CredentialsManagement.Client;
    using Microsoft.WindowsAzure.Storage;

    internal class DsmsAzureStorageHelper
    {
        private readonly DsmsStorageCredentials dsmsCredentials;

        public DsmsAzureStorageHelper(string sourceLocation)
        {
            var configStore = NativeConfigStore.FabricGetConfigStore();
            var ApServiceName = configStore.ReadUnencryptedString(BackupRestoreContants.BrsConfigSectionName, BackupRestoreContants.DsmsAutopilotServiceNameKeyName);

            if(string.IsNullOrEmpty(ApServiceName))
            {
                CredentialManagementClient.Instance.ClientInitialize();
            }
            else
            {
                CredentialManagementClient.Instance.ClientInitializeForAp(ApServiceName);
            }

            this.dsmsCredentials = DsmsStorageCredentials.GetStorageCredentials(sourceLocation);
        }

        public CloudStorageAccount GetStorageAccount()
        {
            return this.dsmsCredentials.GetCloudStorageAccount();
        }
    }
}
