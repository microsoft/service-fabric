// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System.Collections.Generic;
    using System.Fabric.BackupRestore.BackupRestoreTypes;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Base class for Azure blob storage based recovery point manager
    /// </summary>
    internal class DsmsAzureBlobStoreRecoveryPointManager : IRecoveryPointManager
    {
        private readonly Model.DsmsAzureBlobBackupStorageInfo _storeInformation;
        private readonly DsmsAzureBlobRecoveryPointStore blobRecoveryPointStore;

        private const string RecoveryPointMetadataFileExtension = ".bkmetadata";
        private const string ZipFileExtension = ".zip";

        public DsmsAzureBlobStoreRecoveryPointManager(Model.DsmsAzureBlobBackupStorageInfo azureBlobStoreInformation)
        {
            this._storeInformation = azureBlobStoreInformation;
            this.blobRecoveryPointStore = new Common.DsmsAzureBlobRecoveryPointStore(azureBlobStoreInformation);
        }

        #region IRecoveryPointManager methods

        public List<string> EnumerateSubFolders(string relativePath)
        {
            return this.blobRecoveryPointStore.EnumerateSubFolders(relativePath);
        }

        public List<string> EnumerateRecoveryPointMetadataFiles(string relativePath, DateTime startDateTime, DateTime endDateTime, bool isLatestRequested, BRSContinuationToken continuationToken, int maxResults)
        {
            return this.blobRecoveryPointStore.EnumerateRecoveryPointMetadataFiles(relativePath, startDateTime, endDateTime, isLatestRequested, continuationToken, maxResults);
        }

        public async Task<List<RestorePoint>> GetRecoveryPointDetailsAsync(IEnumerable<string> recoveryPointMetadataFiles, CancellationToken cancellationToken)
        {
            return await this.blobRecoveryPointStore.GetRecoveryPointDetailsAsync(recoveryPointMetadataFiles, cancellationToken);
        }

        public async Task<RestorePoint> GetRecoveryPointDetailsAsync(string backupLocation, CancellationToken cancellationToken)
        {
            return await this.blobRecoveryPointStore.GetRecoveryPointDetailsAsync(backupLocation, cancellationToken);
        }

        public async Task<List<string>> GetBackupLocationsInBackupChainAsync(string backupLocation, CancellationToken cancellationToken)
        {
            return await this.blobRecoveryPointStore.GetBackupLocationsInBackupChainAsync(backupLocation, cancellationToken);
        }

        public bool DeleteBackupFiles(string relativefilePathOrSourceFolderPath)
        {
            return this.blobRecoveryPointStore.DeleteBackupFiles(relativefilePathOrSourceFolderPath);
        }
        #endregion
    }
}