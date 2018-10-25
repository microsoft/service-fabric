// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;

    internal class KVList<TKey, TValue>
    {
        private const string ClassName = "KVList";

        private readonly List<TKey> keyList;
        private readonly List<TValue> valueList;

        private readonly int maxSize;

        /// <summary>
        /// Initializes a new instance of the KVList class.
        /// </summary>
        /// <param name="maxSize">Indicates the maximum capacity of the KVList instance.</param>
        public KVList(int maxSize)
        {
            Diagnostics.Assert(maxSize > 0, ClassName, "maxSize > 0");
            this.maxSize = maxSize;
            this.keyList = new List<TKey>();
            this.valueList = new List<TValue>();
        }

        /// <summary>
        /// Initializes a new instance of the KVList class.
        /// </summary>
        /// <param name="initialSize">Size of the first list.</param>
        /// <param name="maxSize">Indicates the maximum capacity of the KVList instance.</param>
        public KVList(int initialSize, int maxSize)
        {
            Diagnostics.Assert(maxSize > 0, ClassName, "maxSize > 0");
            Diagnostics.Assert(initialSize <= maxSize, ClassName, "initialSize <= maxSize");

            this.maxSize = maxSize;
            this.keyList = new List<TKey>(initialSize);
            this.valueList = new List<TValue>(initialSize);
        }

        /// <summary>
        /// Gets the current number of items in the KVList.
        /// </summary>
        public int Count
        {
            get
            {
#if DEBUG
                Diagnostics.Assert(this.keyList.Count == this.valueList.Count, ClassName, "Key and value list must have the same count.");
                Diagnostics.Assert(this.keyList.Count <= this.maxSize, ClassName, "Count cannot be higher than max size.");
#endif
                return this.keyList.Count;
            }
        }

        /// <summary>
        /// Gets the current list of keys.
        /// </summary>
        public IReadOnlyList<TKey> Keys
        {
            get
            {
                return this.keyList;
            }
        }

        /// <summary>
        /// Appends an item to the end of the KVList.
        /// </summary>
        /// <param name="key">Key to be added.</param>
        /// <param name="value">Value to be added.</param>
        public void Add(TKey key, TValue value)
        {
            if (this.Count == this.maxSize)
            {
                throw new InvalidOperationException("Out of capacity");
            }

            this.keyList.Add(key);
            this.valueList.Add(value);
        }

        public TKey GetKey(int index)
        {
            this.ThrowArgumentOutOfRangeIfNecessary(index);

            return this.keyList[index];
        }

        public TValue GetValue(int index)
        {
            this.ThrowArgumentOutOfRangeIfNecessary(index);

            return this.valueList[index];
        }

        public void UpdateValue(int index, TValue value)
        {
            this.ThrowArgumentOutOfRangeIfNecessary(index);

            this.valueList[index] = value;
        }

        public int BinarySearch(TKey key, IComparer<TKey> comparer)
        {
            return this.keyList.BinarySearch(key, comparer);
        }

        private void ThrowArgumentOutOfRangeIfNecessary(int index)
        {
            if (index >= this.Count)
            {
                throw new ArgumentOutOfRangeException("index");
            }
        }
    }
}