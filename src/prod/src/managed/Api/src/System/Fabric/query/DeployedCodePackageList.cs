// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents this list that can be modified only if this property is false.</para>
    /// </summary>
    public sealed class DeployedCodePackageList : IList<DeployedCodePackage>
    {
        IList<DeployedCodePackage> list;

        internal DeployedCodePackageList()
            : this(new List<DeployedCodePackage>())
        {
        }

        internal DeployedCodePackageList(IList<DeployedCodePackage> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Gets the index of the specified item in this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The specified index of the item.</para>
        /// </param>
        /// <returns>
        /// <para>The index of the specified item in this list.</para>
        /// </returns>
        public int IndexOf(DeployedCodePackage item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index at which item should be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The object to insert into the list.</para>
        /// </param>
        public void Insert(int index, DeployedCodePackage item)
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
        /// <para>Gets the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The specified index of the item.</para>
        /// </param>
        /// <returns>
        /// <para>The deployed item at the specified index.</para>
        /// </returns>
        public DeployedCodePackage this[int index]
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
        /// <para>The item to be added.</para>
        /// </param>
        public void Add(DeployedCodePackage item)
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
        /// <para>Returns true if the specified item is in the list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The specified item that contained in the list.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the list contains the specified item; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(DeployedCodePackage item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies items from the list to the specified array at the specified starting index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The one-dimensional array.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The index in the destination array to start copying.</para>
        /// </param>
        public void CopyTo(DeployedCodePackage[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets or sets a value that indicates the number of items in this list.</para>
        /// </summary>
        /// <value>
        /// <para>The number of items in this list.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets or sets a value that indicates whether this list can be modified only if this property is false.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the list can only be modified; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>Removes the specified item from this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The object to remove from the list.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item was successfully removed from the list; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Remove(DeployedCodePackage item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Gets an enumerator to items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator to items in this list </para>
        /// </returns>
        public IEnumerator<DeployedCodePackage> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Gets an enumerator for items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator for items in this list.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe DeployedCodePackageList CreateFromNativeListResult(
            NativeClient.IFabricGetDeployedCodePackageListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_LIST*)result.get_DeployedCodePackageList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe DeployedCodePackageList CreateFromNativeList(
            NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new DeployedCodePackageList();

            var nativeItemArray = (NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(DeployedCodePackage.CreateFromNative(nativeItem));
            }

            return retval;
        }
    }
}