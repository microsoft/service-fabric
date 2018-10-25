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
    /// Backup upload information
    /// </summary>
    internal class UploadBackupInfo
    {
        private readonly string backupMetadataFilePath;
        private readonly string uploadFolderOrFileLocation;
        private readonly string destinationFolderName;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="backupMetadataFilePath">Local path of the backup metadata file which needs to be uploaded</param>
        /// <param name="zippedBackupFileLocation">Local path of the zipped backup which needs to be uploaded</param>
        internal UploadBackupInfo(string backupMetadataFilePath, string zippedBackupFileLocation)
            : this(backupMetadataFilePath, zippedBackupFileLocation, null)
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="backupMetadataFilePath">Local path of the backup metadata file which needs to be uploaded</param>
        /// <param name="backupFolderLocation">Local path of the backup folder which needs to be uploaded</param>
        /// <param name="destinationFolderName">Name of the backup folder at the destination where the contents of <paramref name="backupFolderLocation"/> will be uploaded.</param>
        internal UploadBackupInfo(string backupMetadataFilePath, string backupFolderLocation,
            string destinationFolderName)
        {
            this.backupMetadataFilePath = backupMetadataFilePath;
            this.uploadFolderOrFileLocation = backupFolderLocation;
            this.destinationFolderName = destinationFolderName;
        }

        /// <summary>
        /// Gets a string representation of the upload backup info object
        /// </summary>
        /// <returns>A string represenation of the upload backup info object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();

            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupMetadataFilePath={0}", this.backupMetadataFilePath));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "UploadFolderOrFileLocation={0}", this.uploadFolderOrFileLocation));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "DestinationFolderName={0}", this.destinationFolderName));
            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeUploadInfo = new NativeBackupRestoreTypes.FABRIC_BACKUP_UPLOAD_INFO
            {
                BackupMetadataFilePath = pinCollection.AddObject(this.backupMetadataFilePath),
                LocalBackupPath = pinCollection.AddObject(this.uploadFolderOrFileLocation),
                DestinationFolderName = pinCollection.AddObject(this.destinationFolderName),
                Reserved = IntPtr.Zero,
            };

            return pinCollection.AddBlittable(nativeUploadInfo);
        }

        #endregion
    }
}