// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Represents Azure blob backup store
    /// </summary>
    [Serializable]
    internal class AzureBlobBackupStore : BackupStoreInformation
    {
        /// <summary>
        /// Connection string
        /// </summary>
        public string ConnectionString { get; set; }

        /// <summary>
        /// Container name where backups would be stored
        /// </summary>
        public string ContainerName { get; set; }

        /// <summary>
        /// Path under container for backups to be stored
        /// </summary>
        public string FolderPath { get; set; }

        /// <summary>
        /// Is the account key specified in connection string encrypted?
        /// </summary>
        public bool IsAccountKeyEncrypted { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public AzureBlobBackupStore()
            : base(BackupStoreType.AzureStorage)
        {
        }

        /// <summary>
        /// Gets a string representation of the azure blob backup store object
        /// </summary>
        /// <returns>A string representation of the azure blob backup store object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ContainerName={0}", this.ContainerName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "FolderPath={0}", this.FolderPath));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsAccountKeyEncrypted={0}", this.IsAccountKeyEncrypted));

            return sb.ToString();
        }

        #region Interop Helpers

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStoreInformation = GetNativeBackupStoreInformation(pinCollection);

            var nativeAzureStorageBackupStore = new NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION()
            {
                ConnectionString = pinCollection.AddObject(this.ConnectionString),
                ContainerName= pinCollection.AddObject(this.ContainerName),
                FolderPath = pinCollection.AddObject(this.FolderPath),
                IsConnectionStringEncrypted = NativeTypes.ToBOOLEAN(this.IsAccountKeyEncrypted),
            };

            nativeStoreInformation.StoreAccessInformation = pinCollection.AddBlittable(nativeAzureStorageBackupStore);
            return pinCollection.AddBlittable(nativeStoreInformation);
        }

        internal static unsafe AzureBlobBackupStore FromNative(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_INFORMATION storeInformation)
        {
            var azureBlobStoreInformationNative =
                *(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_AZURE_STORAGE_INFORMATION*)storeInformation.StoreAccessInformation;

            var azureBlobBackupStore = new AzureBlobBackupStore
            {
                ConnectionString = NativeTypes.FromNativeString(azureBlobStoreInformationNative.ConnectionString),
                ContainerName = NativeTypes.FromNativeString(azureBlobStoreInformationNative.ContainerName),
                FolderPath = NativeTypes.FromNativeString(azureBlobStoreInformationNative.FolderPath),
                IsAccountKeyEncrypted = NativeTypes.FromBOOLEAN(azureBlobStoreInformationNative.IsConnectionStringEncrypted)
            };

            return azureBlobBackupStore;
        }

        #endregion
    }
}