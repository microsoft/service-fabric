// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Writes data aligned to a specified block size.
    /// </summary>
    internal class KeyBlockAlignedWriter<TKey, TValue> : IDisposable
    {
        public const int ChecksumSize = sizeof(ulong);
        private int blockAlignmentSize;
        private int currentBlockPosition = 0;

        private KeyCheckpointFile keyCheckpointFile;
        private InMemoryBinaryWriter keyBuffer;
        private FileStream keyFileStream;
        private long timeStamp;
        private IStateSerializer<TKey> keySerializer;
        private string traceType;

        private InMemoryBinaryWriter tempKeyBuffer;

        public KeyBlockAlignedWriter(
            FileStream keyFileStream, KeyCheckpointFile keyCheckpointFile, InMemoryBinaryWriter keyBuffer, IStateSerializer<TKey> keySerializer,
            long timeStamp, string traceType)
        {
            this.keyCheckpointFile = keyCheckpointFile;
            this.timeStamp = timeStamp;
            this.keyFileStream = keyFileStream;
            this.keyBuffer = keyBuffer;
            this.keySerializer = keySerializer;

            this.traceType = traceType;
            this.blockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;

            // Most key records are lesser than 4k, when they get beyond 4k, do not shrink the buffer.
            this.tempKeyBuffer = new InMemoryBinaryWriter(new MemoryStream(capacity: 4*1024));
        }

        /// <summary>
        /// Batches given keys and writes them on disk in 4K aligned batches.
        /// </summary>
        /// <param name="item">Key and the versioned item that contains metadata about the row.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        /// <remarks>
        /// The data is written is 8 bytes aligned.
        /// 
        /// Name                    Type        Size
        /// 
        /// BlockMetadata
        /// BlockSize               int         4
        /// RESERVED                            4
        /// 
        /// Block                   byte[]      N
        /// PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
        ///
        /// Checksum                ulong       8
        /// 
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// 
        /// Note: Larges Block supported is 2GB
        /// </remarks>
        public async Task BlockAlignedWriteKeyAsync(KeyValuePair<TKey, TVersionedItem<TValue>> item)
        {
            Diagnostics.Assert(this.tempKeyBuffer.BaseStream.Position == 0, this.traceType, "Temp key buffer should always start from position zero.");
            this.keyCheckpointFile.WriteItem<TKey, TValue>(this.tempKeyBuffer, item, this.keySerializer, this.timeStamp, this.traceType);

            // Temp key buffer always gets reset after every key copy, so it is safe to assume position as record size.
            var keyRecordsize = (int) this.tempKeyBuffer.BaseStream.Position;

            // Case1: when record size is lesser than or equal to 4k.
            if (keyRecordsize + ChecksumSize + KeyChunkMetadata.Size <= this.blockAlignmentSize)
            {
                // Check if the current record can fit into this block (just account for checksum).
                if (this.currentBlockPosition + keyRecordsize + ChecksumSize <= this.blockAlignmentSize)
                {
                    // Do a mem copy.
                    await this.CopyStreamAsync().ConfigureAwait(false);
                }
                else
                {
                    Diagnostics.Assert(this.currentBlockPosition > 0, this.traceType, "We should have serialized a least one record");

                    // write checksum since we are done with this 4k block.
                    this.WriteEndOfBlock();

                    // Skip to next 4k boundary.
                    // Value does not fit into the current block, move position to the 4k+1 
                    //this.keyBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                    var skipSize = this.blockAlignmentSize - this.currentBlockPosition;
                    BlockAlignedWriter<TKey, TValue>.WriteBadFood(this.keyBuffer, skipSize);

                    await this.ResetCurrentBlockPositionAndFlushIfNecessaryAsync().ConfigureAwait(false);

                    // Reset block size to default if value size is smaller, else pick the size correctly.
                    if (keyRecordsize + ChecksumSize + KeyChunkMetadata.Size <= BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize)
                    {
                        this.blockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;
                    }
                    else
                    {
                        this.GetBlockSize(keyRecordsize + ChecksumSize + KeyChunkMetadata.Size);
                    }

                    // Do a mem copy.
                    await this.CopyStreamAsync().ConfigureAwait(false);
                }
            }
            else
            {
                // Case 2: Value size is greater than block size
                if (this.currentBlockPosition > 0)
                {
                    // write checksum since we are done with this 4k block.
                    this.WriteEndOfBlock();

                    // Move the memory stream ahead to a 4k boundary.
                    // Value does not fit into the current block, move position to the 4k+1 
                    //this.keyBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                    var skipSize = this.blockAlignmentSize - this.currentBlockPosition;
                    BlockAlignedWriter<TKey, TValue>.WriteBadFood(this.keyBuffer, skipSize);

                    // Reset current position.
                    await this.ResetCurrentBlockPositionAndFlushIfNecessaryAsync().ConfigureAwait(false);
                }

                // set block size to be a multiple of 4k bigger than the given value size.
                this.GetBlockSize(keyRecordsize + ChecksumSize + KeyChunkMetadata.Size);

                // Do a mem copy.
                await this.CopyStreamAsync().ConfigureAwait(false);
            }
        }

        public async Task FlushAsync()
        {
            if (this.currentBlockPosition > 0)
            {
                this.WriteEndOfBlock();

                // Move the memory stream ahead to a 4k boundary.
                //this.keyBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                var skipSize = this.blockAlignmentSize - this.currentBlockPosition;
                BlockAlignedWriter<TKey, TValue>.WriteBadFood(this.keyBuffer, skipSize);
            }

            // Flush the memory buffer periodically to disk.
            if (this.keyBuffer.BaseStream.Position > 0)
            {
                await this.keyCheckpointFile.FlushMemoryBufferAsync(this.keyFileStream, this.keyBuffer).ConfigureAwait(false);
            }

            Diagnostics.Assert(
                this.keyFileStream.Position%BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize == 0,
                this.traceType,
                "Key checkpoint file is incorrectly block aligned at the end.");

            await this.keyCheckpointFile.FlushAsync(this.keyFileStream, this.keyBuffer).ConfigureAwait(false);
        }

        public void Dispose()
        {
            if (this.tempKeyBuffer != null)
            {
                this.tempKeyBuffer.Dispose();
                this.tempKeyBuffer = null;
            }
        }

        private async Task CopyStreamAsync()
        {
            if (this.currentBlockPosition == 0)
            {
                this.ReserveSpaceForKeyBlockMetadata();
            }

            // TODO: account for key metadata correctly.
            // 32 is the metadata size of DeletedVersion which has the smallest metadata.
            Diagnostics.Assert(this.tempKeyBuffer.BaseStream.Position >= 32, this.traceType, "Key record size is greater than or equal to 32 bytes.");
            var totalRecordSize = (int) this.tempKeyBuffer.BaseStream.Position;

            this.keyBuffer.Write(this.tempKeyBuffer.BaseStream.GetBuffer(), 0, totalRecordSize);

            // Reset temporary stream.
            this.tempKeyBuffer.BaseStream.Position = 0;

            Diagnostics.Assert(this.currentBlockPosition + totalRecordSize <= this.blockAlignmentSize, this.traceType, "Key records cannot exceed blocksize");

            // Update current block position.
            this.currentBlockPosition += totalRecordSize;

            if (this.currentBlockPosition + ChecksumSize == this.blockAlignmentSize)
            {
                // write checksum since we are done with this 4k block.
                this.WriteEndOfBlock();

                await this.ResetCurrentBlockPositionAndFlushIfNecessaryAsync().ConfigureAwait(false);

                // reset block size to default.
                this.blockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;
            }
        }

        private void GetBlockSize(int recordSize)
        {
            // set block size to be a multiple of 4k bigger than the given value size.
            if (recordSize%BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize == 0)
            {
                this.blockAlignmentSize = recordSize;
            }
            else
            {
                this.blockAlignmentSize = (BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize*
                                           (recordSize/BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize + 1));
            }
        }

        /// <summary>
        /// This function not only writes the end of the block (checksum) but also 
        /// writes the metadata about the block (size and reserved fields in the beginning.
        /// </summary>
        /// <remarks>
        /// The data is written is 8 bytes aligned.
        /// 
        /// Name                    Type        Size
        /// 
        /// Metadata
        /// BlockSize               int         4
        /// RESERVED                            4
        /// 
        /// Block                   byte[]      N
        /// PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
        ///
        /// Checksum                ulong       8
        /// 
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// 
        /// Note: Larges Block supported is 2GB
        /// </remarks>
        /// <remarks>
        /// MCoskun: Note that KeyBlockMetadata.BlockSize includes KeyBlockMetadata and KeyBlock.
        /// It does not include the checksum
        /// </remarks>
        /// <remarks>
        /// MCoskun: Note that checksum includes the KeyBlockMetadata and KeyBlock.
        /// </remarks>
        private void WriteEndOfBlock()
        {
            Diagnostics.Assert(this.currentBlockPosition > 0, this.traceType, "Current block position should be higher than zero when end of block has reached");
            Diagnostics.Assert(this.currentBlockPosition < this.blockAlignmentSize, this.traceType, "It should be strictly lesser than the blockalignment size");
            this.keyBuffer.AssertIfNotAligned();

            // Write blocksize and checksum.
            var blockEndPosition = (int) this.keyBuffer.BaseStream.Position;
            var blockStartPosition = (int) (blockEndPosition - this.currentBlockPosition);

            // Move buffer to start position.
            this.keyBuffer.BaseStream.Position = blockStartPosition;

            // Write block size - current block position + checksum will account for block size.
            var blockSize = this.currentBlockPosition + ChecksumSize;
            Diagnostics.Assert(blockSize > 0, this.traceType, "Block size should be greater than zero.");
            var keyBlockMetadata = new KeyChunkMetadata(blockSize);
            keyBlockMetadata.Write(this.keyBuffer);

            this.keyBuffer.AssertIfNotAligned();

            // Move to end  of stream to write checksum.
            this.keyBuffer.BaseStream.Position = blockEndPosition;
            var checksum = this.keyBuffer.GetChecksum(blockStartPosition, this.currentBlockPosition);
            this.keyBuffer.Write(checksum);

            this.keyBuffer.AssertIfNotAligned();

            this.currentBlockPosition += ChecksumSize;
        }

        private async Task ResetCurrentBlockPositionAndFlushIfNecessaryAsync()
        {
            // Reset current block position.
            this.currentBlockPosition = 0;

            // Flush the memory buffer periodically to disk.
            if (this.keyBuffer.BaseStream.Position >= KeyCheckpointFile.MemoryBufferFlushSize)
            {
                await this.keyCheckpointFile.FlushMemoryBufferAsync(this.keyFileStream, this.keyBuffer).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Reserves enough space for the KeyBlockMetadata.
        /// </summary>
        private void ReserveSpaceForKeyBlockMetadata()
        {
            Diagnostics.Assert(this.currentBlockPosition == 0, this.traceType, "Current block position should be zero.");

            // Write size at the start of every flush block.
            // Leave space for an int 'size' at the front of the memory stream.
            this.keyBuffer.BaseStream.Position += KeyChunkMetadata.Size;

            // Move current position to size + Reserved.
            this.currentBlockPosition += KeyChunkMetadata.Size;
        }
    }
}