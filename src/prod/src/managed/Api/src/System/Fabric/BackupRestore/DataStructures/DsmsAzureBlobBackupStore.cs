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
    /// Represents dSMS managed Azure blob backup store
    /// </summary>
    [Serializable]
    internal class DsmsAzureBlobBackupStore : BackupStoreInformation
    {
        /// <summary>
        /// Storage Credentials source Location
        /// </summary>
        public string StorageCredentialsSourceLocation { get; set; }

        /// <summary>
        /// Container name where backups would be stored
        /// </summary>
        public string ContainerName { get; set; }

        /// <summary>
        /// Path under container for backups to be stored
        /// </summary>
        public string FolderPath { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public DsmsAzureBlobBackupStore()
            : base(BackupStoreType.DsmsAzureStorage)
        {
        }

        /// <summary>
        /// Gets a string representation of the azure blob backup store object
        /// </summary>
        /// <returns>A string representation of the azure blob backup store object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "StorageCredentialsSourceLocation={0}", this.StorageCredentialsSourceLocation));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ContainerName={0}", this.ContainerName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "FolderPath={0}", this.FolderPath));
            return sb.ToString();
        }

        #region Interop Helpers

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStoreInformation = GetNativeBackupStoreInformation(pinCollection);

            var nativeAzureStorageBackupStore = new NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION()
            {
                StorageCredentialsSourceLocation = pinCollection.AddObject(this.StorageCredentialsSourceLocation),
                ContainerName = pinCollection.AddObject(this.ContainerName),
                FolderPath = pinCollection.AddObject(this.FolderPath),
            };

            nativeStoreInformation.StoreAccessInformation = pinCollection.AddBlittable(nativeAzureStorageBackupStore);
            return pinCollection.AddBlittable(nativeStoreInformation);
        }

        internal static unsafe DsmsAzureBlobBackupStore FromNative(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_INFORMATION storeInformation)
        {
            var dsmsAzureBlobStoreInformationNative =
                *(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_DSMS_AZURE_STORAGE_INFORMATION*)storeInformation.StoreAccessInformation;

            var DsmsAzureBlobBackupStore = new DsmsAzureBlobBackupStore
            {
                StorageCredentialsSourceLocation = NativeTypes.FromNativeString(dsmsAzureBlobStoreInformationNative.StorageCredentialsSourceLocation),
                ContainerName = NativeTypes.FromNativeString(dsmsAzureBlobStoreInformationNative.ContainerName),
                FolderPath = NativeTypes.FromNativeString(dsmsAzureBlobStoreInformationNative.FolderPath),
            };

            return DsmsAzureBlobBackupStore;
        }

        #endregion
    }
}