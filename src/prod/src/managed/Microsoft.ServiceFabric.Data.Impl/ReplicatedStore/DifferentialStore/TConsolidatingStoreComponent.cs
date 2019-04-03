// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//// ------------------------------------------------------------
////  Copyright (c) Microsoft Corporation.  All rights reserved.
//// ------------------------------------------------------------

//namespace System.Fabric.Store
//{
//    using System.Collections.Generic;
//    using System.Fabric.Common;
//    using System.Linq;
//    using System.Threading;

//    using System.Fabric.Common.Tracing;
//    using Microsoft.ServiceFabric.Data;
//    using Microsoft.ServiceFabric.Replicator;

//    /// <summary>
//    /// Holds the differential state for the store. It is an aggregated store component.
//    /// </summary>
//    /// <typeparam name="TKey">Key type.</typeparam>
//    /// <typeparam name="TValue">Value type.</typeparam>
//    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
//    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
//    /// <typeparam name="TKeyRangePartitioner"></typeparam>
//    internal class TConsolidatingStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
//        IReadableStoreComponent<TKey, TVersionedItem<TValue>>
//        where TKeyComparer : IComparer<TKey>, new()
//        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
//        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
//    {
//        private static TKeyComparer KeyComparer = new TKeyComparer();

//        #region Instance Members

//        /// <summary>
//        /// Old differential state.
//        /// </summary>
//        private TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> oldDifferentialState;

//        /// <summary>
//        /// Old consolidated state.
//        /// </summary>
//        private IReadableStoreComponent<TKey, TVersionedItem<TValue>> oldConsolidatedState;

//        /// <summary>
//        /// Transactional replicator.
//        /// </summary>
//        private ITransactionalReplicator transactionalReplicator;

//        /// <summary>
//        /// Unique id for a state provider
//        /// </summary>
//        private long stateProviderId;

//        /// <summary>
//        /// Container that holds visbility lsns and their componennts for ongoing snapshot reads.
//        /// </summary>
//        private SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> snapshotContainer;

//        #endregion

//        /// <summary>
//        /// Creates a consolidating component that is responsible for reads during consolidation and mergeing differntial and old consolidated components.
//        /// </summary>
//        public TConsolidatingStoreComponent(
//            TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> oldDifferentialState,
//            IReadableStoreComponent<TKey, TVersionedItem<TValue>> oldConsolidatedState,
//            ITransactionalReplicator transactionalReplicator,
//            long stateProviderId,
//            SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> snapshotContainer)
//        {
//            this.oldDifferentialState = oldDifferentialState;
//            this.oldConsolidatedState = oldConsolidatedState;
//            this.transactionalReplicator = transactionalReplicator;
//            this.stateProviderId = stateProviderId;
//            this.snapshotContainer = snapshotContainer;
//        }

//        #region IReadableStoreComponent<TKey> Members

//        /// <summary>
//        /// Checks to see if a certain key is present in this component.
//        /// </summary>
//        /// <param name="key">Lookup key.</param>
//        /// <returns>True if key already exists in the write set.</returns>
//        public bool ContainsKey(TKey key)
//        {
//            var keyFound = this.oldDifferentialState.ContainsKey(key);
//            if (!keyFound)
//            {
//                keyFound = this.oldConsolidatedState.ContainsKey(key);
//            }

//            return keyFound;
//        }

//        /// <summary>
//        /// Reads the value for a given key.
//        /// </summary>
//        /// <param name="key">Key to read.</param>
//        /// <returns>True and value if the key exists.</returns>
//        public TVersionedItem<TValue> Read(TKey key)
//        {
//            var versionedItemsOldDifferential = this.oldDifferentialState.ReadVersions(key);
//            TVersionedItem<TValue> versionedItem = null;
//            if (versionedItemsOldDifferential == null)
//            {
//                versionedItem = this.oldConsolidatedState.Read(key);
//            }
//            else
//            {
//                versionedItem = versionedItemsOldDifferential.CurrentVersion;
//            }

//            return versionedItem;
//        }

//        /// <summary>
//        /// Reads the value for a given key.
//        /// </summary>
//        /// <param name="key">Key to read.</param>
//        /// <param name="visibilityLsn"></param>
//        /// <returns>True and value if the key exists.</returns>
//        public TVersionedItem<TValue> Read(TKey key, long visibilityLsn)
//        {
//            TVersionedItem<TValue> versionedItem = null;
//            TVersionedItem<TValue> potentialVersionedItem = null;
//            var versionedItemsOldDifferential = this.oldDifferentialState.ReadVersions(key);

//            if (versionedItemsOldDifferential != null)
//            {
//                potentialVersionedItem = versionedItemsOldDifferential.CurrentVersion;

//                if (potentialVersionedItem != null && potentialVersionedItem.VersionSequenceNumber <= visibilityLsn)
//                {
//                    versionedItem = potentialVersionedItem;
//                }
//                else
//                {
//                    potentialVersionedItem = versionedItemsOldDifferential.PreviousVersion;

//                    if (potentialVersionedItem != null && potentialVersionedItem.VersionSequenceNumber <= visibilityLsn)
//                    {
//                        versionedItem = potentialVersionedItem;
//                    }
//                }
//            }
//            if (versionedItem == null)
//            {
//                potentialVersionedItem = this.oldConsolidatedState.Read(key);
//                if (potentialVersionedItem != null && potentialVersionedItem.VersionSequenceNumber <= visibilityLsn)
//                {
//                    versionedItem = potentialVersionedItem;
//                }
//            }

//            return versionedItem;
//        }

//        /// <summary>
//        /// Reads the value for a key greater than the given key.
//        /// </summary>
//        /// <param name="key">Key to read.</param>
//        /// <returns>True and value if the key exists.</returns>
//        public ConditionalValue<TKey> ReadNext(TKey key)
//        {
//            var versionChainOldDifferentialNextKey = this.oldDifferentialState.ReadNext(key);
//            var versionChainOldConsolidatedNextKey = this.oldConsolidatedState.ReadNext(key);
//            if (versionChainOldDifferentialNextKey.HasValue)
//            {
//                if (versionChainOldConsolidatedNextKey.HasValue)
//                {
//                    if (0 < KeyComparer.Compare(versionChainOldDifferentialNextKey.Value, versionChainOldConsolidatedNextKey.Value))
//                    {
//                        return versionChainOldConsolidatedNextKey;
//                    }
//                    else
//                    {
//                        return versionChainOldDifferentialNextKey;
//                    }
//                }
//                else
//                {
//                    return versionChainOldDifferentialNextKey;
//                }
//            }
//            else
//            {
//                if (versionChainOldConsolidatedNextKey.HasValue)
//                {
//                    return versionChainOldConsolidatedNextKey;
//                }
//                else
//                {
//                    return new ConditionalValue<TKey>();
//                }
//            }
//        }

//        /// <summary>
//        /// Returns an enumerable that iterates through the collection.
//        /// </summary>
//        /// <returns>>Enumerator over all versionss from this consolidating store component.</returns>
//        public IEnumerable<TKey> EnumerateKeys()
//        {
//            return this.oldConsolidatedState.EnumerateKeys().Concat(this.oldDifferentialState.EnumerateKeys());
//        }

//        #endregion

//        /// <summary>
//        /// Starts the merge process between the old differential state and the old consolidated state.
//        /// </summary>
//        public TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> Consolidate(
//            string tracer,
//            MetadataTable metadataTable,
//            CancellationToken cancellationToken)
//        {
//            FabricEvents.Events.StartingConsolidating(tracer, this.GetHashCode());

//            // Create next consolidated state. This will be populated during merge.
//            var newConsolidatedState =
//                new TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>();

//            // Checks.
//            Diagnostics.InternalAssert(
//                this.oldConsolidatedState is TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>,
//                "unexpected old consolidated state");

//            this.StoreComponentConsolidate(this.oldConsolidatedState, this.oldDifferentialState, newConsolidatedState, metadataTable, cancellationToken);

//            long deltaCount = this.oldDifferentialState.Count();
//            long oldConsolidatedCount = this.oldConsolidatedState.Count();
//            long newConsolidatedCount = newConsolidatedState.Count();

//            FabricEvents.Events.CompletedConsolidating(tracer, oldConsolidatedCount, deltaCount, newConsolidatedCount);

//            return newConsolidatedState;
//        }

//        public long Count()
//        {
//            Diagnostics.InternalAssert(false, "count should not be queried on consolidating state");
//            return -1;
//        }

//        /// <summary>
//        /// Merges two sorted components (delta differential and consolidated) in to new consolidated.
//        /// </summary>
//        private void StoreComponentConsolidate(
//            IReadableStoreComponent<TKey, TVersionedItem<TValue>> oldConsolidatedState,
//            TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> oldDifferentialState,
//            TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> newConsolidatedState,
//            MetadataTable metadataTable,
//            CancellationToken cancellationToken)
//        {
//            var differentialStateEnumerator = oldDifferentialState.EnumerateKeyAndValue().GetEnumerator();
//            var consolidatedStateKeyEnumerator = oldConsolidatedState.EnumerateKeys().GetEnumerator();

//            var isDiffierentialDrained = !differentialStateEnumerator.MoveNext();
//            var isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();

//            // TODO: We only require two variables.
//            var versionedItems = new List<TVersionedItem<TValue>>();
//            while (isDiffierentialDrained == false && isConsolidatedDrained == false)
//            {
//                Diagnostics.InternalAssert(versionedItems.Count == 0, "must be empty");

//                var differentialKeyValuePair = differentialStateEnumerator.Current;
//                var consolidatedKey = consolidatedStateKeyEnumerator.Current;
//                var keyComparison = KeyComparer.Compare(differentialKeyValuePair.Key, consolidatedKey);

//                // Key in Differential is larger than key in Consolidated.
//                if (keyComparison > 0)
//                {
//                    var valueInConsolidatedState = oldConsolidatedState.Read(consolidatedKey);
//                    Diagnostics.InternalAssert(valueInConsolidatedState != null, "Old consolidated must contain the item.");


//                    // Note: On Hydrate we load all the items including deleted because we could read in any order. Ignore adding them here.
//                    if (valueInConsolidatedState.Kind != RecordKind.DeletedVersion)
//                    {
//                        this.SweepItem(valueInConsolidatedState);
//                        newConsolidatedState.Add(consolidatedKey, valueInConsolidatedState);
//                    }

//                    isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();
//                    continue;
//                }

//                // Key in differential is same sa key in consolidated.
//                else if (keyComparison == 0)
//                {
//                    // Remove the value in consolidated.
//                    var valueInConsolidatedState = oldConsolidatedState.Read(consolidatedKey);
//                    Diagnostics.InternalAssert(valueInConsolidatedState != null, "Old consolidated must contain the item.");
//                    versionedItems.Add(valueInConsolidatedState);

//                    // Decrement number of valid entries.
//                    FileMetadata fileMetadata = null;
//                    if (!metadataTable.Table.TryGetValue(valueInConsolidatedState.FileId, out fileMetadata))
//                    {
//                        Diagnostics.InternalAssert(
//                            fileMetadata != null,
//                            "Failed to find file metadata for versioned item in consolidated with file id {0}.",
//                            __arglist(valueInConsolidatedState.FileId));
//                    }

//                    fileMetadata.DecrementValidEntries();

//                    // Remove the value in differential.
//                    var differentialVersions = oldDifferentialState.ReadVersions(differentialKeyValuePair.Key);
//                    Diagnostics.InternalAssert(differentialVersions != null, "differential version should not be null");
//                    var currentVersion = differentialVersions.CurrentVersion;
//                    Diagnostics.InternalAssert(currentVersion != null, "current version should not be null");
//                    Diagnostics.InternalAssert(
//                        valueInConsolidatedState.VersionSequenceNumber < currentVersion.VersionSequenceNumber,
//                        "valueInConsolidatedState < currentVersion");
//                    Diagnostics.InternalAssert(
//                        currentVersion.VersionSequenceNumber == differentialKeyValuePair.Value.VersionSequenceNumber,
//                        "Current and the version acquired must be same.");

//                    var previousVersion = differentialVersions.PreviousVersion;
//                    if (previousVersion != null)
//                    {
//                        Diagnostics.InternalAssert(
//                            valueInConsolidatedState.VersionSequenceNumber < previousVersion.VersionSequenceNumber,
//                            "valueInConsolidatedState < previousVersion");
//                        versionedItems.Add(previousVersion);
//                    }

//                    versionedItems.Add(currentVersion);
//                    this.ProcessToBeRemovedVersions(consolidatedKey, versionedItems, metadataTable);

//                    if (differentialKeyValuePair.Value.Kind != RecordKind.DeletedVersion)
//                    {
//                        newConsolidatedState.Add(differentialKeyValuePair.Key, differentialKeyValuePair.Value);
//                    }

//                    isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();
//                    isDiffierentialDrained = !differentialStateEnumerator.MoveNext();
//                    continue;
//                }
//                // Key in Differential is smaller than key in Consolidated.
//                else if (keyComparison < 0)
//                {
//                    // Move the value in differential in to consolidated.
//                    var differentialVersions = oldDifferentialState.ReadVersions(differentialKeyValuePair.Key);
//                    Diagnostics.InternalAssert(differentialVersions != null, "differential version should not be null");
//                    var currentVersion = differentialVersions.CurrentVersion;
//                    Diagnostics.InternalAssert(currentVersion != null, "current version should not be null");
//                    Diagnostics.InternalAssert(
//                        currentVersion.VersionSequenceNumber == differentialKeyValuePair.Value.VersionSequenceNumber,
//                        "Current and the version acquired must be same.");

//                    var previousVersion = differentialVersions.PreviousVersion;
//                    if (previousVersion != null)
//                    {
//                        Diagnostics.InternalAssert(
//                            currentVersion.VersionSequenceNumber > previousVersion.VersionSequenceNumber,
//                            "Current and the version acquired must be same.");

//                        versionedItems.Add(previousVersion);
//                        versionedItems.Add(currentVersion);
//                        this.ProcessToBeRemovedVersions(differentialKeyValuePair.Key, versionedItems, metadataTable);
//                    }

//                    if (differentialKeyValuePair.Value.Kind != RecordKind.DeletedVersion)
//                    {
//                        newConsolidatedState.Add(differentialKeyValuePair.Key, differentialKeyValuePair.Value);
//                    }

//                    isDiffierentialDrained = !differentialStateEnumerator.MoveNext();
//                    continue;
//                }
//            }

//            // consolidated key is larger.
//            if (isDiffierentialDrained == true && isConsolidatedDrained == false)
//            {
//                Diagnostics.InternalAssert(versionedItems.Count == 0, "must be empty");

//                do
//                {
//                    var consolidatedKey = consolidatedStateKeyEnumerator.Current;
//                    var valueInConsolidatedState = oldConsolidatedState.Read(consolidatedKey);
//                    Diagnostics.InternalAssert(valueInConsolidatedState != null, "Old consolidated must contain the item.");

//                    // Note: On Hydrate we load all the items including deleted because we could read in any order. Ignore adding them here.
//                    if (valueInConsolidatedState.Kind != RecordKind.DeletedVersion)
//                    {
//                        this.SweepItem(valueInConsolidatedState);
//                        newConsolidatedState.Add(consolidatedKey, valueInConsolidatedState);
//                    }

//                    isConsolidatedDrained = !consolidatedStateKeyEnumerator.MoveNext();
//                } while (isConsolidatedDrained == false);
//            }
//            else if (isConsolidatedDrained == true && isDiffierentialDrained == false)
//            {
//                do
//                {
//                    Diagnostics.InternalAssert(versionedItems.Count == 0, "must be empty");

//                    var differentialKeyValuePair = differentialStateEnumerator.Current;

//                    var differentialVersions = oldDifferentialState.ReadVersions(differentialKeyValuePair.Key);
//                    Diagnostics.InternalAssert(differentialVersions != null, "differential version should not be null");
//                    var currentVersion = differentialVersions.CurrentVersion;
//                    Diagnostics.InternalAssert(currentVersion != null, "current version should not be null");
//                    Diagnostics.InternalAssert(
//                        currentVersion.VersionSequenceNumber == differentialKeyValuePair.Value.VersionSequenceNumber,
//                        "Current and the version acquired must be same.");

//                    var previousVersion = differentialVersions.PreviousVersion;

//                    if (previousVersion != null)
//                    {
//                        Diagnostics.InternalAssert(
//                            currentVersion.VersionSequenceNumber > previousVersion.VersionSequenceNumber,
//                            "Current and the version acquired must be same.");

//                        versionedItems.Add(previousVersion);
//                        versionedItems.Add(currentVersion);
//                        this.ProcessToBeRemovedVersions(differentialKeyValuePair.Key, versionedItems, metadataTable);
//                    }

//                    if (differentialKeyValuePair.Value.Kind != RecordKind.DeletedVersion)
//                    {
//                        newConsolidatedState.Add(differentialKeyValuePair.Key, differentialKeyValuePair.Value);
//                    }

//                    isDiffierentialDrained = !differentialStateEnumerator.MoveNext();
//                } while (isDiffierentialDrained == false);
//            }
//        }

//        /// <summary>
//        /// Retires old versions.
//        /// Note that the list item in the versionItems list is the current item that must not be required.
//        /// </summary>
//        /// <param name="key">The key that all versions belong to.</param>
//        /// <param name="versionedItems">Versioned items for a given key.</param>
//        /// <param name="metadataTable">Metadata table used to move a file into snapshot container.</param>
//        private void ProcessToBeRemovedVersions(TKey key, List<TVersionedItem<TValue>> versionedItems, MetadataTable metadataTable)
//        {
//            // Note that the list item in the versionItems list is the current item that must not be required.
//            for (var i = 0; i < versionedItems.Count - 1; i++)
//            {
//                // Assert that the versionsequence is in the correct order.
//                var sequenceNumberToBeDeleted = versionedItems[i].VersionSequenceNumber;
//                var nextSequenceNumber = versionedItems[i + 1].VersionSequenceNumber;

//                Diagnostics.InternalAssert(sequenceNumberToBeDeleted < nextSequenceNumber, "sequenceNumberToBeDeleted < nextSequenceNumber");

//                var removeVersionResult = this.transactionalReplicator.TryRemoveVersion(
//                    this.stateProviderId,
//                    sequenceNumberToBeDeleted,
//                    nextSequenceNumber);

//                // Move to snapsot container, if needed
//                if (!removeVersionResult.CanBeRemoved)
//                {
//                    var enumerationSet = removeVersionResult.EnumerationSet;

//                    foreach (var snapshotVisibilityLsn in enumerationSet)
//                    {
//                        var snapshotComponent =
//                            this.snapshotContainer.Read(snapshotVisibilityLsn);

//                        if (snapshotComponent == null)
//                        {
//                            snapshotComponent = new SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(this.snapshotContainer);
//                            snapshotComponent = this.snapshotContainer.GetOrAdd(snapshotVisibilityLsn, snapshotComponent);
//                        }

//                        // Move file id for the versioned item, if it is not already present.
//                        var fileId = versionedItems[i].FileId;
//                        if (fileId > 0)
//                        {
//                            FileMetadata fileMetadata = null;
//                            if (!metadataTable.Table.TryGetValue(fileId, out fileMetadata))
//                            {
//                                Diagnostics.InternalAssert(
//                                    fileMetadata != null,
//                                    "Failed to find the file metadata for a versioned item with file id {0}.",
//                                    __arglist(fileId));
//                            }

//                            this.snapshotContainer.TryAddFileMetadata(snapshotVisibilityLsn, fileMetadata);
//                        }
//                        else
//                        {
//                            Diagnostics.InternalAssert(
//                                !versionedItems[i].CanBeSweepedToDisk || (versionedItems[i].CanBeSweepedToDisk && versionedItems[i].Value != null),
//                                "Version item with file id {0} should be in memory",
//                                __arglist(versionedItems[i].FileId));
//                        }

//                        // Add the metadata table, if it doesn't already exist.
//                        snapshotComponent.Add(key, versionedItems[i]);
//                    }

//                    // Wait on notifications and remove the entry from the container
//                    if (removeVersionResult.EnumerationCompletionNotifications != null)
//                    {
//                        foreach (var enumerationResult in removeVersionResult.EnumerationCompletionNotifications)
//                        {
//                            enumerationResult.Notification.ContinueWith(x => { this.snapshotContainer.Remove(enumerationResult.VisibilitySequenceNumber); })
//                                .IgnoreExceptionVoid();
//                        }
//                    }
//                }
//            }

//            versionedItems.Clear();
//        }

//        private void SweepItem(TVersionedItem<TValue> inItem)
//        {
//            Diagnostics.InternalAssert(inItem.Kind != RecordKind.DeletedVersion, "A deleted item kind is not expected to be sweeped.");

//            if (!inItem.CanBeSweepedToDisk)
//            {
//                return;
//            }

//            if (!inItem.InUse)
//            {
//                // Set TValue to be null, default for TValue is null.
//                inItem.Value = default(TValue);

//            }
//            else
//            {
//                inItem.InUse = false;
//            }
//        }
//    }
//}