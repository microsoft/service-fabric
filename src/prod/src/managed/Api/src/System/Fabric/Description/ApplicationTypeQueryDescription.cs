// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ApplicationTypeQueryDescription
    {
        public ApplicationTypeQueryDescription()
            : this(null)
        {
        }

        public ApplicationTypeQueryDescription(string filter)
        {
            this.ApplicationTypeNameFilter = filter;
        }

        public string ApplicationTypeNameFilter { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION();

            nativeDescription.ApplicationTypeNameFilter = pinCollection.AddObject(this.ApplicationTypeNameFilter);
            nativeDescription.Reserved = IntPtr.Zero;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}