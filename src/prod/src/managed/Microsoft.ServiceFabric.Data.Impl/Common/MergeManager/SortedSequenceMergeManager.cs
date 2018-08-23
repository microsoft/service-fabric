// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;

    using Microsoft.ServiceFabric.Data.Common.MergeManager;

    /// <summary>
    /// Provides K-Way Merge: Takes in multiple sorted enumerable (ascending) and merges them into a single sorted enumerable.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    internal class SortedSequenceMergeManager<T>
    {
        private const string ClassName = "SortedSequenceMergeManager";

        private readonly List<IEnumerable<T>> sequenceList;
        private readonly IComparer<T> keyComparer;
        private readonly Func<T, bool> keyFilter;
        private readonly bool useFirstKey;
        private readonly T firstKey;
        private readonly bool useLastKey;
        private readonly T lastKey;

        public SortedSequenceMergeManager(IComparer<T> keyComparer, Func<T, bool> keyFilter)
            : this(keyComparer, keyFilter, false, default(T), false, default(T))
        {
        }

        public SortedSequenceMergeManager(IComparer<T> keyComparer, Func<T, bool> keyFilter, bool useFirstKey, T firstKey, bool useLastKey, T lastKey)
        {
            this.sequenceList = new List<IEnumerable<T>>();
            this.keyComparer = keyComparer;
            this.keyFilter = keyFilter;

            this.useFirstKey = useFirstKey;
            this.firstKey = firstKey;

            this.useLastKey = useLastKey;
            this.lastKey = lastKey;
        }

        public void Add(IEnumerable<T> sortedEnumerable)
        {
#if DEBUG
            // This operation is expensive (O(n)). Only run in debug.
            this.VerifyInputEnumerable(sortedEnumerable);
#endif
            this.sequenceList.Add(sortedEnumerable);
        }

        /// <summary>
        /// Merges the input sequences into a single sorted enumerable.
        /// </summary>
        /// <returns>IEnumerable that represents the resulting merged sorted sequence.</returns>
        public IEnumerable<T> Merge()
        {
            var enumerable = new SortedSequenceMergeEnumerable<T>(this.sequenceList, this.keyComparer, keyFilter, this.useFirstKey, this.firstKey, this.useLastKey, this.lastKey);
            return enumerable;
        }

        /// <summary>
        /// Verifies the input sequence.
        /// </summary>
        /// <param name="items">Input sequence</param>
        /// <remarks>
        /// Ensures that the sequence is ordered.
        /// </remarks>
        private void VerifyInputEnumerable(IEnumerable<T> items)
        {
            bool isInitialized = false;
            T lastItem = default(T);

            int index = 0;
            foreach (var currentItem in items)
            {
                int keyComparison = this.keyComparer.Compare(currentItem, lastItem);

                if (isInitialized)
                {
                    // Following assert uses index instead of ToString of the last and current item to avoid HBI to leak.
                    Diagnostics.Assert(
                        keyComparison > 0,
                        ClassName,
                        "Input sequence must be sorted with no duplicates: Index: {0} ComparisonResult: {1}",
                        index,
                        keyComparison);
                }
                else
                {
                    isInitialized = true;
                }

                lastItem = currentItem;
                index++;
            }
        }
    }
}