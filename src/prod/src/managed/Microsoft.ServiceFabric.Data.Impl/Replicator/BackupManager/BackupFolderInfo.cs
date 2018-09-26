// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Backup
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Class used to analyze an input backup folder.
    /// Three core responsibilities
    /// - Scan the folder to find all backup and create a backup chain
    /// - Trim the chain so that it only contains the longest chain.
    /// - Verify whether or not the input folder is restorable.
    /// </summary>
    internal class BackupFolderInfo
    {
        private readonly string traceType;

        /// <summary>
        /// Input backup folder.
        /// </summary>
        private readonly string backupFolder;

        /// <summary>
        /// Ordered list of backup version.
        /// Note: keep the backupVersion sorted thus corresponding backupMetadataFile will be in right order, 
        /// which can be used to find the longest chain later.
        /// </summary>
        private readonly List<BackupInfo.BackupVersion> backupVersionList;

        /// <summary>
        /// List of backup metadata files.
        /// </summary>
        private readonly List<BackupMetadataFile> backupMetadataFileList;

        /// <summary>
        /// List of backup log file paths.
        /// </summary>
        private readonly List<string> logFileList;

        /// <summary>
        /// Test only flag - DO NOT USE IN PRODUCTION
        /// Normally as part of verification, each log file is opened to verify that they have valid footers (at least).
        /// With this setting, this verification is by passed.
        /// </summary>
        private readonly bool test_doNotVerifyLogFile = false;

        private string fullBackupFolderPath;
        private string stateManagerBackupFolderPath;

        public BackupFolderInfo(string traceType, string backupFolder, bool test_doNotVerifyLogFile = false)
        {
            this.traceType = traceType;
            this.backupFolder = backupFolder;
            this.stateManagerBackupFolderPath = null;
            this.backupVersionList = new List<BackupInfo.BackupVersion>();
            this.backupMetadataFileList = new List<BackupMetadataFile>();
            this.logFileList = new List<string>();

            this.test_doNotVerifyLogFile = test_doNotVerifyLogFile;
        }

        public IList<BackupMetadataFile> BackupMetadataFileList
        {
            get { return this.backupMetadataFileList; }
        }

        public string FullBackupFolderPath
        {
            get { return this.fullBackupFolderPath; }
        }

        public Epoch HighestBackedupEpoch
        {
            get
            {
                Utility.Assert(this.backupMetadataFileList.Count > 0, "HighestBackedupEpoch must not be called if the folder could not be analyzed.");
                int lastIndex = this.backupMetadataFileList.Count - 1;
                return this.backupMetadataFileList[lastIndex].BackupEpoch;
            }
        }

        public long HighestBackedupLsn
        {
            get
            {
                Utility.Assert(this.backupMetadataFileList.Count > 0, "HighestBackedupLsn must not be called if the folder could not be analyzed.");
                int lastIndex = this.backupMetadataFileList.Count - 1;
                return this.backupMetadataFileList[lastIndex].BackupLsn;
            }
        }

        public IList<string> ReplicatorBackupFilePathList
        {
            get { return this.logFileList; }
        }

        public Epoch StartingEpoch
        {
            get { return this.backupMetadataFileList[0].StartingEpoch; }
        }

        public long StartingLsn
        {
            get { return this.backupMetadataFileList[0].StartingLsn; }
        }

        public string StateManagerBackupFolderPath
        {
            get { return this.stateManagerBackupFolderPath; }
        }

        // TODO: Improve.
        public override string ToString()
        {
            return this.backupMetadataFileList[this.backupMetadataFileList.Count - 1].ToString();
        }

        /// <summary>
        /// Reads the input folder to populate, trim and validate the backup folder.
        /// </summary>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous work.</returns>
        public async Task AnalyzeAsync(CancellationToken cancellationToken)
        {
            await this.ReadAsync(cancellationToken).ConfigureAwait(false);
            this.Trim();
            await this.VerifyAsync(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Simply reads the folder to populate the backup folder.
        /// This API does not trim or validate the backup folder.
        /// Note: Using List instead of SortedList to allow same keys added to the List, this is due to 
        /// replicate backup log record may throw transient exception, then user may re-try the 
        /// BackupAsync call and leads to same backups uploaded again. In this case, add to SortedList
        /// will fail since same key is not allowed.
        /// </summary>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous work.</returns>
        /// <exception cref="FabricMissingFullBackupException">A valid full backup folder to be the head of the backup chain could not be found.</exception>
        /// <exception cref="ArgumentException">The relevant folder is not a valid full backup folder.</exception>
        /// <exception cref="InvalidDataException">There is a corruption in the file.</exception>
        internal async Task ReadAsync(CancellationToken cancellationToken)
        {
            string[] list = FabricDirectory.GetFiles(
                this.backupFolder,
                Replicator.Constants.ReplicatorBackupMetadataFileName,
                SearchOption.AllDirectories);
            if (list.Length != 1)
            {
                // Note that we are returning full backup missing if there is no full backup or if there is more than full backup.
                // If this is confusing, we could change this to ArgumentException since it already is in the set of possible exceptions.
                throw new FabricMissingFullBackupException(SR.Error_BackupFolderInfo_MissingFullBackup);
            }

            string fullBackupDirectoryName = Path.GetDirectoryName(list[0]);
            await this.AddFullBackupFolderAsync(fullBackupDirectoryName, cancellationToken).ConfigureAwait(false);

            string[] incrementalBackupMetadataFilePaths = FabricDirectory.GetFiles(
                this.backupFolder,
                Replicator.Constants.ReplicatorIncrementalBackupMetadataFileName,
                SearchOption.AllDirectories);
            foreach (string path in incrementalBackupMetadataFilePaths)
            {
                string incrementalBackupDirectory = Path.GetDirectoryName(path);
                await this.AddIncrementalBackupFolderAsync(incrementalBackupDirectory, cancellationToken).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Add a full backup folder to the chain.
        /// </summary>
        /// <param name="backupFolder">Path of the input full backup folder.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous work.</returns>
        /// <exception cref="ArgumentException">The relevant folder is not a valid full backup folder.</exception>
        /// <exception cref="InvalidDataException">There is a corruption in the file.</exception>
        private async Task AddFullBackupFolderAsync(string backupFolder, CancellationToken cancellationToken)
        {
            Utility.Assert(this.StateManagerBackupFolderPath == null, "There can only be one full backup.");
            Utility.Assert(FabricDirectory.Exists(backupFolder), "Directory must exist.");

            var stateManagerBackupFolderPath = Path.Combine(backupFolder, Replicator.Constants.StateManagerBackupFolderName);
            if (false == FabricDirectory.Exists(stateManagerBackupFolderPath))
            {
                throw new ArgumentException(SR.Error_BackupFolderInfo_NullFolder, "backupFolder");
            }

            this.fullBackupFolderPath = backupFolder;
            this.stateManagerBackupFolderPath = stateManagerBackupFolderPath;

            var metadataPath = Path.Combine(backupFolder, Replicator.Constants.ReplicatorBackupMetadataFileName);

            // Can throw InvalidDataException
            BackupMetadataFile backupMetadataFile;
            try
            {
                backupMetadataFile = await BackupMetadataFile.OpenAsync(metadataPath, cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FabricEvents.Events.RestoreException(
                    this.traceType,
                    Guid.Empty,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);
                throw;
            }

            var version = new BackupInfo.BackupVersion(
                backupMetadataFile.BackupEpoch,
                backupMetadataFile.BackupLsn);
            this.backupVersionList.Add(version);
            this.backupMetadataFileList.Add(backupMetadataFile);

            var logFolderPath = Path.Combine(
                backupFolder,
                Replicator.Constants.ReplicatorBackupFolderName,
                Replicator.Constants.ReplicatorBackupLogName);
            this.logFileList.Add(logFolderPath);
        }

        /// <summary>
        /// Add an incremental backup folder to the chain.
        /// </summary>
        /// <param name="backupFolder">Path of the input full backup folder.</param>
        /// <param name="cancellationToken">Cancellation token.</param>
        /// <returns>Task that represents the asynchronous work.</returns>
        /// <exception cref="InvalidDataException">There is a corruption in the file.</exception>
        private async Task AddIncrementalBackupFolderAsync(string backupFolder, CancellationToken cancellationToken)
        {
            Utility.Assert(
                this.StateManagerBackupFolderPath != null,
                "There must at least be one full backup. Full backup must be added first.");

            var metadataPath = Path.Combine(backupFolder, Replicator.Constants.ReplicatorIncrementalBackupMetadataFileName);

            // Can throw InvalidDataException
            BackupMetadataFile backupMetadataFile;
            try
            {
                backupMetadataFile = await BackupMetadataFile.OpenAsync(metadataPath, cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                FabricEvents.Events.RestoreException(
                    this.traceType,
                    Guid.Empty,
                    e.GetType().ToString(),
                    e.Message,
                    e.StackTrace);
                throw;
            }

            var version = new BackupInfo.BackupVersion(
                backupMetadataFile.BackupEpoch,
                backupMetadataFile.BackupLsn);

            // If item is found, the right position will be returned, otherwise, a negative number that 
            // is the bitwise complement of the index of the next element which is larger than the item or, if there is no
            // larger element, the bitwise complement of list count.
            int pos = backupVersionList.BinarySearch(version);
            int insertPos = pos < 0 ? ~pos : pos;
            this.backupVersionList.Insert(insertPos, version);

            this.backupMetadataFileList.Insert(insertPos, backupMetadataFile);

            var backupLogFilePath = Path.Combine(
                backupFolder,
                Replicator.Constants.ReplicatorBackupFolderName,
                Replicator.Constants.ReplicatorBackupLogName);
            this.logFileList.Insert(insertPos, backupLogFilePath);
        }

        /// <summary>
        /// Function that trims the backup chain to only include the longest chain (removes all unnecessary divergent links).
        /// This is to be able to turn on backup stitching across replicas without requiring customer code change.
        /// As such it only handles issues in the backup chain that can be caused by stitching.
        /// Specifically a shared link can be present in a backup because a replica uploaded the backup folder but failed to 
        /// log/replicate the backup log record.
        /// 
        /// Invariants (for all backups)
        /// 1. A backup folder customer has uploaded, can never have false progress.
        /// 2. All backup data loss numbers across all links in the chain must be same.
        /// 3. First chain in a valid backup chain must start with a full backup and can contain only one full backup.
        /// 
        /// We can only trim when the input chain obeys the following rules
        /// 1. There can be multiple divergent chains from each divergence point, but only one of the divergent chains can have more than one link.
        /// 
        /// </summary>
        /// <devnote>
        /// Terminology
        /// Backup Link:        A backup folder that participates in a backup chain.
        /// Backup Chain:       A set of backup folders that start with a full backup and set of incremental backups that each build on the previous backup.
        /// Divergent Links:    One or more Backup Links that have the same parent.
        /// Shared Link:        A backup link that is parent of multiple backup links.
        /// </devnote>
        /// <exception cref="ArgumentException">Input folder contains a backup chain that cannot be restored.</exception>
        private void Trim()
        {
            Utility.Assert(
                this.backupMetadataFileList != null && this.logFileList != null,
                "{0}: There are no backup folders to trim. Input Folder: {1}",
                this.traceType,
                this.backupFolder);
            Utility.Assert(
                this.backupMetadataFileList.Count == this.logFileList.Count,
                "{0}: Number of log files and metadata files must match. Input Folder: {1}. {2} != {3}",
                this.traceType,
                this.backupFolder,
                this.backupMetadataFileList.Count,
                this.logFileList.Count);

            // Validate that the first backup link in the chain is a full backup.
            BackupMetadataFile headMetadataFile = this.backupMetadataFileList[0];
            if (headMetadataFile.BackupOption != BackupOption.Full)
            {
                var message = string.Format(
                    CultureInfo.CurrentCulture,
                    SR.Error_BackupFolderInfo_MustStartWithFullBackup,
                    this.traceType,
                    this.backupFolder,
                    headMetadataFile.ParentBackupId,
                    headMetadataFile.BackupId,
                    headMetadataFile.BackupEpoch.DataLossNumber,
                    headMetadataFile.BackupEpoch.ConfigurationNumber,
                    headMetadataFile.BackupLsn);
                throw new ArgumentException(message, "backupFolder");
            }

            // If there are less than two backup folders, there is no chain to trim.
            if (this.backupMetadataFileList.Count < 2)
            {
                return;
            }

            // The last backup in the sorted list must be the one with the newest state since it is sorted using the BackupVersion.
            BackupMetadataFile currentMetadataFile = this.backupMetadataFileList[this.backupMetadataFileList.Count - 1];
            int nextIndex = this.backupMetadataFileList.Count - 2;

            // Go back through the chain and remove an divergent branches.
            // Note that head (0) is handled specially because instead of removing we throw an exception:
            // Head which is suppose to be the full backup cannot be trimmed.
            while (nextIndex >= 0)
            {
                BackupMetadataFile nextMetadataFile = this.backupMetadataFileList[nextIndex];
                Utility.Assert(
                    nextIndex == 0 || nextMetadataFile.BackupOption == BackupOption.Incremental, 
                    "{0}: All backups in the chain but the head must be incremental", 
                    this.traceType);

                // Enforcing invariant 2: If data loss number has changed, this is cannot be trimmed or restored.
                if (currentMetadataFile.BackupEpoch.DataLossNumber != nextMetadataFile.BackupEpoch.DataLossNumber)
                {
                    var message = string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_BackupFolderInfo_DifferentDataLossNumber,
                        this.traceType,
                        this.backupFolder,
                        headMetadataFile.ParentBackupId,
                        headMetadataFile.BackupId,
                        headMetadataFile.BackupEpoch.DataLossNumber,
                        headMetadataFile.BackupEpoch.ConfigurationNumber,
                        headMetadataFile.BackupLsn);

                    throw new ArgumentException(message, "backupFolder");
                }

                // Enforcing weakly invariant 1.
                // Note that this also enforces rule 1.
                if (currentMetadataFile.BackupLsn < nextMetadataFile.BackupLsn)
                {
                    var message = string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_BackupFolderInfo_BackupLSNMismatch,
                        nextMetadataFile.BackupLsn,
                        currentMetadataFile.BackupLsn);

                    throw new ArgumentException(message, "backupFolder");
                }

                // If appropriate chain link, continue;
                if (currentMetadataFile.ParentBackupId == nextMetadataFile.BackupId)
                {
                    currentMetadataFile = nextMetadataFile;
                    --nextIndex;
                    continue;
                }

                // If the longest chain does not link into head (full) than throw since full backup cannot be removed.
                if (nextIndex == 0)
                {
                    var message = string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_BackupFolderInfo_IncrementalChainBroken,
                        headMetadataFile.BackupId,
                        currentMetadataFile.ParentBackupId,
                        headMetadataFile.FileName,
                        currentMetadataFile.FileName);
                    throw new ArgumentException(message, "backupFolder");
                }

                // Remove the divergent duplicate link.
                string traceMessage = string.Format("Trimming the following backup chain link. Index: {0} MetadataFile: {1}{2}", nextIndex, Environment.NewLine, nextMetadataFile);
                FabricEvents.Events.Warning(this.traceType, traceMessage);

                this.backupMetadataFileList.RemoveAt(nextIndex);
                this.logFileList.RemoveAt(nextIndex);

                --nextIndex;
            }
        }

        private async Task VerifyAsync(CancellationToken cancellationToken)
        {
            Utility.Assert(this.fullBackupFolderPath != null, "Must at least contain one backup");
            Utility.Assert(this.logFileList.Count > 0, "Must at least contain one backup");

            Utility.Assert(this.backupMetadataFileList != null, "this.orderedBackupMetadataFileList != null");
            Utility.Assert(this.backupMetadataFileList.Count > 0, "Must at least contain one backup");
            Utility.Assert(
                this.backupMetadataFileList.Count == this.logFileList.Count,
                "Must at least contain one backup");

            BackupMetadataFile lastBackupMetadataFile = null;
            foreach (var metadataFile in this.backupMetadataFileList)
            {
                if (lastBackupMetadataFile == null)
                {
                    if (metadataFile.ParentBackupId != metadataFile.BackupId)
                    {
                        var message = string.Format(
                            CultureInfo.CurrentCulture,
                            SR.Error_BackupFolderInfo_FullBackupCorrupted,
                            metadataFile.ParentBackupId,
                            metadataFile.BackupId);
                        throw new InvalidDataException(message);
                    }

                    lastBackupMetadataFile = metadataFile;
                    continue;
                }

                if (lastBackupMetadataFile.BackupId != metadataFile.ParentBackupId)
                {
                    var message = string.Format(
                            CultureInfo.CurrentCulture,
                            SR.Error_BackupFolderInfo_IncrementalChainBroken,
                            lastBackupMetadataFile.BackupId,
                            metadataFile.ParentBackupId,
                            lastBackupMetadataFile.FileName,
                            metadataFile.FileName);
                    throw new ArgumentException(message, "backupFolder");
                }

                if (lastBackupMetadataFile.BackupLsn > metadataFile.BackupLsn)
                {
                    var message = string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_BackupFolderInfo_BackupLSNMismatch,
                        lastBackupMetadataFile.BackupLsn,
                        metadataFile.BackupLsn);

                    throw new InvalidDataException(message);
                }

                lastBackupMetadataFile = metadataFile;
            }

            if (this.test_doNotVerifyLogFile)
            {
                FabricEvents.Events.Warning(this.traceType, "Skipping log file validation because test setting is turned on");
            }
            else
            {
                await this.VerifyLogFilesAsync(cancellationToken).ConfigureAwait(false);
            }
        }

        private async Task VerifyLogFilesAsync(CancellationToken cancellationToken)
        {
            foreach (var backupLogFilePath in this.ReplicatorBackupFilePathList)
            {
                var backupLogFile = await BackupLogFile.OpenAsync(backupLogFilePath, cancellationToken).ConfigureAwait(false);
                await backupLogFile.VerifyAsync().ConfigureAwait(false);
            }
        }
    }
}