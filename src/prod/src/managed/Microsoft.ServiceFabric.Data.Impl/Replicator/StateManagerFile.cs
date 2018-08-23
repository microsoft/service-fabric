// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
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
    using Data;

    /// <summary>
    /// StateManager checkpoint file format.
    /// </summary>
    internal sealed class StateManagerFile
    {
        /// <summary>
        /// Copy protocol version for the <see cref="StateManagerFile"/>.
        /// </summary>
        private const int CopyProtocolVersion = 1;

        /// <summary>
        /// Desired size for a block of metadata before checksumming and flushing. 32 KB.
        /// </summary>
        private const int DesiredBlockSize = 32*1024;

        /// <summary>
        /// Serialization version for the <see cref="StateManagerFile"/>.
        /// </summary>
        private const int Version = 1;

        /// <summary>
        /// The file path for the <see cref="StateManagerFile"/>.
        /// </summary>
        private string filePath;

        /// <summary>
        /// Footer for the <see cref="StateManagerFile"/>.
        /// </summary>
        private FileFooter footer;

        /// <summary>
        /// Properties for the <see cref="StateManagerFile"/>.
        /// </summary>
        private StateManagerFileProperties properties;

        /// <summary>
        /// Initializes a new instance of the <see cref="StateManagerFile"/> class.
        /// </summary>
        /// <param name="filePath">Path to the checkpoint file.</param>
        private StateManagerFile(string filePath)
        {
            // Reserved for StateManagerFile.CreateAsync and StateManagerFile.OpenAsync
            this.filePath = filePath;
        }

        /// <summary>
        /// Gets the file size.
        /// </summary>
        public long FileSize { get; private set; }

        /// <summary>
        /// Gets the Properties for the file.
        /// </summary>
        public StateManagerFileProperties Properties
        {
            get { return this.properties; }

            private set { this.properties = value; }
        }

        /// <summary>
        /// Gets the state provider metadata for the file.
        /// </summary>
        public IReadOnlyList<SerializableMetadata> StateProviderMetadata { get; private set; }

        /// <summary>
        /// Create a new <see cref="StateManagerFile"/> and write it to the given file.
        /// </summary>
        /// <param name="filePath">Path to the checkpoint file.</param>
        /// <param name="partitionId">Service partition Id.</param>
        /// <param name="replicaId">Service replica Id.</param>
        /// <param name="metadataList">The state providers' metadata.</param>
        /// <param name="allowPrepareCheckpointLSNToBeInvalid">
        /// Allow the prepare checkpoint LSN to be invalid. If it is invalid, do not write it.
        /// </param>
        /// <param name="prepareCheckpointLSN">The PrepareCheckpointLSN.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>The new <see cref="BackupLogFile"/>.</returns>
        public static async Task<StateManagerFile> CreateAsync(
            string filePath,
            Guid partitionId,
            long replicaId,
            List<SerializableMetadata> metadataList,
            bool allowPrepareCheckpointLSNToBeInvalid,
            long prepareCheckpointLSN,
            CancellationToken cancellationToken)
        {
            // #12249219: Without the "allowPrepareCheckpointLSNToBeInvalid" being set to true in the backup code path, 
            // It is possible for all backups before the first checkpoint after the upgrade to fail.
            // If the code has the PrepareCheckpointLSN property, it must be larger than or equal to ZeroLSN(0).
            Utility.Assert(
                allowPrepareCheckpointLSNToBeInvalid || prepareCheckpointLSN >= StateManagerConstants.ZeroLSN,
                "{0}:{1} CreateAsync: In the write path, prepareCheckpointLSN must be larger or equal to 0. PrepareCheckpointLSN: {2}.",
                partitionId,
                replicaId,
                prepareCheckpointLSN);

            // Create the file with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Default IoPriorityHint is used.
            // Reason: Used during backup and checkpoint (user operation and life-cycle operation respectively).
            using (var filestream = FabricFile.Open(
                    filePath,
                    FileMode.Create,
                    FileAccess.Write,
                    FileShare.Write,
                    4096,
                    FileOptions.Asynchronous))
            {
                var stateManagerFile = new StateManagerFile(filePath);
                stateManagerFile.StateProviderMetadata = metadataList;

                bool shouldNotWritePrepareCheckpointLSN = prepareCheckpointLSN == StateManagerConstants.InvalidLSN ? true : false;

                stateManagerFile.properties = new StateManagerFileProperties(shouldNotWritePrepareCheckpointLSN);
                stateManagerFile.properties.PartitionId = partitionId;
                stateManagerFile.properties.ReplicaId = replicaId;

                if (shouldNotWritePrepareCheckpointLSN == false)
                {
                    stateManagerFile.properties.PrepareCheckpointLSN = prepareCheckpointLSN;
                }

                // Write the state provider metadata.
                var blocks = await stateManagerFile.WriteMetadataAsync(filestream, metadataList, cancellationToken).ConfigureAwait(false);
                stateManagerFile.properties.MetadataHandle = new BlockHandle(offset: 0, size: filestream.Position);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the blocks and update the properties.
                stateManagerFile.properties.BlocksHandle =
                    await FileBlock.WriteBlockAsync(filestream, (sectionWriter) => blocks.Write(sectionWriter)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the properties.
                var propertiesHandle =
                    await FileBlock.WriteBlockAsync(filestream, writer => stateManagerFile.properties.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the footer.
                stateManagerFile.footer = new FileFooter(propertiesHandle, Version);
                await FileBlock.WriteBlockAsync(filestream, writer => stateManagerFile.footer.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Ensure we fully flush the data to disk before returning.
                await filestream.FlushAsync().ConfigureAwait(false);

                stateManagerFile.FileSize = filestream.Length;
                return stateManagerFile;
            }
        }

        /// <summary>
        /// Open an existing <see cref="StateManagerFile"/> from the given file path.
        /// </summary>
        /// <param name="filePath">Path to the checkpoint file.</param>
        /// <param name="traceType">Tracing information.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>The opened <see cref="StateManagerFile"/>.</returns>
        public static Task<StateManagerFile> OpenAsync(
            string filePath,
            string traceType,
            CancellationToken cancellationToken)
        {
            return OpenAsync(filePath, traceType, true, cancellationToken);
        }

        /// <summary>
        /// Open an existing <see cref="StateManagerFile"/> from the given file path.
        /// </summary>
        /// <param name="filePath">Path to the checkpoint file.</param>
        /// <param name="traceType">Tracing information.</param>
        /// <param name="deserializeTypes">Should the type information be deserialized.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>The opened <see cref="StateManagerFile"/>.</returns>
        internal static async Task<StateManagerFile> OpenAsync(
            string filePath,
            string traceType,
            bool deserializeTypes,
            CancellationToken cancellationToken)
        {
            if (string.IsNullOrEmpty(filePath))
            {
                throw new ArgumentException(SR.Error_FilePath_Null_NoArgs);
            }

            if (!FabricFile.Exists(filePath))
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, SR.Error_FilePath_Null, filePath));
            }

            // Open the file with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Default IoPriorityHint is used.
            // Reason: Used during open, restore and complete checkpoint. 
            using (var filestream = FabricFile.Open(
                    filePath,
                    FileMode.Open,
                    FileAccess.Read,
                    FileShare.Read,
                    4096,
                    FileOptions.Asynchronous))
            {
                var stateManagerFile = new StateManagerFile(filePath);

                // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
                var footerHandle = new BlockHandle(
                    filestream.Length - FileFooter.SerializedSize - sizeof(ulong),
                    FileFooter.SerializedSize);
                stateManagerFile.footer = await FileBlock.ReadBlockAsync(
                            filestream,
                            footerHandle,
                            (reader, handle) => FileFooter.Read(reader, handle)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Verify we know how to deserialize this version of the state manager checkpoint file.
                if (stateManagerFile.footer.Version != Version)
                {
                    throw new InvalidDataException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            "StateManager checkpoint file cannot be deserialized (unknown version number: {0}, expected version number: {1})",
                            stateManagerFile.footer.Version,
                            Version));
                }

                // Read and validate the properties section.
                var propertiesHandle = stateManagerFile.footer.PropertiesHandle;
                stateManagerFile.properties = await FileBlock.ReadBlockAsync(
                            filestream,
                            propertiesHandle,
                            (reader, handle) => FileProperties.Read<StateManagerFileProperties>(reader, handle)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Read the state provider metadata.
                stateManagerFile.StateProviderMetadata = await stateManagerFile.ReadMetadataAsync(filestream, traceType, deserializeTypes, cancellationToken).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                stateManagerFile.FileSize = filestream.Length;
                return stateManagerFile;
            }
        }

        /// <summary>
        /// Moves the temporary checkpoint file to the checkpoint file.
        /// </summary>
        /// <returns>Asynchronous operation the represents the file replace.</returns>
        /// <remarks>
        /// Code depends on only being called in cases where next (tmp) checkpoint exists.
        /// </remarks>
        public static async Task SafeFileReplaceAsync(
            string checkpointFileName,
            string temporaryCheckpointFileName,
            string backupCheckpointFileName,
            string traceType)
        {
            if (string.IsNullOrEmpty(checkpointFileName))
            {
                throw new ArgumentException("Checkpoint file name is null or empty.", "checkpointFileName");
            }

            if (string.IsNullOrEmpty(temporaryCheckpointFileName))
            {
                throw new ArgumentException("Temporary file name is null or empty.", "temporaryCheckpointFileName");
            }

            if (string.IsNullOrEmpty(backupCheckpointFileName))
            {
                throw new ArgumentException("Backup file name is null or empty.", "backupCheckpointFileName");
            }

            // Delete previous backup, if it exists.
            if (FabricFile.Exists(backupCheckpointFileName))
            {
                FabricFile.Delete(backupCheckpointFileName);
            }

            // Previous replace could have failed in the middle before the next checkpoint file got renamed to current.
            if (!FabricFile.Exists(checkpointFileName))
            {
                // Validate the next file is complete (this will throw InvalidDataException if it's not valid).
                var nextCheckpointFile = await OpenAsync(temporaryCheckpointFileName, traceType, CancellationToken.None).ConfigureAwait(false);

                // Next checkpoint file is valid.  Move it to be current.
                // MCoskun: Note that FabricFile.Move is MOVEFILE_WRITE_THROUGH by default.
                // Note using this flag can cause undetected dataloss.
                FabricFile.Move(temporaryCheckpointFileName, checkpointFileName);
            }
            else
            {
                // Current exists, so we must have gotten here only after writing a valid next checkpoint file.
                // MCoskun: Note that FabricFile.Move is MOVEFILE_WRITE_THROUGH by default.
                // NTFS guarantees that this operation will not be lost as long as MOVEFILE_WRITE_THROUGH is on.
                // If NTFS metadata log's tail is not flushed yet, it will be flushed before this operation is logged with FUA.
                // Note that ReplaceFile is not guaranteed to be persisted even after it returns.
                // #9291020: Using ReplaceFile here instead of MoveFile with MOVEFILE_WRITE_THROUGH can cause data-loss
                FabricFile.Move(checkpointFileName, backupCheckpointFileName);
                FabricFile.Move(temporaryCheckpointFileName, checkpointFileName);

                // Delete the backup file.
                FabricFile.Delete(backupCheckpointFileName);
            }
        }

        /// <summary>
        /// Gets the next chunk of copy data from the in-memory state provider metadata enumerator.
        /// </summary>
        /// <param name="stateEnumerator">State provider metadata enumerator.</param>
        /// <param name="traceType">Tracing type information.</param>
        /// <returns>The next chunk of copy data.</returns>
        internal static IEnumerable<OperationData> GetCopyData(
            IEnumerator<SerializableMetadata> stateEnumerator,
            string traceType)
        {
            var metadataCount = 0;

            // Create a single re-usable memory stream for Copy.
            using (var itemStream = new MemoryStream(DesiredBlockSize))
            using (var itemWriter = new InMemoryBinaryWriter(itemStream))
            {
                // Send the copy operation protocol version number first.
                itemWriter.Write(CopyProtocolVersion);
                itemWriter.Write((byte) StateManagerCopyOperation.Version);
                yield return
                    new OperationData(
                        new ArraySegment<byte>(itemStream.GetBuffer(), 0, checked((int) itemStream.Position)));

                // Send the state provider metadata in chunks.
                itemStream.Position = 0;
                while (stateEnumerator.MoveNext())
                {
                    WriteMetadata(itemWriter, stateEnumerator.Current);
                    metadataCount++;

                    if (itemStream.Position >= DesiredBlockSize)
                    {
                        // Send a chunk of state provider metadata.
                        itemWriter.Write((byte) StateManagerCopyOperation.StateProviderMetadata);
                        yield return
                            new OperationData(
                                new ArraySegment<byte>(itemStream.GetBuffer(), 0, checked((int) itemStream.Position)));

                        // Reset the item memory stream for the next chunk.
                        itemStream.Position = 0;
                    }
                }

                // Send the last chunk, if any exists.
                if (itemStream.Position > 0)
                {
                    itemWriter.Write((byte) StateManagerCopyOperation.StateProviderMetadata);
                    yield return
                        new OperationData(
                            new ArraySegment<byte>(itemStream.GetBuffer(), 0, checked((int) itemStream.Position)));
                }

                // End of Copy.
                FabricEvents.Events.StateManagerCopyStreamGetNext(traceType, metadataCount);
            }
        }

        /// <summary>
        /// Reads the state provider metadata from the next chunk of copy data.
        /// </summary>
        /// <param name="operationData">The next chunk of copy data.</param>
        /// <param name="traceType">Tracing type information.</param>
        /// <returns>The copied set of state providers' metadata.</returns>
        internal static IEnumerable<SerializableMetadata> ReadCopyData(OperationData operationData, string traceType)
        {
            if (operationData == null)
            {
                throw new ArgumentNullException("operationData");
            }

            if (operationData.Count <= 0)
            {
                throw new ArgumentException("OperationData contains zero data.", "operationData");
            }

            // The state manager currently sends a single array segment per operation data.
            var itemData = operationData[0];
            var operationType =
                (StateManagerCopyOperation) itemData.Array[itemData.Offset + itemData.Count - 1];

            using (var itemStream = new MemoryStream(itemData.Array, itemData.Offset, itemData.Count - 1))
            using (var itemReader = new InMemoryBinaryReader(itemStream))
            {
                if (operationType == StateManagerCopyOperation.Version)
                {
                    // Verify the copy protocol version is known.
                    var copyProtocolVersion = itemReader.ReadInt32();
                    if (copyProtocolVersion != CopyProtocolVersion)
                    {
                        throw new InvalidDataException(
                            string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_UnknownCopyProtocol_OneArg, copyProtocolVersion));
                    }
                }
                else if (operationType == StateManagerCopyOperation.StateProviderMetadata)
                {
                    // Read the chunk of state provider metadata.
                    while (itemStream.Position < itemStream.Length)
                    {
                        yield return ReadMetadata(itemReader, traceType, true);
                    }
                }
                else
                {
                    throw new InvalidDataException(
                        string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_UnknownSMCopyOperation_OneArg, operationType));
                }
            }
        }

        /// <summary>
        /// Read a single state provider metadata from the stream.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="traceType">Tracing type information.</param>
        /// <param name="deserializeTypes">Should the types be deserialized.</param>
        /// <returns>The read state provider metadata.</returns>
        private static SerializableMetadata ReadMetadata(InMemoryBinaryReader reader, string traceType, bool deserializeTypes = true)
        {
            var startPosition = reader.BaseStream.Position;

            var recordSize = reader.ReadInt32();
            var name = reader.ReadUri();
            Utility.Assert(name != null, "{0}: Name for the State Provider cannot be null.", traceType);

            string typeName = null;
            var type = reader.ReadType(out typeName);
            Utility.Assert(type != null, SR.Assert_SM_TypeNotFound, traceType, typeName, name.ToString());

            var createLsn = reader.ReadInt64();
            var deleteLsn = reader.ReadInt64();
            var initializationParameters = reader.ReadNullableBytes();
            var metadataMode = (MetadataMode) reader.ReadInt32();
            var stateProviderId = reader.ReadInt64();
            var parentStateProviderId = reader.ReadInt64();

            var endPosition = reader.BaseStream.Position;
            var readRecordSize = checked((int) (endPosition - startPosition));
            if (readRecordSize != recordSize)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_RecordSize));
            }

            if (deserializeTypes)
            {
                // Re-create the state provider metadata.
                return new SerializableMetadata(
                    name,
                    type,
                    initializationParameters,
                    stateProviderId,
                    parentStateProviderId,
                    metadataMode,
                    createLsn,
                    deleteLsn);
            }

            // Test only.
            return new SerializableMetadata(
                name,
                typeName,
                initializationParameters,
                stateProviderId,
                parentStateProviderId,
                metadataMode,
                createLsn,
                deleteLsn);
        }

        /// <summary>
        /// Write a single state provider metadata to the stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        /// <param name="metadata">State provider metadata to serialize.</param>
        /// <returns>The record size written.</returns>
        private static int WriteMetadata(InMemoryBinaryWriter writer, SerializableMetadata metadata)
        {
            var startPosition = writer.BaseStream.Position;

            // Reserve an 'int' for the record size at the beginning.
            writer.BaseStream.Position += sizeof(int);

            // Serialize the metadata.
            writer.Write(metadata.Name);
            writer.Write(metadata.Type);
            writer.Write(metadata.CreateLsn);
            writer.Write(metadata.DeleteLsn);
            writer.WriteNullable(metadata.InitializationContext);
            writer.Write((int) metadata.MetadataMode);
            writer.Write(metadata.StateProviderId);
            writer.Write(metadata.ParentStateProviderId);

            // Get the size of the record.
            var endPosition = writer.BaseStream.Position;
            var recordSize = checked((int) (endPosition - startPosition));

            // Write the record size at the start of the block.
            writer.BaseStream.Position = startPosition;
            writer.Write(recordSize);

            // Restore the end position.
            writer.BaseStream.Position = endPosition;

            return recordSize;
        }

        /// <summary>
        /// Read the list of state providers' metadata from the <see cref="StateManagerFile"/>.
        /// </summary>
        /// <param name="stream">Stream to read from.</param>
        /// <param name="traceType">Tracing type information.</param>
        /// <param name="deserializeTypes">Should the types be deserialized.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>The list of state providers' metadata read.</returns>
        private async Task<List<SerializableMetadata>> ReadMetadataAsync(
            Stream stream,
            string traceType,
            bool deserializeTypes,
            CancellationToken cancellationToken)
        {
            var metadataList = new List<SerializableMetadata>((int) this.properties.StateProviderCount);

            // Read the blocks, to improve sequential reads.
            var blocks = await FileBlock.ReadBlockAsync(
                        stream,
                        this.properties.BlocksHandle,
                        (sectionReader, sectionHandle) => StateManagerFileBlocks.Read(sectionReader, sectionHandle)).ConfigureAwait(false);

            // Currently, we expect the block sizes to always be present.
            if (blocks == null || blocks.RecordBlockSizes == null)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_BlockSizesMissing));
            }

            var recordBlockSizes = blocks.RecordBlockSizes;

            // Read blocks from the file.  Each state provider metadata is checksummed individually.
            using (var itemStream = new MemoryStream(DesiredBlockSize*2))
            using (var itemReader = new InMemoryBinaryReader(itemStream))
            {
                stream.Position = this.properties.MetadataHandle.Offset;
                var endOffset = this.properties.MetadataHandle.EndOffset;

                // Each block has one or more state provider metadata records.
                foreach (var recordBlockSize in recordBlockSizes)
                {
                    if (stream.Position + recordBlockSize > endOffset)
                    {
                        throw new InvalidDataException(
                            string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_BlockExtendPastFile));
                    }

                    // Read the block into memory.
                    itemStream.Position = 0;
                    itemStream.SetLength(recordBlockSize);
                    await stream.ReadAsync(itemStream.GetBuffer(), 0, recordBlockSize, cancellationToken).ConfigureAwait(false);

                    // Read to the end of the metadata section.
                    var endBlockOffset = itemStream.Length;
                    while (itemStream.Position < endBlockOffset)
                    {
                        var position = itemStream.Position;

                        // Read the record size and validate it is not obviously corrupted.
                        if (position + sizeof(int) > endBlockOffset)
                        {
                            throw new InvalidDataException(
                                string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_MissingRecordSize));
                        }

                        var recordSize = itemReader.ReadInt32();
                        var recordSizeWithChecksum = recordSize + sizeof(ulong);

                        // We need to do extra validation on the recordSize, because we haven't validated the bits
                        // against the checksum and we need the recordSize to locate the checksum.
                        if (recordSize < 0)
                        {
                            throw new InvalidDataException(
                                string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_NegativeRecordSize));
                        }

                        if (position + recordSize > endBlockOffset)
                        {
                            throw new InvalidDataException(
                                string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_RecordExtendsPastFile));
                        }

                        if (position + recordSizeWithChecksum > endBlockOffset)
                        {
                            throw new InvalidDataException(
                                string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_MissingChecksum));
                        }

                        // Compute the checksum.
                        var computedChecksum = CRC64.ToCRC64(
                            itemStream.GetBuffer(),
                            checked((int) position),
                            recordSize);

                        // Read the checksum (checksum is after the record bytes).
                        itemStream.Position = position + recordSize;
                        var checksum = itemReader.ReadUInt64();

                        // Verify the checksum.
                        if (checksum != computedChecksum)
                        {
                            throw new InvalidDataException(
                                string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_MismatchedChecksum));
                        }

                        // Read and re-create the state provider metadata, now that the checksum is validated.
                        itemStream.Position = position;
                        var metadata = ReadMetadata(itemReader, traceType, deserializeTypes);
                        metadataList.Add(metadata);
                        itemStream.Position = position + recordSizeWithChecksum;
                    }
                }
            }

            // Post-validation.
            if (metadataList.Count != (int) this.Properties.StateProviderCount)
            {
                throw new InvalidDataException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_SMFile_Corrupt_MetadataCountMismatch));
            }

            return metadataList;
        }

        /// <summary>
        /// Write the state provider metadata to the output stream.
        /// </summary>
        /// <param name="outputstream">Output stream.</param>
        /// <param name="metadataList">The state providers' metadata.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task<StateManagerFileBlocks> WriteMetadataAsync(
            Stream outputstream,
            IEnumerable<SerializableMetadata> metadataList,
            CancellationToken cancellationToken)
        {
            // Track blocks of records, to improve sequential reads. 
            var recordBlockSizes = new List<int>();
            var recordBlockSize = 0;

            using (var itemStream = new MemoryStream(DesiredBlockSize*2))
            using (var itemWriter = new InMemoryBinaryWriter(itemStream))
            {
                foreach (var metadata in metadataList)
                {
                    // Write each state provider metadata into the memory stream.
                    var recordPosition = checked((int) itemStream.Position);
                    var recordSize = WriteMetadata(itemWriter, metadata);

                    // Checksum this metadata record (with the record size included).
                    var recordChecksum = CRC64.ToCRC64(itemStream.GetBuffer(), recordPosition, recordSize);

                    // Add the checksum at the end of the memory stream.
                    itemWriter.Write(recordChecksum);

                    // Track record block sizes.  After enough bytes are buffered in memory, flush to disk.
                    recordBlockSize = checked((int) itemStream.Position);
                    if (recordBlockSize >= DesiredBlockSize)
                    {
                        // Flush the metadata records to the output stream.
                        await outputstream.WriteAsync(itemStream.GetBuffer(), 0, recordBlockSize, cancellationToken).ConfigureAwait(false);

                        recordBlockSizes.Add(recordBlockSize);
                        itemStream.Position = 0;
                    }

                    // Update properties.
                    this.Properties.StateProviderCount++;
                    if (metadata.ParentStateProviderId == DynamicStateManager.EmptyStateProviderId)
                    {
                        this.Properties.RootStateProviderCount++;
                    }
                }

                // Flush any remaining bytes to disk, and add the last record block size (if any).
                recordBlockSize = checked((int) itemStream.Position);
                if (recordBlockSize > 0)
                {
                    await outputstream.WriteAsync(itemStream.GetBuffer(), 0, recordBlockSize, cancellationToken).ConfigureAwait(false);
                    recordBlockSizes.Add(recordBlockSize);
                }
            }

            // Return the record block sizes.
            var blocks = new StateManagerFileBlocks(recordBlockSizes.ToArray());
            return blocks;
        }
    }
}