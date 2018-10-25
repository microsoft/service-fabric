// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal abstract class ResourceIdentifier
    {
        protected ResourceIdentifier(ResourceIdentifierKind kind)
        {
            this.Kind = kind;
        }

        public ResourceIdentifierKind Kind
        {
            get;
            private set;
        }

        internal abstract IntPtr ToNativeValue(PinCollection pinCollection);

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeResourceIdentifier = new NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER();
            nativeResourceIdentifier.Kind = (NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND)this.Kind;
            nativeResourceIdentifier.Value = this.ToNativeValue(pinCollection);

            return pinCollection.AddBlittable(nativeResourceIdentifier);
        }

        internal static unsafe ResourceIdentifier FromNative(NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER* nativePtr)
        {
            ResourceIdentifier resourceIdentifier = null;

            switch (nativePtr->Kind)
            {
                case NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_PATH_IN_FABRIC_IMAGESTORE:
                    {
                        resourceIdentifier = FabricImageStorePathResourceIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_IMAGESTORE_PATH_RESOURCE_IDENTIFIER*)nativePtr->Value);
                        break;
                    }

                case NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION_TYPE:
                    {
                        resourceIdentifier = ApplicationTypeResourceIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_APPLICATION_TYPE_RESOURCE_IDENTIFIER*)nativePtr->Value);
                        break;
                    }

                case NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION:
                    {
                        resourceIdentifier = ApplicationResourceIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_APPLICATION_RESOURCE_IDENTIFIER*)nativePtr->Value);
                        break;
                    }

                case NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_SERVICE:
                    {
                        resourceIdentifier = ServiceResourceIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_SERVICE_RESOURCE_IDENTIFIER*)nativePtr->Value);
                        break;
                    }

                case NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_NAME:
                    {
                        resourceIdentifier = NameResourceIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_NAME_RESOURCE_IDENTIFIER*)nativePtr->Value);
                        break;
                    }

                default:
                    {
                        AppTrace.TraceSource.WriteError(
                           "ResourceIdentifier.FromNative",
                           "Unknown FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND : {0}",
                           nativePtr->Kind);
                        throw new ArgumentOutOfRangeException("FABRIC_SECURITY_RESOURCE_IDENTIFIER.Kind");
                    }
            }

            return resourceIdentifier;
        }
    }
}