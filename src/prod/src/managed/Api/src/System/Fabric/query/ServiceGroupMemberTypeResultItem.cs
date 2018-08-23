// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>The type that stores the type of service group member query result.</para>
    /// </summary>
    public class ServiceGroupMemberType
    {
        internal ServiceGroupMemberType(
            ICollection<ServiceGroupTypeMemberDescription> serviceGroupMemberTypeDescription,
            string serviceManifestVersion,
            string serviceManifestName)
        {
            this.ServiceGroupMemberTypeDescription = serviceGroupMemberTypeDescription ?? new List<ServiceGroupTypeMemberDescription>();
            this.ServiceManifestVersion = serviceManifestVersion;
            this.ServiceManifestName = serviceManifestName;
        }

        private ServiceGroupMemberType() { }

        /// <summary>
        /// <para>The type description of the service group.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Collections.Generic.ICollection{T}" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceGroupTypeMemberDescription)]
        public ICollection<ServiceGroupTypeMemberDescription> ServiceGroupMemberTypeDescription { get; private set; }

        /// <summary>
        /// <para>The service manifest version of the service group.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string ServiceManifestVersion { get; private set; }

        /// <summary>
        /// <para>The service manifest name of the service group.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string ServiceManifestName { get; private set; }

        internal static unsafe ServiceGroupMemberType CreateFromNative(
           NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM nativeResultItem)
        {
            NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST* nativeMemberList =
                (NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION_LIST*)nativeResultItem.ServiceGroupMemberTypeDescription;

            ICollection<ServiceGroupTypeMemberDescription> serviceGroupMemberTypeDescription = new Collection<ServiceGroupTypeMemberDescription>();
            for (int i = 0; i < nativeMemberList->Count; i++)
            {
                serviceGroupMemberTypeDescription.Add(
                    ServiceGroupTypeMemberDescription.CreateFromNative(nativeMemberList->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_GROUP_TYPE_MEMBER_DESCRIPTION))), true));
            }

            return new ServiceGroupMemberType(
                serviceGroupMemberTypeDescription,
                NativeTypes.FromNativeString(nativeResultItem.ServiceManifestVersion),
                NativeTypes.FromNativeString(nativeResultItem.ServiceManifestName));
        }
    }
}