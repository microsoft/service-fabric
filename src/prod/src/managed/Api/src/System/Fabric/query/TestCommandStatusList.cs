// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;

    /// <summary>
    /// An <see cref="System.Collections.Generic.IList{T}" /> of TestCommandStatus objects.
    /// </summary>
    public sealed class TestCommandStatusList : IList<TestCommandStatus>
    {
        private IList<TestCommandStatus> list;

        internal TestCommandStatusList()
        {
            this.list = new List<TestCommandStatus>();
        }

        internal TestCommandStatusList(IList<TestCommandStatus> input)
        {
            this.list = input;
        }

        /// <summary>
        /// <para>Gets the index of the specified item in this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The specified item.</para>
        /// </param>
        /// <returns>
        /// <para>The index of the specified item in this list.</para>
        /// </returns>
        public int IndexOf(TestCommandStatus item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The location where the item will be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The item to insert.</para>
        /// </param>
        public void Insert(int index, TestCommandStatus item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// <para>Removes the item at the specified index</para>
        /// </summary>
        /// <param name="index">
        /// <para>The index of the item to remove.</para>
        /// </param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// <para>Gets the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The index of the item.</para>
        /// </param>
        /// <returns>
        /// <para>The item at the specified index.</para>
        /// </returns>
        public TestCommandStatus this[int index]
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
        /// <para>Adds the specified item to the list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to be added to the list.</para>
        /// </param>
        public void Add(TestCommandStatus item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>Removes all items from the list.</para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>Indicates whether the list contains a specified item.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to search.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the list contains a specified item; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(TestCommandStatus item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies items from the list to the specified array at the specified starting index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The array of items to copy.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The index where the array of items will be copied to.</para>
        /// </param>
        public void CopyTo(TestCommandStatus[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets or sets the number of items in this list.</para>
        /// </summary>
        /// <value>
        /// <para>The number of items in this list.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets or sets a flag that indicates whether the list is read only.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the list is read only; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>Remove the specified item from this list</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to remove.</para>
        /// </param>
        /// <returns>
        /// <para>The list with removed item.</para>
        /// </returns>
        public bool Remove(TestCommandStatus item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Gets an enumerator to items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>The enumerator that iterates through the list.</para>
        /// </returns>
        public IEnumerator<TestCommandStatus> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Gets an enumerator for items in this list</para>
        /// </summary>
        /// <returns>
        /// <para>The enumerator that iterates through the list.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }
    }
}
