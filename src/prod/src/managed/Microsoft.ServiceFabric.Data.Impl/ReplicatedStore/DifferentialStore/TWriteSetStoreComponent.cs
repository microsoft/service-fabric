// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Holds store transaction write set state for the store.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    internal class TWriteSetStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
    {
        private static TKeyComparer KeyComparer = new TKeyComparer();
        private static TKeyEqualityComparer KeyEqualityComparer = new TKeyEqualityComparer();

        #region Instance Members

        /// <summary>
        /// Write set for this transaction. Records all store modifications performed by this store transaction.
        /// May contain keys from multiple differential store components.
        /// </summary>
        private Dictionary<TKey, TWriteSetItemContext> writeSet = new Dictionary<TKey, TWriteSetItemContext>(KeyEqualityComparer);

        #endregion

        private readonly string traceType;

        public TWriteSetStoreComponent(string traceType)
        {
            this.traceType = traceType;
        }

        /// <summary>
        /// Checks to see if a certain key is present in the version chain.
        /// </summary>
        /// <param name="key">Lookup key.</param>
        /// <returns>True if key already exists in the write set.</returns>
        public bool ContainsKey(TKey key)
        {
            return this.writeSet.ContainsKey(key);
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public TVersionedItem<TValue> Read(TKey key)
        {
            TWriteSetItemContext context;
            if (this.writeSet.TryGetValue(key, out context))
                return context.LatestValue;

            return null;
        }

        /// <summary>
        /// Reads the value for a key greater than the given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public ConditionalValue<TKey> ReadNext(TKey key)
        {
            var minKeyFound = false;
            var minKey = default(TKey);

            foreach (var x in this.writeSet)
            {
                if (KeyComparer.Compare(x.Key, key) <= 0)
                {
                    continue;
                }

                if (!minKeyFound)
                {
                    minKeyFound = true;
                    minKey = x.Key;
                }
                else if (KeyComparer.Compare(x.Key, minKey) < 0)
                {
                    minKey = x.Key;
                }
            }

            return new ConditionalValue<TKey>(minKeyFound, minKey);
        }

        /// <summary>
        /// Returns an enumerable that iterates through the collection.
        /// </summary>
        /// <returns>Enumerator over all versions from this write set.</returns>
        public IEnumerable<TKey> GetSortedKeyEnumerable(Func<TKey, bool> keyFilter, bool useFirstKey, TKey firstKey, bool useLastKey, TKey lastKey)
        {
            var snapWriteSet = this.writeSet;

            var numberOfItems = snapWriteSet.Count;
            if (numberOfItems == 0)
            {
                return null;
            }

            List<TKey> keyList = new List<TKey>(numberOfItems);
            foreach (var row in snapWriteSet)
            {
                // If current item is smaller than the "first key", ignore the key.
                if (useFirstKey && KeyComparer.Compare(row.Key, firstKey) < 0)
                {
                    continue;
                }

                // If current item is larger than the "last key", ignore the key.
                if (useLastKey && KeyComparer.Compare(row.Key, lastKey) > 0)
                {
                    continue;
                }

                // If current item does not fit the filter, go to next.
                if (keyFilter != null && keyFilter.Invoke(row.Key) == false)
                {
                    continue;
                }

                keyList.Add(row.Key);
            }

            keyList.Sort(KeyComparer);
            return keyList;
        }

        /// <summary>
        /// Adds a version (insert or delete) to the store component for a given key.
        /// </summary>
        /// <param name="isIdempotent">Specifies if the add operation is idempotent.</param>
        /// <param name="key">Key for which the version is added.</param>
        /// <param name="item">Version item to add.</param>
        public void Add(bool isIdempotent, TKey key, TVersionedItem<TValue> item)
        {
            // Checks.
            Diagnostics.Assert(
                item.VersionSequenceNumber >= 0,
                this.traceType,
                "unexpected replication sequence number {0}",
                item.VersionSequenceNumber);

            TWriteSetItemContext context;
            if (this.writeSet.TryGetValue(key, out context))
            {
                if (isIdempotent)
                {
                    // Apply only if the version sequence number is higher.
                    if (item.VersionSequenceNumber > context.LatestValue.VersionSequenceNumber)
                    {
                        this.Update(key, context, item);
                    }
                }
                else
                {
                    // Update must succeed
                    this.Update(key, context, item);
                }
            }
            else
            {
                // First time this key is modified - add it.
                this.writeSet.Add(key, new TWriteSetItemContext(item));
            }
        }

        /// <summary>
        /// Clears the version chain.
        /// </summary>
        /// <returns></returns>
        public void Clear()
        {
            this.writeSet = null;
        }

        private void Update(TKey key, TWriteSetItemContext currentContext, TVersionedItem<TValue> newItem)
        {
            var lastVersionedItem = currentContext.LatestValue;

            // Further checks.
            if (lastVersionedItem.VersionSequenceNumber > 0)
            {
                Diagnostics.Assert(
                    lastVersionedItem.VersionSequenceNumber.CompareTo(newItem.VersionSequenceNumber) < 0,
                    this.traceType,
                    "unexpected version sequence number ({0},{1})",
                    lastVersionedItem.VersionSequenceNumber,
                    newItem.VersionSequenceNumber);
            }

            if (newItem is TDeletedItem<TValue>)
            {
                Diagnostics.Assert(
                    lastVersionedItem is TInsertedItem<TValue> || lastVersionedItem is TUpdatedItem<TValue>,
                    this.traceType,
                    "unexpected deleted/deleted version sequence ({0},{1})",
                    lastVersionedItem.VersionSequenceNumber,
                    newItem.VersionSequenceNumber);
            }
            else if (newItem is TInsertedItem<TValue>)
            {
                Diagnostics.Assert(
                    lastVersionedItem is TDeletedItem<TValue>,
                    this.traceType,
                    "unexpected inserted/inserted version sequence ({0},{1})",
                    lastVersionedItem.VersionSequenceNumber,
                    newItem.VersionSequenceNumber);
            }
            else if (newItem is TUpdatedItem<TValue>)
            {
                Diagnostics.Assert(
                    lastVersionedItem is TInsertedItem<TValue> || lastVersionedItem is TUpdatedItem<TValue>,
                    this.traceType,
                    "unexpected deleted/updated version sequence ({0},{1})",
                    lastVersionedItem.VersionSequenceNumber,
                    newItem.VersionSequenceNumber);
            }

            // Update last versioned item.
            this.writeSet[key] = new TWriteSetItemContext(currentContext, newItem);
        }

        /// <summary>
        /// Tracks the last value written, create LSN, and the type of the first write (insert, update, remove) for notifications.
        /// </summary>
        private struct TWriteSetItemContext
        {
            public readonly TVersionedItem<TValue> LatestValue;
            public readonly long CreateSequenceNumber;
            public readonly RecordKind FirstVersionKind;

            public TWriteSetItemContext(TVersionedItem<TValue> value)
            {
                this.LatestValue = value;
                this.CreateSequenceNumber = value.VersionSequenceNumber;
                this.FirstVersionKind = value.Kind;
            }

            public TWriteSetItemContext(TWriteSetItemContext previous, TVersionedItem<TValue> latestValue)
            {
                this.LatestValue = latestValue;
                this.CreateSequenceNumber = previous.CreateSequenceNumber;
                this.FirstVersionKind = previous.FirstVersionKind;
            }
        }
    }
}