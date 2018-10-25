// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class DeployedServiceTypeQueryDescription
    {
        public DeployedServiceTypeQueryDescription()
            : this(null, null, null, null)
        {
        }

        public DeployedServiceTypeQueryDescription(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter)
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestNameFilter = serviceManifestNameFilter;
            this.ServiceTypeNameFilter = codePackageNameFilter;
        }

        public string NodeName { get; set; }
        public Uri ApplicationName { get; set; }
        public string ServiceManifestNameFilter { get; set; }
        public string ServiceTypeNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDescription.ServiceManifestNameFilter = pinCollection.AddObject(this.ServiceManifestNameFilter);
            nativeDescription.ServiceManifestNameFilter = pinCollection.AddObject(this.ServiceManifestNameFilter);
            nativeDescription.ServiceTypeNameFilter = pinCollection.AddObject(this.ServiceTypeNameFilter);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}