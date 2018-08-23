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
    /// The service group member type list that contains the service group member types.
    /// </summary>
    public sealed class ServiceGroupMemberTypeList : IList<ServiceGroupMemberType>
    {
        IList<ServiceGroupMemberType> list;

        internal ServiceGroupMemberTypeList()
            : this(new List<ServiceGroupMemberType>())
        {
        }

        internal ServiceGroupMemberTypeList(IList<ServiceGroupMemberType> list)
        {
            this.list = list;
        }

        /// <summary>
        /// Search for the index of the service group member type.
        /// </summary>
        /// <param name="item">The service group member type to be searched.</param>
        /// <returns>The index of the service group member type.</returns>
        public int IndexOf(ServiceGroupMemberType item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// Insert the service group member type.
        /// </summary>
        /// <param name="index">The index to be inserted.</param>
        /// <param name="item">The service group member type to be inserted.</param>
        public void Insert(int index, ServiceGroupMemberType item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// Remove service group member type.
        /// </summary>
        /// <param name="index">The index of service group member type to be removed.</param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// Access the service group member type.
        /// </summary>
        /// <param name="index">The index of service group member type to be accessed.</param>
        /// <value>Index of the service group member type.</value>
        public ServiceGroupMemberType this[int index]
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
        /// Add the service group member type.
        /// </summary>
        /// <param name="item">The service group member type to be added.</param>
        public void Add(ServiceGroupMemberType item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// Clear the service group member types.
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// Search if contains the service group member type.
        /// </summary>
        /// <param name="item">The service group member type to be searched.</param>
        /// <returns>The result of the search.</returns>
        public bool Contains(ServiceGroupMemberType item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// Copy the service group member type.
        /// </summary>
        /// <param name="array">The service group member type to be copied.</param>
        /// <param name="arrayIndex">The index to begin the copy.</param>
        public void CopyTo(ServiceGroupMemberType[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// The count of the service group member type.
        /// </summary>
        /// <value>The count of the service group member type.</value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// The flag of read only.
        /// </summary>
        /// <value>The flag of read only.</value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// Remove the service group member type.
        /// </summary>
        /// <param name="item">The service group member type to be removed.</param>
        /// <returns>The result of the remove.</returns>
        public bool Remove(ServiceGroupMemberType item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// Get the enumerator of the service group member types.
        /// </summary>
        /// <returns>The enumerator of the service group member types.</returns>
        public IEnumerator<ServiceGroupMemberType> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ServiceGroupMemberTypeList CreateFromNativeListResult(
            NativeClient.IFabricGetServiceGroupMemberTypeListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST*)result.get_ServiceGroupMemberTypeList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ServiceGroupMemberTypeList CreateFromNativeList(
            NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new ServiceGroupMemberTypeList();

            var nativeItemArray = (NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ServiceGroupMemberType.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}