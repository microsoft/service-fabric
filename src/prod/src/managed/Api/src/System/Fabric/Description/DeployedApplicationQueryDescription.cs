// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class DeployedApplicationQueryDescription
    {
        public DeployedApplicationQueryDescription()
            : this(null, null)
        {
        }

        public DeployedApplicationQueryDescription(string nodeName, Uri applicationNameFilter)
        {
            this.NodeName = nodeName;
            this.ApplicationNameFilter = applicationNameFilter;
        }

        public string NodeName { get; set; }
        public Uri ApplicationNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_DESCRIPTION();

            if (this.ApplicationNameFilter != null)
            {
                nativeDescription.ApplicationNameFilter = pinCollection.AddObject(this.ApplicationNameFilter);
            }

            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName); // Required parameter

            nativeDescription.Reserved = IntPtr.Zero;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}
