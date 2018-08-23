// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Disk Metadata.
    /// </summary>
    internal static class MetadataManager
    {
        public const int FileVersion = 1;
        public const int MemoryBufferFlushSize = 32*1024;

        private const string currentDiskMetadataFileName = "current_metadata.mmt";
        private const string tempDiskMetadataFileName = "temp_metadata.mmt";
        private const string bkpDiskMetadataFileName = "backup_metadata.mmt";

        public static void AddFile(Dictionary<uint, FileMetadata> metadataTable, uint fileId, FileMetadata metadata)
        {
            if (metadataTable.ContainsKey(fileId))
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, SR.Error_MetadataManager_FileExists, fileId));
            }

            metadataTable.Add(fileId, metadata);
        }
        public static void RemoveFile(Dictionary<uint, FileMetadata> metadataTable, uint fileId, string traceType)
        {
            Diagnostics.Assert(metadataTable.ContainsKey(fileId), traceType, "Fild id {0} does not exists", fileId);
            metadataTable.Remove(fileId);
        }

        public static Task<TValue> ReadValueAsync<TValue>(
            Dictionary<uint, FileMetadata> metadataTable, TVersionedItem<TValue> item, IStateSerializer<TValue> valueSerializer,
            CancellationToken cancellationToken,
            string traceType)
        {
            FileMetadata metadata = null;
            if (!metadataTable.TryGetValue(item.FileId, out metadata))
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, SR.Error_MetadataManager_FileDoesNotExist, item.FileId));
            }

            // Consistency checks.
            Diagnostics.Assert(metadata.CheckpointFile != null, traceType, "Checkpoint file with id '{0}' does not exist in memory.", item.FileId);

            // Read from disk.
            return ReadValueAsync<TValue>(metadata, item, valueSerializer, cancellationToken, traceType);
        }

        public static Task<TValue> ReadValueAsync<TValue>(
            FileMetadata fileMetadata, TVersionedItem<TValue> item, IStateSerializer<TValue> valueSerializer, CancellationToken cancellationToken, string traceType)
        {
            // Consistency checks.
            Diagnostics.Assert(fileMetadata.CheckpointFile != null, traceType, "Checkpoint file with id '{0}' does not exist in memory.", item.FileId);

            // Read from disk.
            return fileMetadata.CheckpointFile.ReadValueAsync<TValue>(item, valueSerializer);
        }

        public static Task<byte[]> ReadValueAsync<TValue>(
            Dictionary<uint, FileMetadata> metadataTable, TVersionedItem<TValue> item, CancellationToken cancellationToken, string traceType)
        {
            FileMetadata fileMetadata = null;
            if (!metadataTable.TryGetValue(item.FileId, out fileMetadata))
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, SR.Error_MetadataManager_FileDoesNotExist, item.FileId));
            }

            // Consistency checks.
            Diagnostics.Assert(fileMetadata.CheckpointFile != null, traceType, "Checkpoint file with id '{0}' does not exist in memory.", item.FileId);

            // Read from disk.
            return fileMetadata.CheckpointFile.ReadValueAsync<TValue>(item);
        }

        public static async Task OpenAsync(MetadataTable metadataTable, string path, string traceType)
        {
            // assert count is zero.
            Diagnostics.Assert(metadataTable.Table.Count == 0, traceType, "Count should be zero");

            // MCoskun: Default IoPriorityHint is used.
            // Reason: Used during recovery, restore and backup. These operations either might be required to reach write quorum or is a customer requested operation.
            using (var filestream = FabricFile.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous))
            {
                await PopulateMetadataAsync(metadataTable, filestream, traceType).ConfigureAwait(false);
                metadataTable.MetadataFileSize = filestream.Length;
            }
        }

        public static async Task OpenAsync(MetadataTable metadataTable, Stream stream, string traceType)
        {
            // assert count is zero.
            Diagnostics.Assert(metadataTable.Table.Count == 0, traceType, "Count should be zero");

            await PopulateMetadataAsync(metadataTable, stream, traceType).ConfigureAwait(false);
        }

        internal static async Task PopulateMetadataAsync(MetadataTable metadataTable, string path, string traceType)
        {
            // MCoskun: Unit test only API hence uses default IO priority.
            using (var filestream = FabricFile.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous))
            {
                await PopulateMetadataAsync(metadataTable, filestream, traceType);
                metadataTable.MetadataFileSize = filestream.Length;
            }
        }

        internal static async Task ValidateAsync(string path, string traceType)
        {
            // MCoskun: Open of metadata file's IO priority is not set.
            // Reason: USed during CompleteCheckpoint which can block copy and backup.
            using (var stream = FabricFile.Open(path, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous))
            {
                // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
                var footerHandle = new BlockHandle(stream.Length - FileFooter.SerializedSize - sizeof(ulong), FileFooter.SerializedSize);
                var footer = await FileBlock.ReadBlockAsync(stream, footerHandle, (sectionReader, sectionHandle) => FileFooter.Read(sectionReader, sectionHandle)).ConfigureAwait(false);

                // Verify we know how to deserialize this version of the file.
                if (footer.Version != FileVersion)
                {
                    throw new InvalidDataException(SR.Error_MetadataManager_Deserialized);
                }

                // Read and validate the Properties section.
                var propertiesHandle = footer.PropertiesHandle;
                var properties = await FileBlock.ReadBlockAsync(
                            stream,
                            propertiesHandle,
                            (sectionReader, sectionHandle) => FilePropertySection.Read<MetadataManagerFileProperties>(sectionReader, sectionHandle)).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <remarks>Exposed for testability.</remarks>
        internal static async Task PopulateMetadataAsync(MetadataTable metadataTable, Stream stream, string traceType)
        {
            // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
            var footerHandle = new BlockHandle(stream.Length - FileFooter.SerializedSize - sizeof(ulong), FileFooter.SerializedSize);
            var footer = await FileBlock.ReadBlockAsync(stream, footerHandle, (sectionReader, sectionHandle) => FileFooter.Read(sectionReader, sectionHandle)).ConfigureAwait(false);

            // Verify we know how to deserialize this version of the file.
            if (footer.Version != FileVersion)
            {
                throw new InvalidDataException(SR.Error_MetadataManager_Deserialized);
            }

            // Read and validate the Properties section.
            var propertiesHandle = footer.PropertiesHandle;
            var properties =
                await
                    FileBlock.ReadBlockAsync(
                        stream,
                        propertiesHandle,
                        (sectionReader, sectionHandle) => FilePropertySection.Read<MetadataManagerFileProperties>(sectionReader, sectionHandle)).ConfigureAwait(false);

            // Read disk metadata into memory.
            await ReadDiskMetadataAsync(metadataTable.Table, stream, properties, traceType).ConfigureAwait(false);
            metadataTable.CheckpointLSN = properties.CheckpointLSN;
        }

        public static async Task WriteAsync(MetadataTable metadataTable, string path)
        {
            // MCoskun: Default IoPriorityHint is used.
            // Reason: Used during Perform checkpoint to write the new metadata file. Perform checkpoint should complete as fast as possible.
            using (var filestream = FabricFile.Open(path, FileMode.Create, FileAccess.Write, FileShare.Write, 4096, FileOptions.Asynchronous))
            {
                await WriteAsync(metadataTable, filestream).ConfigureAwait(false);
                metadataTable.MetadataFileSize = filestream.Length;
            }
        }

        public static async Task WriteAsync(MetadataTable metadataTable, Stream stream)
        {
            // Write all metadata to disk.
            var properties = await WriteDiskMetadataAsync(metadataTable.Table, stream).ConfigureAwait(false);
            properties.CheckpointLSN = metadataTable.CheckpointLSN;

            // Write the Properties.
            var propertiesHandle = await FileBlock.WriteBlockAsync(stream, (sectionWriter) => properties.Write(sectionWriter)).ConfigureAwait(false);

            // Write the Footer.
            var footer = new FileFooter(propertiesHandle, FileVersion);
            await FileBlock.WriteBlockAsync(stream, (sectionWriter) => footer.Write(sectionWriter)).ConfigureAwait(false);

            // Finally, flush to disk.
            await stream.FlushAsync().ConfigureAwait(false);
        }

        /// <summary>
        /// Read all file metadata from the metadata file.
        /// </summary>
        /// <param name="metadataTable">The metadata table.</param>
        /// <param name="filestream">The file stream to read from.</param>
        /// <param name="properties">The metadata manager file properties.</param>
        /// <param name="traceType">Tracing information.</param>
        /// <returns></returns>
        private static async Task<int> ReadDiskMetadataAsync(
            Dictionary<uint, FileMetadata> metadataTable, Stream filestream, MetadataManagerFileProperties properties, string traceType)
        {
            var startOffset = properties.MetadataHandle.Offset;
            var endOffset = properties.MetadataHandle.EndOffset;
            var metadataCount = 0;
            var fileId = 0;

            // No metadata to read (there are no metadata chunks).
            if (startOffset + sizeof(int) >= endOffset)
            {
                return fileId;
            }

            filestream.Position = startOffset;

            using (var metadataStream = new MemoryStream(capacity: 64*1024))
            using (var metadataReader = new InMemoryBinaryReader(metadataStream))
            {
                // Read the first key chunk size into memory.
                metadataStream.SetLength(64*1024);
                await filestream.ReadAsync(metadataStream.GetBuffer(), 0, PropertyChunkMetadata.Size).ConfigureAwait(false);
                var propertyChunkMetadata = PropertyChunkMetadata.Read(metadataReader);
                var chunkSize = propertyChunkMetadata.BlockSize;
                filestream.Position -= PropertyChunkMetadata.Size;

                while (filestream.Position + chunkSize + sizeof(ulong) <= endOffset)
                {
                    // Consistency checks.
                    if (chunkSize < 0)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_Metadata_Corrupt_NegativeSize_OneArgs, chunkSize));
                    }

                    // Read the entire chunk (plus the checksum and next chunk size) into memory.
                    metadataStream.SetLength(chunkSize + sizeof(ulong) + sizeof(int));
                    await filestream.ReadAsync(metadataStream.GetBuffer(), 0, chunkSize + sizeof(ulong) + sizeof(int)).ConfigureAwait(false);

                    // Read the checksum.
                    metadataStream.Position = chunkSize;
                    var checksum = metadataReader.ReadUInt64();

                    // Re-compute the checksum.
                    var expectedChecksum = CRC64.ToCRC64(metadataStream.GetBuffer(), 0, chunkSize);
                    if (checksum != expectedChecksum)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_Metadata_Corrupt_ChecksumMismatch_TwoArgs, checksum, expectedChecksum));
                    }

                    // Deserialize the value into memory.
                    metadataStream.Position = sizeof(int);
                    metadataReader.ReadPaddingUntilAligned(true);
                    while (metadataStream.Position < chunkSize)
                    {
                        var fileMetadata = FileMetadata.Read(metadataReader, traceType);
                        if (metadataTable.ContainsKey(fileMetadata.FileId))
                        {
                            throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_DuplicateFileId_Found_OneArgs, fileMetadata.FileId));
                        }

                        metadataTable.Add(fileMetadata.FileId, fileMetadata);
                        metadataCount++;
                    }

                    // Read the next chunk size.
                    chunkSize = BitConverter.ToInt32(metadataStream.GetBuffer(), chunkSize + sizeof(ulong));
                    filestream.Position -= sizeof(int);
                }

                // Consistency checks.
                if (filestream.Position != endOffset)
                {
                    throw new InvalidDataException(SR.Error_Metadata_Corrupt_IncorrectSize);
                }

                if (metadataCount != properties.FileCount)
                {
                    throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_Metadata_Corrupt_FileCountMismatch_TwoArgs, metadataCount, properties.FileCount));
                }

                return fileId;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <remarks>On open, metadata table needs to be created.</remarks>
        private static async Task<MetadataManagerFileProperties> WriteDiskMetadataAsync(Dictionary<uint, FileMetadata> metadataTable, Stream outputstream)
        {
            var properties = new MetadataManagerFileProperties();

            var startOfChunk = true;
            using (var metadataStream = new MemoryStream(capacity: 64*1024))
            using (var metadataWriter = new InMemoryBinaryWriter(metadataStream))
            {
                // Buffer metadata into memory, and periodically flush chunks to disk.
                foreach (var metadata in metadataTable.Values)
                {
                    if (startOfChunk)
                    {
                        // Leave space for an int 'size' at the front of the memory stream.
                        // Leave space for size and RESERVED
                        metadataStream.Position += PropertyChunkMetadata.Size;

                        startOfChunk = false;
                    }

                    metadata.Write(metadataWriter);
                    properties.FileCount++;

                    // Flush the memory buffer periodically to disk.
                    if (metadataStream.Position >= MemoryBufferFlushSize)
                    {
                        await FlushMemoryBufferAsync(outputstream, metadataWriter).ConfigureAwait(false);
                        startOfChunk = true;
                    }
                }

                // If there's any remaining buffered metadata in memory, flush them to disk.
                if (metadataStream.Position > PropertyChunkMetadata.Size)
                {
                    await FlushMemoryBufferAsync(outputstream, metadataWriter).ConfigureAwait(false);
                }
            }

            // Update properties.
            properties.MetadataHandle = new BlockHandle(offset: 0, size: outputstream.Position);

            return properties;
        }

        /// <summary>
        /// Replaces current master table file with the next file and handles intermittent failures of ReplaceW.
        /// SafeFileReplaceAsync assumes that the tempFilePath already exists.
        /// </summary>
        /// <returns></returns>
        internal static async Task SafeFileReplaceAsync(string currentFilePath, string bkpFilePath, string tempFilePath, string traceType)
        {
            // Temp should always exist
            // Current or Backup may or may not exist
            //  1. Non exists : First ever checkpoint
            //  2. Current exists but no backup : Last safe file replace went through without any failure.
            //  3. Backup exists but no current : Last safe file replace in the replace step. Moved the current to backup but could not move temp to current.
            //  4. Both current and backup exist : A previous safe file replace failed after moving current to backup and temp to current but before backup could be deleted.
            //      The CompleteCheckpointAsync was retried but as next does not exist, SafeFileReplace was not executed. Hence backup could not be completed.
            //      A new checkpoint was taken at a later stage. Current, Backup already existed and Temp was added. So all 3 exist now.
            var currentExists = FabricFile.Exists(currentFilePath);
            var tempExists = FabricFile.Exists(tempFilePath);
            var bkpExists = FabricFile.Exists(bkpFilePath);

            FabricEvents.Events.CompleteCheckpointAsync(
                traceType,
                string.Format(
                    "Safe file replace starting. CurrentFilePath : {0}, TempFilePath : {1}, BackupFilePath : {2}, CurrentExists : {3}, TempExists : {4}, BackupExists : {5}",
                    currentFilePath,
                    tempFilePath,
                    bkpFilePath,
                    currentExists,
                    tempExists,
                    bkpExists));

            // SafeFileReplace assumes that the temp checkpoint file always exists
            Diagnostics.Assert(
                tempExists,
                traceType,
                string.Format("Temp checkpoint file does not exist. TempFilePath : {0}", tempFilePath));

            // If the backup file exists, or the checkpoint file does not exist, then we need to validate the temp file before we delete the backup.
            // If the temp is corrupted, this would the best effort to save the last good checkpoint file.
            // In case the temp is corrupted, we throw and the replica would get stuck in an open loop.
            //  Mitigation for this would be to copy all the checkpoint files for RCA. Then force drop the replica and do manual cleanup for the same.
            if (bkpExists || !currentExists)
            {
                try
                {
                    // Check if the next file is valid by opening and reading it.
                    await ValidateAsync(tempFilePath, traceType).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    FabricEvents.Events.SafeFileReplace(
                        traceType,
                        "MetadataManager.SafeFileReplaceAsync failed due to temp file validation check failure. Exception : ",
                        ex.ToString());
                    throw;
                }
            }

            // If a previous iteration failed, the backup file might exist.
            // FabricFile.Move does not overwrite the target. If the target exists, FabricFile.Move will throw with an error code 0x800700B7.
            // Hence we need to remove the bkpFilePath before we start the file replace.
            // Fix for ICM 43706105.
            if (bkpExists)
            {
                FabricFile.Delete(bkpFilePath);
            }

            // Previous replace could have failed in the middle before the next metadata table file got renamed to current.
            if (!currentExists)
            {
                FabricFile.Move(tempFilePath, currentFilePath);
            }
            else
            {
                // Note: Using FabricFile.Replace causes security issues during subsequent FileReplace for the current checkpoint file (second FileReplace will throw AccessDenied Exception) in container scenarios.
                FabricFile.Move(currentFilePath, bkpFilePath);
                FabricFile.Move(tempFilePath, currentFilePath);

                // Since the move completed successfully, we need to delete the bkpFilePath. This is best-effort cleanup.
                FabricFile.Delete(bkpFilePath);
            }

            FabricEvents.Events.CompleteCheckpointAsync(traceType, "safe file replace: completed");
        }

        private static async Task FlushMemoryBufferAsync(Stream fileStream, InMemoryBinaryWriter memoryBuffer)
        {
            // Write the chunk length at the beginning of the stream.
            var chunkSize = checked((int) memoryBuffer.BaseStream.Position);
            memoryBuffer.BaseStream.Position = 0;
            var propertyChunkMetadata = new PropertyChunkMetadata(chunkSize);
            propertyChunkMetadata.Write(memoryBuffer);

            // Move to end.
            memoryBuffer.BaseStream.Position = chunkSize;

            // Checksum the chunk.
            var checksum = memoryBuffer.GetChecksum();
            memoryBuffer.Write(checksum);

            // Write to disk.
            await memoryBuffer.WriteAsync(fileStream).ConfigureAwait(false);

            // Reset the memory buffer.
            memoryBuffer.BaseStream.Position = 0;
        }
    }
}