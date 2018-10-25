// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class X509PrincipalIdentifier : PrincipalIdentifier
    {

        public X509PrincipalIdentifier(string commonName)
            : base(PrincipalIdentifierKind.X509)
        {
            Requires.Argument<string>("commonName", commonName).NotNullOrEmpty();
            this.CommonName = commonName;
        }
        
        public string CommonName 
        { 
            get; 
            private set; 
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeX509PrincipalIdentifier = new NativeTypes.FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER();
            nativeX509PrincipalIdentifier.CommonName = pinCollection.AddObject(this.CommonName);

            return pinCollection.AddBlittable(nativeX509PrincipalIdentifier);
        }

        internal static unsafe X509PrincipalIdentifier FromNative(NativeTypes.FABRIC_SECURITY_X509_PRINCIPAL_IDENTIFIER* nativePtr)
        {
            var commonName = NativeTypes.FromNativeString(nativePtr->CommonName);
            return new X509PrincipalIdentifier(commonName);
        }
    }
}