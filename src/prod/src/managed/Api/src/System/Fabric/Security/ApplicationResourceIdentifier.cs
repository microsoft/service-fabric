// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Common;

    internal class ApplicationResourceIdentifier : ResourceIdentifier
    {
        public ApplicationResourceIdentifier(Uri applicationName)
            : base(ResourceIdentifierKind.Application)
        {
            Requires.Argument<Uri>("applicationName", applicationName).NotNull();
            this.ApplicationName = applicationName;
        }

        public Uri ApplicationName
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(Interop.PinCollection pinCollection)
        {
            var nativeApplicationResourceIdentifier = new NativeTypes.FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER();
            nativeApplicationResourceIdentifier.ApplicationName = pinCollection.AddObject(this.ApplicationName);

            return pinCollection.AddBlittable(nativeApplicationResourceIdentifier);
        }

        internal static unsafe ApplicationResourceIdentifier FromNative(
            NativeTypes.FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER* nativePtr)
        {
            var applicationName = NativeTypes.FromNativeUri(nativePtr->ApplicationName);
            return new ApplicationResourceIdentifier(applicationName);
        }
    }
}