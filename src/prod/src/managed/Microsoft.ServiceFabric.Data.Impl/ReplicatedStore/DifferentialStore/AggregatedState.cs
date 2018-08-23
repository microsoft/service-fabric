// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Fabric.Data.Common;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Data;
    using System.Collections.Concurrent;

    using Microsoft.ServiceFabric.Data.Common;
    using Microsoft.ServiceFabric.Data.ReplicatedStore.DifferentialStore;
    /// <summary>
    /// Holds to-be consolidated and consolidated states.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    /// <typeparam name="TKeyComparer">Key comparer type.</typeparam>
    /// <typeparam name="TKeyEqualityComparer">Key equality comparer type.</typeparam>
    /// <typeparam name="TKeyRangePartitioner"></typeparam>
    internal class AggregatedState<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
        IReadableStoreComponent<TKey, TVersionedItem<TValue>>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        private ConcurrentDictionary<int, TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>> deltaDifferentialStateList =
          new ConcurrentDictionary<int, TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>>();

        // Index starts with 1, zero is not a valid index.
        private int index;
        private static TKeyComparer KeyComparer = new TKeyComparer();
        private readonly TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> consolidatedState;
        private readonly string traceType;

        public AggregatedState(string traceType)
        {
            this.traceType = traceType;
            this.index = 0;
            this.consolidatedState = new TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>(this.traceType);
        }

       public AggregatedState(TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> consolidatedState, string traceType)
       {
           this.consolidatedState = consolidatedState;
           this.index = 0;
           this.traceType = traceType;
        }

        public ConcurrentDictionary<int, TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>> DeltaDifferentialStateList
        {
            get
            {
                return this.deltaDifferentialStateList;
            }
        }

        public TConsolidatedStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> ConsolidatedState 
        {
            get
            {
                return this.consolidatedState;
            }
        }

        public int Index 
        { 
            get
            {
                return this.index;
            }
        }

        #region IReadableStoreComponent<TKey> Members

        /// <summary>
        /// Checks to see if a certain key is present in the version chain.
        /// </summary>
        /// <param name="key">Lookup key.</param>
        /// <returns>True if key already exists in the write set.</returns>
        public bool ContainsKey(TKey key)
        {
            // Read from delta states in the reverse order of deltas.
            foreach (var deltaDifferentialState in this.GetInReverseOrder())
            {
                if(deltaDifferentialState.ContainsKey(key))
                {
                    return true;
                }
            }

            return this.consolidatedState.ContainsKey(key);
        }

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>True and value if the key exists.</returns>
        public TVersionedItem<TValue> Read(TKey key)
        {
            TVersionedItem<TValue> versionedItem = null;
            foreach (var deltaDifferentialState in this.GetInReverseOrder())
            {
                versionedItem = deltaDifferentialState.Read(key);
                if (versionedItem != null)
                {
                    break;
                }
            }
            if (versionedItem == null)
            {
                versionedItem = this.consolidatedState.Read(key);
            }

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
            TVersionedItem<TValue> versionedItem = null;
            foreach (var deltaDifferentialState in this.GetInReverseOrder())
            {
                versionedItem = deltaDifferentialState.Read(key, visbilityLsn);
                if (versionedItem != null)
                {
                    return versionedItem;
                }
            }

            versionedItem = this.consolidatedState.Read(key, visbilityLsn);
            if (versionedItem != null)
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
            var nextKeyCandidates = this.GetNextKeyCandidates(key);
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
            return this.GetSortedKeyEnumerable(null, false, default(TKey), false, default(TKey));
        }

        public IEnumerable<TKey> GetSortedKeyEnumerable(Func<TKey, bool> keyFilter, bool useFirstKey, TKey firstKey, bool useLastKey, TKey lastKey)
        {
            SortedSequenceMergeManager<TKey> mergeManager = new SortedSequenceMergeManager<TKey>(KeyComparer, null, useFirstKey, firstKey, useLastKey, lastKey);

            foreach (var kvp in this.deltaDifferentialStateList)
            {
                var deltaDifferentialState = kvp.Value;
                mergeManager.Add(deltaDifferentialState.GetEnumerableNewKeys());
            }

            mergeManager.Add(this.consolidatedState.EnumerateKeys());

            return mergeManager.Merge();
        }

        public IEnumerable<TVersionedItem<TValue>> GetValuesForSweep()
        {
            int i = 0;
            foreach (var deltaState in this.GetInReverseOrder())
            {
                i++;

                // Skip the last delta state since we do not want to sweep the items that comes from the differntial state yet.
                // Due to background consolidation, file metadata associated with items that just came from the differentrial state might not yet be set to Next Metadata table,
                // so last delta needs to be skipped to avoid reads from failing.
                if (i == 1)
                {
                    continue;
                }

                foreach (var item in deltaState.EnumerateValues())
                {
                    yield return item;
                }
            }
        }

        public long Count()
        {
            long count = 0;
            // Read from delta states in the reverse order of deltas.
            foreach (var kvp in this.deltaDifferentialStateList)
            {
                var deltaDifferentialState = kvp.Value;
                count = count + deltaDifferentialState.Count();
            }

            return count + this.consolidatedState.Count();
        }

        #endregion

        public void AppendToDeltaDifferentialState(TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> deltaState)
        {
            Diagnostics.Assert(deltaState != null, this.traceType, "Delta state cannot be null");

            // Avoid incrementing index beore insertion.
            int localIndex = this.index;
            bool add = this.deltaDifferentialStateList.TryAdd(++localIndex, deltaState);
            Diagnostics.Assert(add, this.traceType, "Failed to add index {0} to differential list", localIndex);
            this.index = localIndex;
        }

        /// <summary>
        /// Get the list next keys.
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        /// <devnote>This is very inefficient today, once we support add list, this will be fixed.</devnote>
        private IEnumerable<TKey> GetNextKeyCandidates(TKey key)
        {
            var nextKey = this.ConsolidatedState.ReadNext(key);
            if (nextKey.HasValue)
            {
                yield return nextKey.Value;
            }

            foreach (var kvp in this.deltaDifferentialStateList)
            {
                var deltaState = kvp.Value;
                nextKey = deltaState.ReadNext(key);
                if (nextKey.HasValue)
                {
                    yield return nextKey.Value;
                }
            }
        }

        private IEnumerable<TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>> GetInReverseOrder()
        {
            // Snap the highest index
            int highestIndex = this.index;
            for(int i=highestIndex; i>=1; i--)
            {
                TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> diffComponent;
                bool result = this.deltaDifferentialStateList.TryGetValue(i, out diffComponent);
                Diagnostics.Assert(result, this.traceType, "All indexes should be present");
                Diagnostics.Assert(diffComponent != null, this.traceType, "Delta state cannot be null");
                yield return diffComponent;
            }
        }
    }
}
