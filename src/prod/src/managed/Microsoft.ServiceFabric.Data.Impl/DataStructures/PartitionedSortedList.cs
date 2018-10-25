// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;

    /// <summary>
    /// Provides a Sorted List that
    /// - Avoids LOH upto high number of items (~64 million)
    /// - O(1) Adds instead of O(log N)
    /// - O(log n) Get and Update
    /// - O(n) enumeration (with key only support)
    /// 
    /// To provide O(1) adds, the data strucuture takes dependency on the user adding items in order.
    /// This is the case for TStore Recovery and Consolidation.
    /// </summary>
    /// <typeparam name="TKey">Type of the key.</typeparam>
    /// <typeparam name="TValue">>Type of the value.</typeparam>
    internal class PartitionedSortedList<TKey, TValue> : PartitionedList<TKey, TValue>
    {
        /// <summary>
        /// The Key comparer.
        /// </summary>
        private readonly IComparer<TKey> keyComparer;

        /// <summary>
        /// The partition comparer. First item in the partition is used for comparison.
        /// </summary>
        private readonly IComparer<IPartition<TKey, TValue>> partitionComparer;

        /// <summary>
        /// Initializes a new instance of the PartitionedSortedList.
        /// </summary>
        public PartitionedSortedList() : this(Comparer<TKey>.Default)
        {
        }

        /// <summary>
        /// Initializes a new instance of the PartitionedSortedList using a Key Comparer.
        /// </summary>
        public PartitionedSortedList(IComparer<TKey> keyComparer) : base()
        {
            this.keyComparer = keyComparer;
            this.partitionComparer = new PartitionComparer<TKey, TValue>(this.keyComparer);
        }

        /// <summary>
        /// Initializes a new instance of the PartitionedSortedList.
        /// </summary>
        public PartitionedSortedList(int maxNumberOfSubLists, int maxSubListSize) : this(Comparer<TKey>.Default, maxNumberOfSubLists, maxSubListSize)
        {
        }

        /// <summary>
        /// Initializes a new instance of the PartitionedSortedList.
        /// </summary>
        public PartitionedSortedList(IComparer<TKey> keyComparer, int maxNumberOfSubLists, int maxSubListSize) : base(maxNumberOfSubLists, maxSubListSize)
        {
            this.keyComparer = keyComparer;
            this.partitionComparer = new PartitionComparer<TKey, TValue>(this.keyComparer);
        }

        /// <summary>
        /// Gets of sets the value of row identified by the key.
        /// </summary>
        /// <param name="key">Key of the row.</param>
        /// <returns></returns>
        public TValue this[TKey key]
        {
            get
            {
                Index index;
                bool itemExists = this.TryGetIndex(key, out index);
                if (itemExists == false)
                {
                    throw new InvalidOperationException("key does not exist.");
                }

                return this.partitionList[index.PartitionIndex].GetValue(index.ItemIndex);
            }
            set
            {
                Index index;
                bool itemExists = this.TryGetIndex(key, out index);
                if (itemExists == false)
                {
                    throw new InvalidOperationException("key does not exist.");
                }

                this.partitionList[index.PartitionIndex].UpdateValue(index.ItemIndex, value);
            }
        }

        public override void Add(TKey key, TValue value)
        {
            this.ThrowIfKeyIsNull(key);

#if DEBUG
            if (this.Count != 0)
            {
                var lastKey = this.GetLastKey();
                int comparison = this.keyComparer.Compare(lastKey, key);
                Diagnostics.Assert(comparison < 0, "Ordered Inserts");
            }
#endif

            base.Add(key, value);
        }

        /// <summary>
        /// Determines whether the container contains a specific key.
        /// </summary>
        /// <param name="key">The key to locate.</param>
        /// <returns>
        /// true if the PartitionedSortedList contains an element with the specified key; otherwise, false.
        /// </returns>
        public bool ContainsKey(TKey key)
        {
            Index index;
            bool itemExists = this.TryGetIndex(key, out index);

            return itemExists;
        }

        /// <summary>
        /// Gets the value of the key if it exists.
        /// </summary>
        /// <param name="key">The key to locate.</param>
        /// <param name="value">Out value to be returned if the key exists.</param>
        /// <returns>
        /// true if the PartitionedSortedList contains an element with the specified key; otherwise, false.
        /// </returns>
        public bool TryGetValue(TKey key, out TValue value)
        {
            Index index;
            bool itemExists = this.TryGetIndex(key, out index);

            if (itemExists == false)
            {
                value = default(TValue);
                return false;
            }

            value = this.partitionList[index.PartitionIndex].GetValue(index.ItemIndex);
            return true;
        }

        private void ThrowIfKeyIsNull(TKey key)
        {
             if (key == null)
             {
                 throw new ArgumentNullException("null", "key");
             }
        }

        private bool TryGetIndex(TKey key, out Index index)
        {
            this.ThrowIfKeyIsNull(key);

            if (this.Count == 0)
            {
                index = this.InvalidIndex;
                return false;
            }

            PartitionSearchKey<TKey, TValue> searchKey = new PartitionSearchKey<TKey, TValue>(key);
            var partitionIndex = this.partitionList.BinarySearch(searchKey, this.partitionComparer);

            if (partitionIndex >= 0)
            {
                index = new Index(partitionIndex, 0);
                return true;
            }

            partitionIndex = (~partitionIndex) - 1;

            if (partitionIndex < 0)
            {
                index = this.InvalidIndex;
                return false;
            }

            var itemIndex = this.partitionList[partitionIndex].BinarySearch(key, this.keyComparer);
            if (itemIndex >= 0)
            {
                index = new Index(partitionIndex, itemIndex);
                return true;
            }

            index = this.InvalidIndex;
            return false;
        }
    }
}