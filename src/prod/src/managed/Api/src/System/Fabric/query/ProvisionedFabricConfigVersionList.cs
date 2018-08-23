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
    /// <para>Represents the list of provisioned Service Fabric configuration (Cluster Manifest) versions retrieved by 
    /// calling <see cref="System.Fabric.FabricClient.QueryClient.GetProvisionedFabricConfigVersionListAsync(System.String)" />.</para>
    /// </summary>
    public sealed class ProvisionedFabricConfigVersionList : IList<ProvisionedFabricConfigVersion>
    {
        IList<ProvisionedFabricConfigVersion> list;

        internal ProvisionedFabricConfigVersionList()
            : this(new List<ProvisionedFabricConfigVersion>())
        {
        }

        internal ProvisionedFabricConfigVersionList(IList<ProvisionedFabricConfigVersion> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Returns the index of the specified item in this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The specified item to locate in the list.</para>
        /// </param>
        /// <returns>
        /// <para>The index of the specified item in this list.</para>
        /// </returns>
        public int IndexOf(ProvisionedFabricConfigVersion item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts the specified item into this list at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index at which item should be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The object to insert into the list.</para>
        /// </param>
        public void Insert(int index, ProvisionedFabricConfigVersion item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// <para>Removes the item from the specified index.</para>
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
        /// <para>The specified index.</para>
        /// </param>
        /// <returns>
        /// <para>The item at the specified index.</para>
        /// </returns>
        public ProvisionedFabricConfigVersion this[int index]
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
        /// <para>Adds the specified item to this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The specified item to add in the list.</para>
        /// </param>
        public void Add(ProvisionedFabricConfigVersion item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>Removes all items from this list.</para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>Returns true if the specified item is contained in this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The specified item that is contained in the list.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the specified item is contained in this list; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(ProvisionedFabricConfigVersion item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies items from this list to the specified array at the specified index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The one-dimensional array that is the destination of elements copied from <see cref="System.Fabric.Query.ProvisionedFabricConfigVersionList" />. 
        /// The array must have zero-based indexing.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The zero-based index in array at which copying begins.</para>
        /// </param>
        public void CopyTo(ProvisionedFabricConfigVersion[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets the number of items in this list.</para>
        /// </summary>
        /// <value>
        /// <para>The number of items in this list.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets a value that indicates whether the list can be modified only if this property is false.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the list can be modified only; otherwise, <languageKeyword>false</languageKeyword>.</para>
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
        public bool Remove(ProvisionedFabricConfigVersion item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Returns an enumerator to the items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator to the items in this list.</para>
        /// </returns>
        public IEnumerator<ProvisionedFabricConfigVersion> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Gets an enumerator to the items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator to the items in this list.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ProvisionedFabricConfigVersionList CreateFromNativeListResult(
            NativeClient.IFabricGetProvisionedConfigVersionListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST*)result.get_ProvisionedConfigVersionList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ProvisionedFabricConfigVersionList CreateFromNativeList(
            NativeTypes.FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new ProvisionedFabricConfigVersionList();

            var nativeItemArray = (NativeTypes.FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ProvisionedFabricConfigVersion.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}