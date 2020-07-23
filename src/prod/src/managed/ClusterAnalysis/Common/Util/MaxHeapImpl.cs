// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Util
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public class MaxHeapImpl<T> : IMaxHeap<T> where T : IComparable<T>
    {
        private IList<T> heapItems;

        public MaxHeapImpl()
        {
            this.heapItems = new List<T>();
        }

        public int ItemCount
        {
            get { return this.heapItems.Count; }
        }

        public bool IsEmpty()
        {
            return !this.heapItems.Any();
        }

        public T ExtractMax()
        {
            var countOfItems = this.heapItems.Count;
            if (countOfItems == 0)
            {
                throw new Exception();
            }

            var firstItem = this.heapItems[0];

            if (countOfItems == 1)
            {
                this.heapItems.Clear();
                return firstItem;
            }

            var lastItem = this.heapItems.Last();
            this.heapItems[0] = lastItem;
            this.heapItems.RemoveAt(countOfItems - 1);

            this.Heapify(0);

            return firstItem;
        }

        public T PeekMax()
        {
            if (!this.heapItems.Any())
            {
                throw new Exception();
            }

            return this.heapItems[0];
        }

        public void Insert(T value)
        {
            this.heapItems.Add(value);

            var current = this.heapItems.Count - 1;
            var currentParent = Parent(current);
            while (current > 0 && this.heapItems[currentParent].CompareTo(this.heapItems[current]) < 0)
            {
                var temp = this.heapItems[currentParent];
                this.heapItems[currentParent] = this.heapItems[current];
                this.heapItems[current] = temp;

                current = currentParent;
                currentParent = Parent(current);
            }
        }

        private static int Parent(int i)
        {
            return (i - 1) / 2;
        }

        private void Heapify(int i)
        {
            var leftChild = (2 * i) + 1;
            var rightChild = (2 * i) + 2;

            var largerOneIndex = i;

            if (leftChild < this.heapItems.Count && this.heapItems[leftChild].CompareTo(this.heapItems[i]) > 0)
            {
                largerOneIndex = leftChild;
            }

            if (rightChild < this.heapItems.Count && this.heapItems[rightChild].CompareTo(this.heapItems[largerOneIndex]) > 0)
            {
                largerOneIndex = rightChild;
            }

            // It implies that element at ith index violate the max heap invariant and balancing is required.
            if (largerOneIndex != i)
            {
                var temp = this.heapItems[largerOneIndex];
                this.heapItems[largerOneIndex] = this.heapItems[i];
                this.heapItems[i] = temp;

                this.Heapify(largerOneIndex);
            }
        }
    }
}