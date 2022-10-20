// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading.Tasks;

    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;

    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Represents a TStore checkpoint file containing the serialized values.
    /// </summary>
    internal sealed class ValueCheckpointFile : IDisposable
    {
        /// <summary>
        /// TODO: resolve this with BlockAlignedWriter.DefaultBlockAlignmentSize;
        /// </summary>
        public const int DefaultBlockAlignmentSize = 4*1024;

        /// <summary>
        /// The currently supported value checkpoint file version.
        /// </summary>
        private const int FileVersion = 1;

        /// <summary>
        /// Buffer in memory approximately 32 KB of data before flushing to disk.
        /// </summary>
        public const int MemoryBufferFlushSize = 32*1024;

        /// <summary>
        /// The file extension for TStore checkpoint files that hold the serialized values.
        /// </summary>
        public const string FileExtension = ".sfv";

        /// <summary>
        /// Io Priority Hint to be used to read value files.
        /// </summary>
        private Kernel32Types.PRIORITY_HINT priorityHint;

        /// <summary>
        /// Fixed sized metadata about the checkpoint file.  E.g. location of the Properties section.
        /// </summary>
        private FileFooter Footer;

        /// <summary>
        /// Variable sized properties (metadata) about the checkpoint file.
        /// </summary>
        public ValueCheckpointFileProperties Properties;

        /// <summary>
        /// Pool of reusable FileStreams for the value checkpoint file.
        /// </summary>
        private StreamPool ReaderPool;

        /// <summary>
        /// Gets the number of keys in the checkpoint file.
        /// </summary>
        public long ValueCount
        {
            get { return this.Properties.ValueCount; }
        }

        /// <summary>
        /// Gets the file Id of the checkpoint file.
        /// </summary>
        public uint FileId
        {
            get { return this.Properties.FileId; }
        }

        private readonly string traceType;

        /// <summary>
        /// Create a new value checkpoint file with the given filename.
        /// </summary>
        /// <param name="filename">Path to the checkpoint file.</param>
        /// <param name="fileId">File Id of the checkpoint file.</param>
        /// <param name="traceType">trace id</param>
        /// <param name="priorityHint">The priority hint.</param>
        public ValueCheckpointFile(string filename, uint fileId, string traceType, Kernel32Types.PRIORITY_HINT priorityHint = Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal)
            : this(filename, priorityHint)
        {
            this.traceType = traceType;
            this.Properties = new ValueCheckpointFileProperties()
            {
                FileId = fileId,
            };
        }

        /// <summary>
        /// Reserved for private static ValueCheckpointFile.OpenAsync().
        /// </summary>
        private ValueCheckpointFile(string filename, Kernel32Types.PRIORITY_HINT priorityHint = Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal)
        {
            // Open the file for read with asynchronous flag and 4096 cache size (C# default).

            // Because of the issue at https://github.com/dotnet/corefx/issues/6007
            // where asynchonous reads can internally be synchronous reads with wait calls, we can be
            // bottlenecked by the limited number of I/O completion ports in overlapped I/O
            // causing a lot of blocked threads and extreme slowdown when met with a large number
            // of concurrent reads. Due to this, we chose to disable the FileOptions.Asynchronous option.
            this.ReaderPool = new StreamPool(
                    () => FabricFile.Open(filename, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.RandomAccess));

            this.priorityHint = priorityHint;
        }

        public void SetIoPriority(Kernel32Types.PRIORITY_HINT priorityHint)
        {
            this.priorityHint = priorityHint;
        }

        /// <summary>
        /// Disposes the checkpoint's streams.
        /// </summary>
        public void Dispose()
        {
            if (this.ReaderPool != null)
            {
                this.ReaderPool.Dispose();
            }
        }

        /// <summary>
        /// Opens a <see cref="ValueCheckpointFile"/> from the given file.
        /// The file stream will be disposed when the checkpoint file is disposed.
        /// </summary>
        /// <param name="filename">The file to open that contains an existing checkpoint file.</param>
        /// <param name="traceType">Tracing information.</param>
        public static async Task<ValueCheckpointFile> OpenAsync(string filename, string traceType)
        {

            FabricEvents.Events.ValueCheckpointFileAsyncOpen(traceType, DiskConstants.state_opening, filename);
            Diagnostics.Assert(FabricFile.Exists(filename), traceType, "File name {0} does not exist", filename);

            ValueCheckpointFile checkpointFile = null;
            try
            {
                checkpointFile = new ValueCheckpointFile(filename);
                await checkpointFile.ReadMetadataAsync().ConfigureAwait(false);
            }
            catch (Exception)
            {
                // Ensure the checkpoint file get disposed quickly if we get an exception.
                if (checkpointFile != null)
                    checkpointFile.Dispose();

                throw;
            }

            FabricEvents.Events.ValueCheckpointFileAsyncOpen(traceType, DiskConstants.state_complete, filename);
            return checkpointFile;
        }

        public async Task<byte[]> ReadValueAsync<TValue>(TVersionedItem<TValue> item)
        {
            Stream fileStream = null;
            try
            {
                // Acquire a re-usable file stream for exclusive use during this read.
                fileStream = this.ReaderPool.AcquireStream();

                var snapFileStream = fileStream as FileStream;
                Diagnostics.Assert(snapFileStream != null, this.traceType, "fileStream must be a FileStream");
                Microsoft.ServiceFabric.Replicator.Utility.SetIoPriorityHint(snapFileStream.SafeFileHandle, this.priorityHint);

                // TODO: use memory stream pool here.
                using (var stream = new MemoryStream(capacity: item.ValueSize))
                {
                    // Read the value bytes and the checksum into memory.
                    fileStream.Position = item.Offset;
                    stream.SetLength(item.ValueSize);
                    await fileStream.ReadAsync(stream.GetBuffer(), 0, item.ValueSize).ConfigureAwait(false);

                    // Read the checksum from memory.
                    stream.Position = item.ValueSize;
                    var checksum = item.ValueChecksum;

                    // Re-compute the checksum.
                    var expectedChecksum = CRC64.ToCRC64(stream.GetBuffer(), 0, item.ValueSize);
                    if (checksum != expectedChecksum)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FailedReadValue_ChecksumMismatch_TwoArgs, checksum, expectedChecksum));
                    }

                    return stream.GetBuffer();
                }
            }
            finally
            {
                // Return the file stream to the pool for re-use.
                this.ReaderPool.ReleaseStream(fileStream);
            }
        }

        /// <summary>
        /// Read the given value from disk.
        /// </summary>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="item"></param>
        /// <param name="valueSerializer"></param>
        /// <returns></returns>
        public async Task<TValue> ReadValueAsync<TValue>(TVersionedItem<TValue> item, IStateSerializer<TValue> valueSerializer)
        {
            // Validate the item has a value that can be read from disk.
            if (item == null)
            {
                throw new ArgumentNullException(SR.Error_Item);
            }
            if (item.Kind == RecordKind.DeletedVersion)
            {
                throw new ArgumentException(SR.Error_ValueCheckpoint_DeletedItemValue, SR.Error_Item);
            }

            // Validate that the item's disk properties are valid.
            if (item.Offset < this.Properties.ValuesHandle.Offset)
            {
                throw new ArgumentOutOfRangeException(SR.Error_Item, SR.Error_ValueCheckpoint_TVersionedItem_Offset_Negative);
            }
            if (item.ValueSize < 0)
            {
                throw new ArgumentOutOfRangeException(string.Format(CultureInfo.CurrentCulture, SR.Error_Item, SR.Error_ValueCheckpoint_TVersionedItem_ValueSize_Negative, item.ValueSize));
            }
            if (item.Offset + item.ValueSize > this.Properties.ValuesHandle.EndOffset)
            {
                throw new ArgumentOutOfRangeException(SR.Error_Item, SR.Error_ValueCheckpoint_StreamRangeExceeded);
            }

            Stream fileStream = null;
            try
            {
                // Acquire a re-usable file stream for exclusive use during this read.
                fileStream = this.ReaderPool.AcquireStream();

                var snapFileStream = fileStream as FileStream;
                Diagnostics.Assert(snapFileStream != null, this.traceType, "fileStream must be a FileStream");
                Microsoft.ServiceFabric.Replicator.Utility.SetIoPriorityHint(snapFileStream.SafeFileHandle, this.priorityHint);

                // TODO: use memory stream pool here.
                using (var stream = new MemoryStream(capacity: item.ValueSize))
                using (var reader = new BinaryReader(stream))
                {
                    // Read the value bytes and the checksum into memory.
                    fileStream.Position = item.Offset;
                    stream.SetLength(item.ValueSize);
                    await fileStream.ReadAsync(stream.GetBuffer(), 0, item.ValueSize).ConfigureAwait(false);

                    // Read the checksum from memory.
                    stream.Position = item.ValueSize;
                    var checksum = item.ValueChecksum;

                    // Re-compute the checksum.
                    var expectedChecksum = CRC64.ToCRC64(stream.GetBuffer(), 0, item.ValueSize);
                    if (checksum != expectedChecksum)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FailedReadValue_ChecksumMismatch_TwoArgs, checksum, expectedChecksum));
                    }

                    // Deserialize the value into memory.
                    stream.Position = 0;
                    return valueSerializer.Read(reader);
                }
            }
            finally
            {
                // Return the file stream to the pool for re-use.
                this.ReaderPool.ReleaseStream(fileStream);
            }
        }

        /// <summary>
        /// Add a value to the given file stream, using the memory buffer to stage writes before issuing bulk disk IOs.
        /// </summary>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="fileStream"></param>
        /// <param name="memoryBuffer"></param>
        /// <param name="item"></param>
        /// <param name="valueSerializer"></param>
        /// <param name="traceType"></param>
        /// <returns></returns>
        public void WriteItem<TValue>(
            Stream fileStream, InMemoryBinaryWriter memoryBuffer, TVersionedItem<TValue> item, IStateSerializer<TValue> valueSerializer, string traceType)
        {
            // Write the value into the memory buffer.
            this.WriteValue(memoryBuffer, fileStream.Position, item, valueSerializer);
        }

        /// <summary>
        /// Add a value to the given file stream, using the memory buffer to stage writes before issuing bulk disk IOs.
        /// </summary>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="fileStream"></param>
        /// <param name="memoryBuffer"></param>
        /// <param name="item"></param>
        /// <param name="value"></param>
        /// <param name="traceType"></param>
        /// <returns></returns>
        public void WriteItem<TValue>(Stream fileStream, InMemoryBinaryWriter memoryBuffer, TVersionedItem<TValue> item, byte[] value, string traceType)
        {
            // Write the value into the memory buffer.
            this.WriteValue(memoryBuffer, fileStream.Position, item, value);
        }


        /// <summary>
        /// A Flush indicates that all values have been written to the checkpoint file (via AddItemAsync), so
        /// the checkpoint file can finish flushing any remaining in-memory buffered data, write any extra
        /// metadata (e.g. properties and footer), and flush to disk.
        /// </summary>
        /// <param name="fileStream"></param>
        /// <param name="memoryBuffer"></param>
        /// <returns></returns>
        public async Task FlushAsync(Stream fileStream, InMemoryBinaryWriter memoryBuffer)
        {
            // If there's any buffered values in memory, flush them to disk first.
            if (memoryBuffer.BaseStream.Position > 0)
            {
                await this.WriteToFileBufferAsync(fileStream, memoryBuffer).ConfigureAwait(false);
            }

            Diagnostics.Assert(fileStream.Position%DefaultBlockAlignmentSize == 0, this.traceType, "Value checkpoint file is incorrectly block aligned at the end.");

            // Update the Properties.
            this.Properties.ValuesHandle = new BlockHandle(0, fileStream.Position);

            // Write the Properties.
            var propertiesHandle = await FileBlock.WriteBlockAsync(fileStream, (sectionWriter) => this.Properties.Write(sectionWriter)).ConfigureAwait(false);

            // Write the Footer.
            this.Footer = new FileFooter(propertiesHandle, FileVersion);
            await FileBlock.WriteBlockAsync(fileStream, (sectionWriter) => this.Footer.Write(sectionWriter)).ConfigureAwait(false);

            // Finally, flush to disk.
            await fileStream.FlushAsync().ConfigureAwait(false);
        }

        private void WriteValue<TValue>(
            InMemoryBinaryWriter memoryBuffer, long basePosition, TVersionedItem<TValue> item, IStateSerializer<TValue> valueSerializer)
        {
            // Deleted items don't have values.  Only serialize valid items.
            if (item.Kind != RecordKind.DeletedVersion)
            {
                // WriteItemAsync valueSerializer followed by checksum.
                // Serialize the value.
                var valueStartPosition = memoryBuffer.BaseStream.Position;
                valueSerializer.Write(item.Value, memoryBuffer);
                var valueEndPosition = memoryBuffer.BaseStream.Position;
                Diagnostics.Assert(
                    valueEndPosition >= valueStartPosition,
                    DifferentialStoreConstants.TraceType,
                    "User's value IStateSerializer moved the stream position backwards unexpectedly!");

                // Write the checksum of just that value's bytes.
                var valueSize = checked((int) (valueEndPosition - valueStartPosition));
                var checksum = CRC64.ToCRC64(memoryBuffer.BaseStream.GetBuffer(), checked((int) valueStartPosition), valueSize);

                // Update the in-memory offset and size for this item.
                item.Offset = basePosition + valueStartPosition;
                item.ValueSize = valueSize;
                item.ValueChecksum = checksum;

                // Update checkpoint file in-memory metadata.
                this.Properties.ValueCount++;
            }

            // Update the in-memory metadata about which file this key-value exists in on disk.
            item.FileId = this.FileId;
        }

        private void WriteValue<TValue>(InMemoryBinaryWriter memoryBuffer, long basePosition, TVersionedItem<TValue> item, byte[] value)
        {
            // Deleted items don't have values.  Only serialize valid items.
            if (item.Kind != RecordKind.DeletedVersion)
            {
                // WriteItemAsync valueSerializer followed by checksum.
                // Serialize the value.
                var valueStartPosition = memoryBuffer.BaseStream.Position;
                memoryBuffer.Write(value);
                var valueEndPosition = memoryBuffer.BaseStream.Position;
                Diagnostics.Assert(
                    valueEndPosition >= valueStartPosition,
                    DifferentialStoreConstants.TraceType,
                    "User's value IStateSerializer moved the stream position backwards unexpectedly!");

                // Write the checksum of just that value's bytes.
                var valueSize = checked((int) (valueEndPosition - valueStartPosition));
                var checksum = CRC64.ToCRC64(memoryBuffer.BaseStream.GetBuffer(), checked((int) valueStartPosition), valueSize);

                // Update the in-memory offset and size for this item.
                item.Offset = basePosition + valueStartPosition;
                item.ValueSize = valueSize;
                item.ValueChecksum = checksum;

                // Update checkpoint file in-memory metadata.
                this.Properties.ValueCount++;
            }

            // Update the in-memory metadata about which file this key-value exists in on disk.
            item.FileId = this.FileId;
        }

        public async Task WriteToFileBufferAsync(Stream fileStream, InMemoryBinaryWriter memoryBuffer)
        {
            // The values are each individually checksummed, so we don't need to separately checksum the block being written.

            // Write to disk.
            await memoryBuffer.WriteAsync(fileStream).ConfigureAwait(false);

            // Reset the memory buffer to be able to re-use it.
            memoryBuffer.BaseStream.Position = 0;
        }

        /// <summary>
        /// Deserializes the metadata (footer, properties, etc.) for this checkpoint file.
        /// </summary>
        private async Task ReadMetadataAsync()
        {
            Stream filestream = null;
            try
            {
                filestream = this.ReaderPool.AcquireStream();

                var snapFileStream = filestream as FileStream;
                Diagnostics.Assert(snapFileStream != null, this.traceType, "fileStream must be a FileStream");
                Microsoft.ServiceFabric.Replicator.Utility.SetIoPriorityHint(snapFileStream.SafeFileHandle, this.priorityHint);

                // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
                var footerHandle = new BlockHandle(filestream.Length - FileFooter.SerializedSize - sizeof(ulong), FileFooter.SerializedSize);
                this.Footer =
                    await FileBlock.ReadBlockAsync(filestream, footerHandle, (sectionReader, sectionHandle) => FileFooter.Read(sectionReader, sectionHandle)).ConfigureAwait(false);

                // Verify we know how to deserialize this version of the checkpoint file.
                if (this.Footer.Version != FileVersion)
                {
                    throw new InvalidDataException(SR.Error_ValueCheckpoint_Deserialized);
                }

                // Read and validate the Properties section.
                var propertiesHandle = this.Footer.PropertiesHandle;
                this.Properties =
                    await
                        FileBlock.ReadBlockAsync(
                            filestream,
                            propertiesHandle,
                            (sectionReader, sectionHandle) => FilePropertySection.Read<ValueCheckpointFileProperties>(sectionReader, sectionHandle)).ConfigureAwait(false);
            }
            finally
            {
                this.ReaderPool.ReleaseStream(filestream, true);
            }
        }
    }
}