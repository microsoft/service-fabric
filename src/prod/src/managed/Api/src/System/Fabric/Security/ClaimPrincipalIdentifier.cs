// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class ClaimPrincipalIdentifier : PrincipalIdentifier
    {
        public ClaimPrincipalIdentifier(string claim)
            : base(PrincipalIdentifierKind.Claim)
        {
            Requires.Argument<string>("claim", claim).NotNullOrEmpty();
            this.Claim = claim;
        }

        public string Claim
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeClaimPrincipalIdentifier = new NativeTypes.FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER();
            nativeClaimPrincipalIdentifier.Claim = pinCollection.AddObject(this.Claim);

            return pinCollection.AddBlittable(nativeClaimPrincipalIdentifier);
        }

        internal static unsafe ClaimPrincipalIdentifier FromNative(NativeTypes.FABRIC_SECURITY_CLAIM_PRINCIPAL_IDENTIFIER* nativePtr)
        {
            var claim = NativeTypes.FromNativeString(nativePtr->Claim);
            return new ClaimPrincipalIdentifier(claim);
        }
    }
}