// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class ApplicationTypeResourceIdentifier : ResourceIdentifier
    {
        public ApplicationTypeResourceIdentifier(string applicationTypeName)
            : base(ResourceIdentifierKind.ApplicationType)
        {
            Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrEmpty();

            this.ApplicationTypeName = applicationTypeName;
        }

        public string ApplicationTypeName
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(Interop.PinCollection pinCollection)
        {
            var nativeApplicationTypeResourceIdentifier = new NativeTypes.FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER();
            nativeApplicationTypeResourceIdentifier.ApplicationTypeName = pinCollection.AddObject(this.ApplicationTypeName);

            return pinCollection.AddBlittable(nativeApplicationTypeResourceIdentifier);
        }

        internal static unsafe ApplicationTypeResourceIdentifier FromNative(
            NativeTypes.FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER* nativePtr)
        {
            var applicationTypeName = NativeTypes.FromNativeString(nativePtr->ApplicationTypeName);
            return new ApplicationTypeResourceIdentifier(applicationTypeName);
        }
    }
}