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
    /// <para>Represents a list of service type.</para>
    /// </summary>
    public sealed class ServiceTypeList : IList<ServiceType>
    {
        IList<ServiceType> list;

        internal ServiceTypeList()
            : this(new List<ServiceType>())
        {
        }

        internal ServiceTypeList(IList<ServiceType> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Searches for the specified <see cref="System.Fabric.Query.ServiceType" />  and returns the zero-based index of the first 
        /// occurrence within the entire <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to search.</para>
        /// </param>
        /// <returns>
        /// <para>The zero-based index of the first occurrence within the entire <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </returns>
        public int IndexOf(ServiceType item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts an element into the <see cref="System.Fabric.Query.ServiceTypeList" />  at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The index.</para>
        /// </param>
        /// <param name="item">
        /// <para>The item to insert.</para>
        /// </param>
        public void Insert(int index, ServiceType item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// <para>Removes the element at the specified index of the <see cref="System.Fabric.Query.ServiceTypeList" />..</para>
        /// </summary>
        /// <param name="index">
        /// <para>The index where to remove the item.</para>
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
        public ServiceType this[int index]
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
        /// <para>Adds an object to the end of the <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to add.</para>
        /// </param>
        public void Add(ServiceType item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>Removes all elements from the <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>Determines whether an element is in the <see cref="System.Fabric.Query.ServiceTypeList" />..</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to search.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item is in the <see cref="System.Fabric.Query.ServiceTypeList" />; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(ServiceType item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies the entire <see cref="System.Fabric.Query.ServiceTypeList" /> to a compatible one-dimensional 
        /// <see cref="System.Fabric.Query.ServiceTypeList" />, starting at the specified index of the target array.</para>
        /// </summary>
        /// <param name="array">
        /// <para>An array of service type.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The array index.</para>
        /// </param>
        public void CopyTo(ServiceType[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets or sets the number of elements in the <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </summary>
        /// <value>
        /// <para>The number of elements in the <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets or sets whether <see cref="System.Fabric.Query.ServiceTypeList" /> is read-only.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the <see cref="System.Fabric.Query.ServiceTypeList" /> is read-only; 
        /// otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>Removes the first occurrence of a specific object from the <see cref="System.Fabric.Query.ServiceTypeList" />..</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to remove.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item exist; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Remove(ServiceType item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Returns an enumerator for the entire <see cref="System.Fabric.Query.ServiceTypeList" />..</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator for the entire <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </returns>
        public IEnumerator<ServiceType> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Returns the enumerator for the <see cref="System.Fabric.Query.ServiceTypeList" />..</para>
        /// </summary>
        /// <returns>
        /// <para>The numerator for the <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ServiceTypeList CreateFromNativeListResult(
            NativeClient.IFabricGetServiceTypeListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST*)result.get_ServiceTypeList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ServiceTypeList CreateFromNativeList(
            NativeTypes.FABRIC_SERVICE_TYPE_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new ServiceTypeList();

            var nativeItemArray = (NativeTypes.FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ServiceType.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}