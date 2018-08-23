// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;

    internal class PriorityQueue<T> where T : IComparable<T>
    {
        private SortedList<T, bool> queue = new SortedList<T, bool>();
        private IList<T> keys;

        public PriorityQueue()
        {
            this.keys = this.queue.Keys;
        }

        public int Count
        {
            get { return this.queue.Count; }
        }

        public void Push(T item)
        {
            this.queue.Add(item, true);
        }

        /// <summary>
        /// Pushes an item in to the priority queue.
        /// If a duplicate priority item already exists, returns false.
        /// </summary>
        /// <param name="item">The input item.</param>
        /// <returns>Boolean that represents wheterh the priority of the item is duplicate or not (pushed or not)</returns>
        public bool TryPush(T item)
        {
            if(this.queue.ContainsKey(item))
            {
                return false;
            }

            this.queue.Add(item, true);

            return true;
        }

        public T Peek()
        {
            return this.keys[0];
        }

        public T Pop()
        {
            var top = this.keys[0];
            this.queue.RemoveAt(0);
            return top;
        }
    }
}