// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class FabricImageStorePathResourceIdentifier : ResourceIdentifier
    {
        public FabricImageStorePathResourceIdentifier(string path)
            : base(ResourceIdentifierKind.PathInFabricImageStore)
        {
            Requires.Argument<string>("path", path).NotNullOrEmpty();
            this.Path = path;
        }

        public string Path
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeImageStorePathResourceIdentifier = new NativeTypes.FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER();
            nativeImageStorePathResourceIdentifier.Path = pinCollection.AddObject(this.Path);

            return pinCollection.AddBlittable(nativeImageStorePathResourceIdentifier);
        }

        internal static unsafe FabricImageStorePathResourceIdentifier FromNative(
            NativeTypes.FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER* nativePtr)
        {
            var path = NativeTypes.FromNativeString(nativePtr->Path);
            return new FabricImageStorePathResourceIdentifier(path);
        }
    }
}