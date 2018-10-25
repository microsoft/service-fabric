// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Globalization;

    /// <summary>
    /// Represents a chunk list that contains a list of health state chunk items.
    /// </summary>
    /// <typeparam name="T">The type of the items in the list.</typeparam>
    /// <remarks><para>The chunk list is obtained from queries that can potentially have more results than can fit a message. 
    /// Only the entries that fit are returned. The list included the total number of items that should have been returned if there was enough space.</para></remarks>
    public abstract class HealthStateChunkList<T> : IList<T>
    {
        private IList<T> list;

        /// <summary>
        /// Instantiates an empty <see cref="System.Fabric.Health.HealthStateChunkList{T}"/> class.
        /// </summary>
        protected HealthStateChunkList() : this(new List<T>())
        {
        }

        /// <summary>
        /// Instantiates a <see cref="System.Fabric.Health.HealthStateChunkList{T}"/> class with the items of another list.
        /// </summary>
        /// <param name="list">The list with items used to create the paged list.</param>
        protected HealthStateChunkList(IList<T> list)
        {
            this.list = list;
        }
        
        /// <summary>
        /// Gets the index in this list for the specified item.
        /// </summary>
        /// <param name="item">The item.</param>
        /// <returns>The index in this list for the specified item. </returns>
        public int IndexOf(T item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// Inserts an item into this list at the specified index.
        /// </summary>
        /// <param name="index">The index where the item will be inserted.</param>
        /// <param name="item">The item to insert.</param>
        public void Insert(int index, T item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// Removes the item at the specified index from this list. 
        /// </summary>
        /// <param name="index">The index where the item will be removed.</param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// Gets the item at the specified index.
        /// </summary>
        /// <param name="index">The index.</param>
        /// <returns>The item at the specified index.</returns>
        public T this[int index]
        {
            get
            {
                return this.list[index];
            }
            set
            {
                this.list[index] = value;
            }
        }

        /// <summary>
        /// Gets the total number of items to be returned in one or more messages.
        /// </summary>
        /// <value>The total number of items available in the system, out of which the current items were returned.</value>
        public long TotalCount
        {
            get;
            set;
        }

        /// <summary>
        /// Adds an item to this list.
        /// </summary>
        /// <param name="item">The item to add to the list.</param>
        public void Add(T item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// Removes all items from this list.
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// Specifies whether the list contains a specific item.
        /// </summary>
        /// <param name="item">The item to search.</param>
        /// <returns>true if the list contains a specific item; otherwise, false.</returns>
        public bool Contains(T item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// Copies items from this list to the specified array starting at the specified index.
        /// </summary>
        /// <param name="array">The array.</param>
        /// <param name="arrayIndex">The array index.</param>
        public void CopyTo(T[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// Gets or sets the number of items in the list.
        /// </summary>
        /// <value>The number of items in the list.</value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// Gets or sets a flag that indicated whether the list can be modified.
        /// </summary>
        /// <value>Flag indicating whether the list can be modified.</value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// Removes the specified item from this list. 
        /// </summary>
        /// <param name="item">The item to be removed.</param>
        /// <returns>true if the item is removed; otherwise, false.</returns>
        public bool Remove(T item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// Gets an enumerator to items in this list. 
        /// </summary>
        /// <returns>The enumerator to items in this list. </returns>
        public IEnumerator<T> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// Returns a string representation of the chunk list.
        /// </summary>
        /// <returns>A string representation of the chunk list.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.CurrentCulture, "{0} items, TotalCount {1}", this.Count, this.TotalCount);
        }

        /// <summary>
        /// Gets an enumerator to items in this list. 
        /// </summary>
        /// <returns>The enumerator to items in this list. </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }
    }

    /// <summary>
    /// Extensions for health state chunk list operations.
    /// </summary>
    public static class HealthStateChunkListHelper
    {
        /// <summary>
        /// Returns an array with the items in the chunk list.
        /// </summary>
        /// <typeparam name="T">The type of the list objects.</typeparam>
        /// <param name="list">The health state chunk list.</param>
        /// <returns>The array.</returns>
        public static T[] ToArray<T>(this HealthStateChunkList<T> list)
        {
            if (list == null)
            {
                return new T[0];
            }

            T[] returnValue = new T[list.Count];
            for (int i = 0; i < list.Count; i++)
            {
                returnValue[i] = list[i];
            }

            return returnValue;
        }
    }
}