// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal abstract class PrincipalIdentifier
    {
        protected PrincipalIdentifier(PrincipalIdentifierKind kind)
        {
            this.Kind = kind;
        }

        public PrincipalIdentifierKind Kind
        {
            get;
            private set;
        }

        internal abstract IntPtr ToNativeValue(PinCollection pinCollection);

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativePrincipalIdentifier = new NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER();
            nativePrincipalIdentifier.Kind = (NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND)this.Kind;
            nativePrincipalIdentifier.Value = this.ToNativeValue(pinCollection);

            return pinCollection.AddBlittable(nativePrincipalIdentifier);
        }

        internal static unsafe PrincipalIdentifier FromNative(NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER* nativePtr)
        {
            PrincipalIdentifier principalIdentifier = null;

            switch (nativePtr->Kind)
            {
                case NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_X509:
                    {
                        principalIdentifier = X509PrincipalIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER*)nativePtr->Value);
                        break;
                    }
                case NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_ROLE:
                    {
                        principalIdentifier = RolePrincipalIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_ROLE_PRINCIPAL_IDENTIFIER*)nativePtr->Value);
                        break;
                    }
                case NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_CLAIM:
                    {
                        principalIdentifier = ClaimPrincipalIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER*)nativePtr->Value);
                        break;
                    }
                case NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_WINDOWS:
                    {
                        principalIdentifier = WindowsPrincipalIdentifier.FromNative(
                            (NativeTypes.FABRIC_SECURITY_WINDOWS_PRINCIPAL_IDENTIFIER*)nativePtr->Value);
                        break;
                    }
                default:
                    {
                        AppTrace.TraceSource.WriteError(
                            "PrincipalIdentifier.FromNative",
                            "Unknown FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND : {0}",
                            nativePtr->Kind);
                        throw new ArgumentOutOfRangeException("FABRIC_SECURITY_PRINCIPAL_IDENTIFIER.Kind");
                    }
            }

            return principalIdentifier;
        }
    }

}