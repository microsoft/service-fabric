// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Runtime.Serialization;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Abstract class that represents a backup store
    /// </summary>
    [Serializable]
    [KnownType(typeof(FileShareBackupStore))]
    [KnownType(typeof(AzureBlobBackupStore))]
    [KnownType(typeof(DsmsAzureBlobBackupStore))]
    internal abstract class BackupStoreInformation
    {
        /// <summary>
        /// Indicates the store type
        /// </summary>
        public BackupStoreType StoreType { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="storeType">Store type</param>
        protected BackupStoreInformation(BackupStoreType storeType)
        {
            this.StoreType = storeType;
        }

        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "StoreType={0}", this.StoreType));

            return sb.ToString();
        }

        #region Interop Helpers

        internal abstract IntPtr ToNative(PinCollection pinCollection);

        internal static unsafe BackupStoreInformation FromNative(IntPtr pointer)
        {
            var nativeStoreInformation = *(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_INFORMATION*)pointer;
            if (nativeStoreInformation.StoreType ==
                NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_FILE_SHARE)
            {
                return FileShareBackupStore.FromNative(nativeStoreInformation);
            }
            else if (nativeStoreInformation.StoreType ==
                     NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_AZURE_STORE)
            {
                return AzureBlobBackupStore.FromNative(nativeStoreInformation);
            }
            else if (nativeStoreInformation.StoreType == 
                NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE.FABRIC_BACKUP_STORE_TYPE_DSMS_AZURE_STORE)
            {
                return DsmsAzureBlobBackupStore.FromNative(nativeStoreInformation);
            }


            return null;
        }

        protected NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_INFORMATION GetNativeBackupStoreInformation(PinCollection pinCollection)
        {
            var nativeStoreInformation = new NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_INFORMATION
            {
                StoreType = (NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_TYPE)StoreType,
            };

            return nativeStoreInformation;
        }

        #endregion
    }
}