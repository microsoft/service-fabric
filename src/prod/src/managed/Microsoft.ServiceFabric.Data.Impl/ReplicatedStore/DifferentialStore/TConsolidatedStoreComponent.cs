// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Linq;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Data.DataStructures;

    /// <summary>
    /// Holds the differential state for the store. It is an aggregated store component.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    /// <typeparam name="TKeyRangePartitioner"></typeparam>
    internal class TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
        IReadableStoreComponent<TKey, TVersionedItem<TValue>>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        /// <summary>
        /// Used for order by clauses.
        /// </summary>
        private static TKeyComparer KeyComparer = new TKeyComparer();

        #region Instance Members

        private readonly PartitionedSortedList<TKey, TVersionedItem<TValue>> component;

        #endregion

        private readonly string traceType;

        public TConsolidatedStoreComponent(string traceType)
        {
            this.traceType = traceType;
            this.component = new PartitionedSortedList<TKey, TVersionedItem<TValue>>(KeyComparer);
        }

        public TConsolidatedStoreComponent(long count, string traceType)
        {
            this.traceType = traceType;
            int intCount = checked((int)count);
            this.component = new PartitionedSortedList<TKey, TVersionedItem<TValue>>(KeyComparer);
        }

        #region IReadableStoreComponent<TKey> Members

        /// <summary>
        /// Checks to see if a certain key is present in the version chain.
        /// </summary>
        /// <param name="key">Lookup key.</param>
        /// <returns>True if key already exists in the write set.</returns>
        public bool ContainsKey(TKey key)
        {
            return this.component.ContainsKey(key);
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public TVersionedItem<TValue> Read(TKey key)
        {
            TVersionedItem<TValue> versionedItem;
            this.component.TryGetValue(key, out versionedItem);
            return versionedItem;
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <param name="visbilityLsn"></param>
        /// <returns>True and value if the key exists.</returns>
        public TVersionedItem<TValue> Read(TKey key, long visbilityLsn)
        {
            var versionedItem = this.Read(key);
            if (versionedItem != null && versionedItem.VersionSequenceNumber <= visbilityLsn)
            {
                return versionedItem;
            }

            return null;
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
            var nextKeyCandidates = this.component.Keys.Where(x => 0 < KeyComparer.Compare(x, key));
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

        /// <summary>
        /// Returns an enumerable for this collection.
        /// </summary>
        /// <returns>An enumerable that can be used to iterate through the collection.</returns>
        public IEnumerable<TKey> EnumerateKeys()
        {
            return this.component.Keys;
        }

        /// <summary>
        /// Returns an enumerable for this collection.
        /// </summary>
        /// <returns>An enumerable that can be used to iterate through the collection.</returns>
        public IEnumerable<TVersionedItem<TValue>> EnumerateValues()
        {
            foreach (var kvp in this.component)
            {
                yield return kvp.Value;
            }
        }

        #endregion

        /// <summary>
        /// Adds a new entry to this store component.
        /// </summary>
        /// <param name="key">Key to add.</param>
        /// <param name="value">Value to add.</param>
        public void Add(TKey key, TVersionedItem<TValue> value)
        {
            // There are no concurrent checkpoint, so it is okay to do it without a lock.
            Diagnostics.Assert(value != null, this.traceType, "value cannot be null");
            Diagnostics.Assert(value.Kind != RecordKind.DeletedVersion, this.traceType, "Deleted items are not added from consolidation.");

            var existingValue = this.Read(key);
            Diagnostics.Assert(existingValue == null, this.traceType, "existing item is being added.");

            try
            {
                this.component.Add(key, value);
            }
            catch (ArgumentException)
            {
                Diagnostics.Assert(false, this.traceType, "failed to add to consolidated component");
            }
        }

        /// <summary>
        /// Update used for copy-on write for onees created on merge.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="value"></param>
        public void Update(TKey key, TVersionedItem<TValue> value)
        {
            var existingValue = this.Read(key);
            Diagnostics.Assert(value != null, this.traceType, "value cannot be null");
            Diagnostics.Assert(
                existingValue.VersionSequenceNumber == value.VersionSequenceNumber,
                this.traceType,
                "Update must replace the item with the same version.");

            this.component[key] = value;
        }

        public long Count()
        {
            return this.component.Count;
        }
    }
}