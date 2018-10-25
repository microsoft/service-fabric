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
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.ReplicatedStore.Diagnostics;
    using Microsoft.ServiceFabric.Replicator;

    using Utility = Microsoft.ServiceFabric.Replicator.Utility;

    /// <summary>
    /// Consolidation manager manages in-memory consolidation, 
    /// </summary>
    /// <typeparam name="TKey"></typeparam>
    /// <typeparam name="TValue"></typeparam>
    /// <typeparam name="TKeyComparer"></typeparam>
    /// <typeparam name="TKeyEqualityComparer"></typeparam>
    /// <typeparam name="TKeyRangePartitioner"></typeparam>
    internal class ConsolidationManager<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
        IReadableStoreComponent<TKey, TVersionedItem<TValue>>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        private static TKeyComparer KeyComparer = new TKeyComparer();
        private AggregatedState<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> aggregatedState;
        private AggregatedState<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> newAggregatedState;
        private Object indexLock;
        private int snapshotOfHighestIndexOnConsolidation;
        private TStore<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> store;

        public ConsolidationManager(TStore<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> store)
        {
            this.store = store;
            this.aggregatedState = new AggregatedState<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(this.store.TraceType);
            this.indexLock = new Object();
            this.NumberOfDeltasToBeConsolidated = TStoreConstants.DefaultNumberOfDeltasTobeConsolidated;
        }

        /// <summary>
        /// Specifies the minimum number deltas to start consolidation.
        /// </summary>
        /// <devnote>Setter is used by unit tests only.</devnote>
        internal int NumberOfDeltasToBeConsolidated { get; set; }

        public void AppendToDeltaDifferentialState(TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> deltaState)
        {
            Diagnostics.Assert(deltaState != null, this.store.TraceType, "Delta state cannot be null");

            // Lock to protect against swapping with new aggregated state.
            lock (indexLock)
            {
                this.aggregatedState.AppendToDeltaDifferentialState(deltaState);

                // With background consolidation, this is no longer valid.
                //Diagnostics.InternalAssert(this.aggregatedState.Index <= this.NumberOfDeltasToBeConsolidated,
                //    "Number of deltas {0} should be leser than or equal to {1}", __arglist(this.aggregatedState.Index, this.NumberOfDeltasToBeConsolidated));

                if (this.aggregatedState.Index > this.NumberOfDeltasToBeConsolidated + 7)
                {
                    // Trace error for a large difference with index which indicates that bg consolidation is making very slow progress.
                    FabricEvents.Events.AppendToDeltaDifferentialState_SlowConsolidation(
                        this.store.TraceType,
                        this.aggregatedState.Index,
                        TStoreConstants.DefaultNumberOfDeltasTobeConsolidated);
                }
            }
        }

        public void Reset()
        {
            this.aggregatedState = new AggregatedState<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(this.store.TraceType);

            if (this.newAggregatedState != null)
            {
                this.newAggregatedState = null;
            }
        }

        public void ResetToNewAggregatedState()
        {
            // NewAggregatedState not null cannot be asserted since there can be cases when consolidation is not needed. Since this is co-ordintade by performcheckpoint,
            // not asserting here.
            if (this.newAggregatedState != null)
            {
                lock (indexLock)
                {
                    // Fetch new delta states, if any from aggregatedstate, before restting it
                    for (int i = this.snapshotOfHighestIndexOnConsolidation + 1; i <= this.aggregatedState.Index; i++)
                    {
                        TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> diffComponent;
                        this.aggregatedState.DeltaDifferentialStateList.TryGetValue(i, out diffComponent);
                        Diagnostics.Assert(diffComponent != null, this.store.TraceType, "All indexes should be present");

                        // Add it in the correct order.
                        this.newAggregatedState.AppendToDeltaDifferentialState(diffComponent);
                    }

                    this.aggregatedState = this.newAggregatedState;
                    this.newAggregatedState = null;
                    this.store.TryStartSweep();
                }
            }
        }
        public void Sweep(CancellationToken cancellationToken)
        {
            // Iterate the differential list and consolidated list
            foreach (var item in this.aggregatedState.ConsolidatedState.EnumerateValues())
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    break;
                }

                if (item.Kind != RecordKind.DeletedVersion)
                {
                    this.SweepItem(item);
                }
            }

            foreach (var item in this.aggregatedState.GetValuesForSweep())
            {
                if (item.Kind != RecordKind.DeletedVersion)
                {
                    this.SweepItem(item);
                }
            }
        }

        /// <summary>
        /// Merges sorted components (delta differentials and consolidated) in to new consolidated.
        /// </summary>
        public async Task<PostMergeMetadatableInformation> ConsolidateAsync(
            string traceType,
            FabricPerformanceCounterSetInstance perfCounters,
            MetadataTable metadataTable,
            ConsolidationMode mode,
            CancellationToken cancellationToken)
        {

            // Create write only store transaction not associated with a replicator transaction.
            var rwtx = new StoreReadWriteTransaction<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(
                TransactionBase.CreateTransactionId(),
                null,
                this.store,
                this.store.TraceType,
                null,
                false,
                false);

            try
            {
                try
                {
                    // Acquire shared lock on store component.
                    await rwtx.AcquirePrimeLocksAsync(this.store.LockManager, LockMode.Shared, TStoreConstants.CheckpointOrCopyLockTimeoutInMilliseconds, true).ConfigureAwait(false);

                    // If the operation was cancelled during the lock wait, then terminate.
                    cancellationToken.ThrowIfCancellationRequested();
                }
                catch (Exception e)
                {
                    // Check inner exception.
                    e = Diagnostics.ProcessException(string.Format("TStore.CheckpointAsync@{0}", this.store.TraceType), e, "store=acquiring prime locks ");
                    Diagnostics.Assert(e is TimeoutException || e is OperationCanceledException, this.store.TraceType, "unexpected exception {0}", e.GetType());

                    // Rethrow inner exception.
                    throw e;
                }

                // Assert that new aggregated state is null
                Diagnostics.Assert(this.newAggregatedState == null, this.store.TraceType, "New aggregated state should be null");
                Diagnostics.Assert(this.store.MergeMetadataTable == null, this.store.TraceType, "Merged metadata table should be null");

                this.snapshotOfHighestIndexOnConsolidation = this.aggregatedState.Index;

                // Index starts with 1, zero is not a valid index.
                if (this.snapshotOfHighestIndexOnConsolidation > 0)
                {
                    this.MovePreviousVersionItemsToSnapshotContainerIfNeeded(this.snapshotOfHighestIndexOnConsolidation, metadataTable);
                }

                // Assert that new aggregated state is null
                Diagnostics.Assert(this.newAggregatedState == null, this.store.TraceType, "New aggregated state should be null");

                if (this.aggregatedState.Index < this.NumberOfDeltasToBeConsolidated)
                {
                    return null;
                }

                PostMergeMetadatableInformation postMergeMetadatableInformation = null;

                // TODO: do we need to AddRef/ReleaseRef on metadataTable for background consolidation?

                var oldConsolidatedState = this.aggregatedState.ConsolidatedState;

                // Assumption: new consolidated state's size will be similar to old consolidated state's size.
                TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> newConsolidatedState = new TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(oldConsolidatedState.Count(), this.store.TraceType);
                var differentialEnumeratorsQueue = new PriorityQueue<DifferentialStateEnumerator<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>>();

                for (int i = this.snapshotOfHighestIndexOnConsolidation; i >= 1; i--)
                {
                    TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> differentialState;
                    this.aggregatedState.DeltaDifferentialStateList.TryGetValue(i, out differentialState);
                    Diagnostics.Assert(differentialState != null, this.store.TraceType, "All indexes should be present");

                    var enumerator = new DifferentialStateEnumerator<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(differentialState);

                    if (enumerator.MoveNext())
                    {
                        differentialEnumeratorsQueue.Push(enumerator);
                    }
                }

                var differentialStateEnumerator = this.GetDifferentialData(differentialEnumeratorsQueue, metadataTable).GetEnumerator();
                var consolidatedStateKeyEnumerator = this.aggregatedState.ConsolidatedState.EnumerateKeys().GetEnumerator();

                var isDifferentialDrained = !differentialStateEnumerator.MoveNext();
                var isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();

                var versionedItems = new List<TVersionedItem<TValue>>();
                while (isDifferentialDrained == false && isConsolidatedDrained == false)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    Diagnostics.Assert(versionedItems.Count == 0, this.store.TraceType, "must be empty");

                    var differentialKeyValuePair = differentialStateEnumerator.Current.KeyValuePair;
                    var oldDifferentialState = differentialStateEnumerator.Current.DifferentialState;
                    var consolidatedKey = consolidatedStateKeyEnumerator.Current;
                    var keyComparison = KeyComparer.Compare(differentialKeyValuePair.Key, consolidatedKey);

                    // Key in consolidation is smaller than key in differential.
                    if (keyComparison > 0)
                    {
                        var valueInConsolidatedState = oldConsolidatedState.Read(consolidatedKey);
                        Diagnostics.Assert(valueInConsolidatedState != null, this.store.TraceType, "Old consolidated must contain the item.");

                        // Note: On Hydrate we load all the items including deleted because we could read in any order. Ignore adding them here.
                        if (valueInConsolidatedState.Kind != RecordKind.DeletedVersion)
                        {
                            newConsolidatedState.Add(consolidatedKey, valueInConsolidatedState);
                        }

                        isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();
                        continue;
                    }

                    // Key in differential is same as key in consolidated.
                    else if (keyComparison == 0)
                    {
                        // Remove the value in consolidated.
                        var valueInConsolidatedState = oldConsolidatedState.Read(consolidatedKey);
                        Diagnostics.Assert(valueInConsolidatedState != null, this.store.TraceType, "Old consolidated must contain the item.");
                        versionedItems.Add(valueInConsolidatedState);

                        // Decrement number of valid entries.
                        FileMetadata fileMetadata = null;
                        if (!metadataTable.Table.TryGetValue(valueInConsolidatedState.FileId, out fileMetadata))
                        {
                            Diagnostics.Assert(
                                fileMetadata != null,
                                this.store.TraceType,
                                "Failed to find file metadata for versioned item in consolidated with file id {0}.",
                                valueInConsolidatedState.FileId);
                        }

                        fileMetadata.AddInvalidKey(differentialKeyValuePair.Key);

                        // Remove the value in differential.
                        var differentialVersions = oldDifferentialState.ReadVersions(differentialKeyValuePair.Key);
                        Diagnostics.Assert(differentialVersions != null, this.store.TraceType, "differential version should not be null");
                        var currentVersion = differentialVersions.CurrentVersion;
                        Diagnostics.Assert(currentVersion != null, this.store.TraceType, "current version should not be null");
                        Diagnostics.Assert(
                            valueInConsolidatedState.VersionSequenceNumber < currentVersion.VersionSequenceNumber,
                            this.store.TraceType,
                            "valueInConsolidatedState: {0} < currentVersion: {1}",
                            valueInConsolidatedState.VersionSequenceNumber, currentVersion.VersionSequenceNumber);
                        Diagnostics.Assert(
                            currentVersion.VersionSequenceNumber == differentialKeyValuePair.Value.VersionSequenceNumber,
                            this.store.TraceType,
                            "Current and the version acquired must be same.");

                        var previousVersion = differentialVersions.PreviousVersion;
                        if (previousVersion != null)
                        {
                            Diagnostics.Assert(
                                valueInConsolidatedState.VersionSequenceNumber < previousVersion.VersionSequenceNumber,
                                this.store.TraceType,
                                "valueInConsolidatedState: {0} < previousVersion: {1}",
                                valueInConsolidatedState.VersionSequenceNumber, previousVersion.VersionSequenceNumber);
                            versionedItems.Add(previousVersion);
                        }

                        versionedItems.Add(currentVersion);
                        this.ProcessToBeRemovedVersions(consolidatedKey, versionedItems, metadataTable);

                        if (differentialKeyValuePair.Value.Kind != RecordKind.DeletedVersion)
                        {
                            newConsolidatedState.Add(differentialKeyValuePair.Key, differentialKeyValuePair.Value);
                        }

                        isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();
                        isDifferentialDrained = !differentialStateEnumerator.MoveNext();
                        continue;
                    }
                    // Key in Differential is smaller than key in Consolidated.
                    else if (keyComparison < 0)
                    {
                        // Move the value in differential in to consolidated.
                        var differentialVersions = oldDifferentialState.ReadVersions(differentialKeyValuePair.Key);
                        Diagnostics.Assert(differentialVersions != null, this.store.TraceType, "differential version should not be null");
                        var currentVersion = differentialVersions.CurrentVersion;
                        Diagnostics.Assert(currentVersion != null, this.store.TraceType, "current version should not be null");
                        Diagnostics.Assert(
                            currentVersion.VersionSequenceNumber == differentialKeyValuePair.Value.VersionSequenceNumber,
                            this.store.TraceType,
                            "Current and the version acquired must be same.");

                        var previousVersion = differentialVersions.PreviousVersion;
                        if (previousVersion != null)
                        {
                            Diagnostics.Assert(
                                currentVersion.VersionSequenceNumber > previousVersion.VersionSequenceNumber,
                                this.store.TraceType,
                                "Current and the version acquired must be same.");

                            versionedItems.Add(previousVersion);
                            versionedItems.Add(currentVersion);
                            this.ProcessToBeRemovedVersions(differentialKeyValuePair.Key, versionedItems, metadataTable);
                        }

                        if (differentialKeyValuePair.Value.Kind != RecordKind.DeletedVersion)
                        {
                            newConsolidatedState.Add(differentialKeyValuePair.Key, differentialKeyValuePair.Value);
                        }

                        isDifferentialDrained = !differentialStateEnumerator.MoveNext();
                        continue;
                    }
                }

                if (isDifferentialDrained == true && isConsolidatedDrained == false)
                {
                    Diagnostics.Assert(versionedItems.Count == 0, this.store.TraceType, "must be empty");

                    do
                    {
                        var consolidatedKey = consolidatedStateKeyEnumerator.Current;
                        var valueInConsolidatedState = oldConsolidatedState.Read(consolidatedKey);
                        Diagnostics.Assert(valueInConsolidatedState != null, this.store.TraceType, "Old consolidated must contain the item.");

                        // Note: On Hydrate we load all the items including deleted because we could read in any order. Ignore adding them here.
                        if (valueInConsolidatedState.Kind != RecordKind.DeletedVersion)
                        {
                            newConsolidatedState.Add(consolidatedKey, valueInConsolidatedState);
                        }

                        isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();
                    } while (isConsolidatedDrained == false);
                }
                else if (isConsolidatedDrained == true && isDifferentialDrained == false)
                {
                    do
                    {
                        Diagnostics.Assert(versionedItems.Count == 0, this.store.TraceType, "must be empty");

                        var differentialKeyValuePair = differentialStateEnumerator.Current.KeyValuePair;
                        var oldDifferentialState = differentialStateEnumerator.Current.DifferentialState;

                        var differentialVersions = oldDifferentialState.ReadVersions(differentialKeyValuePair.Key);
                        Diagnostics.Assert(differentialVersions != null, this.store.TraceType, "differential version should not be null");
                        var currentVersion = differentialVersions.CurrentVersion;
                        Diagnostics.Assert(currentVersion != null, this.store.TraceType, "current version should not be null");
                        Diagnostics.Assert(
                            currentVersion.VersionSequenceNumber == differentialKeyValuePair.Value.VersionSequenceNumber,
                            this.store.TraceType,
                            "Current and the version acquired must be same.");

                        var previousVersion = differentialVersions.PreviousVersion;

                        if (previousVersion != null)
                        {
                            Diagnostics.Assert(
                                currentVersion.VersionSequenceNumber > previousVersion.VersionSequenceNumber,
                                this.store.TraceType,
                                "Current and the version acquired must be same.");

                            versionedItems.Add(previousVersion);
                            versionedItems.Add(currentVersion);
                            this.ProcessToBeRemovedVersions(differentialKeyValuePair.Key, versionedItems, metadataTable);
                        }

                        if (differentialKeyValuePair.Value.Kind != RecordKind.DeletedVersion)
                        {
                            newConsolidatedState.Add(differentialKeyValuePair.Key, differentialKeyValuePair.Value);
                        }

                        isDifferentialDrained = !differentialStateEnumerator.MoveNext();
                    } while (isDifferentialDrained == false);
                }

                // Call merge before switching states

                // If any files fall below the threshold, merge them together.
                List<uint> mergeFileIds = null;
                bool shouldMerge = this.store.MergeHelper.ShouldMerge(metadataTable, mode, out mergeFileIds);

                if (shouldMerge)
                {
                    postMergeMetadatableInformation = await this.MergeAsync(
                        metadataTable,
                        mergeFileIds,
                        newConsolidatedState,
                        perfCounters,
                        cancellationToken).ConfigureAwait(false);
                }

                this.newAggregatedState = new AggregatedState<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(newConsolidatedState, this.store.TraceType);

                if (this.store.EnableBackgroundConsolidation)
                {
                    // It is safe to reset aggregated to new aggregated state here before next metadata table is set to temp metdata table.
                    // New file metadata comes in either from merge or perf checkpoint.
                    // File metadata that comes from perf checkpoint is always in memory, will never be sweeped so it is okay to set the temp metadata table to next after consolidation manager has been reset.
                    // Reads will not need to lookup the filemetadata (this is the state that comes from differential and is moved to new aggregared state).
                    // File metadata that comes from merge has been included to merge metadata table. Merge metadata table is included for lookup on reads.
                    this.ResetToNewAggregatedState();
                }

                return postMergeMetadatableInformation;
            }
            finally
            {
                // Release all acquired store locks; if any.
                rwtx.Dispose();
            }
        }

        private void MovePreviousVersionItemsToSnapshotContainerIfNeeded(int highesIndex, MetadataTable metadataTable)
        {
            TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> differentialState;
            this.aggregatedState.DeltaDifferentialStateList.TryGetValue(highesIndex, out differentialState);
            Diagnostics.Assert(differentialState != null, this.store.TraceType, "All indexes should be present");

            // Preethas: TODO, move this under checkpoint file that enumerates over the key and value while writing into checkpoint file 
            // to avoid enumerating again.
            foreach (TKey key in differentialState.EnumerateKeys())
            {
                DifferentialStateVersions<TValue> diffStateVersions = differentialState.ReadVersions(key);
                if (diffStateVersions.PreviousVersion != null)
                {
                    this.ProcessToBeRemovedVersions(key, diffStateVersions.CurrentVersion, diffStateVersions.PreviousVersion, metadataTable);
                    diffStateVersions.PreviousVersion = null;
                }
            }
        }

        /// <summary>
        /// Retires old versions.
        /// Note that the list item in the versionItems list is the current item that must not be required.
        /// </summary>
        /// <param name="key">The key that all versions belong to.</param>
        /// <param name="versionedItems">Versioned items for a given key.</param>
        /// <param name="metadataTable">Metadata table used to move a file into snapshot container.</param>
        private void ProcessToBeRemovedVersions(TKey key, List<TVersionedItem<TValue>> versionedItems, MetadataTable metadataTable)
        {
            for (var i = 0; i < versionedItems.Count - 1; i++)
            {
                // Assert that the versionsequence is in the correct order.
                var versionToBeDeleted = versionedItems[i];
                var nextVersion = versionedItems[i + 1];

                this.ProcessToBeRemovedVersions(key, nextVersion, versionToBeDeleted, metadataTable);
            }

            versionedItems.Clear();
        }

        private void ProcessToBeRemovedVersions(TKey key, TVersionedItem<TValue> nextVersion, TVersionedItem<TValue> versionToBeDeleted, MetadataTable metadataTable)
        {
            // Assert that the versionsequence is in the correct order.
            var sequenceNumberToBeDeleted = versionToBeDeleted.VersionSequenceNumber;
            var nextSequenceNumber = nextVersion.VersionSequenceNumber;

            Diagnostics.Assert(sequenceNumberToBeDeleted < nextSequenceNumber, this.store.TraceType, "sequenceNumberToBeDeleted < nextSequenceNumber");

            var removeVersionResult = this.store.TransactionalReplicator.TryRemoveVersion(
                this.store.StateProviderId,
                sequenceNumberToBeDeleted,
                nextSequenceNumber);

            // Move to snapsot container, if needed
            if (!removeVersionResult.CanBeRemoved)
            {
                var enumerationSet = removeVersionResult.EnumerationSet;

                foreach (var snapshotVisibilityLsn in enumerationSet)
                {
                    var snapshotComponent =
                        this.store.SnapshotContainer.Read(snapshotVisibilityLsn);

                    if (snapshotComponent == null)
                    {
                        snapshotComponent = new SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(this.store.SnapshotContainer, this.store.LoadValueCounter, store.IsValueAReferenceType, this.store.TraceType);
                        snapshotComponent = this.store.SnapshotContainer.GetOrAdd(snapshotVisibilityLsn, snapshotComponent);
                    }

                    // Move file id for the versioned item, if it is not already present.
                    var fileId = versionToBeDeleted.FileId;
                    if (fileId > 0)
                    {
                        FileMetadata fileMetadata = null;
                        if (!metadataTable.Table.TryGetValue(fileId, out fileMetadata))
                        {
                            Diagnostics.Assert(
                                fileMetadata != null,
                                this.store.TraceType,
                                "Failed to find the file metadata for a versioned item with file id {0}.",
                                fileId);
                        }

                        // It is okay to not assert here that the lsn got added because duplicate adds could come in.
                        this.store.SnapshotContainer.TryAddFileMetadata(snapshotVisibilityLsn, fileMetadata);
                    }
                    else
                    {
                        // Should always be in memory, so safe to assert on value directly.
                        // This can happen only with previous version items from differential state.
                        this.store.AssertIfUnexpectedVersionedItemValue(versionToBeDeleted, versionToBeDeleted.Value);
                    }

                    // Add the key to the component.
                    snapshotComponent.Add(key, versionToBeDeleted);
                }

                // Wait on notifications and remove the entry from the container
                if (removeVersionResult.EnumerationCompletionNotifications != null)
                {
                    foreach (var enumerationResult in removeVersionResult.EnumerationCompletionNotifications)
                    {
                        enumerationResult.Notification.ContinueWith(x => { this.store.SnapshotContainer.Remove(enumerationResult.VisibilitySequenceNumber); })
                            .IgnoreExceptionVoid();
                    }
                }
            }
        }

        private void SweepItem(TVersionedItem<TValue> item)
        {
            Diagnostics.Assert(item.Kind != RecordKind.DeletedVersion, this.store.TraceType, "A deleted item kind is not expected to be sweeped.");
            Diagnostics.Assert(item.FileId > 0, this.store.TraceType, "An item qualified to be sweeped should have a valid file id.");


#if DEBUG
            Diagnostics.Assert(this.IsIdPresentInMetadataTable(item.FileId), this.store.TraceType, "File id {0} is not present in metadata table", item.FileId);
#endif

            TVersionedItem<TValue> outItem = item;

            // Do not sweep if is is a value type or if it has already been sweeped.
            // The second check could race with a reader trying to load the value, but in that case we should not be sweeping anyway 
            // and the subsequent sweep will address it.
            if (!this.store.IsValueAReferenceType || item.Value == null)
            {
                return;
            }

            if (item.InUse)
            {
                // Do not assert that value is not null because a reader sets the Inuse flag first before it loads the value, so the value can be null.
                // Diagnostics.InternalAssert(item.Value != null, "Value cannot be null");
                item.SetInUseToFalse(this.store.IsValueAReferenceType);
                return;
            }

            Diagnostics.Assert(item.CanBeSweepedToDisk, this.store.TraceType, "Can be sweeped to disk should be true.");

            item.Value = default(TValue);
        }

        private IEnumerable<DifferentialData<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>> GetDifferentialData(
            PriorityQueue<DifferentialStateEnumerator<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>> priorityQueue,
            MetadataTable metadataTable)
        {
            while (priorityQueue.Count > 0)
            {
                var enumerator = priorityQueue.Pop();
                var key = enumerator.Current.Key;
                var value = enumerator.Current.Value;
                var differentialState = enumerator.Component;

                var nextValue = value;
                // Ignore all duplicate keys with smaller LSNs.
                while (priorityQueue.Count > 0 && KeyComparer.Compare(priorityQueue.Peek().Current.Key, key) == 0)
                {
                    var poppedEnumerator = priorityQueue.Pop();

                    // Verify the item being skipped has an earlier LSN than the one being returned.
                    var skipLsn = poppedEnumerator.Current.Value.VersionSequenceNumber;
                    Diagnostics.Assert(skipLsn < value.VersionSequenceNumber, this.store.TraceType, "Enumeration is returning a key with an earlier LSN.");

                    var valueToBeDeleted = poppedEnumerator.Current.Value;

                    if (valueToBeDeleted.FileId != 0 && valueToBeDeleted.Kind != RecordKind.DeletedVersion)
                    {
                        // Decrement number of valid entries.
                        FileMetadata fileMetadata = null;
                        if (!metadataTable.Table.TryGetValue(valueToBeDeleted.FileId, out fileMetadata))
                        {
                            Diagnostics.Assert(
                                fileMetadata != null,
                                this.store.TraceType,
                                "Failed to find file metadata for versioned item in differential with file id {0}.",
                                valueToBeDeleted.FileId);
                        }

                        fileMetadata.AddInvalidKey(key);

                        Diagnostics.Assert(
                            fileMetadata.NumberOfValidEntries >= 0,
                            this.store.TraceType,
                            "Number of valid entries in file with file id {0} is negative {1}",
                            valueToBeDeleted.FileId,
                            fileMetadata.NumberOfValidEntries);

                        Diagnostics.Assert(
                            fileMetadata.NumberOfInvalidEntries > 0,
                            this.store.TraceType,
                            "Number of invalid entries in file with file id {0} must be greater than 0",
                            valueToBeDeleted.FileId);
                    }

                    // Check if if it needs to be moved into snapshot container.
                    this.ProcessToBeRemovedVersions(key, nextValue, valueToBeDeleted, metadataTable);

                    // This value becomes the next value.
                    nextValue = valueToBeDeleted;

                    if (poppedEnumerator.MoveNext())
                    {
                        priorityQueue.Push(poppedEnumerator);
                    }
                    else
                    {
                        poppedEnumerator.Dispose();
                    }
                }

                // Move the enumerator to the next key and add it back to the heap.
                if (enumerator.MoveNext())
                {
                    priorityQueue.Push(enumerator);
                }
                else
                {
                    enumerator.Dispose();
                }

                var kvp = new KeyValuePair<TKey, TVersionedItem<TValue>>(key, value);
                var differentialData = new DifferentialData<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>();
                differentialData.DifferentialState = differentialState;
                differentialData.KeyValuePair = kvp;

                yield return differentialData;
            }
        }

        private void SetCheckpointFileIoPriorities(MetadataTable mergeTable, List<uint> listOfFileIds, Kernel32Types.PRIORITY_HINT priorityHint, bool assertIfCheckpointDoesNotExist)
        {
            foreach (var fileIdToMerge in listOfFileIds)
            {
                bool isContained = mergeTable.Table.ContainsKey(fileIdToMerge);
                Diagnostics.Assert(!assertIfCheckpointDoesNotExist || isContained, this.store.TraceType, "If not merged, must be there.");

                if (isContained)
                {
                    var fileMetadata = mergeTable.Table[fileIdToMerge];
                    fileMetadata.CheckpointFile.SetValueFileIoPriority(priorityHint);
                }
            }
        }

        private async Task<PostMergeMetadatableInformation> MergeAsync(
            MetadataTable mergeTable,
            List<uint> listOfFileIds,
            TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> newConsolidatedState,
            FabricPerformanceCounterSetInstance perfCounters,
            CancellationToken cancellationToken)
        {
            return await this.MergeAsync(
                mergeTable,
                listOfFileIds,
                newConsolidatedState,
                perfCounters,
                cancellationToken,
                Kernel32Types.PRIORITY_HINT.IoPriorityHintLow).ConfigureAwait(false);
        }

        private async Task<PostMergeMetadatableInformation> MergeAsync(
            MetadataTable mergeTable, List<uint> listOfFileIds,
            TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> newConsolidatedState,
            FabricPerformanceCounterSetInstance perfCounters,
            CancellationToken cancellationToken,
            Kernel32Types.PRIORITY_HINT priorityHint)
        {
            FabricEvents.Events.MergeAsync(this.store.TraceType, "started");
            List<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> enumerators = new List<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>();

            try
            {
                PostMergeMetadatableInformation mergeMetadataTableInformation = null;
                FileMetadata mergedFileMetadata = null;
                var fileIsEmpty = true;
                var priorityQueue = new PriorityQueue<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>();

                // Get Enumerators for each file from the MetadataTable
                foreach (var fileIdToMerge in listOfFileIds)
                {
                    var fileMetadata = mergeTable.Table[fileIdToMerge];
                    var enumerator = fileMetadata.CheckpointFile.GetAsyncEnumerator<TKey, TValue>(this.store.KeyConverter, this.store.TraceType, priorityHint);
                    enumerator.KeyComparer = this.store.KeyComparer;

                    Diagnostics.Assert(await enumerator.MoveNextAsync(cancellationToken), this.store.TraceType, "KeyCheckpointFile enumerator had zero items.");

                    // Merge cannot be interrupted before enumertaor is added here so races here can be ignored.
                    enumerators.Add(enumerator);
                    priorityQueue.Push(enumerator);
#if !DotNetCoreClr
                    FabricEvents.Events.CheckpointFileMetadata(this.store.TraceType, DifferentialStoreConstants.Consolidation_MergeSelected, fileMetadata.FileName, fileMetadata.FileId,
                        fileMetadata.TotalNumberOfEntries, fileMetadata.NumberOfDeletedEntries, fileMetadata.NumberOfInvalidEntries);
#endif
#if DEBUG
                    // reset 
                    fileMetadata.numInvalidKeysForAssert = 0;
#endif
                }

                var fileName = Guid.NewGuid().ToString();
                var keyFileName = Path.Combine(store.WorkingDirectory, fileName + KeyCheckpointFile.FileExtension);
                var valueFileName = Path.Combine(this.store.WorkingDirectory, fileName + ValueCheckpointFile.FileExtension);
                var fileId = this.store.IncrementFileId();
                var timeStamp = this.store.NextCheckpointFileTimeStamp();
                long oldestDeletedEntryTimeStamp = FileMetadata.InvalidTimeStamp;
                long latestDeletedEntryTimeStamp = FileMetadata.InvalidTimeStamp;

                var keyFile = new KeyCheckpointFile(keyFileName, fileId, this.store.IsValueAReferenceType);
                var valueFile = new ValueCheckpointFile(valueFileName, fileId, this.store.TraceType, priorityHint);

                // Create the key checkpoint file and memory buffer.
                using (var keyFileStream = FabricFile.Open(keyFileName, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 4096, FileOptions.Asynchronous))
                using (var keyMemoryBuffer = new InMemoryBinaryWriter(new MemoryStream(capacity: 64 * 1024)))
                {
                    // Merge is a background operation that does not affect public APIs performance in most cases.
                    // Lower the io priority to allow IOs in the life-cycle and write-path to go through faster.
                    Utility.SetIoPriorityHint(keyFileStream.SafeFileHandle, priorityHint);

                    // Create the value checkpoint file and memory buffer.
                    using (var valueFileStream = FabricFile.Open(valueFileName, FileMode.Create, FileAccess.ReadWrite, FileShare.None, 4096, FileOptions.Asynchronous))
                    using (var valueMemoryBuffer = new InMemoryBinaryWriter(new MemoryStream(capacity: 64 * 1024)))
                    {
                        // Merge is a background operation that does not affect public APIs performance in most cases.
                        // Lower the io priority to allow IOs in the life-cycle and write-path to go through faster.
                        Utility.SetIoPriorityHint(valueFileStream.SafeFileHandle, priorityHint);

                        var blockAlignedWriter = new BlockAlignedWriter<TKey, TValue>(
                            valueFileStream,
                            keyFileStream,
                            valueFile,
                            keyFile,
                            valueMemoryBuffer,
                            keyMemoryBuffer,
                            this.store.ValueConverter,
                            this.store.KeyConverter,
                            timeStamp,
                            this.store.TraceType);

                        var checkpointRateCounterWriter = new TStoreCheckpointRateCounterWriter(perfCounters);

                        while (priorityQueue.Count > 0)
                        {
                            cancellationToken.ThrowIfCancellationRequested();
                            var enumeratorToWrite = priorityQueue.Pop();
                            var enumerator = enumeratorToWrite;

                            var keyToWrite = enumerator.Current.Key;
                            var valueToWrite = enumerator.Current.Value;
                            var timeStampForValueToWrite = enumerator.Current.TimeStamp;

                            // Ignore all duplicate keys with smaller LSNs.
                            while (priorityQueue.Count > 0 && this.store.KeyComparer.Compare(priorityQueue.Peek().Current.Key, keyToWrite) == 0)
                            {
                                var poppedEnumerator = priorityQueue.Pop();

                                // Verify the item being skipped has an earlier LSN than the one being returned.
                                var skipLsn = poppedEnumerator.Current.Value.VersionSequenceNumber;
                                Diagnostics.Assert(skipLsn <= valueToWrite.VersionSequenceNumber, this.store.TraceType, "Enumeration is returning a key with an earlier LSN.");

                                var fileMatadataOfSkipItem = mergeTable.Table[poppedEnumerator.Current.Value.FileId];
                                fileMatadataOfSkipItem.IncrementInvalidCountForAssert(poppedEnumerator.Current.Value.Kind);

                                if (await poppedEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                                {
                                    priorityQueue.Push(poppedEnumerator);
                                }
                                else
                                {
                                    poppedEnumerator.Dispose();
                                }
                            }

                            var latestValue = newConsolidatedState.Read(keyToWrite);
                            var shouldKeyBeWritten = false;
                            var shouldWriteSerializedValue = false;
                            byte[] serializedValue = null;

                            if (latestValue == null)
                            {
                                // Cannot assert that it is deleted because the old occurence of the key could be encountered on merge.
                                // Diagnostics.InternalAssert(valueToWrite.Kind == RecordKind.DeletedVersion, "It should be a deleted item.");

                                // Check if it needs to be written by checking the metaadata table to see if it contains 
                                foreach (var fMetadata in mergeTable.Table.Values)
                                {
                                    if (fMetadata.TimeStamp < timeStampForValueToWrite)
                                    {
                                        // Assert that it is deleted item because non-zero time stamp mean deleted item only.
                                        Diagnostics.Assert(valueToWrite.Kind == RecordKind.DeletedVersion, this.store.TraceType, "Record kind should be deleted kind");

                                        // If fileid is part of the merge list then skip writing the delete key onto the merged file.
                                        // If fileMetadata does not contain invalid entries then skip this file
                                        if (!listOfFileIds.Contains(fMetadata.FileId) && fMetadata.NumberOfInvalidEntries > 0)
                                        {
                                            if (fMetadata.ContainsInvalidKey<TKey>(keyToWrite))
                                            {
                                                // If there is any file with time stamp lesser than the time stamp of the delete record and it is not part of the merge list, it should be written.
                                                shouldKeyBeWritten = true;
                                                if (oldestDeletedEntryTimeStamp == FileMetadata.InvalidTimeStamp || timeStampForValueToWrite < oldestDeletedEntryTimeStamp)
                                                {
                                                    oldestDeletedEntryTimeStamp = timeStampForValueToWrite;
                                                }
                                                if (latestDeletedEntryTimeStamp == FileMetadata.InvalidTimeStamp || timeStampForValueToWrite > latestDeletedEntryTimeStamp)
                                                {
                                                    latestDeletedEntryTimeStamp = timeStampForValueToWrite;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }

                                mergeTable.Table[valueToWrite.FileId].IncrementInvalidCountForAssert(valueToWrite.Kind);
                            }
                            else if (latestValue.FileId == valueToWrite.FileId)
                            {
                                // assert that lsns match.
                                Diagnostics.Assert(
                                    latestValue.VersionSequenceNumber == valueToWrite.VersionSequenceNumber,
                                    this.store.TraceType,
                                    "Merged items lsn's do not match.");
                                shouldKeyBeWritten = true;

                                if (valueToWrite.Kind != RecordKind.DeletedVersion)
                                {
                                    // Read value for key
                                    var latestUserValue = latestValue.Value;
                                    if (!latestValue.ShouldValueBeLoadedFromDisk(this.store.IsValueAReferenceType, latestUserValue, this.store.TraceType))
                                    {
                                        // Do not load from disk if you don't need to!
                                        valueToWrite.Value = latestUserValue;
                                    }
                                    else
                                    {
                                        shouldWriteSerializedValue = true;
                                        // Load the serialized value into memory from disk(do not deserialize it)
                                        serializedValue = await MetadataManager.ReadValueAsync<TValue>(mergeTable.Table, valueToWrite, CancellationToken.None, this.store.TraceType).ConfigureAwait(false);
                                    }
                                }
                            }
                            else
                            {
                                mergeTable.Table[valueToWrite.FileId].IncrementInvalidCountForAssert(valueToWrite.Kind);
                            }

                            if (this.store.TestDelayOnConsolidation != null)
                            {
                                await this.store.TestDelayOnConsolidation.Task;
                            }

                            if (shouldKeyBeWritten)
                            {
                                fileIsEmpty = false;

                                var kvpToWrite = new KeyValuePair<TKey, TVersionedItem<TValue>>(keyToWrite, valueToWrite);

                                Diagnostics.Assert(
                                    shouldWriteSerializedValue == (serializedValue != null),
                                    this.store.TraceType,
                                    "shouldWriteSerializedValue should equal to (serializedValue != null)");

                                // Write the value.
                                checkpointRateCounterWriter.StartMeasurement();
                                await blockAlignedWriter.BlockAlignedWriteItemAsync(kvpToWrite, serializedValue, !shouldWriteSerializedValue).ConfigureAwait(false);
                                checkpointRateCounterWriter.StopMeasurement();

                                if (kvpToWrite.Value.Kind != RecordKind.DeletedVersion)
                                {
                                    // Copy-on-write the versioned value in-memory into the next consolidated state, to avoid taking locks.
                                    // TODO: check on perf testing for this allocation.
                                    newConsolidatedState.Update(keyToWrite, valueToWrite);
                                }
                            }

                            // Move the enumerator to the next key and add it back to the heap.
                            if (await enumeratorToWrite.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                            {
                                priorityQueue.Push(enumeratorToWrite);
                            }
                            else
                            {
                                enumeratorToWrite.Dispose();
                            }
                        }

                        if (!fileIsEmpty)
                        {
                            string fullFileName = Path.Combine(this.store.WorkingDirectory, fileName);

                            checkpointRateCounterWriter.StartMeasurement();

                            // Flush both key and value checkpoints to disk.
                            await blockAlignedWriter.FlushAsync().ConfigureAwait(false);

                            checkpointRateCounterWriter.StopMeasurement();

                            var checkpointFile = new CheckpointFile(fullFileName, keyFile, valueFile, this.store.TraceType);

                            mergedFileMetadata = new FileMetadata(
                                this.store.TraceType,
                                fileId,
                                fileName,
                                checkpointFile.KeyCount,
                                checkpointFile.KeyCount,
                                timeStamp,
                                checkpointFile.DeletedKeyCount,
                                false,
                                oldestDeletedEntryTimeStamp,
                                latestDeletedEntryTimeStamp,
                                this.store.MergeHelper.PercentageOfInvalidEntriesPerFile);
                            mergedFileMetadata.Validate();
                            mergedFileMetadata.CheckpointFile = checkpointFile;

                            long writeBytes = keyFileStream.Length + valueFileStream.Length;
                            long writeBytesPerSecond = checkpointRateCounterWriter.UpdatePerformanceCounter(writeBytes);

#if !DotNetCoreClr
                            // todo: preethas trace the merged file id as well after perf and complete checkpoint start tracing file id.
                            FabricEvents.Events.MergeFile(
                                this.store.TraceType,
                                fullFileName,
                                fileId,
                                checkpointFile.KeyCount,
                                checkpointFile.ValueCount,
                                checkpointFile.DeletedKeyCount,
                                writeBytesPerSecond);
#endif
                        }
                    }
                }

                if (fileIsEmpty)
                {
                    // Delete the 'empty' files on disk.
                    FabricFile.Delete(valueFileName);
                    FabricFile.Delete(keyFileName);
                }

                List<uint> deletedFileIds = new List<uint>();
                foreach (var fileIdToDelete in listOfFileIds)
                {
#if DEBUG
                    Diagnostics.Assert(mergeTable.Table[fileIdToDelete].numInvalidKeysForAssert == mergeTable.Table[fileIdToDelete].NumberOfInvalidEntries,
                        this.store.TraceType,
                        "Incorrect invalid count for fileId {0} Expected : {1} Actual : {2}",
                        deletedFileIds, mergeTable.Table[fileIdToDelete].numInvalidKeysForAssert, mergeTable.Table[fileIdToDelete].NumberOfInvalidEntries);
#endif
                    deletedFileIds.Add(fileIdToDelete);
                }

                if (mergedFileMetadata != null)
                {
                    mergeMetadataTableInformation = new PostMergeMetadatableInformation(deletedFileIds, mergedFileMetadata);
                    MetadataTable mergedMetadataTable = new MetadataTable(this.store.TraceType);
                    mergedMetadataTable.Table.Add(mergedFileMetadata.FileId, mergedFileMetadata);

                    this.store.MergeMetadataTable = mergedMetadataTable;
                }
                else
                {
                    // there could be deleted file ids w/o a new merged file depending on invalid entries.
                    mergeMetadataTableInformation = new PostMergeMetadatableInformation(deletedFileIds, null);
                }

                FabricEvents.Events.MergeAsync(this.store.TraceType, "completed");

                return mergeMetadataTableInformation;
            }
            finally
            {
                foreach (var enumeratorToDispose in enumerators)
                {
                    enumeratorToDispose.Dispose();
                }
            }
        }

        public bool ContainsKey(TKey key)
        {
            return this.aggregatedState.ContainsKey(key);
        }

        public TVersionedItem<TValue> Read(TKey key)
        {
            return this.aggregatedState.Read(key);
        }

        public TVersionedItem<TValue> Read(TKey key, long visbilityLsn)
        {
            return this.aggregatedState.Read(key, visbilityLsn);
        }

        public Microsoft.ServiceFabric.Data.ConditionalValue<TKey> ReadNext(TKey key)
        {
            return this.aggregatedState.ReadNext(key);
        }

        public IEnumerable<TKey> EnumerateKeys()
        {
            return this.aggregatedState.EnumerateKeys();
        }

        public IEnumerable<TKey> GetSortedKeyEnumerable(Func<TKey, bool> keyFilter, bool useFirstKey, TKey firstKey, bool useLastKey, TKey lastKey)
        {
            return this.aggregatedState.GetSortedKeyEnumerable(keyFilter, useFirstKey, firstKey, useLastKey, lastKey);
        }

        public long Count()
        {
            return this.aggregatedState.Count();
        }

        public void Add(TKey key, TVersionedItem<TValue> value)
        {
            this.aggregatedState.ConsolidatedState.Add(key, value);
        }

        private bool IsIdPresentInMetadataTable(uint fileId)
        {
            MetadataTable mergeMetadataTable = this.store.MergeMetadataTable;
            MetadataTable nextMetadataTable = this.store.NextMetadataTable;
            bool idPresent = false;

            // Order of accessing the metadatable is important, do not change it.
            if (mergeMetadataTable != null)
            {
                idPresent = mergeMetadataTable.Table.ContainsKey(fileId);
            }

            if (!idPresent)
            {
                if (nextMetadataTable != null)
                {
                    idPresent = nextMetadataTable.Table.ContainsKey(fileId);
                }

                if (!idPresent)
                {
                    idPresent = this.store.CurrentMetadataTable.Table.ContainsKey(fileId);
                }
            }

            return idPresent;
        }
    }

    internal struct DifferentialData<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        public KeyValuePair<TKey, TVersionedItem<TValue>> KeyValuePair{get; set;}
        public TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> DifferentialState { get; set; }
    }
}