// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class ServiceResourceIdentifier : ResourceIdentifier
    {
        public ServiceResourceIdentifier()
            : this(null)
        {
        }

        public ServiceResourceIdentifier(Uri serviceName)
            : base(ResourceIdentifierKind.Service)
        {
            Requires.Argument<Uri>("serviceName", serviceName).NotNull();
            this.ServiceName = serviceName;
        }

        public Uri ServiceName
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeServiceResourceIdentifier = new NativeTypes.FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER();
            nativeServiceResourceIdentifier.ServiceName = pinCollection.AddObject(this.ServiceName);

            return pinCollection.AddBlittable(nativeServiceResourceIdentifier);
        }

        internal static unsafe ServiceResourceIdentifier FromNative(
            NativeTypes.FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER* nativePtr)
        {
            var serviceName = NativeTypes.FromNativeUri(nativePtr->ServiceName);
            return new ServiceResourceIdentifier(serviceName);
        }
    }
}