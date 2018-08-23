// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Interop;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.ReplicatedStore.Diagnostics;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Represents a TStore checkpoint.
    /// </summary>
    internal sealed class CheckpointFile : IDisposable
    {
        /// <summary>
        /// Tracing type information.
        /// </summary>
        private readonly string TraceType;

        private string FileNameBase;
        private KeyCheckpointFile KeyCheckpointFile;
        private ValueCheckpointFile ValueCheckpointFile;
        private bool disposed = false;
        private long KeyCheckpointFileSize = -1;
        private long ValueCheckpointFileSize = -1;
        public string KeyCheckpointFileName
        {
            get { return this.FileNameBase + KeyCheckpointFile.FileExtension; }
        }

        public string ValueCheckpointFileName
        {
            get { return this.FileNameBase + ValueCheckpointFile.FileExtension; }
        }

        public long KeyCount
        {
            get { return this.KeyCheckpointFile.KeyCount; }
        }

        public long ValueCount
        {
            get { return this.ValueCheckpointFile.ValueCount; }
        }

        public long DeletedKeyCount
        {
            get { return this.KeyCount - this.ValueCount; }
        }

        public BlockHandle KeyBlockHandle
        {
            get { return this.KeyCheckpointFile.Properties.KeysHandle; }
        }

        public BlockHandle ValueBlockHandle
        {
            get { return this.ValueCheckpointFile.Properties.ValuesHandle; }
        }

        public CheckpointFile(string filename, KeyCheckpointFile keyCheckpointFile, ValueCheckpointFile valueCheckpointName, string traceType)
        {
            // Reserved for private static CheckpointFile.OpenAsync() or CheckpointFile.CreateAsync().
            this.FileNameBase = filename;
            this.KeyCheckpointFile = keyCheckpointFile;
            this.ValueCheckpointFile = valueCheckpointName;
            this.TraceType = traceType;
        }

        /// <summary>
        /// Gets the total size of the checkpoint on disk (key + value file)
        /// </summary>
        /// <returns>Size of the checkpoint in bytes.</returns>
        public long GetFileSize()
        {
            return GetKeyCheckpointFileSize() + GetValueCheckpointFileSize();
        }

        public long GetKeyCheckpointFileSize()
        {
            if (KeyCheckpointFileSize == -1)
            {
                KeyCheckpointFileSize = FabricFile.GetSize(this.KeyCheckpointFileName);
            }
            return KeyCheckpointFileSize;
        }

        public long GetValueCheckpointFileSize()
        { 
            if (ValueCheckpointFileSize == -1)
            {
                ValueCheckpointFileSize = FabricFile.GetSize(this.ValueCheckpointFileName);
            }
            return ValueCheckpointFileSize;
        }

        public void SetValueFileIoPriority(Kernel32Types.PRIORITY_HINT priorityHint)
        {
            this.ValueCheckpointFile.SetIoPriority(priorityHint);
        }

        /// <summary>
        /// Disposes the table's stream.
        /// </summary>
        public void Dispose()
        {
            if(this.disposed)
            {
                return;
            }

            if (this.KeyCheckpointFile != null)
            {
                this.KeyCheckpointFile.Dispose();
            }

            if (this.ValueCheckpointFile != null)
            {
                this.ValueCheckpointFile.Dispose();
            }

            this.disposed = true;
        }

        /// <summary>
        /// Opens a TStore checkpoint from the given file name.
        /// </summary>
        /// <param name="filename">The file to open that contains an existing checkpoint.</param>
        /// <param name="traceType">Tracing information.</param>
        /// <param name="isValueReferenceType"></param>
        public static async Task<CheckpointFile> OpenAsync(string filename, string traceType, bool isValueReferenceType)
        {
            KeyCheckpointFile keyFile = null;
            ValueCheckpointFile valueFile = null;

            try
            {
                keyFile = await KeyCheckpointFile.OpenAsync(filename + KeyCheckpointFile.FileExtension, traceType, isValueReferenceType).ConfigureAwait(false);
                valueFile = await ValueCheckpointFile.OpenAsync(filename + ValueCheckpointFile.FileExtension, traceType).ConfigureAwait(false);

                return new CheckpointFile(filename, keyFile, valueFile, traceType);
            }
            catch (Exception)
            {
                // Ensure the key and value files get disposed quickly if we get an exception.
                // Normally the CheckpointFile would dispose them, but it may not get constructed.
                if (keyFile != null)
                    keyFile.Dispose();
                if (valueFile != null)
                    valueFile.Dispose();

                throw;
            }
        }

        /// <summary>
        /// Create a new <see cref="CheckpointFile"/> from the given file, serializing the given key-values into the file.
        /// </summary>
        /// <param name="fileId">File identifier.</param>
        /// <param name="filename">File to create and write to.</param>
        /// <param name="sortedItemData">Sorted key-value pairs to include in the table.</param>
        /// <param name="keySerializer"></param>
        /// <param name="valueSerializer"></param>
        /// <param name="timeStamp"></param>
        /// <param name="traceType">Tracing information.</param>
        /// <param name="perfCounters">Store performance counters instance.</param>
        /// <param name="isValueAReferenceType">Indicated if the value type is reference type.</param>
        /// <returns>The new <see cref="CheckpointFile"/>.</returns>
        public static async Task<CheckpointFile> CreateAsync<TKey, TValue>(
            uint fileId,
            string filename,
            IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> sortedItemData,
            IStateSerializer<TKey> keySerializer,
            IStateSerializer<TValue> valueSerializer,
            long timeStamp,
            string traceType,
            FabricPerformanceCounterSetInstance perfCounters,
            bool isValueAReferenceType)
        {
            var keyFileName = filename + KeyCheckpointFile.FileExtension;
            var valueFileName = filename + ValueCheckpointFile.FileExtension;

            var keyFile = new KeyCheckpointFile(keyFileName, fileId, isValueAReferenceType);
            var valueFile = new ValueCheckpointFile(valueFileName, fileId, traceType);

            var checkpointFile = new CheckpointFile(filename, keyFile, valueFile, traceType);

            var perfCounterWriter = new TStoreCheckpointRateCounterWriter(perfCounters);

            // Create the key checkpoint file and memory buffer.
            // MCoskun: Creation of checkpoint file's IO priority is not set.
            // Reason: Checkpointing is a life-cycle operation for State Provider that can cause throttling of backup, copy and (extreme cases) write operations.
            using (var keyFileStream = FabricFile.Open(keyFileName, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 4096, FileOptions.Asynchronous))
            using (var keyMemoryBuffer = new InMemoryBinaryWriter(new MemoryStream(capacity: 64*1024)))
            {
                // Create the value checkpoint file and memory buffer.
                // MCoskun: Creation of checkpoint file's IO priority is not set.
                // Reason: Checkpointing is a life-cycle operation for State Provider that can cause throttling of backup, copy and (extreme cases) write operations.
                using (var valueFileStream = FabricFile.Open(valueFileName, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 4096, FileOptions.Asynchronous))
                using (var valueMemoryBuffer = new InMemoryBinaryWriter(new MemoryStream(capacity: 64*1024)))
                {
                    var blockAlignedWriter = new BlockAlignedWriter<TKey, TValue>(
                        valueFileStream,
                        keyFileStream,
                        valueFile,
                        keyFile,
                        valueMemoryBuffer,
                        keyMemoryBuffer,
                        valueSerializer,
                        keySerializer,
                        timeStamp,
                        traceType);

                    perfCounterWriter.StartMeasurement();

                    foreach (var item in sortedItemData)
                    {
                        await blockAlignedWriter.BlockAlignedWriteItemAsync(item, null, true).ConfigureAwait(false);
                    }

                    // Flush both key and value checkpoints to disk.
                    await blockAlignedWriter.FlushAsync().ConfigureAwait(false);

                    perfCounterWriter.StopMeasurement();
                }
            }

            long writeBytes = checkpointFile.GetFileSize();
            long writeBytesPerSec = perfCounterWriter.UpdatePerformanceCounter(writeBytes);
#if !DotNetCoreClr
            FabricEvents.Events.CheckpointFileWriteBytesPerSec(traceType, writeBytesPerSec);
#endif

            return checkpointFile;
        }

        /// <summary>
        /// Read the given value from disk.
        /// </summary>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="item"></param>
        /// <param name="valueSerializer"></param>
        /// <returns></returns>
        public Task<TValue> ReadValueAsync<TValue>(TVersionedItem<TValue> item, IStateSerializer<TValue> valueSerializer)
        {
            return this.ValueCheckpointFile.ReadValueAsync(item, valueSerializer);
        }

        /// <summary>
        /// Read the given value from disk.
        /// </summary>
        /// <typeparam name="TValue"></typeparam>
        /// <param name="item"></param>
        /// <returns></returns>
        public Task<byte[]> ReadValueAsync<TValue>(TVersionedItem<TValue> item)
        {
            return this.ValueCheckpointFile.ReadValueAsync(item);
        }

        public KeyCheckpointFileAsyncEnumerator<TKey, TValue> GetAsyncEnumerator<TKey, TValue>(
            IStateSerializer<TKey> keySerializer, 
            string traceType, 
            Kernel32Types.PRIORITY_HINT priorityHint = Kernel32Types.PRIORITY_HINT.IoPriorityHintNormal)
        {
            return this.KeyCheckpointFile.GetAsyncEnumerator<TKey, TValue>(keySerializer, traceType, priorityHint);
        }
    }
}