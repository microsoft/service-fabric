// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ApplicationLoadInformationQueryDescription
    {
        public ApplicationLoadInformationQueryDescription()
            : this(null)
        {
        }

        public ApplicationLoadInformationQueryDescription(string applicationName)
        {
            this.ApplicationName = applicationName;
        }

        public string ApplicationName { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_LOAD_INFORMATION_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}
