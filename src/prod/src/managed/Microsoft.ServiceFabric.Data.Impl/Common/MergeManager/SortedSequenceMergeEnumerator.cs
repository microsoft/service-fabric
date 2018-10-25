// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Common.MergeManager
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Fabric.Store;

    /// <summary>
    /// Enumerator that K-Way Merges the input sequence lists as it moves forward (late bound).
    /// </summary>
    /// <typeparam name="T">Type of the input.</typeparam>
    internal class SortedSequenceMergeEnumerator<T> : IEnumerator<T>
    {
        private const string ClassName = "SortedSequenceMergeEnumerator";

        private readonly IComparer<T> keyComparer;
        private readonly PriorityQueue<ComparableSortedSequenceEnumerator<T>> priorityQueue;
        private readonly Func<T, bool> keyFilter;

        private readonly bool useFirstKey;
        private readonly T firstKey;
        private readonly bool useLastKey;
        private readonly T lastKey;

        private bool isCurrentSet;
        private T current;

        public SortedSequenceMergeEnumerator(IList<IEnumerable<T>> inputList, IComparer<T> keyComparer, Func<T, bool> keyFilter, bool useFirstKey, T firstKey, bool useLastKey, T lastKey)
        {
            Diagnostics.Assert(inputList != null, ClassName, "inputList must not be null.");
            Diagnostics.Assert(keyComparer != null, ClassName, "inputList must not be null.");

            this.isCurrentSet = false;
            this.current = default(T);
            this.keyComparer = keyComparer;
            this.keyFilter = keyFilter;

            this.useFirstKey = useFirstKey;
            this.firstKey = firstKey;

            this.useLastKey = useLastKey;
            this.lastKey = lastKey;

            this.priorityQueue = new PriorityQueue<ComparableSortedSequenceEnumerator<T>>();
            this.InitializePriorityQueue(inputList);
        }

        public T Current
        {
            get
            {
                Diagnostics.Assert(this.isCurrentSet, ClassName, "Current got called before MoveNext()");
                return this.current;
            }
        }

        object IEnumerator.Current
        {
            get
            {
                Diagnostics.Assert(this.isCurrentSet, ClassName, "Current got called before MoveNext()");
                return this.current;
            }
        }

        public void Dispose()
        {
            if (this.priorityQueue == null)
            {
                return;
            }

            while (this.priorityQueue.Count != 0)
            {
                var enumerator = this.priorityQueue.Pop();
                enumerator.Dispose();
            }
        }

        public bool MoveNext()
        {
            var lastCurrent = this.current;

            while (this.priorityQueue.Count != 0)
            {
                var currentEnumerator = this.priorityQueue.Pop();
                this.current = currentEnumerator.Current;

                bool hasMoreItems = currentEnumerator.MoveNext();
                if (hasMoreItems == false)
                {
                    currentEnumerator.Dispose();
                }
                else
                {
                    // TryPush is used to weed out duplicate keys that are already in the priority queue.
                    this.InsertToPriortyQueueIfNecessary(currentEnumerator);
                }

                // If current item is larger than the "last key", merge is finished.
                // This must be done before the key filter check.
                if (this.useLastKey && this.keyComparer.Compare(this.current, this.lastKey) > 0)
                {
                    return false;
                }

                // If current item does not fit the filter, go to next.
                if (this.keyFilter != null && this.keyFilter.Invoke(this.current) == false)
                {
                    continue;
                }

                if (this.isCurrentSet == false)
                {
                    this.isCurrentSet = true;
                    return true;
                }

                // De-duplicate the items.
                int comparisonWithLastCurrent = this.keyComparer.Compare(lastCurrent, this.current);
                Diagnostics.Assert(
                    comparisonWithLastCurrent <= 0,
                    ClassName,
                    "Last item must be less than or equal to current item. ComparisonResult: {0}",
                    comparisonWithLastCurrent);

                // if last item is less than new item, then this is new valid item.
                if (comparisonWithLastCurrent < 0)
                {
                    return true;
                }
            }

            return false;
        }

        public void Reset()
        {
            throw new NotImplementedException();
        }

        private void InitializePriorityQueue(IList<IEnumerable<T>> inputList)
        {
            foreach (var list in inputList)
            {
                ComparableSortedSequenceEnumerator<T> enumerator = new ComparableSortedSequenceEnumerator<T>(list, this.keyComparer);
                bool enumeratorIsNotEmpty = this.PrepareEnumerator(enumerator);
                if (enumeratorIsNotEmpty == false)
                {
                    // Since enumerator does not have any items that fit the filter, discard the enumerator.
                    enumerator.Dispose();
                    continue;
                }

                this.InsertToPriortyQueueIfNecessary(enumerator);                
            }
        }

        /// <summary>
        /// Prepares the enumerator to be added to the priority queue.
        /// Preparation includes moving the enumerator until the current key obides filters.
        /// </summary>
        /// <param name="enumerator">The enumerator.</param>
        /// <returns>Boolean that indicates that the enumerator is now empty.</returns>
        private bool PrepareEnumerator(ComparableSortedSequenceEnumerator<T> enumerator)
        {
            while (true)
            {
                bool itemExists = enumerator.MoveNext();
                if (itemExists == false)
                {
                    return false;
                }

                if (this.useFirstKey)
                {
                    int firstKeyComparison = this.keyComparer.Compare(this.firstKey, enumerator.Current);

                    // If firstKey for the filter is greater than the current, continue moving the enumerator.
                    if (firstKeyComparison > 0)
                    {
                        continue;
                    }
                }

                if (this.useLastKey)
                {
                    int lastKeyComparison = this.keyComparer.Compare(this.lastKey, enumerator.Current);

                    // If lastKey for the filter is less than the current, indicate that the enumerator is practically empty
                    if (lastKeyComparison < 0)
                    {
                        return false;
                    }
                }

                // If current item does not fit the filter, go to next.
                if (this.keyFilter != null && this.keyFilter.Invoke(enumerator.Current) == false)
                {
                    continue;
                }

                return true;
            }
        }

        private bool InsertToPriortyQueueIfNecessary(ComparableSortedSequenceEnumerator<T> enumerator)
        {
            Diagnostics.Assert(enumerator != null, ClassName, "Input enumerator must not be null.");

            while (true)
            {
                bool wasPushed = this.priorityQueue.TryPush(enumerator);
                if (wasPushed)
                {
                    return true;
                }

                var isNotEmpty = enumerator.MoveNext();
                if (isNotEmpty == false)
                {
                    enumerator.Dispose();
                    return false;
                }
            }
        }
    }
}