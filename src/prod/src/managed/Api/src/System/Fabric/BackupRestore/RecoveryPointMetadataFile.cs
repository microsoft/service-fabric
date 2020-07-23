// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.FileFormat;
using System.Fabric.Common;
using System.Fabric.Interop;
using System.Globalization;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace System.Fabric.BackupRestore
{
    internal class RecoveryPointMetadataFile
    {
        /// <summary>
        /// Serialization version for the RecoveryPointMetadataFile.
        /// </summary>
        private const int Version = 1;

        /// <summary>
        /// Footer for the RecoveryPointMetadataFile.
        /// </summary>
        private FileFooter footer;

        /// <summary>
        /// Properties for the RecoveryPointMetadataFile.
        /// </summary>
        private RecoveryPointMetadataFileProperties properties;

        /// <summary>
        /// Initialize a new instance of the <see cref="RecoveryPointMetadataFile"/> class.
        /// </summary>
        private RecoveryPointMetadataFile(string fileName)
        {
            this.FileName = fileName;
        }

        /// <summary>
        /// Gets the file name.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets the backup location
        /// </summary>
        public string BackupLocation
        {
            get { return this.properties.BackupLocation; }
        }

        /// <summary>
        /// Gets the parent backup location
        /// </summary>
        public string ParentBackupLocation
        {
            get { return this.properties.ParentBackupLocation; }
        }

        /// <summary>
        /// Gets the unique recovery point ID
        /// </summary>
        public Guid BackupId
        {
            get { return this.properties.BackupId; }
        }

        /// <summary>
        /// Gets the unique parent recovery point ID.
        /// Returns Guid.Empty in case this is a full recovery point.
        /// </summary>
        public Guid ParentBackupId
        {
            get { return this.properties.ParentBackupId; }
        }

        /// <summary>
        /// Gets the ID of the backup chain to which this backup belongs to
        /// </summary>
        public Guid BackupChainId
        {
            get { return this.properties.BackupChainId; }
        }

        /// <summary>
        /// Gets the Epoch for the last backup log record
        /// </summary>
        public Epoch EpochOfLastBackupRecord
        {
            get { return this.properties.EpochOfLastBackupRecord; }
        }

        /// <summary>
        /// Gets the LSN for the last backup log record
        /// </summary>
        public long LsnOfLastBackupRecord
        {
            get { return this.properties.LsnOfLastBackupRecord; }
        }

        /// <summary>
        /// Gets the backup time
        /// </summary>
        public DateTime BackupTime
        {
            get { return this.properties.BackupTime; }
        }

        /// <summary>
        /// Gets the partition information
        /// </summary>
        public ServicePartitionInformation PartitionInformation
        {
            get { return this.properties.PartitionInformation; }
        }

        public string ServiceManifestVersion
        {
            get { return this.properties.ServiceManifestVersion; }
        }

        /// <summary>
        /// Create a new <see cref="RecoveryPointMetadataFile"/> and write it to the given file.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public static async Task<RecoveryPointMetadataFile> CreateAsync(
            string fileName,
            DateTime backupTime,
            Guid backupId,
            Guid parentBackupId,
            Guid backupChainId,
            Epoch epochOfLastBackupRecord,
            long lsnOfLastBackupRecord,
            string backupLocation,
            string parentBackupLocation,
            ServicePartitionInformation partitionInformation,
            string serviceManifestVersion,
            CancellationToken cancellationToken)
        {
            // Create the file with asynchronous flag and 4096 cache size (C# default).
            using (var filestream = FabricFile.Open(
                    fileName,
                    FileMode.CreateNew,
                    FileAccess.Write,
                    FileShare.Read,
                    4096,
                    FileOptions.Asynchronous))
            {
                BackupRestoreUtility.SetIoPriorityHint(filestream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);

                var recoveryPointMetadataFile = new RecoveryPointMetadataFile(fileName);
                recoveryPointMetadataFile.properties = new RecoveryPointMetadataFileProperties
                {
                    BackupTime = backupTime,
                    BackupLocation = backupLocation,
                    ParentBackupLocation = parentBackupLocation,
                    BackupId = backupId,
                    ParentBackupId = parentBackupId,
                    BackupChainId = backupChainId,
                    EpochOfLastBackupRecord = epochOfLastBackupRecord,
                    LsnOfLastBackupRecord = lsnOfLastBackupRecord,
                    PartitionInformation = partitionInformation,
                    ServiceManifestVersion = serviceManifestVersion,
                };

                // Write the properties.
                var propertiesHandle =
                    await FileBlock.WriteBlockAsync(filestream, writer => recoveryPointMetadataFile.properties.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the footer.
                recoveryPointMetadataFile.footer = new FileFooter(propertiesHandle, Version);
                await FileBlock.WriteBlockAsync(filestream, writer => recoveryPointMetadataFile.footer.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Flush to underlying stream.
                await filestream.FlushAsync(cancellationToken).ConfigureAwait(false);

                return recoveryPointMetadataFile;
            }
        }

        /// <summary>
        /// Open an existing <see cref="RecoveryPointMetadataFile"/> from the given file path.
        /// </summary>
        /// <param name="recoveryPointMetadataFilePath">Path of the recovery point meta data file.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The read <see cref="RecoveryPointMetadataFile"/>.</returns>
        public static async Task<RecoveryPointMetadataFile> OpenAsync(
            string recoveryPointMetadataFilePath,
            CancellationToken cancellationToken)
        {
            if (string.IsNullOrEmpty(recoveryPointMetadataFilePath))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.CurrentCulture,
                    SR.Error_NullArgument_Formatted,
                    "recoveryPointMetadataFilePath"));
            }

            if (!FabricFile.Exists(recoveryPointMetadataFilePath))
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_FilePath_Null,
                        recoveryPointMetadataFilePath),
                    "recoveryPointMetadataFilePath");
            }

            // Open the file with asynchronous flag and 4096 cache size (C# default).
            using (var filestream = FabricFile.Open(
                recoveryPointMetadataFilePath,
                FileMode.Open,
                FileAccess.Read,
                FileShare.Read,
                4096,
                FileOptions.Asynchronous))
            {
                return await OpenAsync(filestream, recoveryPointMetadataFilePath, cancellationToken);
            }
        }

        public static async Task<RecoveryPointMetadataFile> OpenAsync(
            Stream filestream,
            string recoveryPointMetadataFilePath,
            CancellationToken cancellationToken)
        {
            var recoveryPointMetadataFile = new RecoveryPointMetadataFile(recoveryPointMetadataFilePath);

            // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
            var footerHandle = new BlockHandle(
                filestream.Length - FileFooter.SerializedSize - sizeof(ulong),
                FileFooter.SerializedSize);
            recoveryPointMetadataFile.footer =
                        await
                            FileBlock.ReadBlockAsync(
                                filestream,
                                footerHandle,
                                (reader, handle) => FileFooter.Read(reader, handle)).ConfigureAwait(false);

            cancellationToken.ThrowIfCancellationRequested();

            // Verify we know how to deserialize this version of the backup log file.
            if (recoveryPointMetadataFile.footer.Version != Version)
            {
                throw new InvalidDataException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        SR.Error_BackupMetadata_Deserialized,
                        recoveryPointMetadataFile.footer.Version,
                        Version));
            }

            // Read and validate the properties section.
            var propertiesHandle = recoveryPointMetadataFile.footer.PropertiesHandle;
            recoveryPointMetadataFile.properties =
                            await
                                FileBlock.ReadBlockAsync(
                                    filestream,
                                    propertiesHandle,
                                    (reader, handle) => FileProperties.Read<RecoveryPointMetadataFileProperties>(reader, handle)).ConfigureAwait(false);

            cancellationToken.ThrowIfCancellationRequested();

            return recoveryPointMetadataFile;
        }
    }
}