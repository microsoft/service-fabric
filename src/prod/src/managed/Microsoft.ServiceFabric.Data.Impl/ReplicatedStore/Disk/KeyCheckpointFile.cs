// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    using Utility = Microsoft.ServiceFabric.Replicator.Utility;

    /// <summary>
    /// Represents a TStore checkpoint file containing the serialized keys and metadata.
    /// </summary>
    internal sealed class KeyCheckpointFile : IDisposable
    {
        /// <summary>
        /// The currently supported key checkpoint file version.
        /// </summary>
        private const int FileVersion = 1;

        /// <summary>
        /// Buffer in memory approximately 32 KB of data before flushing to disk.
        /// </summary>
        public const int MemoryBufferFlushSize = 32*1024;

        /// <summary>
        /// The file extension for TStore checkpoint files that hold the keys and metadata.
        /// </summary>
        public const string FileExtension = ".sfk";

        /// <summary>
        /// Fixed sized metadata about the checkpoint file.  E.g. location of the Properties section.
        /// </summary>
        private FileFooter Footer;

        private bool isValueAReferenceType;

        /// <summary>
        /// Variable sized properties (metadata) about the checkpoint file.
        /// </summary>
        public KeyCheckpointFileProperties Properties;

        /// <summary>
        /// Pool of reusable FileStreams for the checkpoint file.
        /// </summary>
        public StreamPool ReaderPool;

        /// <summary>
        /// Gets the number of keys in the checkpoint file.
        /// </summary>
        public long KeyCount
        {
            get { return this.Properties.KeyCount; }
        }

        /// <summary>
        /// Gets the file Id of the checkpoint file.
        /// </summary>
        public uint FileId
        {
            get { return this.Properties.FileId; }
        }

        /// <summary>
        /// Create a new key checkpoint file with the given filename.
        /// </summary>
        /// <param name="filename">Path to the checkpoint file.</param>
        /// <param name="fileId">File Id of the checkpoint file.</param>
        /// <param name="isValueAReferenceType"></param>
        public KeyCheckpointFile(string filename, uint fileId, bool isValueAReferenceType)
            : this(filename, isValueAReferenceType)
        {
            this.Properties = new KeyCheckpointFileProperties()
            {
                FileId = fileId,
            };

            this.isValueAReferenceType = isValueAReferenceType;
        }

        /// <summary>
        /// Reserved for private static KeyCheckpointFile.OpenAsync().
        /// </summary>
        private KeyCheckpointFile(string filename, bool isValueAReferenceType)
        {
            // Open the file for read with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Used for recovery and merge. Hence the Io Priority is set at AcquireStream time.
            this.ReaderPool = new StreamPool(
                    () => FabricFile.Open(filename, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous | FileOptions.RandomAccess));

            this.isValueAReferenceType = isValueAReferenceType;
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
        /// Opens a <see cref="KeyCheckpointFile"/> from the given file.
        /// </summary>
        /// <param name="filename">The file to open that contains an existing checkpoint file.</param>
        /// <param name="traceType">Tracing information.</param>
        /// <param name="isValueReferenceType"></param>
        public static async Task<KeyCheckpointFile> OpenAsync(string filename, string traceType, bool isValueReferenceType)
        {
            FabricEvents.Events.OpenKeyCheckpointFile(traceType, filename, DiskConstants.state_opening);
            Diagnostics.Assert(FabricFile.Exists(filename), traceType, "File name {0} does not exist", filename);

            KeyCheckpointFile checkpointFile = null;
            try
            {
                checkpointFile = new KeyCheckpointFile(filename, isValueReferenceType);
                await checkpointFile.ReadMetadataAsync().ConfigureAwait(false);
            }
            catch (Exception)
            {
                // Ensure the checkpoint file get disposed quickly if we get an exception.
                if (checkpointFile != null)
                    checkpointFile.Dispose();

                throw;
            }

            FabricEvents.Events.OpenKeyCheckpointFile(traceType, filename, DiskConstants.state_complete);
            return checkpointFile;
        }

        /// <summary>
        /// Add a key to the given file stream, using the memory buffer to stage writes before issuing bulk disk IOs.
        /// </summary>
        /// <typeparam name="TKey"></typeparam>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="memoryBuffer"></param>
        /// <param name="item"></param>
        /// <param name="keySerializer"></param>
        /// <param name="timeStamp"></param>
        /// <param name="traceType"></param>
        /// <returns></returns>
        public void WriteItem<TKey, TValue>(
            InMemoryBinaryWriter memoryBuffer, KeyValuePair<TKey, TVersionedItem<TValue>> item, IStateSerializer<TKey> keySerializer, long timeStamp,
            string traceType)
        {
            // Write the key into the memory buffer.
            this.WriteKey(memoryBuffer, item, keySerializer, timeStamp);
        }

        /// <summary>
        /// A Flush indicates that all keys have been written to the checkpoint file (via AddItemAsync), so
        /// the checkpoint file can finish flushing any remaining in-memory buffered data, write any extra
        /// metadata (e.g. properties and footer), and flush to disk.
        /// </summary>
        /// <param name="fileStream"></param>
        /// <param name="memoryBuffer"></param>
        /// <returns></returns>
        public async Task FlushAsync(Stream fileStream, InMemoryBinaryWriter memoryBuffer)
        {
            // If there's any buffered keys in memory, flush them to disk first.
            if (memoryBuffer.BaseStream.Position > 0)
            {
                await this.FlushMemoryBufferAsync(fileStream, memoryBuffer).ConfigureAwait(false);
            }

            // Update the Properties.
            this.Properties.KeysHandle = new BlockHandle(0, fileStream.Position);

            // Write the Properties.
            var propertiesHandle = await FileBlock.WriteBlockAsync(fileStream, (sectionWriter) => this.Properties.Write(sectionWriter)).ConfigureAwait(false);

            // Write the Footer.
            this.Footer = new FileFooter(propertiesHandle, FileVersion);
            await FileBlock.WriteBlockAsync(fileStream, (sectionWriter) => this.Footer.Write(sectionWriter)).ConfigureAwait(false);

            // Finally, flush to disk.
            await fileStream.FlushAsync().ConfigureAwait(false);
        }

        public KeyCheckpointFileAsyncEnumerator<TKey, TValue> GetAsyncEnumerator<TKey, TValue>(
            IStateSerializer<TKey> keySerializer, 
            string traceType, 
            Kernel32Types.PRIORITY_HINT priorityHint = Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal)
        {
            return new KeyCheckpointFileAsyncEnumerator<TKey, TValue>(
                this,
                keySerializer,
                this.Properties.KeysHandle.Offset,
                this.Properties.KeysHandle.EndOffset,
                traceType, 
                priorityHint);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <typeparam name="TKey"></typeparam>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="memoryBuffer"></param>
        /// <param name="item"></param>
        /// <param name="keySerializer"></param>
        /// <param name="timeStamp"></param>
        /// <remarks>
        /// The data is written is 8 bytes aligned.
        /// 
        /// Name                    Type        Size
        /// 
        /// KeySize                 int         4
        /// Kind                    byte        1
        /// RESERVED                            3
        /// VersionSequenceNumber   long        8
        /// 
        /// (DeletedVersion)
        /// timeStamp               long        8
        /// 
        /// (Inserted || Updated)
        /// Offset                  long        8
        /// ValueChecksum           ulong       8
        /// ValueSize               int         4
        /// RESERVED                            4
        /// 
        /// Key                     TKey        N
        /// PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
        ///
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// 
        /// Note: Larges Key size supported is int.MaxValue in bytes.
        /// </remarks>
        private void WriteKey<TKey, TValue>(
            InMemoryBinaryWriter memoryBuffer, KeyValuePair<TKey, TVersionedItem<TValue>> item, IStateSerializer<TKey> keySerializer, long timeStamp)
        {
            Utility.Assert(memoryBuffer.IsAligned(), "must be aligned");
            var recordPosition = memoryBuffer.BaseStream.Position;

            memoryBuffer.BaseStream.Position += sizeof(int);
            memoryBuffer.Write((byte) item.Value.Kind); // RecordKind
            memoryBuffer.WritePaddingUntilAligned(); // RESERVED

            memoryBuffer.Write(item.Value.VersionSequenceNumber); // LSN

            if (item.Value.Kind == RecordKind.DeletedVersion)
            {
                memoryBuffer.Write(timeStamp);
            }
            else
            {
                // Deleted items don't have a value.  We only serialize value properties for non-deleted items.
                memoryBuffer.Write(item.Value.Offset); // value offset
                memoryBuffer.Write(item.Value.ValueChecksum); // value checksum
                memoryBuffer.Write(item.Value.ValueSize); // value size
                memoryBuffer.WritePaddingUntilAligned(); // RESERVED
            }

            var keyPosition = memoryBuffer.BaseStream.Position;
            keySerializer.Write(item.Key, memoryBuffer);
            var keyEndPosition = memoryBuffer.BaseStream.Position;
            Diagnostics.Assert(
                keyEndPosition >= keyPosition,
                DifferentialStoreConstants.TraceType,
                "User's key IStateSerializer moved the stream position backwards unexpectedly!");

            memoryBuffer.BaseStream.Position = recordPosition;
            memoryBuffer.Write(checked((int) (keyEndPosition - keyPosition)));
            memoryBuffer.BaseStream.Position = keyEndPosition;

            memoryBuffer.WritePaddingUntilAligned();
            Utility.Assert(memoryBuffer.IsAligned(), "must be aligned");

            // Update in-memory metadata.
            this.Properties.KeyCount++;
        }

        /// <summary>
        /// Read key from file for merge.
        /// </summary>
        /// <remarks>
        /// The data is written is 8 bytes aligned.
        /// 
        /// Name                    Type        Size
        /// 
        /// KeySize                 int         4
        /// Kind                    byte        1
        /// RESERVED                            3
        /// VersionSequenceNumber   long        8
        /// 
        /// (DeletedVersion)
        /// TimeStamp               long        8
        /// 
        /// (Inserted || Updated)
        /// Offset                  long        8
        /// ValueChecksum           ulong       8
        /// ValueSize               int         4
        /// RESERVED                            4
        /// 
        /// Key                     TKey        N
        /// PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
        ///
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// 
        /// Note: Larges Key size supported is int.MaxValue in bytes.
        /// </remarks>
        public KeyData<TKey, TValue> ReadKey<TKey, TValue>(InMemoryBinaryReader memoryBuffer, IStateSerializer<TKey> keySerializer)
        {
            memoryBuffer.ThrowIfNotAligned();

            // This mirrors WriteKey().
            var keySize = memoryBuffer.ReadInt32();
            var kind = (RecordKind) memoryBuffer.ReadByte();
            memoryBuffer.ReadPaddingUntilAligned(true);

            var lsn = memoryBuffer.ReadInt64();

            long valueOffset = 0;
            var valueSize = 0;
            ulong valueChecksum = 0;
            long TimeStamp = 0;

            if (kind == RecordKind.DeletedVersion)
            {
                TimeStamp = memoryBuffer.ReadInt64();
            }
            else
            {
                valueOffset = memoryBuffer.ReadInt64();
                valueChecksum = memoryBuffer.ReadUInt64();
                valueSize = memoryBuffer.ReadInt32();
                memoryBuffer.ReadPaddingUntilAligned(true);
            }

            // Protection in case the user's key serializer doesn't leave the stream at the correct end point.
            var keyPosition = memoryBuffer.BaseStream.Position;
            var key = keySerializer.Read(memoryBuffer);
            memoryBuffer.BaseStream.Position = keyPosition + keySize;
            memoryBuffer.ReadPaddingUntilAligned(false);

            TVersionedItem<TValue> value = null;
            switch (kind)
            {
                case RecordKind.DeletedVersion:
                    value = new TDeletedItem<TValue>(lsn, this.FileId);
                    break;
                case RecordKind.InsertedVersion:
                    value = new TInsertedItem<TValue>(lsn, this.FileId, valueOffset, valueSize, valueChecksum);
                    break;
                case RecordKind.UpdatedVersion:
                    value = new TUpdatedItem<TValue>(lsn, this.FileId, valueOffset, valueSize, valueChecksum);
                    break;

                default:
                    throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_KeyCheckpointFile_RecordKind, (byte) kind));
            }

            return new KeyData<TKey, TValue>(key, value, TimeStamp);
        }

        internal async Task FlushMemoryBufferAsync(Stream fileStream, InMemoryBinaryWriter memoryBuffer)
        {
            //// Checksum the chunk.
            //ulong checksum = memoryBuffer.GetChecksum();
            //memoryBuffer.Write(checksum);

            // Write to disk.
            await memoryBuffer.WriteAsync(fileStream).ConfigureAwait(false);

            // Reset the memory buffer.
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

                // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
                var footerHandle = new BlockHandle(filestream.Length - FileFooter.SerializedSize - sizeof(ulong), FileFooter.SerializedSize);
                this.Footer =
                    await FileBlock.ReadBlockAsync(filestream, footerHandle, (sectionReader, sectionHandle) => FileFooter.Read(sectionReader, sectionHandle)).ConfigureAwait(false);

                // Verify we know how to deserialize this version of the checkpoint file.
                if (this.Footer.Version != FileVersion)
                {
                    throw new InvalidDataException(SR.Error_KeyCheckpointFile_Deserialized);
                }

                // Read and validate the Properties section.
                var propertiesHandle = this.Footer.PropertiesHandle;
                this.Properties =
                    await
                        FileBlock.ReadBlockAsync(
                            filestream,
                            propertiesHandle,
                            (sectionReader, sectionHandle) => FilePropertySection.Read<KeyCheckpointFileProperties>(sectionReader, sectionHandle)).ConfigureAwait(false);
            }
            finally
            {
                this.ReaderPool.ReleaseStream(filestream, true);
            }
        }
    }
}