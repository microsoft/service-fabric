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
    /// <para>Represents a list of <see cref="System.Fabric.Query.DeployedServicePackage" />.</para>
    /// </summary>
    public sealed class DeployedServicePackageList : IList<DeployedServicePackage>
    {
        IList<DeployedServicePackage> list;

        internal DeployedServicePackageList()
            : this(new List<DeployedServicePackage>())
        {
        }

        internal DeployedServicePackageList(IList<DeployedServicePackage> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Gets the index of the specified item in this list</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item.</para>
        /// </param>
        /// <returns>
        /// <para>The index of the specified item in this list.</para>
        /// </returns>
        public int IndexOf(DeployedServicePackage item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index at which <paramref name="item" /> should be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The object to insert.</para>
        /// </param>
        public void Insert(int index, DeployedServicePackage item)
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
        /// <para>Gets the item at the specified index</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index of the item to get.</para>
        /// </param>
        /// <returns>
        /// <para>The item at the specified index.</para>
        /// </returns>
        public DeployedServicePackage this[int index]
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
        /// <para>The item to add.</para>
        /// </param>
        public void Add(DeployedServicePackage item)
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
        /// <para>Indicates whether the specified item is in the list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to search.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the specified item is in the list; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(DeployedServicePackage item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies items from the list to the specified array at the specified starting index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The destination of the items copied from the list.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The zero-based index in <paramref name="array" /> at which copying begins.</para>
        /// </param>
        public void CopyTo(DeployedServicePackage[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets the number of items in this list</para>
        /// </summary>
        /// <value>
        /// <para>The number of items in this list.</para>
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
        ///     <languageKeyword>true</languageKeyword> if the list is read-only; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>Removes the specified item from this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to remove from the list.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if successfully removed; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Remove(DeployedServicePackage item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Gets an enumerator to items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator to items in this list.</para>
        /// </returns>
        public IEnumerator<DeployedServicePackage> GetEnumerator()
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
        internal static unsafe DeployedServicePackageList CreateFromNativeListResult(
            NativeClient.IFabricGetDeployedServicePackageListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST*)result.get_DeployedServicePackageList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe DeployedServicePackageList CreateFromNativeList(
            NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new DeployedServicePackageList();

            var nativeItemArray = (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(DeployedServicePackage.CreateFromNative(nativeItem));
            }

            return retval;
        }
    }
}