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
    /// The service group member list that contains service group members.
    /// </summary>
    public sealed class ServiceGroupMemberList : IList<ServiceGroupMember>
    {
        IList<ServiceGroupMember> list;

        internal ServiceGroupMemberList()
            : this(new List<ServiceGroupMember>())
        {
        }

        internal ServiceGroupMemberList(IList<ServiceGroupMember> list)
        {
            this.list = list;
        }

        /// <summary>
        /// Find the index of the service group member.
        /// </summary>
        /// <param name="item">The service group member to be searched.</param>
        /// <returns>The index of the service group member.</returns>
        public int IndexOf(ServiceGroupMember item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// Insert the service group member.
        /// </summary>
        /// <param name="index">The index to be inserted.</param>
        /// <param name="item">The service group member to be inserted.</param>
        public void Insert(int index, ServiceGroupMember item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// Remove the service group member.
        /// </summary>
        /// <param name="index">The index of service group member to be removed.</param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// Access the service group member.
        /// </summary>
        /// <param name="index">The index of the service group member.</param>
        /// <value>Index of the service group member.</value>
        public ServiceGroupMember this[int index]
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
        /// Add service group member.
        /// </summary>
        /// <param name="item">The service group member to be added.</param>
        public void Add(ServiceGroupMember item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// Clear the service group members.
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// Search if contains service group member.
        /// </summary>
        /// <param name="item">The service group member to be searched.</param>
        /// <returns>The result of the search.</returns>
        public bool Contains(ServiceGroupMember item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// Copy the service group members.
        /// </summary>
        /// <param name="array">The service group members to be copied.</param>
        /// <param name="arrayIndex">The index of the array to begin copy.</param>
        public void CopyTo(ServiceGroupMember[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// The count of the service group members.
        /// </summary>
        /// <value>The count of the service group members.</value>
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
        /// Remove the service group member.
        /// </summary>
        /// <param name="item">The service group member to be removed.</param>
        /// <returns>The result of the remove.</returns>
        public bool Remove(ServiceGroupMember item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// Get the enumerator of the service group members.
        /// </summary>
        /// <returns>The enumerator of the service group members.</returns>
        public IEnumerator<ServiceGroupMember> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ServiceGroupMemberList CreateFromNativeListResult(
            NativeClient.IFabricGetServiceGroupMemberListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST*)result.get_ServiceGroupMemberList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ServiceGroupMemberList CreateFromNativeList(
            NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new ServiceGroupMemberList();

            var nativeItemArray = (NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ServiceGroupMember.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}
