// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Runtime.InteropServices;
    using System.Threading;

    /// <summary>
    /// Partitioned List is a list designed to aviod Large Object Heap.
    /// List implementation grows the underlying items by allocating a bigger array and copying into it.
    /// This means that after 84K / sizeof(TValue), the array is allocated from LOH instead of SOH (Gen 0).
    /// 
    /// Partitioned List instead creates a new list when it needs to grow.
    /// </summary>
    /// <remarks>
    /// Note that since there is only a single level hierarchy, after ~115 million items the partition list will get into LOH.
    /// </remarks>
    internal class PartitionedList<TKey, TValue> : IEnumerable<KeyValuePair<TKey, TValue>>
    {
        /// <summary>
        /// Invalid index.
        /// </summary>
        public readonly Index InvalidIndex = new Index(-1, -1);

        /// <summary>
        /// Size of reference.
        /// </summary>
        private const int SizeOfReference = 8;

        /// <summary>
        /// Large Object Heap limit is 85 KBytes. Using 64 KBytes to guarantee SOH allocation.
        /// Assumption: This number if power of two. If not, more compute would need to be done in happy path.
        /// </summary>
        private const int LOHLimit = 65536; 

        /// <summary>
        /// Default for max number of sublists. Set to 8192 if you want to never use LOH.
        /// </summary>
        private const int DefaultMaxNumberOfSubLists = int.MaxValue;

        /// <summary>
        /// Default for size for sublists.
        /// </summary>
        private const int DefaultMaxSubListSize = 1024;

        /// <summary>
        /// Size of the sublists.
        /// </summary>
        private static int SubListSize = GetMaxSubListSize();

        /// <summary>
        /// The partition list.
        /// </summary>
        protected readonly List<IPartition<TKey, TValue>> partitionList;

        /// <summary>
        /// Max number of sublists. Set to 8192 if you want to never use LOH.
        /// </summary>
        private readonly int maxNumberOfSubLists;

        /// <summary>
        /// Max number of items in a sublist. 8192 ensures that LOH is never used.
        /// </summary>
        private readonly int maxSubListSize;

        /// <summary>
        /// Current number of items.
        /// </summary>
        private long count;

        /// <summary>
        /// Current partition accepting adds.
        /// </summary>
        private Partition<TKey, TValue> currentPartition;

        /// <summary>
        /// Initializes a new instance of the Partitioned List class.
        /// </summary>
        public PartitionedList() : this (DefaultMaxNumberOfSubLists, SubListSize)
        {
        }

        public PartitionedList(int maxNumberOfSubLists, int maxSubListSize)
        {
            this.maxNumberOfSubLists = maxNumberOfSubLists;
            this.maxSubListSize = maxSubListSize;
            this.count = 0;

            this.partitionList = new List<IPartition<TKey, TValue>>();

            this.AddNewPartition(true);
        }

        public int Count
        {
            get
            {
                return (int)Interlocked.Read(ref this.count);
            }
        }

        public IEnumerable<TKey> Keys
        {
            get
            {
                return new KeyEnumerable(this.partitionList);
            }
        }

        /// <summary>
        /// Max number of items in a sublist.
        /// </summary>
        internal int MaxSubListSize
        {
            get
            {
                return this.maxSubListSize;
            }
        }

        public virtual void Add(TKey key, TValue value)
        {
            this.GrowIfNecessary();

            this.currentPartition.Add(key, value);
            Interlocked.Increment(ref this.count);
        }

        public TKey GetLastKey()
        {
            var currentPartitionCount = this.currentPartition.Count;
            if (currentPartitionCount == 0)
            {
                throw new InvalidOperationException("Empty Partition List.");
            }

            return this.currentPartition.GetKey(currentPartitionCount - 1);
        }

        // Clears the contents of List.
        public void Clear()
        {
            if (this.Count > 0)
            {
                this.partitionList.Clear();
                Interlocked.Exchange(ref this.count, 0);
            }
        }

        private void GrowIfNecessary()
        {
            if (this.currentPartition.Count == this.maxSubListSize)
            {
                this.AddNewPartition();
            }
        }

        private void AddNewPartition(bool isFirstPartition = false)
        {
            if (this.partitionList.Count == this.maxNumberOfSubLists)
            {
                throw new InvalidOperationException();
            }


            Partition<TKey, TValue> newPartition;
            if (isFirstPartition)
            {
                newPartition = new Partition<TKey, TValue>(this.maxSubListSize);
            }
            else
            {
                newPartition = new Partition<TKey, TValue>(this.maxSubListSize, this.maxSubListSize);
            }
          
            this.partitionList.Add(newPartition);
            this.currentPartition = newPartition;
        }

        public IEnumerator<KeyValuePair<TKey, TValue>> GetEnumerator()
        {
            for (int i = 0; i < this.partitionList.Count; i++)
            {
                var partition = this.partitionList[i];

                for (int j = 0; j < partition.Count; j++)
                {
                    yield return new KeyValuePair<TKey, TValue>(partition.GetKey(j), partition.GetValue(j));
                }
            }
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        private static int GetMaxSubListSize()
        {
            int numberOfItemsForKey = GetMaxSubListSize<TKey>();
            int numberOfItemsForValue = GetMaxSubListSize<TValue>();

            return Math.Min(numberOfItemsForKey, numberOfItemsForValue);
        }

        private static int GetMaxSubListSize<T>()
        {
            try
            {
                // If Reference type
                if (typeof(T).IsValueType == false)
                {
                    // Assumes LOHLimit is power of two.
                    return LOHLimit / SizeOfReference;
                }

                if (typeof(T).IsEnum)
                {
                    int sizeOfEmum = Marshal.SizeOf(Enum.GetUnderlyingType(typeof(T)));
                    return GetPowerOfTwoSmallerThanOrEqualTo(LOHLimit / sizeOfEmum);
                }

                if (typeof(T).IsGenericType == false)
                {
                    int sizeOfValueType = Marshal.SizeOf(typeof(T));
                    return GetPowerOfTwoSmallerThanOrEqualTo(LOHLimit / sizeOfValueType);
                }
            }
            catch (ArgumentException)
            {
                // default to DefaultMaxSubListSize.
            }

            return DefaultMaxSubListSize;
        }

        private static int GetPowerOfTwoSmallerThanOrEqualTo(int input)
        {
            Diagnostics.Assert(input > 0, "No type can have 0 or less size.");

            if (input == 1)
            {
                return 1;
            }

            int powerOfTwo = 1;
            while (true)
            {
                int newPower = powerOfTwo * 2;

                if (newPower == input)
                {
                    return newPower;
                }
                else if (newPower > input)
                {
                    return powerOfTwo;
                }

                powerOfTwo = newPower;
            }
        }

        private class KeyEnumerable : IEnumerable<TKey>
        {
            private readonly List<IPartition<TKey, TValue>> partitionList;

            public KeyEnumerable(List<IPartition<TKey, TValue>> partitionList)
            {
                this.partitionList = partitionList;
            }

            public IEnumerator<TKey> GetEnumerator()
            {
                for (int i = 0; i < this.partitionList.Count; i++)
                {
                    var partition = this.partitionList[i];

                    var partitionKeys = partition.Keys;
                    for (int j = 0; j < partitionKeys.Count; j++)
                    {
                        yield return partitionKeys[j];
                    }
                }
            }

            IEnumerator IEnumerable.GetEnumerator()
            {
                return this.GetEnumerator();
            }
        }
    }
}