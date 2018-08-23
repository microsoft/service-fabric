// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ServiceGroupMemberQueryDescription
    {
        public ServiceGroupMemberQueryDescription()
            : this(null, null)
        {
        }

        public ServiceGroupMemberQueryDescription(Uri applicationName, Uri serviceNameFilter)
        {
            this.ApplicationName = applicationName;
            this.ServiceNameFilter = serviceNameFilter;
        }

        public Uri ApplicationName { get; set; }
        public Uri ServiceNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_GROUP_MEMBER_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.ServiceNameFilter = pinCollection.AddObject(this.ServiceNameFilter);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}