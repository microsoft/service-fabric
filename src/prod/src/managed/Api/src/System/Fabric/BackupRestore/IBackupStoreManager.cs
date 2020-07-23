// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Interface which must be implemented by any backup store manager
    /// </summary>
    internal interface IBackupStoreManager
    {
        /// <summary>
        /// Uploads the backup to the store
        /// </summary>
        /// <param name="sourceFileOrFolderPath">Local source location of the backup</param>
        /// <param name="destinationFolderName">Folder name at the destination side where to upload this backup</param>
        /// <param name="sourceIsFile">Whether source is a file or else folder</param>
        void Upload(string sourceFileOrFolderPath, string destinationFolderName, bool sourceIsFile = false);

        /// <summary>
        /// Downloads the backup from the store
        /// </summary>
        /// <param name="sourceFileOrFolderName">Source location of the backup in the store</param>
        /// <param name="destinationFolder">Local path where to download the backup</param>
        /// /// <param name="sourceIsFile">Whether source is a file or else folder</param>
        void Download(string sourceFileOrFolderName, string destinationFolder, bool sourceIsFile = false);
    }
}