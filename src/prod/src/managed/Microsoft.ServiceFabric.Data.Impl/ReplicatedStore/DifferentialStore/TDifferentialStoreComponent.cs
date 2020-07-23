// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Data.Common;
    using System.Linq;
    using System.Threading;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Holds differential state for the store. It is an aggregated store component.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    /// <typeparam name="TKeyRangePartitioner"></typeparam>
    internal class TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
        IReadableStoreComponent<TKey, TVersionedItem<TValue>>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        private static TKeyComparer KeyComparer = new TKeyComparer();
        private static TKeyEqualityComparer KeyEqualityComparer = new TKeyEqualityComparer();
        
        #region Instance Members

        private readonly ITransactionalReplicator transactionalReplicator;
        private readonly long stateProviderId;
        private readonly SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> snapshotContainer;
        private readonly bool isValueAReferenceType;
        private readonly LoadValueCounter loadValueCounter;

        private IDictionary<TKey, DifferentialStateVersions<TValue>> component;
        private AddList<TKey> addList;
        private readonly string traceType;
        private bool isReadonly;

        #endregion

        /// <summary>
        /// Constructor.
        /// </summary>
        public TDifferentialStoreComponent(
            ITransactionalReplicator transactionalReplicator,
            long stateProviderId,
            SnapshotContainer<TKey, TValue, TKeyComparer, TKeyEqualityComparer> snapshotContainer,
            LoadValueCounter loadValueCounter,
            bool isValueAReferenceType,
            string traceType)
        {
            this.traceType = traceType;
            this.transactionalReplicator = transactionalReplicator;
            this.stateProviderId = stateProviderId;
            this.snapshotContainer = snapshotContainer;

            this.component = new ConcurrentDictionary<TKey, DifferentialStateVersions<TValue>>(KeyEqualityComparer);
            this.addList = new AddList<TKey>(KeyComparer);
            this.isReadonly = false;
            this.isValueAReferenceType = isValueAReferenceType;
            this.loadValueCounter = loadValueCounter;
        }

        internal AddList<TKey> AddList
        {
            get
            {
                return this.addList;
            }
        }

        /// <summary>
        /// Mutates the differential store from a concurrent read write data structure into a readonly sorted data structure.
        /// </summary>
        public void Sort()
        {
            var concurrentDictionary = this.component as ConcurrentDictionary<TKey, DifferentialStateVersions<TValue>>;
            Diagnostics.Assert(concurrentDictionary != null, this.traceType, "write operation came after becoming readonly.");

            // Using SortedList instead of SortedDictionary because
            // 1. SortedList uses less memory
            // 2. Populating all from sorted input is faster in SortedList than Sorted Dictionary
            // 3. SortedList keeps the state as two sorted arrays, which enables binarysearch and next/previous key searches (ReadNext).
            this.component = new SortedList<TKey, DifferentialStateVersions<TValue>>(concurrentDictionary, KeyComparer);
            Volatile.Write(ref this.isReadonly, true);

            // MCoskun: At this point, addList is not required anymore. AddList is still more efficient (since it only contains deltas) but it has memory overhead.
            // Using the "pay-as-you-go" philosophy, I am penalizing enumerators for smaller memory overhead in common case.
            this.addList = null;
        }

        #region IWritableStoreComponent<TWriteSetKey<TKey>, TVersionedItem<TValue>> Members

        /// <summary>
        /// Adds a new entry to this store component.
        /// </summary>
        /// <devnote>
        /// Consolidated Manager is only required for the AddList feature.
        /// </devnote>
        public void Add(TKey key, TVersionedItem<TValue> value, IReadableStoreComponent<TKey, TVersionedItem<TValue>> consolidationManager)
        {
            bool isInReadOnlyMode = Volatile.Read(ref this.isReadonly);
            Diagnostics.Assert(isInReadOnlyMode == false, this.traceType, "Write operation cannot come after the differential has become readonly.");

            var concurrentDictionary = this.component as ConcurrentDictionary<TKey, DifferentialStateVersions<TValue>>;
            Diagnostics.Assert(concurrentDictionary != null, this.traceType, "write operation came after becoming readonly.");

            var differentialVersions = concurrentDictionary.GetOrAdd(key, k => new DifferentialStateVersions<TValue>());
            if (differentialVersions.CurrentVersion == null)
            {
                Diagnostics.Assert(differentialVersions.PreviousVersion == null, this.traceType, "previous version should be null");
                differentialVersions.CurrentVersion = value;
                if (value.Kind == RecordKind.InsertedVersion)
                {
                    Diagnostics.Assert(
                        (differentialVersions.flags & DifferentialStateFlags.NewKey) == DifferentialStateFlags.None,
                        this.traceType,
                        "DifferentialStateFlags.NewKey is already set");
                    differentialVersions.flags |= DifferentialStateFlags.NewKey;
                }
            }
            else
            {
                // Assert that version sequence number is lesser or equal compared to the new one.
                // Sequence number can be equal if the same key gets updated multiple times in the same transaction.
                if (differentialVersions.CurrentVersion.VersionSequenceNumber > value.VersionSequenceNumber)
                {
                    var message = string.Format(@"New lsn {0} should be greater than or equal the existing current lsn {1}. 
This error could be caused by an incorrect implementation of key comparer or key serializer. 
Please check your implementation of IEquatable, IComparable, and custom serializer (if applicable).",
                            value.VersionSequenceNumber, differentialVersions.CurrentVersion.VersionSequenceNumber);

                    FabricEvents.Events.StoreDiagnosticError(this.traceType, message);
                    Diagnostics.Assert(
                        false,
                        this.traceType,
                        message);
                }

                if (differentialVersions.CurrentVersion.VersionSequenceNumber == value.VersionSequenceNumber)
                {
                    // Set the latest value in case there are multiple updates in the same transaction(same lsn).
                    differentialVersions.CurrentVersion = value;
                    this.UpdateAddList(key, value, consolidationManager);
                    return;
                }

                // Check if previous version is null
                if (differentialVersions.PreviousVersion == null)
                {
                    differentialVersions.PreviousVersion = differentialVersions.CurrentVersion;
                    differentialVersions.CurrentVersion = value;
                }
                else
                {
                    // Move to snapshot container, if needed.
                    var removeVersionResult = this.transactionalReplicator.TryRemoveVersion(
                        this.stateProviderId,
                        differentialVersions.PreviousVersion.VersionSequenceNumber,
                        differentialVersions.CurrentVersion.VersionSequenceNumber);

                    if (!removeVersionResult.CanBeRemoved)
                    {
                        var enumerationSet = removeVersionResult.EnumerationSet;

                        foreach (var snapshotVisibilityLsn in enumerationSet)
                        {
                            var snapshotComponent = this.snapshotContainer.Read(snapshotVisibilityLsn);

                            if (snapshotComponent == null)
                            {
                                snapshotComponent = new SnapshotComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>(this.snapshotContainer, this.loadValueCounter, this.isValueAReferenceType, this.traceType);
                                snapshotComponent = this.snapshotContainer.GetOrAdd(snapshotVisibilityLsn, snapshotComponent);
                            }

                            snapshotComponent.Add(key, differentialVersions.PreviousVersion);
                        }

                        // Wait on notifications and remove the entry from the container
                        if (removeVersionResult.EnumerationCompletionNotifications != null)
                        {
                            foreach (var enumerationResult in removeVersionResult.EnumerationCompletionNotifications)
                            {
                                enumerationResult.Notification.ContinueWith(x => { this.snapshotContainer.Remove(enumerationResult.VisibilitySequenceNumber); })
                                    .IgnoreExceptionVoid();
                            }
                        }
                    }

                    // Remove from differential state
                    differentialVersions.PreviousVersion = differentialVersions.CurrentVersion;
                    differentialVersions.CurrentVersion = value;
                }
            }

            this.UpdateAddList(key, value, consolidationManager);
        }

        /// <summary>
        /// Called during false progress processing for multi-versioned and historical stores.
        /// </summary>
        /// <param name="keyLockResourceNameHash"></param>
        /// <param name="sequenceNumber">Sequence number at which the false progress occurred.</param>
        /// <param name="storeModificationType">Specifies the type of item to be removed.</param>
        /// <param name="key"></param>
        /// <returns>True and value if the key exists.</returns>
        /// <devnote>AddList update not required since it is ok to have more keys then necessary.</devnote>
        internal bool UndoFalseProgress(TKey key, ulong keyLockResourceNameHash, long sequenceNumber, StoreModificationType storeModificationType)
        {
            // In the current model, checkpoint cannot have false progress.
            Diagnostics.Assert(this.isReadonly == false, this.traceType, "Write operation cannot come after the differential has become readonly.");

            // Find the item sequence number.
            var differentialStateVersions = this.ReadVersions(key);

            // Remove item.
            TVersionedItem<TValue> undoItem = null;
            if (differentialStateVersions != null)
            {
                var previousVersion = differentialStateVersions.PreviousVersion;
                if (previousVersion != null && previousVersion.VersionSequenceNumber == sequenceNumber)
                {
                    undoItem = previousVersion;
                    differentialStateVersions.PreviousVersion = null;
                }
                else
                {
                    var currentVersion = differentialStateVersions.CurrentVersion;
                    Diagnostics.Assert(currentVersion != null, this.traceType, "current version cannot be null");
                    if (currentVersion.VersionSequenceNumber == sequenceNumber)
                    {
                        undoItem = currentVersion;

                        // Update current version and make prev version null.
                        if (differentialStateVersions.PreviousVersion != null)
                        {
                            differentialStateVersions.CurrentVersion = differentialStateVersions.PreviousVersion;
                            differentialStateVersions.PreviousVersion = null;
                        }
                        else
                        {
                            // Remove key
                            this.RemoveKey(key);
                        }
                    }
                }

                if (undoItem != null)
                {
                    switch (storeModificationType)
                    {
                        case StoreModificationType.Add:
                            // Remove implies undoing insert.
                            Diagnostics.Assert(undoItem is TInsertedItem<TValue>, this.traceType, "unexpected deleted version");
                            break;
                        case StoreModificationType.Remove:
                            // Add implies undoing delete.
                            Diagnostics.Assert(undoItem is TDeletedItem<TValue>, this.traceType, "unexpected inserted version");
                            break;
                        case StoreModificationType.Update:
                            Diagnostics.Assert(undoItem is TUpdatedItem<TValue> || undoItem is TInsertedItem<TValue>, this.traceType, "unexpected updated version");
                            break;
                    }

                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Returns addList if possible. If not all keys are returned.
        /// </summary>
        /// <returns>New keys or all keys.</returns>
        public IEnumerable<TKey> GetEnumerableNewKeys()
        {
            var snapAddList = this.addList;
            if (snapAddList != null)
            {
                return snapAddList;
            }

            Thread.MemoryBarrier();

            var sortedList = this.component as SortedList<TKey, DifferentialStateVersions<TValue>>;
            Diagnostics.Assert(sortedList != null, this.traceType, "write operation came after becoming readonly.");

            return sortedList.Keys;
        }

        /// <summary>
        /// Returns an enumerable for this collection.
        /// </summary>
        /// <returns>An enumerable that can be used to iterate through the collection.</returns>
        public IEnumerable<TKey> EnumerateKeys()
        {
            var snappedComponent = this.component;
            foreach (var kvp in snappedComponent)
            {
                // Returning just keys causes dictionary to do a tolist, so for now this is better even though the value is ignored.
                yield return kvp.Key;
            }
        }

        /// <summary>
        /// Returns an enumerable for this collection.
        /// </summary>
        /// <param name="enumerable"></param>
        /// <param name="skipEphemeralKeys">ephemeral keys : keys which are added and finally deleted in same differential</param>
        /// <returns>An enumerable that can be used to iterate through the collection.</returns>
        public bool TryEnumerateKeyAndValue(out IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> enumerable, bool skipEphemeralKeys = false)
        {
            var snappedComponent = this.component;
            enumerable = this.InternalEnumerateKeyAndValue(snappedComponent, skipEphemeralKeys);
            if (skipEphemeralKeys)
            {
                var enumerator = enumerable.GetEnumerator();
                return enumerator.MoveNext() != false;
            }
            else
            {
                return snappedComponent.Count != 0;
            }
        }

        internal IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> InternalEnumerateKeyAndValue(IDictionary<TKey, DifferentialStateVersions<TValue>> component, bool skipEphemeralKeys)
        {
            foreach (var kvp in component)
            {
                if (skipEphemeralKeys == true && kvp.Value.CurrentVersion.Kind == RecordKind.DeletedVersion && (kvp.Value.flags & DifferentialStateFlags.NewKey) != DifferentialStateFlags.None)
                {
                    // Skip keys which were inserted and then finally removed in current differential
                    continue;
                }

                yield return new KeyValuePair<TKey, TVersionedItem<TValue>>(kvp.Key, kvp.Value.CurrentVersion);
            }
        }

        /// <summary>
        /// Returns an enumerable for this collection.This returns current items only and i used by sweep only.
        /// </summary>
        /// <returns>An enumerable that can be used to iterate through the collection.</returns>
        public IEnumerable<TVersionedItem<TValue>> EnumerateValues()
        {
            // Once moving previous version items is moved in as a part of perf checkpoint as apposed to consoliodate
            // assert that previous version item is null.
            var snappedComponent = this.component;

            foreach (var kvp in snappedComponent)
            {
                yield return kvp.Value.CurrentVersion;
            }
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public DifferentialStateVersions<TValue> ReadVersions(TKey key)
        {
            DifferentialStateVersions<TValue> versionedItems;
            this.component.TryGetValue(key, out versionedItems);
            return versionedItems;
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public TVersionedItem<TValue> Read(TKey key)
        {
            DifferentialStateVersions<TValue> versionedItems;
            this.component.TryGetValue(key, out versionedItems);

            if (versionedItems != null)
            {
                return versionedItems.CurrentVersion;
            }

            return null;
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <param name="visibilityLsn"></param>
        /// <returns>True and value if the key exists.</returns>
        public TVersionedItem<TValue> Read(TKey key, long visibilityLsn)
        {
            TVersionedItem<TValue> versionedItem = null;
            TVersionedItem<TValue> potentialVersionedItem = null;
            var versionedItemsOldDifferential = this.ReadVersions(key);

            if (versionedItemsOldDifferential != null)
            {
                potentialVersionedItem = versionedItemsOldDifferential.CurrentVersion;

                if (potentialVersionedItem != null && potentialVersionedItem.VersionSequenceNumber <= visibilityLsn)
                {
                    versionedItem = potentialVersionedItem;
                }
                else
                {
                    potentialVersionedItem = versionedItemsOldDifferential.PreviousVersion;

                    if (potentialVersionedItem != null && potentialVersionedItem.VersionSequenceNumber <= visibilityLsn)
                    {
                        versionedItem = potentialVersionedItem;
                    }
                }
            }

            return versionedItem;
        }

        /// <summary>
        /// Reads the value for a key greater than the given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public ConditionalValue<TKey> ReadNext(TKey key)
        {
            var minWasSeen = false;
            var minKey = default(TKey);
            var nextKeyCandidates = this.component.Where(x => 0 < KeyComparer.Compare(x.Key, key)).Select(x => x.Key);
            foreach (var x in nextKeyCandidates)
            {
                if (!minWasSeen)
                {
                    minWasSeen = true;
                    minKey = x;
                }

                if (0 < KeyComparer.Compare(minKey, x))
                {
                    minKey = x;
                }
            }

            if (minWasSeen)
            {
                return new ConditionalValue<TKey>(true, minKey);
            }
            else
            {
                return new ConditionalValue<TKey>();
            }
        }

        #endregion

        /// <summary>
        /// Checks to see if a certain key is present in this component.
        /// </summary>
        /// <param name="key">Lookup key.</param>
        /// <returns>True if key already exists in the write set.</returns>
        public bool ContainsKey(TKey key)
        {
            return this.component.ContainsKey(key);
        }

        /// <summary>
        /// Used during consolidation.
        /// </summary>
        /// <param name="key">Key to trim.</param>
        public void RemoveKey(TKey key)
        {
            Diagnostics.Assert(this.isReadonly == false, this.traceType, "Write operation cannot come after the differential has become readonly.");

            var result = this.component.Remove(key);
            Diagnostics.Assert(result, this.traceType, "remove key failed from differential state");
        }

        public long Count()
        {
            // Note: Do not call this method on a concurrent dictionary.
            // This always queries from sorted list so there is no dictionary level locking involved for count.
            // The only time this is a concurrent dictionary is when it is cleared in which case it is always zero and no contention.
            return this.component.Count;
        }

        private void UpdateAddList(TKey key, TVersionedItem<TValue> value, IReadableStoreComponent<TKey, TVersionedItem<TValue>> consolidationManager)
        {
            // Note: Temporary Assert that should not be required.
            bool isInReadOnlyMode = Volatile.Read(ref this.isReadonly);
            Diagnostics.Assert(isInReadOnlyMode == false, this.traceType, "Write operation cannot come after the differential has become readonly.");

            var snapAddList = this.addList;
            Diagnostics.Assert(
                snapAddList != null,
                this.traceType,
                "If Apply's are coming, perform checkpoint could not have been called (hence Sort). Version: {0} IsReadOnly: {1}",
                value.VersionSequenceNumber, Volatile.Read(ref this.isReadonly));

            // If the verion being added is a delete then it does not need to be added to the add list.
            // Either 
            // a) Insert is in LC - 1 and hence in addList already
            // b) Insert is in the same txn and hence it is not needed in addlist
            // c) Insert/Update is in Consolidated and hence not needed in addlist.
            if (value.Kind == RecordKind.DeletedVersion)
            {
                return;
            }

            // If consolidated component does not have the key or the latest version it has is a delete, we need to add it to addList.
            // Reminder: AddList is a list of all keys that are in the differential but not in consolidated.
            var valueInConsolidationManager = consolidationManager.Read(key);
            if (valueInConsolidationManager == null || valueInConsolidationManager.Kind == RecordKind.DeletedVersion)
            {
                snapAddList.TryAdd(key);
            }
        }
    }
}