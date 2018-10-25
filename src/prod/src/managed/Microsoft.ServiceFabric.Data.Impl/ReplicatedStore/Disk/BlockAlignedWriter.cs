// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Writes data aligned to a specified block size.
    /// </summary>
    internal class BlockAlignedWriter<TKey, TValue>
    {
        // TODO: Make it configurable.
        public const int DefaultBlockAlignmentSize = ValueCheckpointFile.DefaultBlockAlignmentSize;

        private ValueCheckpointFile valueCheckpointFile;
        private KeyCheckpointFile keyCheckpointFile;
        private InMemoryBinaryWriter valueBuffer;
        private InMemoryBinaryWriter keyBuffer;
        private FileStream valueFileStream;
        private FileStream keyFileStream;
        private long timeStamp;
        private IStateSerializer<TKey> keySerializer;
        private IStateSerializer<TValue> valueSerializer;
        private readonly string traceType;

        private KeyBlockAlignedWriter<TKey, TValue> keyBlockAlignedWriter;
        private ValueBlockAlignedWriter<TKey, TValue> valueBlockAlignedWriter;

        private static byte[] BadFood = new byte[] {0x0B, 0xAD, 0xF0, 0x0D};
        private static int BadFoodLength = BadFood.Length;

        public BlockAlignedWriter(
            FileStream valueFileStream, FileStream keyFileStream, ValueCheckpointFile valueCheckpointFile, KeyCheckpointFile keyCheckpointFile,
            InMemoryBinaryWriter valueBuffer, InMemoryBinaryWriter keyBuffer, IStateSerializer<TValue> valueSerializer, IStateSerializer<TKey> keySerializer,
            long timeStamp, string traceType)
        {
            this.valueCheckpointFile = valueCheckpointFile;
            this.keyCheckpointFile = keyCheckpointFile;
            this.timeStamp = timeStamp;
            this.valueFileStream = valueFileStream;
            this.keyFileStream = keyFileStream;
            this.valueBuffer = valueBuffer;
            this.keyBuffer = keyBuffer;
            this.valueSerializer = valueSerializer;
            this.keySerializer = keySerializer;

            this.traceType = traceType;

            this.keyBlockAlignedWriter = new KeyBlockAlignedWriter<TKey, TValue>(
                keyFileStream,
                keyCheckpointFile,
                keyBuffer,
                keySerializer,
                timeStamp,
                traceType);
            this.valueBlockAlignedWriter = new ValueBlockAlignedWriter<TKey, TValue>(
                valueFileStream,
                valueCheckpointFile,
                valueBuffer,
                valueSerializer,
                traceType);
        }

        public async Task BlockAlignedWriteItemAsync(KeyValuePair<TKey, TVersionedItem<TValue>> item, byte[] value, bool shouldSerialize)
        {
            await this.valueBlockAlignedWriter.BlockAlignedWriteValueAsync(item, value, shouldSerialize).ConfigureAwait(false);
            await this.keyBlockAlignedWriter.BlockAlignedWriteKeyAsync(item).ConfigureAwait(false);
        }

        public async Task FlushAsync()
        {
            await this.keyBlockAlignedWriter.FlushAsync().ConfigureAwait(false);
            await this.valueBlockAlignedWriter.FlushAsync().ConfigureAwait(false);
        }

        public static void WriteBadFood(InMemoryBinaryWriter binaryWriter, int size)
        {
            var quotient = size/BadFoodLength;
            for (var i = 0; i < quotient; i++)
            {
                binaryWriter.Write(BadFood);
            }

            var remainder = size%BadFoodLength;
            binaryWriter.Write(BadFood, 0, remainder);
        }
    }
}