// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class NodeLoadInformationQueryDescription
    {
        public NodeLoadInformationQueryDescription()
            : this(null)
        {
        }

        public NodeLoadInformationQueryDescription(string nodeName)
        {
            this.NodeName = nodeName;
        }

        public string NodeName { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_NODE_LOAD_INFORMATION_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}
