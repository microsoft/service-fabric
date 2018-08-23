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
    /// <para>Represents a list of provisioned Service Fabric code versions retrieved by calling 
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetProvisionedFabricConfigVersionListAsync()" />.</para>
    /// </summary>
    public sealed class ProvisionedFabricCodeVersionList : IList<ProvisionedFabricCodeVersion>
    {
        IList<ProvisionedFabricCodeVersion> list;

        internal ProvisionedFabricCodeVersionList()
            : this(new List<ProvisionedFabricCodeVersion>())
        {
        }

        internal ProvisionedFabricCodeVersionList(IList<ProvisionedFabricCodeVersion> list)
        {
            this.list = list;
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
        public int IndexOf(ProvisionedFabricCodeVersion item)
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
        public void Insert(int index, ProvisionedFabricCodeVersion item)
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
        public ProvisionedFabricCodeVersion this[int index]
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
        public void Add(ProvisionedFabricCodeVersion item)
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
        public bool Contains(ProvisionedFabricCodeVersion item)
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
        public void CopyTo(ProvisionedFabricCodeVersion[] array, int arrayIndex)
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
        public bool Remove(ProvisionedFabricCodeVersion item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Gets an enumerator to items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>The enumerator that iterates through the list.</para>
        /// </returns>
        public IEnumerator<ProvisionedFabricCodeVersion> GetEnumerator()
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

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ProvisionedFabricCodeVersionList CreateFromNativeListResult(
            NativeClient.IFabricGetProvisionedCodeVersionListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST*)result.get_ProvisionedCodeVersionList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ProvisionedFabricCodeVersionList CreateFromNativeList(
            NativeTypes.FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new ProvisionedFabricCodeVersionList();

            var nativeItemArray = (NativeTypes.FABRIC_PROVISIONED_CODE_VERSION_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ProvisionedFabricCodeVersion.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}