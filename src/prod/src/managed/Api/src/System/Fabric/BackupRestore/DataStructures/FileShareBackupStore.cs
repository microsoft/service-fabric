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
    /// Represents file share backup store
    /// </summary>
    [Serializable]
    internal class FileShareBackupStore : BackupStoreInformation
    {
        /// <summary>
        /// File share path where backups would be stored
        /// </summary>
        public string FileSharePath { get; set; }

        /// <summary>
        /// Access type for the specified file share
        /// </summary>
        public FileShareAccessType AccessType { get; set; }

        /// <summary>
        /// Primary account user name details
        /// </summary>
        public string PrimaryUserName { get; set; }

        /// <summary>
        /// Password for primary account
        /// </summary>
        public string PrimaryPassword { get; set; }

        /// <summary>
        /// Secondary account user name details. This is optional and used in credentials rollover scenario
        /// </summary>
        public string SecondaryUserName { get; set; }

        /// <summary>
        /// Password for secondary account
        /// </summary>
        public string SecondaryPassword { get; set; }

        /// <summary>
        /// Is the password specified encrypted
        /// </summary>
        public bool IsPasswordEncrypted { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public FileShareBackupStore()
            : base(BackupStoreType.FileShare)
        {
        }

        /// <summary>
        /// Gets a string representation of the file share backup store object
        /// </summary>
        /// <returns>A string representation of the file share backup store object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "FileSharePath={0}", this.FileSharePath));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "AccessType={0}", this.AccessType));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "SecondaryCredentialsProvided={0}", !String.IsNullOrEmpty(this.SecondaryUserName)));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsPasswordEncrypted={0}", this.IsPasswordEncrypted));

            return sb.ToString();
        }

        #region Interop Helpers

        internal override IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeStoreInformation = GetNativeBackupStoreInformation(pinCollection);

            var nativeFileShareBackupStore = new NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION
            {
                FileSharePath = pinCollection.AddObject(this.FileSharePath),
                AccessType = (NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_FILE_SHARE_ACCESS_TYPE)AccessType,
                PrimaryUserName = pinCollection.AddObject(this.PrimaryUserName),
                PrimaryPassword = pinCollection.AddObject(this.PrimaryPassword),
                SecondaryUserName = pinCollection.AddObject(this.SecondaryUserName),
                SecondaryPassword = pinCollection.AddObject(this.SecondaryPassword),
                IsPasswordEncrypted = NativeTypes.ToBOOLEAN(this.IsPasswordEncrypted),
            };

            nativeStoreInformation.StoreAccessInformation = pinCollection.AddBlittable(nativeFileShareBackupStore);
            return pinCollection.AddBlittable(nativeStoreInformation);
        }

        internal static unsafe FileShareBackupStore FromNative(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_INFORMATION storeInformation)
        {
            var fileShareStoreInformationNative =
                *(NativeBackupRestoreTypes.FABRIC_BACKUP_STORE_FILE_SHARE_INFORMATION*)storeInformation.StoreAccessInformation;

            var fileShareBackupStore = new FileShareBackupStore
            {
                AccessType = (FileShareAccessType) fileShareStoreInformationNative.AccessType,
                FileSharePath = NativeTypes.FromNativeString(fileShareStoreInformationNative.FileSharePath),
                PrimaryUserName = NativeTypes.FromNativeString(fileShareStoreInformationNative.PrimaryUserName),
                PrimaryPassword = NativeTypes.FromNativeString(fileShareStoreInformationNative.PrimaryPassword),
                SecondaryUserName = NativeTypes.FromNativeString(fileShareStoreInformationNative.SecondaryUserName),
                SecondaryPassword = NativeTypes.FromNativeString(fileShareStoreInformationNative.SecondaryPassword),
                IsPasswordEncrypted = NativeTypes.FromBOOLEAN(fileShareStoreInformationNative.IsPasswordEncrypted)
            };

            return fileShareBackupStore;
        }

        #endregion
    }
}