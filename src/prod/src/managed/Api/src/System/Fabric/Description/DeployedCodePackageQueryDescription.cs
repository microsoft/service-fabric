// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class DeployedCodePackageQueryDescription
    {
        public DeployedCodePackageQueryDescription()
            : this(null, null, null, null)
        {
        }

        public DeployedCodePackageQueryDescription(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter)
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestNameFilter = serviceManifestNameFilter;
            this.CodePackageNameFilter = codePackageNameFilter;
        }

        public string NodeName { get; set; }
        public Uri ApplicationName { get; set; }
        public string ServiceManifestNameFilter { get; set; }
        public string CodePackageNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDescription.ServiceManifestNameFilter = pinCollection.AddObject(this.ServiceManifestNameFilter);
            nativeDescription.CodePackageNameFilter = pinCollection.AddObject(this.CodePackageNameFilter);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}