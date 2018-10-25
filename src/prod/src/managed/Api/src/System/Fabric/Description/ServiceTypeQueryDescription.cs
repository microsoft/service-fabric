// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ServiceTypeQueryDescription
    {
        public ServiceTypeQueryDescription()
            : this(null, null, null)
        {
        }

        public ServiceTypeQueryDescription(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter)
        {
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ServiceTypeNameFilter = serviceTypeNameFilter;
        }

        public string ApplicationTypeName { get; set; }
        public string ApplicationTypeVersion { get; set; }
        public string ServiceTypeNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_TYPE_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationTypeName = pinCollection.AddObject(ApplicationTypeName);
            nativeDescription.ApplicationTypeVersion = pinCollection.AddObject(ApplicationTypeVersion);
            nativeDescription.ServiceTypeNameFilter = pinCollection.AddObject(ServiceTypeNameFilter);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}