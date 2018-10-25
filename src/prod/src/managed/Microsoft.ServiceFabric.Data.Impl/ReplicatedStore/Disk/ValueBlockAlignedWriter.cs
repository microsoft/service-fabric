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
    /// Wtites data aligned to a sepcified block size.
    /// </summary>
    internal class ValueBlockAlignedWriter<TKey, TValue>
    {
        private int blockAlignmentSize;
        private int currentBlockPosition = 0;

        private ValueCheckpointFile valueCheckpointFile;
        private InMemoryBinaryWriter valueBuffer;
        private FileStream valueFileStream;
        private IStateSerializer<TValue> valueSerializer;
        private string traceType;

        public ValueBlockAlignedWriter(
            FileStream valueFileStream, ValueCheckpointFile valueCheckpointFile, InMemoryBinaryWriter valueBuffer, IStateSerializer<TValue> valueSerializer,
            string traceType)
        {
            this.valueCheckpointFile = valueCheckpointFile;
            this.valueFileStream = valueFileStream;
            this.valueBuffer = valueBuffer;
            this.valueSerializer = valueSerializer;

            this.traceType = traceType;
            this.blockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;
        }

        public async Task BlockAlignedWriteValueAsync(KeyValuePair<TKey, TVersionedItem<TValue>> item, byte[] value, bool shouldSerialize)
        {
            if (item.Value.Kind == RecordKind.DeletedVersion)
            {
                this.valueCheckpointFile.WriteItem(this.valueFileStream, this.valueBuffer, item.Value, this.valueSerializer, this.traceType);

                return;
            }

            // Case 1: The value size is smaller than 4k
            if (item.Value.ValueSize <= this.blockAlignmentSize)
            {
                // If it falls within the block
                if (this.currentBlockPosition + item.Value.ValueSize <= this.blockAlignmentSize)
                {
                    await this.WriteItemAsync(item, value, shouldSerialize).ConfigureAwait(false);
                }
                else
                {
                    // Value does not fit into the current block, move position to the 4k+1 
                    //this.valueBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                    var skipSize = this.blockAlignmentSize - this.currentBlockPosition;
                    BlockAlignedWriter<TKey, TValue>.WriteBadFood(this.valueBuffer, skipSize);

                    // Reset current position.
                    this.currentBlockPosition = 0;

                    // Reset block size to default if value size is smaller, else pick the size correctly.
                    if (item.Value.ValueSize <= BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize)
                    {
                        this.blockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;
                    }
                    else
                    {
                        this.SetBlockSize(item);
                    }

                    // Write key and value
                    await this.WriteItemAsync(item, value, shouldSerialize).ConfigureAwait(false);
                }
            }
            else
            {
                // Case 2: Value size is greater than block size
                if (this.currentBlockPosition > 0)
                {
                    // Move the memory stream ahead to a 4k boundary.
                    // Value does not fit into the current block, move position to the 4k+1 
                    // this.valueBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);

                    var skipSize = this.blockAlignmentSize - this.currentBlockPosition;
                    BlockAlignedWriter<TKey, TValue>.WriteBadFood(this.valueBuffer, skipSize);

                    // Reset current position.
                    this.currentBlockPosition = 0;
                }

                // set block size to be a multiple of 4k bigger than the given value size.
                this.SetBlockSize(item);

                // Write key and value
                await this.WriteItemAsync(item, value, shouldSerialize).ConfigureAwait(false);
            }
        }

        public async Task FlushAsync()
        {
            if (this.currentBlockPosition > 0)
            {
                // Move the memory stream ahead to a 4k boundary.
                //this.valueBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);

                var skipSize = this.blockAlignmentSize - this.currentBlockPosition;
                BlockAlignedWriter<TKey, TValue>.WriteBadFood(this.valueBuffer, skipSize);
            }

            await this.valueCheckpointFile.FlushAsync(this.valueFileStream, this.valueBuffer).ConfigureAwait(false);
        }

        private void SetBlockSize(KeyValuePair<TKey, TVersionedItem<TValue>> item)
        {
            // set block size to be a multiple of 4k bigger than the given value size.
            if (item.Value.ValueSize%BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize == 0)
            {
                this.blockAlignmentSize = item.Value.ValueSize;
            }
            else
            {
                this.blockAlignmentSize = (BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize*
                                           (item.Value.ValueSize/BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize + 1));
            }
        }

        private async Task WriteItemAsync(KeyValuePair<TKey, TVersionedItem<TValue>> item, byte[] serializedValue, bool shouldSerialize)
        {
            // Consistency checks.
            Diagnostics.Assert(this.currentBlockPosition < this.blockAlignmentSize, this.traceType, "Current block position beyond block alignment size.");

            // Write the value.
            var startPosition = checked((int) this.valueBuffer.BaseStream.Position);
            var startBlockPosition = this.currentBlockPosition;
            var originalValueSize = item.Value.ValueSize;
            Diagnostics.Assert(originalValueSize >= 0, this.traceType, "Original value size should be greater than or equal to zero.");

            if (shouldSerialize)
            {
                this.valueCheckpointFile.WriteItem(this.valueFileStream, this.valueBuffer, item.Value, this.valueSerializer, this.traceType);
            }
            else
            {
                this.valueCheckpointFile.WriteItem(this.valueFileStream, this.valueBuffer, item.Value, serializedValue, this.traceType);
            }

            var endPosition = checked((int) this.valueBuffer.BaseStream.Position);
            var valueSize = endPosition - startPosition;
            this.currentBlockPosition += valueSize;

            // Validate value size.
            if (valueSize > originalValueSize)
            {
                if (this.currentBlockPosition > this.blockAlignmentSize)
                {
                    var shiftOffset = this.blockAlignmentSize - startBlockPosition;
                    var dstOffset = startPosition + shiftOffset;

                    // Ensure the memory stream has enough space to copy the bytes.
                    if (this.valueBuffer.BaseStream.Length < dstOffset + valueSize)
                    {
                        this.valueBuffer.BaseStream.SetLength(dstOffset + valueSize);
                    }

                    // Calculate the new block.
                    this.currentBlockPosition = 0;
                    this.SetBlockSize(item);

                    var buffer = this.valueBuffer.BaseStream.GetBuffer();
                    Buffer.BlockCopy(buffer, startPosition, buffer, dstOffset, valueSize);
                    item.Value.Offset += shiftOffset;
                    this.valueBuffer.BaseStream.Position = dstOffset + valueSize;

                    this.currentBlockPosition += valueSize;
                }
            }

            // Flush the memory buffer periodically to disk.
            if (this.valueBuffer.BaseStream.Position >= ValueCheckpointFile.MemoryBufferFlushSize)
            {
                await this.valueCheckpointFile.WriteToFileBufferAsync(this.valueFileStream, this.valueBuffer).ConfigureAwait(false);
            }

            // Assert that the current block position is lesser than or equal to BlockAlignmentSize.
            Diagnostics.Assert(
                this.currentBlockPosition <= this.blockAlignmentSize,
                this.traceType,
                "Inconsistent serialization- serialization produces varying byte length for the same value.");

            if (this.currentBlockPosition == this.blockAlignmentSize)
            {
                // Reset current position.
                this.currentBlockPosition = 0;

                // reset block size to default.
                this.blockAlignmentSize = BlockAlignedWriter<TKey, TValue>.DefaultBlockAlignmentSize;
            }
        }
    }
}