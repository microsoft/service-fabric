// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Enumerates the keys and values in a SortedTable.
    /// </summary>
    internal class KeyCheckpointFileAsyncEnumerator<TKey, TValue> :
        IAsyncEnumerator<KeyData<TKey, TValue>>,
        IComparable<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>
    {
        private const int ChunkSize = 64*1024;
        private const int ReadChunkSize = 32*1024;
        private List<KeyData<TKey, TValue>> itemsBuffer;
        private int index = 0;
        private long keyCount = 0;
        private bool stateZero;
        private KeyCheckpointFile keyCheckpointFile;
        private readonly string traceType;
        private long startOffset;
        private long endOffset;
        private MemoryStream memoryStream;
        private InMemoryBinaryReader reader;
        private Stream fileStream;
        private IStateSerializer<TKey> keySerializer;
        private IComparer<TKey> keyComparer;
        
        /// <summary>
        /// IO priority hint to use fore IOs.
        /// </summary>
        private readonly Kernel32Types.PRIORITY_HINT priorityHint;

        public KeyCheckpointFileAsyncEnumerator(
            KeyCheckpointFile keyCheckpointFile,
            IStateSerializer<TKey> keySerializer,
            long startOffset,
            long endOffset,
            string traceType,
            Kernel32Types.PRIORITY_HINT priorityHint = Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal)
        {
            this.traceType = traceType;
            this.keyCheckpointFile = keyCheckpointFile;
            this.stateZero = true;
            this.index = -1;
            this.keySerializer = keySerializer;
            this.startOffset = startOffset;
            this.endOffset = endOffset;

            this.priorityHint = priorityHint;
        }

        public IComparer<TKey> KeyComparer
        {
            get { return this.keyComparer; }
            set { this.keyComparer = value; }
        }

        public KeyData<TKey, TValue> Current { get; private set; }

        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            // Starting from state zero.
            if (this.stateZero)
            {
                // Assert that startOffset - endOffset is a multiple of 4k

                this.stateZero = false;
                this.itemsBuffer = new List<KeyData<TKey, TValue>>();
                this.index = 0;

                // Call read keys and populate list.
                this.memoryStream = new MemoryStream(capacity: ChunkSize);
                this.reader = new InMemoryBinaryReader(this.memoryStream);

                // Assert file stream is null;
                Diagnostics.Assert(this.fileStream == null, this.traceType, "fileSteam should be null to start with.");
                this.fileStream = this.keyCheckpointFile.ReaderPool.AcquireStream();

                var snapFileStream = this.fileStream as FileStream;
                Diagnostics.Assert(snapFileStream != null, this.traceType, "fileStream must be a FileStream");
                Microsoft.ServiceFabric.Replicator.Utility.SetIoPriorityHint(snapFileStream.SafeFileHandle, this.priorityHint);

                this.fileStream.Position = this.startOffset;

                var result = await this.ReadChunkAsync().ConfigureAwait(false);
                if (result)
                {
                    this.Current = this.itemsBuffer[this.index];
                    return true;
                }
                else
                {
                    Diagnostics.Assert(this.keyCount == this.keyCheckpointFile.Properties.KeyCount, this.traceType, "Failed to read the expected number of keys.");
                    return false;
                }
            }
            else
            {
                this.index++;

                // Check if it is in the buffer.
                if (this.index < this.itemsBuffer.Count)
                {
                    this.Current = this.itemsBuffer[this.index];
                    return true;
                }
                else
                {
                    // read next block
                    var result = await this.ReadChunkAsync().ConfigureAwait(false);
                    if (result)
                    {
                        this.Current = this.itemsBuffer[this.index];
                        return true;
                    }
                    else
                    {
                        Diagnostics.Assert(this.keyCount == this.keyCheckpointFile.Properties.KeyCount, this.traceType, "Failed to read the expected number of keys.");
                        return false;
                    }
                }
            }
        }

        private async Task<bool> ReadChunkAsync()
        {
            const int BlockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;
            this.itemsBuffer.Clear();
            this.index = 0;

            // Pick a chunk size that is a multiple of 4k lesser than the end offset.
            var chunkSize = this.GetChunkSize();
            if (chunkSize == 0)
            {
                return false;
            }

            // Read the entire chunk (plus the checksum and next chunk size) into memory.
            this.memoryStream.Position = 0;
            this.memoryStream.SetLength(chunkSize);
            await this.fileStream.ReadAsync(this.memoryStream.GetBuffer(), 0, chunkSize).ConfigureAwait(false);

            while (true)
            {
                var alignedStartBlockOffset = checked((int) this.reader.BaseStream.Position);
                var blockMetadata = KeyChunkMetadata.Read(this.reader);
                var currentBlockSize = blockMetadata.BlockSize;
                var alignedBlockSize = GetBlockSize(currentBlockSize);

                // Check if the next block was only partially read.  If so, read the remainder of it into memory.
                if ((alignedStartBlockOffset + alignedBlockSize) > chunkSize)
                {
                    var remainingBlockSize = (alignedStartBlockOffset + alignedBlockSize) - chunkSize;
                    Diagnostics.Assert(remainingBlockSize%BlockAlignmentSize == 0, this.traceType, "Remaining block size should be 4K aligned.");

                    this.memoryStream.SetLength(chunkSize + remainingBlockSize);
                    await this.fileStream.ReadAsync(this.memoryStream.GetBuffer(), chunkSize, remainingBlockSize).ConfigureAwait(false);
                    chunkSize = chunkSize + remainingBlockSize;
                }

                var keysFromBlock = this.ReadBlock(currentBlockSize);

                foreach (var keyData in keysFromBlock)
                {
                    this.itemsBuffer.Add(keyData);
                }

                // Move the reader ahead to the next block, if possible, else reset and break.
                this.reader.BaseStream.Position = alignedStartBlockOffset + alignedBlockSize;
                if (alignedStartBlockOffset + alignedBlockSize >= chunkSize)
                {
                    Diagnostics.Assert(
                        alignedStartBlockOffset + alignedBlockSize == chunkSize,
                        this.traceType,
                        "Failed to read block of keys due to chunk size not aligned to blocks.");
                    break;
                }
            }

            // Track the number of keys returned.
            this.keyCount += this.itemsBuffer.Count;

            Diagnostics.Assert(this.itemsBuffer.Count > 0, this.traceType, "Unexpectedly read a chunk that has zero keys.");
            return true;
        }

        private IEnumerable<KeyData<TKey, TValue>> ReadBlock(int blockSize)
        {
            var blockStartPosition = checked((int) this.reader.BaseStream.Position);
            var alignedBlockStartPosition = blockStartPosition - KeyChunkMetadata.Size;

            this.reader.BaseStream.Position = alignedBlockStartPosition + blockSize - sizeof(ulong);
            var expectedChecksum = this.reader.ReadUInt64();
            this.reader.BaseStream.Position = blockStartPosition;

            // Verify checksum.
            var actualChecksum = CRC64.ToCRC64(this.reader.BaseStream.GetBuffer(), alignedBlockStartPosition, blockSize - sizeof(ulong));
            if (actualChecksum != expectedChecksum)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_KeyCheckpoint_FailedToRead_Checksum, actualChecksum, expectedChecksum));
            }

            while (this.reader.BaseStream.Position < (alignedBlockStartPosition + blockSize - sizeof(ulong)))
            {
                var keyData = this.keyCheckpointFile.ReadKey<TKey, TValue>(this.reader, this.keySerializer);
                yield return keyData;
            }

            Diagnostics.Assert(
                this.reader.BaseStream.Position == (alignedBlockStartPosition + blockSize - sizeof(ulong)),
                this.traceType,
                "Failed to read block of keys due to block alignment mismatch.");
        }

        public void Reset()
        {
            // Empty list and clean the index.
            // state zero indicates start.
            // this.stateZero = true;
            throw new NotImplementedException();
        }

        public void Dispose()
        {
            //Dispose memory stream, file stream, 

            if (this.reader != null)
            {
                this.reader.Dispose();
                this.reader = null;
            }

            if (this.memoryStream != null)
            {
                this.memoryStream.Dispose();
                this.memoryStream = null;
            }

            this.keyCheckpointFile.ReaderPool.ReleaseStream(this.fileStream, true);
            this.fileStream = null;
        }

        public int CompareTo(KeyCheckpointFileAsyncEnumerator<TKey, TValue> other)
        {
            // Compare Keys
            var compare = this.keyComparer.Compare(this.Current.Key, other.Current.Key);
            if (compare != 0)
            {
                return compare;
            }

            // Compare LSNs
            var currentLsn = this.Current.Value.VersionSequenceNumber;
            var otherLsn = other.Current.Value.VersionSequenceNumber;

            if (currentLsn > otherLsn)
            {
                return -1;
            }
            else if (currentLsn < otherLsn)
            {
                return 1;
            }

            // If they are both deleted items, compare timestamps.
            TVersionedItem<TValue> currentItem = this.Current.Value;
            TVersionedItem<TValue> otherItem = other.Current.Value;

            if (currentItem.Kind == RecordKind.DeletedVersion && otherItem.Kind == RecordKind.DeletedVersion)
            {
                Diagnostics.Assert(currentItem.FileId != otherItem.FileId, this.traceType, "Current item file id should not be same as other item");
                if (this.Current.TimeStamp > other.Current.TimeStamp)
                {
                    return -1;
                }
                else if (this.Current.TimeStamp < other.Current.TimeStamp)
                {
                    return 1;
                }

                return 0;
            }
            
            return 0;
        }

        private int GetChunkSize()
        {
            // Get chunk size of 64k if available, else remaining size.
            if (this.fileStream.Position < this.endOffset)
            {
                var remainingSize = this.endOffset - this.fileStream.Position;

                if (remainingSize < ReadChunkSize)
                {
                    return checked((int) remainingSize);
                }
                else
                {
                    return ReadChunkSize;
                }
            }

            Diagnostics.Assert(this.fileStream.Position == this.endOffset, this.traceType, "File stream offset should end exactly on the property's end offset.");
            return 0;
        }

        private static int GetBlockSize(int chunkSize)
        {
            // set block size to be a multiple of 4k bigger than the given value size.
            if (chunkSize%BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize == 0)
            {
                return chunkSize;
            }
            else
            {
                return (BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize*(chunkSize/BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize + 1));
            }
        }
    }
}