// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a list of <see cref="System.Fabric.Repair.RepairTask" /> objects.</para>
    /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
    /// </summary>
    public sealed class RepairTaskList : IList<RepairTask>
    {
        private IList<RepairTask> list;

        internal RepairTaskList()
            : this(new List<RepairTask>())
        {
        }

        internal RepairTaskList(IList<RepairTask> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Determines the index of a specific item in the <see cref="System.Fabric.Repair.RepairTaskList" />.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The object to locate in the list.</para>
        /// </param>
        /// <returns>
        /// <para>The index of item if found in the list; otherwise, -1.</para>
        /// </returns>
        public int IndexOf(RepairTask item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts an item to the <see cref="System.Fabric.Repair.RepairTaskList" /> at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index at which item should be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The object to insert into the list.</para>
        /// </param>
        public void Insert(int index, RepairTask item)
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
        /// <para>The element at the specified index.</para>
        /// </returns>
        public RepairTask this[int index]
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
        /// <para>Adds an item to the <see cref="System.Fabric.Repair.RepairTaskList" />.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The object to add to the list.</para>
        /// </param>
        public void Add(RepairTask item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>Removes all items from the <see cref="System.Fabric.Repair.RepairTaskList" />.</para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>Determines whether the <see cref="System.Fabric.Repair.RepairTaskList" /> contains a specific value.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The object to locate in the list.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the <see cref="System.Fabric.Repair.RepairTaskList" /> contains a 
        /// specific value; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(RepairTask item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies the elements of the <see cref="System.Fabric.Repair.RepairTaskList" /> to an Array, starting at a 
        /// particular Array index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The one-dimensional Array that is the destination of elements copied from 
        /// <see cref="System.Fabric.Repair.RepairTaskList" />. The Array must have zero-based indexing.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The zero-based index in array at which copying begins.</para>
        /// </param>
        public void CopyTo(RepairTask[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets the number of elements contained in the <see cref="System.Fabric.Repair.RepairTaskList" />.</para>
        /// </summary>
        /// <value>
        /// <para>The number of elements contained in the <see cref="System.Fabric.Repair.RepairTaskList" />.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets a value indicating whether the <see cref="System.Fabric.Repair.RepairTaskList" /> is read-only.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the <see cref="System.Fabric.Repair.RepairTaskList" /> is read-only; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>Removes the first occurrence of a specific object from the <see cref="System.Fabric.Repair.RepairTaskList" />.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The object to remove from the list.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item was successfully removed from the list; otherwise, 
        /// <languageKeyword>false</languageKeyword>. This method also returns false if item is not found in the original list.</para>
        /// </returns>
        public bool Remove(RepairTask item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through the collection.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator that can be used to iterate through the collection.</para>
        /// </returns>
        public IEnumerator<RepairTask> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Returns an enumerator that iterates through the collection.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator that can be used to iterate through the collection.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe RepairTaskList CreateFromNativeListResult(
            NativeClient.IFabricGetRepairTaskListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_REPAIR_TASK_LIST*)result.get_Tasks());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe RepairTaskList CreateFromNativeList(
            NativeTypes.FABRIC_REPAIR_TASK_LIST* nativeList)
        {
            var retval = new RepairTaskList();

            var nativeItemArray = (NativeTypes.FABRIC_REPAIR_TASK*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = nativeItemArray + i;
                retval.Add(RepairTask.FromNative(nativeItem));
            }

            return retval;
        }
    }
}