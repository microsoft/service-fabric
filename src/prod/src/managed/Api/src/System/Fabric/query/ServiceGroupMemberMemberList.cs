// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// The service group member member list that contains service group member members.
    /// </summary>
    public sealed class ServiceGroupMemberMemberList : IList<ServiceGroupMemberMember>
    {
        IList<ServiceGroupMemberMember> list;

        internal ServiceGroupMemberMemberList()
            : this(new List<ServiceGroupMemberMember>())
        {
        }

        internal ServiceGroupMemberMemberList(IList<ServiceGroupMemberMember> list)
        {
            this.list = list;
        }

        /// <summary>
        /// Search for the index of the service group member member.
        /// </summary>
        /// <param name="item">The service group member member to be searched.</param>
        /// <returns>The index of the service group member member.</returns>
        public int IndexOf(ServiceGroupMemberMember item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// Insert the service group member member.
        /// </summary>
        /// <param name="index">The index of the service group member member to insert.</param>
        /// <param name="item">The service group member member to be inserted.</param>
        public void Insert(int index, ServiceGroupMemberMember item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// Remove the service group member member.
        /// </summary>
        /// <param name="index">The index of service group member member to be removed.</param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// Access the service group member member.
        /// </summary>
        /// <param name="index">The index of the service group member member to access.</param>
        /// <value>Index of the service group member member.</value>
        public ServiceGroupMemberMember this[int index]
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
        /// Add service group member member.
        /// </summary>
        /// <param name="item">The service group member member to be added.</param>
        public void Add(ServiceGroupMemberMember item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// Clear the service group member members.
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// Search if contains the service group member member.
        /// </summary>
        /// <param name="item">The service group member member to be searched.</param>
        /// <returns>The result of the search.</returns>
        public bool Contains(ServiceGroupMemberMember item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// Copy to the service group member member.
        /// </summary>
        /// <param name="array">The service group member member to be copied.</param>
        /// <param name="arrayIndex">The index to begin copy.</param>
        public void CopyTo(ServiceGroupMemberMember[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// The count of the service group member members.
        /// </summary>
        /// <value>The count of the service group member members.</value>
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
        /// Remove the service group member member.
        /// </summary>
        /// <param name="item">The service group member member to be removed.</param>
        /// <returns>The result of the remove.</returns>
        public bool Remove(ServiceGroupMemberMember item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// Get the enumerator of the service group member members.
        /// </summary>
        /// <returns>The enumerator of the service group member members.</returns>
        public IEnumerator<ServiceGroupMemberMember> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        internal static unsafe ServiceGroupMemberMemberList CreateFromNativeList(
            NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new ServiceGroupMemberMemberList();

            var nativeItemArray = (NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = ServiceGroupMemberMember.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}