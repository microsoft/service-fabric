// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ApplicationNameQueryDescription
    {
        public ApplicationNameQueryDescription(Uri serviceName)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            this.ServiceName = serviceName;
        }

        public Uri ServiceName { get; private set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_NAME_QUERY_DESCRIPTION();

            nativeDescription.ServiceName = pinCollection.AddObject(this.ServiceName);
            nativeDescription.Reserved = IntPtr.Zero;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}