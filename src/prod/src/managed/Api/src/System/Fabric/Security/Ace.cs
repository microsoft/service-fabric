// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal abstract class Ace
    {
        protected Ace(AceType aceType, PrincipalIdentifier principal)
            : this(aceType, principal, AccessMask.None)
        {
        }

        protected Ace(AceType aceType, PrincipalIdentifier principal, AccessMask accessMask)
        {
            Requires.Argument<PrincipalIdentifier>("principal", principal).NotNull();

            this.AceType = aceType;
            this.Principal = principal;
            this.AccessMask = accessMask;
        }

        public AceType AceType
        {
            get;
            private set;
        }

        public PrincipalIdentifier Principal
        {
            get;
            private set;
        }

        public AccessMask AccessMask
        {
            get;
            set;
        }

        internal void ToNative(PinCollection pinCollection, out NativeTypes.FABRIC_SECURITY_ACE nativeAce)
        {
            nativeAce.AccessMask = (uint)this.AccessMask;
            nativeAce.AceType = (NativeTypes.FABRIC_SECURITY_ACE_TYPE)this.AceType;
            nativeAce.Principal = this.Principal.ToNative(pinCollection);
            nativeAce.Reserved = IntPtr.Zero;
        }

        internal static unsafe Ace FromNative(NativeTypes.FABRIC_SECURITY_ACE nativeAce)
        {
            var principal = PrincipalIdentifier.FromNative((NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER*)nativeAce.Principal);
            var accessMask = (AccessMask)nativeAce.AccessMask;
            var aceType = (AceType)nativeAce.AceType;

            if (aceType == AceType.Allowed)
            {
                return new AllowedAce(principal, accessMask);
            }
            else
            {
                AppTrace.TraceSource.WriteError(
                           "Ace.FromNative",
                           "Unknown FABRIC_SECURITY_ACE_TYPE : {0}",
                           nativeAce.AceType);
                throw new ArgumentOutOfRangeException("FABRIC_SECURITY_ACE.AceType");
            }
        }
    }
}