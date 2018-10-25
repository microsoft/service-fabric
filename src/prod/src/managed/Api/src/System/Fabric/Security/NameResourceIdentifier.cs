// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class NameResourceIdentifier : ResourceIdentifier
    {
        public NameResourceIdentifier(Uri name)
            : base(ResourceIdentifierKind.Name)
        {
            Requires.Argument<Uri>("name", name).NotNull();
            this.Name = name;
        }

        public Uri Name
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(Interop.PinCollection pinCollection)
        {
            var nativeApplicationResourceIdentifier = new NativeTypes.FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER();
            nativeApplicationResourceIdentifier.Name = pinCollection.AddObject(this.Name);

            return pinCollection.AddBlittable(nativeApplicationResourceIdentifier);
        }

        internal static unsafe NameResourceIdentifier FromNative(
            NativeTypes.FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER* nativePtr)
        {
            var name = NativeTypes.FromNativeUri(nativePtr->Name);
            return new NameResourceIdentifier(name);
        }
    }
}