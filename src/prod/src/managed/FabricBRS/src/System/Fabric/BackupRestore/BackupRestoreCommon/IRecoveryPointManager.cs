// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Fabric.BackupRestore.BackupRestoreTypes;
using System.Threading;
using System.Threading.Tasks;
using BRSContinuationToken = System.Fabric.BackupRestore.Common.BRSContinuationToken;

namespace System.Fabric.BackupRestore.Common
{
    /// <summary>
    /// Interface which must be implemented by any recovery point manager
    /// </summary>
    internal interface IRecoveryPointManager
    {
        /// <summary>
        /// Enumerates sub folders inside a particular folder
        /// </summary>
        /// <param name="relativePath">Relative path from the root of the store under which sub direcotries need to be enumerated</param>
        /// <returns>List of sub folder names</returns>
        List<string> EnumerateSubFolders(string relativePath);


        /// <summary>
        /// Enumerate recovery point details asynchronously
        /// </summary>
        /// <param name="recoveryPointMetadataFiles">List of recovery points files to fetch details</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>List of recovery point objects</returns>
        Task<List<RestorePoint>> GetRecoveryPointDetailsAsync(IEnumerable<string> recoveryPointMetadataFiles, CancellationToken cancellationToken);

        /// <summary>
        /// Enumerate recovery point metadata files
        /// </summary>
        /// <param name="relativePath">Relative path from the root of the store. Using this path we enumerate the
        /// metadata files for a application/ service/ partition.</param>
        /// <param name="startDateTime">Start date filter.</param>
        /// <param name="endDateTime">End date filter.</param>
        /// <param name="isLatestRequested">To check if the only latest from the partitions is required.</param>
        /// <param name="continuationTokenList">Continuation Token object containing continuationToken from previous 
        /// query and the continuationToken for this one. It is equal to null when all the files are enumerated.</param>
        /// <param name="maxResults">Max number of results to list. For all results, pass 0.</param>
        /// <returns>List of recovery point file names</returns>
        List<string> EnumerateRecoveryPointMetadataFiles(string relativePath, DateTime startDateTime, DateTime endDateTime, bool isLatestRequested, BRSContinuationToken continuationTokenList, int maxResults);

        /// <summary>
        /// Gets the recovery point details for the specified backup location
        /// </summary>
        /// <param name="backupLocation">Backup location relative to this store root</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>Recover point details</returns>
        Task<RestorePoint> GetRecoveryPointDetailsAsync(string backupLocation, CancellationToken cancellationToken);

        /// <summary>
        /// Enumerate all backup locations present in backup chain above the specified backup location
        /// </summary>
        /// <param name="backupLocation">Backup location relative to store root</param>
        /// <param name="cancellationToken">For handling cancellation</param>
        /// <returns>All backup locations needed to restore using the specified recovery point</returns>
        Task<List<string>> GetBackupLocationsInBackupChainAsync(string backupLocation, CancellationToken cancellationToken);

        /// <summary>
        /// Delete the backup files(both metadata and zip/folder) specified by relativefilePathOrSourceFolderPath
        /// </summary>
        /// <param name="relativefilePathOrSourceFolderPath">Backup location of the zip file/fodlerpath of a backup.</param>
        /// <returns>All backup locations needed to restore using the specified recovery point</returns>
        bool DeleteBackupFiles(string relativefilePathOrSourceFolderPath);
    }
}