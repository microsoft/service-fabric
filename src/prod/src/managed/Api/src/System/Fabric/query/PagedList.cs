// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;

    /// <summary>
    /// <para>
    /// Represents a paged list that contains a list of items and a continuation token.
    /// </para>
    /// </summary>
    /// <typeparam name="T">
    /// <para>The type of the items returned by query.</para>
    /// </typeparam>
    /// <remarks>
    /// <para>The paged list is obtained from queries that have more results than can fit a message. 
    /// The next results can be obtained by executing the same query with the previous continuation token.</para>
    /// </remarks>
    public abstract class PagedList<T> : IList<T>
    {
        private IList<T> list;

        /// <summary>
        /// <para>
        /// Instantiates an empty PagedList class.
        /// </para>
        /// </summary>
        protected PagedList() : this(new List<T>())
        {
        }

        /// <summary>
        /// <para>
        /// Instantiates a PagedList class with the items of another list.
        /// </para>
        /// </summary>
        /// <param name="list">
        /// <para>The list with items used to create the paged list.</para>
        /// </param>
        protected PagedList(IList<T> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>
        /// Gets the index in this list for the specified item.
        /// </para>
        /// </summary>
        /// <param name="item">
        /// <para>The item.</para>
        /// </param>
        /// <returns>
        /// <para>The index in this list for the specified item. </para>
        /// </returns>
        public int IndexOf(T item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>
        /// Inserts an item into this list at the specified index.
        /// </para>
        /// </summary>
        /// <param name="index">
        /// <para>The index where the item will be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The item to insert.</para>
        /// </param>
        public void Insert(int index, T item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// <para>
        /// Removes the item at the specified index from this list. 
        /// </para>
        /// </summary>
        /// <param name="index">
        /// <para>The index where the item will be removed.</para>
        /// </param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// <para>
        /// Gets the item at the specified index.
        /// </para>
        /// </summary>
        /// <param name="index">
        /// <para>The index.</para>
        /// </param>
        /// <returns>
        /// <para>The item at the specified index.</para>
        /// </returns>
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
        /// <para>
        /// The continuation token. Can be used by queries to get next pages of results.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Gets or sets the continuation token.</para>
        /// </value>
        public string ContinuationToken
        {
            get;
            set;
        }

        /// <summary>
        /// <para>
        /// Adds an item to this list.
        /// </para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to add to the list.</para>
        /// </param>
        public void Add(T item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>
        /// Removes all items from this list.
        /// </para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>
        /// Specifies whether the list contains a specific item.
        /// </para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to search.</para>
        /// </param>
        /// <returns>
        /// <para>true if the list contains a specific item; otherwise, false.</para>
        /// </returns>
        public bool Contains(T item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>
        /// Copies items from this list to the specified array starting at the specified index.
        /// </para>
        /// </summary>
        /// <param name="array">
        /// <para>The array.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The array index.</para>
        /// </param>
        public void CopyTo(T[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>
        /// Gets the number of items in the list.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The number of items in the list.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>
        /// Gets a flag that indicated whether the list can be modified.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>Flag indicating whether the list can be modified.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>
        /// Removes the specified item from this list. 
        /// </para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to be removed.</para>
        /// </param>
        /// <returns>
        /// <para>true if the item is removed; otherwise, false.</para>
        /// </returns>
        public bool Remove(T item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>
        /// Gets an enumerator to items in this list. 
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>The enumerator to items in this list. </para>
        /// </returns>
        public IEnumerator<T> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>
        /// Gets an enumerator to items in this list. 
        /// </para>
        /// </summary>
        /// <returns>
        /// <para>The enumerator to items in this list. </para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }
    }

    /// <summary>
    /// <para>
    /// Extensions for paged list operations.
    /// </para>
    /// </summary>
    public static class PagedListHelper
    {
        /// <summary>
        /// <para>
        /// Returns an array with the items in the paged list.
        /// </para>
        /// </summary>
        /// <param name="list">
        /// <para>The paged list.</para>
        /// </param>
        /// <typeparam name="T">
        /// <para>The type of the list objects.</para>
        /// </typeparam>
        /// <returns>
        /// <para>The array.</para>
        /// </returns>
        public static T[] ToArray<T>(this PagedList<T> list)
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