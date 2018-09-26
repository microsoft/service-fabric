// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Backup Meta data File.
    /// </summary>
    /// <remarks>
    /// Uses Buffer in Memory Stream, issue Asynchronous IO model.
    /// </remarks>
    internal class BackupMetadataFile : IEquatable<BackupMetadataFile>
    {
        /// <summary>
        /// Serialization version for the backup metadata file.
        /// </summary>
        private const int Version = 1;

        /// <summary>
        /// Footer for the BackupMetadataFile.
        /// </summary>
        private FileFooter footer;

        /// <summary>
        /// Properties for the BackupMetadataFile.
        /// </summary>
        private BackupMetadataFileProperties properties;

        /// <summary>
        /// Initialize a new instance of the <see cref="BackupMetadataFile"/> class.
        /// 
        /// Reserved for BackupMetadataFile.CreateAsync and BackupMetadataFile.OpenAsync.
        /// </summary>
        private BackupMetadataFile(string fileName)
        {
            this.FileName = fileName;
        }

        /// <summary>
        /// Gets the backup epoch.
        /// </summary>
        public Epoch BackupEpoch
        {
            get { return this.properties.BackupEpoch; }
        }

        /// <summary>
        /// Gets the backup id.
        /// </summary>
        public Guid BackupId
        {
            get { return this.properties.BackupId; }
        }

        /// <summary>
        /// Gets the backup logical sequence number.
        /// </summary>
        public long BackupLsn
        {
            get { return this.properties.BackupLsn; }
        }

        /// <summary>
        /// Gets the backup option.
        /// </summary>
        public BackupOption BackupOption
        {
            get { return this.properties.BackupOption; }
        }

        /// <summary>
        /// Gets the file name.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets the parent backup id.
        /// </summary>
        public Guid ParentBackupId
        {
            get { return this.properties.ParentBackupId; }
        }

        /// <summary>
        /// Gets the partition id.
        /// </summary>
        public Guid PartitionId
        {
            get { return this.properties.PartitionId; }
        }

        /// <summary>
        /// Gets the replica id.
        /// </summary>
        public long ReplicaId
        {
            get { return this.properties.ReplicaId; }
        }

        /// <summary>
        /// Gets the backup epoch.
        /// </summary>
        public Epoch StartingEpoch
        {
            get { return this.properties.StartingEpoch; }
        }

        /// <summary>
        /// Gets the backup logical sequence number.
        /// </summary>
        public long StartingLsn
        {
            get { return this.properties.StartingLsn; }
        }

        /// <summary>
        /// Create a new <see cref="BackupMetadataFile"/> and write it to the given file.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public static async Task<BackupMetadataFile> CreateAsync(
            string fileName,
            BackupOption backupOption,
            Guid parentBackupId,
            Guid backupId,
            Guid partitionId,
            long replicaId,
            Epoch startingEpoch,
            long startingLsn,
            Epoch backupEpoch,
            long backupLsn,
            CancellationToken cancellationToken)
        {
            // Create the file with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Default IoPriorityHint is used.
            // Reason: Backup is a user operation.
            using (var filestream = FabricFile.Open(
                    fileName,
                    FileMode.CreateNew,
                    FileAccess.Write,
                    FileShare.Read,
                    4096,
                    FileOptions.Asynchronous))
            {
                Utility.SetIoPriorityHint(filestream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);

                var backupMetadataFile = new BackupMetadataFile(fileName);
                backupMetadataFile.properties = new BackupMetadataFileProperties()
                {
                    BackupOption = backupOption,
                    ParentBackupId = parentBackupId,
                    BackupId = backupId,
                    PartitionId = partitionId,
                    ReplicaId = replicaId,
                    StartingEpoch = startingEpoch,
                    StartingLsn = startingLsn,
                    BackupEpoch = backupEpoch,
                    BackupLsn = backupLsn,
                };

                // Write the properties.
                var propertiesHandle =
                    await FileBlock.WriteBlockAsync(filestream, writer => backupMetadataFile.properties.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the footer.
                backupMetadataFile.footer = new FileFooter(propertiesHandle, Version);
                await FileBlock.WriteBlockAsync(filestream, writer => backupMetadataFile.footer.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Flush to underlying stream.
                await filestream.FlushAsync(cancellationToken).ConfigureAwait(false);

                return backupMetadataFile;
            }
        }

        /// <summary>
        /// Open an existing <see cref="BackupMetadataFile"/> from the given file path.
        /// </summary>
        /// <param name="backupMetadataFilePath">Path of the backup meta data file.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The read <see cref="BackupMetadataFile"/>.</returns>
        public static async Task<BackupMetadataFile> OpenAsync(
            string backupMetadataFilePath,
            CancellationToken cancellationToken)
        {
            if (string.IsNullOrEmpty(backupMetadataFilePath))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture, 
                    SR.Error_NullArgument_Formatted, 
                    "backupMetadataFilePath"));
            }

            if (!FabricFile.Exists(backupMetadataFilePath))
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_FilePath_Null,
                        backupMetadataFilePath),
                    "backupMetadataFilePath");
            }

            // Open the file with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Default IoPriorityHint is used since this operation is called during restore.
            using (var filestream = FabricFile.Open(
                    backupMetadataFilePath,
                    FileMode.Open,
                    FileAccess.Read,
                    FileShare.Read,
                    4096,
                    FileOptions.Asynchronous))
            {
                var backupMetadataFile = new BackupMetadataFile(backupMetadataFilePath);

                // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
                var footerHandle = new BlockHandle(
                    filestream.Length - FileFooter.SerializedSize - sizeof(ulong),
                    FileFooter.SerializedSize);
                backupMetadataFile.footer =
                    await
                        FileBlock.ReadBlockAsync(
                            filestream,
                            footerHandle,
                            (reader, handle) => FileFooter.Read(reader, handle)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Verify we know how to deserialize this version of the backup log file.
                if (backupMetadataFile.footer.Version != Version)
                {
                    throw new InvalidDataException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            SR.Error_BackupMetadata_Deserialized,
                            backupMetadataFile.footer.Version,
                            Version));
                }

                // Read and validate the properties section.
                var propertiesHandle = backupMetadataFile.footer.PropertiesHandle;
                backupMetadataFile.properties =
                    await
                        FileBlock.ReadBlockAsync(
                            filestream,
                            propertiesHandle,
                            (reader, handle) => FileProperties.Read<BackupMetadataFileProperties>(reader, handle)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                return backupMetadataFile;
            }
        }

        /// <summary>
        /// Determines whether two BackupMetadataFile objects have the same value.
        /// </summary>
        /// <param name="other">The BackupMetadataFile to compare to this instance.</param>
        /// <returns>
        /// true if the value of the value parameter is the same as the value of this instance; otherwise, false. 
        /// If value is null, the method returns false.
        /// </returns>
        public bool Equals(BackupMetadataFile other)
        {
            if (other == null)
            {
                return false;
            }

            return this.BackupOption == other.BackupOption && this.BackupId == other.BackupId
                   && this.PartitionId == other.PartitionId && this.ReplicaId == other.ReplicaId
                   && this.BackupEpoch == other.BackupEpoch && this.BackupLsn == other.BackupLsn;
        }

        /// <summary>
        /// Determines whether two BackupMetadataFile objects have the same value.
        /// </summary>
        /// <param name="obj">The object to compare to this instance.</param>
        /// <returns>
        /// true if the value of the value parameter is the same as the value of this instance; otherwise, false. 
        /// If value is null, the method returns false.
        /// </returns>
        public override bool Equals(object obj)
        {
            var other = obj as BackupMetadataFile;

            if (other == null)
            {
                return false;
            }

            return this.Equals(other);
        }

        /// <summary>
        /// Returns the hash code for this backup meta data file.
        /// </summary>
        /// <returns>The hash code.</returns>
        public override int GetHashCode()
        {
            return Version.GetHashCode() ^ this.BackupOption.GetHashCode() ^ this.BackupId.GetHashCode()
                   ^ this.PartitionId.GetHashCode() ^ this.ReplicaId.GetHashCode() ^ this.BackupEpoch.GetHashCode()
                   ^ this.BackupLsn.GetHashCode();
        }

        /// <summary>
        /// Returns the string representation of the class.
        /// </summary>
        /// <returns>String that represents the object.</returns>
        public override string ToString()
        {
            return
                string.Format(
                    "Version: {0} BackupId: {1} BackupOption: {2} ParentBackupId: {3} PartitionId: {4} ReplicaId: {5} BackupEpoch: <{6}, {7}> BackupLsn: {8} Path: {9}.",
                    Version,
                    this.BackupId,
                    this.BackupOption,
                    this.ParentBackupId,
                    this.PartitionId,
                    this.ReplicaId,
                    this.BackupEpoch.DataLossNumber,
                    this.BackupEpoch.ConfigurationNumber,
                    this.BackupLsn,
                    this.FileName);
        }
    }
}