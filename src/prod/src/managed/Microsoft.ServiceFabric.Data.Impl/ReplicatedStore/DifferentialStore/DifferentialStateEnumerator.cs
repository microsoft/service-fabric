// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;

    internal class DifferentialStateEnumerator<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> :
        IEnumerator<KeyValuePair<TKey, TVersionedItem<TValue>>>,
        IComparable<DifferentialStateEnumerator<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner>>
        where TKeyComparer : IComparer<TKey>, new()
        where TKeyEqualityComparer : IEqualityComparer<TKey>, new()
        where TKeyRangePartitioner : IRangePartitioner<TKey>, new()
    {
        private static TKeyComparer KeyComparer = new TKeyComparer();
        private readonly IEnumerator<KeyValuePair<TKey, TVersionedItem<TValue>>> sortedEnumerator;

        public DifferentialStateEnumerator(TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> component)
        {
            this.Component = component;
            IEnumerable<KeyValuePair<TKey, TVersionedItem<TValue>>> enumerable;
            this.Component.TryEnumerateKeyAndValue(out enumerable);
            this.sortedEnumerator = enumerable.GetEnumerator();
        }

        public TDifferentialStoreComponent<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> Component { get; set; }

        public int CompareTo(DifferentialStateEnumerator<TKey, TValue, TKeyComparer, TKeyEqualityComparer, TKeyRangePartitioner> other)
        {
            // Compare Keys
            var compare = KeyComparer.Compare(this.Current.Key, other.Current.Key);
            if (compare != 0)
            {
                return compare;
            }

            // Compare LSNs
            var currentLsn = this.Current.Value.VersionSequenceNumber;
            var otherLsn = other.Current.Value.VersionSequenceNumber;

            if (currentLsn > otherLsn)
            {
                return -1;
            }
            else if (currentLsn < otherLsn)
            {
                return 1;
            }

            return 0;
        }

        public KeyValuePair<TKey, TVersionedItem<TValue>> Current
        {
            get { return this.sortedEnumerator.Current; }
        }

        public bool MoveNext()
        {
            return this.sortedEnumerator.MoveNext();
        }

        public void Dispose()
        {
            this.sortedEnumerator.Dispose();
        }

        object Collections.IEnumerator.Current
        {
            get { throw new NotImplementedException(); }
        }

        public void Reset()
        {
            throw new NotImplementedException();
        }
    }
}