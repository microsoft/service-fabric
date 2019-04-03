// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    using System.Fabric.BackupRestore.DataStructures;
    using System.Fabric.Security;
    using System.Runtime.InteropServices;
    using System.Security;

    using Microsoft.WindowsAzure.Storage;

    /// <summary>
    /// Azure blob BackupStoreManager
    /// </summary>
    internal class AzureBlobBackupStoreManager : AzureBlobBackupStoreManagerBase
    {
        private const string TraceType = "AzureBlobBackupStore";

        private readonly AzureBlobBackupStore azureBlobBackupStore;

        public AzureBlobBackupStoreManager(AzureBlobBackupStore azureBlobBackupStore) : base(TraceType)
        {
            this.azureBlobBackupStore = azureBlobBackupStore;
        }

        #region Base class abstract methods
        protected override string GetContainerName()
        {
            return this.azureBlobBackupStore.ContainerName;
        }

        protected override string GetFolderPath()
        {
            return this.azureBlobBackupStore.FolderPath;
        }

        protected override CloudStorageAccount GetCloudStorageAccount()
        {
            CloudStorageAccount storageAccount;

            if (this.azureBlobBackupStore.IsAccountKeyEncrypted)
            {
                storageAccount = CloudStorageAccount.Parse(this.SecureStringToString(EncryptionUtility.DecryptText(this.azureBlobBackupStore.ConnectionString)));
            }
            else
            {
                storageAccount = CloudStorageAccount.Parse(this.azureBlobBackupStore.ConnectionString);
            }            return storageAccount;        }

        private String SecureStringToString(SecureString value)
        {
            IntPtr valuePtr = IntPtr.Zero;
            try
            {
                valuePtr = Marshal.SecureStringToGlobalAllocUnicode(value);
                return Marshal.PtrToStringUni(valuePtr);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(valuePtr);
            }
        }
        #endregion
    }
}