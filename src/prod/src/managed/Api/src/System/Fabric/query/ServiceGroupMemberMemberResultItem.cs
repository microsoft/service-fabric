// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>The type that contains a service group member.</para>
    /// </summary>
    public class ServiceGroupMemberMember
    {
        internal ServiceGroupMemberMember(
            Uri serviceName,
            string serviceTypeName)
        {
            this.ServiceName = serviceName;
            this.ServiceTypeName = serviceTypeName;
        }

        private ServiceGroupMemberMember() { }

        /// <summary>
        /// <para>The service name of a service group member.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Uri" />.</para>
        /// </value>
        public Uri ServiceName { get; private set; }

        /// <summary>
        /// <para>The type of a service group member.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string ServiceTypeName { get; private set; }

        internal static unsafe ServiceGroupMemberMember CreateFromNative(
           NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_MEMBER_QUERY_RESULT_ITEM nativeResultItem)
        {
            return new ServiceGroupMemberMember(
                new Uri(NativeTypes.FromNativeString(nativeResultItem.ServiceName)),
                NativeTypes.FromNativeString(nativeResultItem.ServiceTypeName));
        }
    }
}