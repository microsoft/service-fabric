// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Fabric.BackupRestore.Enums;
    using System.Fabric.Common;
    using System.Fabric.Security;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents Azure Blob Storage based recovery point store.
    /// </summary>
    internal class AzureBlobRecoveryPointStore : AzureBlobRecoveryPointStoreBase
    {
        private readonly Model.AzureBlobBackupStorageInfo _storeInformation;

        public AzureBlobRecoveryPointStore(Model.AzureBlobBackupStorageInfo azureBlobStoreInformation) : base(azureBlobStoreInformation)
        {
            this._storeInformation = azureBlobStoreInformation;

            if (azureBlobStoreInformation.IsConnectionStringEncrypted)
            {
                using (var secureString = EncryptionUtility.DecryptText(azureBlobStoreInformation.ConnectionString))
                {
                    this.container = AzureBlobStoreHelper.GetContainer(UtilityHelper.ConvertToUnsecureString(secureString), this._storeInformation.ContainerName);
                }
            }
            else
            {
                this.container = AzureBlobStoreHelper.GetContainer(this._storeInformation.ConnectionString, this._storeInformation.ContainerName);
            }
        }
    }
}