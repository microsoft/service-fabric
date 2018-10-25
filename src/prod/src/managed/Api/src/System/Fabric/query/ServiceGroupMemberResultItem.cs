// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>The type that stores the service group member query.</para>
    /// </summary>
    public class ServiceGroupMember
    {
        internal ServiceGroupMember(
            Uri serviceName,
            ServiceGroupMemberMemberList serviceGroupMemberMembers)
        {
            this.ServiceName = serviceName;
            this.ServiceGroupMemberMembers = serviceGroupMemberMembers;
        }

        private ServiceGroupMember() { }

        /// <summary>
        /// <para>The service name of the service group query.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Uri" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ServiceName { get; private set; }

        /// <summary>
        /// <para>The members of this service group member query.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Query.ServiceGroupMemberMemberList" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceGroupMemberDescription)]
        public ServiceGroupMemberMemberList ServiceGroupMemberMembers { get; private set; }

        internal static unsafe ServiceGroupMember CreateFromNative(
           NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_QUERY_RESULT_ITEM nativeResultItem)
        {
            NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_LIST* nativeServiceGroupMembmerQueryResult =
                (NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_LIST*)nativeResultItem.Members;

            ServiceGroupMemberMemberList serviceGroupMemberMemberList = ServiceGroupMemberMemberList.CreateFromNativeList(nativeServiceGroupMembmerQueryResult);

            ServiceGroupMember serviceGroupMember =
                new ServiceGroupMember(new Uri(NativeTypes.FromNativeString(nativeResultItem.ServiceName)), serviceGroupMemberMemberList);

            return serviceGroupMember;
        }
    }
}