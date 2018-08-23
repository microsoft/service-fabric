// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a collection of <see cref="System.Fabric.Health.ApplicationHealthState" /> 
    /// that can be individually accessed by index.</para>
    /// </summary>
    public sealed class ApplicationHealthStateList : IList<ApplicationHealthState>
    {
        IList<ApplicationHealthState> list;

        internal ApplicationHealthStateList()
            : this(new List<ApplicationHealthState>())
        {
        }

        internal ApplicationHealthStateList(IList<ApplicationHealthState> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Determines the index of a specific item in the collection.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to locate in the collection.</para>
        /// </param>
        /// <returns>
        /// <para>An <see cref="System.Int32" /> which represents the index of the item if found in the collection; otherwise, -1.</para>
        /// </returns>
        public int IndexOf(ApplicationHealthState item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts an item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index at which value should be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The item to be inserted.</para>
        /// </param>
        public void Insert(int index, ApplicationHealthState item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// <para>Removes the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index of the item to remove.</para>
        /// </param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// <para>Gets or sets the element at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index of the element to get or set.</para>
        /// </param>
        /// <returns>
        /// <para>The <see cref="System.Fabric.Health.ApplicationHealthState" /> at the specified index.</para>
        /// </returns>
        public ApplicationHealthState this[int index]
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
        /// <para>Adds an item to the collection.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to add.</para>
        /// </param>
        public void Add(ApplicationHealthState item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>Removes all items from the collection.</para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>Determines whether the collection contains a specific value.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to locate in the collection.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item is found; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(ApplicationHealthState item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies the elements of the ICollection to an Array, starting at a particular Array index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The one-dimensional Array that is the destination of the elements copied from ICollection. The Array must have zero-based indexing.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The zero-based index in array at which copying begins.</para>
        /// </param>
        public void CopyTo(ApplicationHealthState[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets the number of elements.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Int32" /> representing the number of elements.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets a value indicating whether the list is read-only.</para>
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
        /// <para>Removes the first occurrence of a specific item from the collection.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to be removed.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item was removed; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Remove(ApplicationHealthState item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through a collection.</para>
        /// </summary>
        /// <returns>
        /// <para>The <see cref="System.Collections.Generic.IEnumerator{T}" /> object that can be used to iterate through the collection.</para>
        /// </returns>
        public IEnumerator<ApplicationHealthState> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through a collection.</para>
        /// </summary>
        /// <returns>
        /// <para>An <see cref="System.Collections.IEnumerator" /> object that can be used to iterate through the collection.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        internal static unsafe ApplicationHealthStateList FromNativeList(
            NativeTypes.FABRIC_APPLICATION_HEALTH_STATE_LIST* nativeList)
        {
            var retval = new ApplicationHealthStateList();

            var nativeItemArray = (NativeTypes.FABRIC_APPLICATION_HEALTH_STATE*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ApplicationHealthState.FromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}