// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.ReplicatedStore.DifferentialStore
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using System.Fabric.Store;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data.DataStructures;

    /// <summary>
    /// Temporary store component used for restoring.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    /// <typeparam name="TKeyRangePartitioner">Key range partitioner type.</typeparam>
    internal class RecoveryStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> : IDisposable
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        private const uint InvalidFileId = 0;
        private const long InvalidTimeStamp = 0;

        private readonly MetadataTable metadataTable;

        private readonly string workDirectory;

        private readonly string traceType;

        private readonly IStateSerializer<TKey> keyConverter;

        private uint fileId;

        private long logicalCheckpointFileTimeStamp;

        /// <summary>
        /// Used for order by clauses.
        /// </summary>
        private static TKeyComparer KeyComparer = new TKeyComparer();

        private readonly PartitionedSortedList<TKey, TVersionedItem<TValue>> component;

        private readonly bool isValueReferenceType;

        /// <summary>
        /// Initialize a new instance of the RecoveryStoreComponent class.
        /// </summary>
        /// <param name="metadataTable"></param>
        /// <param name="workDirectory"></param>
        /// <param name="traceType"></param>
        /// <param name="keyConverter"></param>
        /// <param name="isValueReferenceType"></param>
        public RecoveryStoreComponent(MetadataTable metadataTable, string workDirectory, string traceType, IStateSerializer<TKey> keyConverter, bool isValueReferenceType)
        {
            this.metadataTable = metadataTable;
            this.workDirectory = workDirectory;
            this.traceType = traceType;
            this.keyConverter = keyConverter;

            this.fileId = InvalidFileId;
            this.logicalCheckpointFileTimeStamp = InvalidTimeStamp;
            this.component = new PartitionedSortedList<TKey, TVersionedItem<TValue>>(KeyComparer);
            this.isValueReferenceType = isValueReferenceType;
        }

        public uint FileId
        {
            get
            {
                return this.fileId;
            }
        }

        public long LogicalCheckpointFileTimeStamp
        {
            get
            {
                // If there was no file, logicalCheckpointFileTimeStamp will be invalid, otherwise greater than invalid.
                Diagnostics.Assert(this.logicalCheckpointFileTimeStamp >= InvalidTimeStamp, this.traceType, "LogicalCheckpointFileTimeStamp must be greater than or equal to 0");
                return this.logicalCheckpointFileTimeStamp;
            }
        }

        /// <summary>
        /// Disposing the component to release resources as soon as possible.
        /// </summary>
        public void Dispose()
        {
            // Clear is idempotent.
            this.component.Clear();
        }

        /// <summary>
        /// Enumerates the rows in the store component.
        /// </summary>
        /// <returns></returns>
        public IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> GetEnumerable()
        {
            return this.component;
        }

        /// <summary>
        /// Recover the state.
        /// </summary>
        /// <returns></returns>
        public async Task RecoveryAsync(CancellationToken cancellationToken)
        {
            Diagnostics.Assert(metadataTable != null, this.traceType, "metadataTable cannot be null");
            Diagnostics.Assert(metadataTable.Table != null, this.traceType, "metadataTable.Table cannot be null");

            List<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> keyCheckpointFileList = new List<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>(this.metadataTable.Table.Count);

            try
            {
                foreach (var item in this.metadataTable.Table)
                {
                    var fileMetadata = item.Value;
                    Diagnostics.Assert(fileMetadata.CanBeDeleted == false, this.traceType, "fileMetadata.CanBeDeleted == false");

                    // Set file id.
                    if (fileMetadata.FileId > this.fileId)
                    {
                        this.fileId = fileMetadata.FileId;
                    }

                    // Set file timestamp.
                    if (fileMetadata.TimeStamp > this.logicalCheckpointFileTimeStamp)
                    {
                        this.logicalCheckpointFileTimeStamp = fileMetadata.TimeStamp;
                    }

                    var fileName = Path.Combine(this.workDirectory, fileMetadata.FileName);
                    fileMetadata.CheckpointFile = await CheckpointFile.OpenAsync(fileName, this.traceType, this.isValueReferenceType).ConfigureAwait(false);

                    // MCoskun: Checkpoint recoveries IO priority is not set.
                    // Reason: We do not know whether this replica's recovery is required for write quorum.
                    keyCheckpointFileList.Add(fileMetadata.CheckpointFile.GetAsyncEnumerator<TKey, TValue>(this.keyConverter, this.traceType));

#if !DotNetCoreClr
                    FabricEvents.Events.CheckpointFileMetadata(this.traceType, DifferentialStoreConstants.Recovery_MergeSelected, fileMetadata.FileName, fileMetadata.FileId,
                        fileMetadata.TotalNumberOfEntries, fileMetadata.NumberOfDeletedEntries, fileMetadata.NumberOfInvalidEntries);
#endif

                    // reset invalid count as it may contain incorrect value
                    fileMetadata.ResetInvalidCount();
                }

                await this.MergeAsync(keyCheckpointFileList, cancellationToken);
            }
            finally
            {
                foreach (var keyCheckpointFile in keyCheckpointFileList)
                {
                    keyCheckpointFile.Dispose();
                }
            }
        }

        /// <summary>
        /// Merges multiple key checkpoint files in to a list.
        /// </summary>
        /// <param name="keyCheckpointFileEnumeratorList">List of key checkpoints to merge.</param>
        /// <param name="cancellationToken">Used to signal cancellation</param>
        /// <returns></returns>
        private async Task MergeAsync(IEnumerable<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> keyCheckpointFileEnumeratorList, CancellationToken cancellationToken)
        {
            var priorityQueue = new PriorityQueue<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>();

            FabricEvents.Events.MergeKeyCheckpointFilesAsync(this.traceType, DifferentialStoreConstants.MergeKeyCheckpointFilesAsync_Starting);

            // Get Enumerators for each file from the MetadataTable
            foreach (var keyCheckpointEnumerator in keyCheckpointFileEnumeratorList)
            {
                keyCheckpointEnumerator.KeyComparer = KeyComparer;

                // Move the enumerator once to make it point at the first item
                await keyCheckpointEnumerator.MoveNextAsync(cancellationToken);

                priorityQueue.Push(keyCheckpointEnumerator);
            }

            while (priorityQueue.Count != 0)
            {
                var currentEnumerator = priorityQueue.Pop();
                var row = currentEnumerator.Current;

                // Ignore all duplicate keys with smaller LSNs.
                while (priorityQueue.Count > 0 && KeyComparer.Compare(priorityQueue.Peek().Current.Key, row.Key) == 0)
                {
                    var poppedEnumerator = priorityQueue.Pop();
                    var skipRow = poppedEnumerator.Current;

                    // Verify the item being skipped has an earlier LSN than the one being returned.
                    var skipLsn = skipRow.Value.VersionSequenceNumber;
                    Diagnostics.Assert(skipLsn <= row.Value.VersionSequenceNumber, this.traceType, "Enumeration is returning a key with an earlier LSN.");

                    var fileMetadata = this.metadataTable.Table[skipRow.Value.FileId];

                    if (skipRow.Value.Kind != RecordKind.DeletedVersion)
                    {
                        fileMetadata.AddInvalidKey(skipRow.Key);
                    }

                    if (skipRow.Value.Kind == RecordKind.DeletedVersion)
                    {
                        fileMetadata.UpdateDeleteEntryTimeStamps(skipRow.TimeStamp);
                    }

                    if (await poppedEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                    {
                        priorityQueue.Push(poppedEnumerator);
                    }
                    else
                    {
                        poppedEnumerator.Dispose();
                    }
                }

                this.AddOrUpdate(row.Key, row.Value, row.TimeStamp);

                if (await currentEnumerator.MoveNextAsync(cancellationToken) == true)
                {
                    priorityQueue.Push(currentEnumerator);
                }
            }

#if !DotNetCoreClr
            foreach (var item in this.metadataTable.Table)
            {
                var fileMetadata = item.Value;
                FabricEvents.Events.CheckpointFileMetadata(this.traceType, DifferentialStoreConstants.AfterRecoveryMerge, fileMetadata.FileName, fileMetadata.FileId,
                    fileMetadata.TotalNumberOfEntries, fileMetadata.NumberOfDeletedEntries, fileMetadata.NumberOfInvalidEntries);
            }
#endif

            FabricEvents.Events.MergeKeyCheckpointFilesAsync(this.traceType, DifferentialStoreConstants.MergeKeyCheckpointFilesAsync_Completed);

#if DEBUG
            this.VerifyStoreComponent();
            this.VerifyFileMetadata();
#endif
        }

        /// <summary>
        /// Verifies the state of the recovery store componenet.
        /// </summary>
        /// <remarks>
        /// Ensures that all items are in key order and there are no duplicates.
        /// </remarks>
        private void VerifyStoreComponent()
        {
            if (this.component.Count == 0)
            {
                return;
            }
            
            using (var keysEnumerator = this.component.Keys.GetEnumerator())
            {
                keysEnumerator.MoveNext();
                var lastItem = keysEnumerator.Current;

                while (keysEnumerator.MoveNext())
                {
                    var key = keysEnumerator.Current;
                    int keyComparison = KeyComparer.Compare(lastItem, key);
                    Diagnostics.Assert(keyComparison < 0, this.traceType, "last item must be larger than current");
                    lastItem = key;
                }
            }
        }

#if DEBUG
        /// <summary>
        /// Verifies in memory fileMetadata after recovery.
        /// </summary>
        /// <remarks>
        /// Ensures that metadata corresponding to deleted entries are valid
        /// </remarks>
        private void VerifyFileMetadata()
        {
            HashSet<long> checkpointFileTimeStampSet = new HashSet<long>();
            foreach (var item in this.metadataTable.Table)
            {
                var fileMetadata = item.Value;
                Diagnostics.Assert(!checkpointFileTimeStampSet.Contains(fileMetadata.TimeStamp), "Duplicate timestamps found");
                checkpointFileTimeStampSet.Add(fileMetadata.TimeStamp);
                fileMetadata.Validate();
            }
        }
#endif

        /// <summary>
        /// Adds a new entry to this store component.
        /// </summary>
        /// <param name="inputKey">Key to add.</param>
        /// <param name="inputValue">Value to add.</param>
        /// <param name="timeStamp">TimeStamp of entry.</param>
        /// <remarks>
        /// This function is not thread-safe.
        /// </remarks>
        private void AddOrUpdate(TKey inputKey, TVersionedItem<TValue> inputValue, long timeStamp)
        {
            // There are no concurrent checkpoint, so it is okay to do it without a lock.
            Diagnostics.Assert(inputValue != null, this.traceType, "value cannot be null");

            if (inputValue.Kind == RecordKind.DeletedVersion)
            {
                var fileMetadata = this.metadataTable.Table[inputValue.FileId];
                fileMetadata.UpdateDeleteEntryTimeStamps(timeStamp);
            }

            // If this is a new key, append it to the end of the list.
            // List maintains count.
            if (this.component.Count == 0)
            {
                if (inputValue.Kind != RecordKind.DeletedVersion)
                {
                    this.component.Add(inputKey, inputValue);
                }
                return;
            }

            var lastItem = this.component.GetLastKey();

            int keyComparison = KeyComparer.Compare(lastItem, inputKey);
            Diagnostics.Assert(keyComparison <= 0, this.traceType, "input key must be greater than or equal to last key processed.");

            // If this is a new key, append it to the end of the list.
            if (keyComparison < 0)
            {
                if (inputValue.Kind != RecordKind.DeletedVersion)
                {
                    this.component.Add(inputKey, inputValue);
                }
                return;
            }

            Diagnostics.Assert(lastItem != null, this.traceType, "existingValue cannot be null");

            // Existing value is older than new version, accept the new version
            var lastItemValue = this.component[lastItem];
            if (lastItemValue.VersionSequenceNumber < inputValue.VersionSequenceNumber)
            {
                this.component.Add(inputKey, inputValue);
            }
            else if (lastItemValue.VersionSequenceNumber == inputValue.VersionSequenceNumber)
            {
                // This case only happen in copy and recovery cases where the log replays the operations which gets checkpointed.
                // To strengthen this assert, we could not taken in replayed items to differential in future.
                Diagnostics.Assert(lastItemValue.Kind == RecordKind.DeletedVersion, this.traceType, "Existing must be delete");
                Diagnostics.Assert(inputValue.Kind == RecordKind.DeletedVersion, this.traceType, "Input must be delete");
            }
        }
    }
}