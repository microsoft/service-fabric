// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.BackupRestore.Interop;
using System.Fabric.Interop;
using System.Globalization;
using System.Text;

namespace System.Fabric.BackupRestore.DataStructures
{
    /// <summary>
    /// Backup download information
    /// </summary>
    internal class DownloadBackupInfo
    {
        private readonly List<String> backupLocations;
        private readonly string destinationRoot;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="backupLocations">List of backup locations</param>
        /// <param name="destinationRoot">Path to the destination root</param>
        internal DownloadBackupInfo(IEnumerable<string> backupLocations, string destinationRoot)
        {
            this.backupLocations = new List<string>(backupLocations);
            this.destinationRoot = destinationRoot;
        }

        /// <summary>
        /// Gets a string representation of the download backup info object
        /// </summary>
        /// <returns>A string represenation of the download backup info object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();

            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupLocations={0}", String.Join(",", this.backupLocations)));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "DestinationRoot={0}", this.destinationRoot));
            return sb.ToString();
        }

        #region Interop Helpers

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDownloadInfo = new NativeBackupRestoreTypes.FABRIC_BACKUP_DOWNLOAD_INFO
            {
                BackupLocations = NativeTypes.ToNativeStringList(pinCollection, this.backupLocations),
                DestinationRootPath = pinCollection.AddObject(this.destinationRoot),
                Reserved = IntPtr.Zero,
            };

            return pinCollection.AddBlittable(nativeDownloadInfo);
        }

        #endregion
    }
}