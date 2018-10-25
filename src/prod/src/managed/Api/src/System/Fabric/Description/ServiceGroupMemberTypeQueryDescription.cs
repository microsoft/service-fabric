// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ServiceGroupMemberTypeQueryDescription
    {
        public ServiceGroupMemberTypeQueryDescription()
            : this(null, null, null)
        {
        }

        public ServiceGroupMemberTypeQueryDescription(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter)
        {
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ServiceGroupTypeNameFilter = serviceGroupTypeNameFilter;
        }

        public string ApplicationTypeName { get; set; }
        public string ApplicationTypeVersion { get; set; }
        public string ServiceGroupTypeNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationTypeName = pinCollection.AddObject(ApplicationTypeName);
            nativeDescription.ApplicationTypeVersion = pinCollection.AddObject(ApplicationTypeVersion);
            nativeDescription.ServiceGroupTypeNameFilter = pinCollection.AddObject(ServiceGroupTypeNameFilter);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}