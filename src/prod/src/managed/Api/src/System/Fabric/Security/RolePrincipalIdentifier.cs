// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class RolePrincipalIdentifier : PrincipalIdentifier
    {
        public RolePrincipalIdentifier(string roleName)
            : base(PrincipalIdentifierKind.Role)
        {
            Requires.Argument<string>("roleName", roleName).NotNullOrEmpty();
            this.RoleName = roleName;
        }

        public string RoleName
        {
            get;
            private set;
        }

        internal override IntPtr ToNativeValue(PinCollection pinCollection)
        {
            var nativeRolePrincipalIdentifier = new NativeTypes.FABRIC_SECURITY_ROLE_PRINCIPAL_IDENTIFIER();
            nativeRolePrincipalIdentifier.RoleName = pinCollection.AddObject(this.RoleName);

            return pinCollection.AddBlittable(nativeRolePrincipalIdentifier);
        }

        internal static unsafe RolePrincipalIdentifier FromNative(NativeTypes.FABRIC_SECURITY_ROLE_PRINCIPAL_IDENTIFIER* nativePtr)
        {
            var roleName = NativeTypes.FromNativeString(nativePtr->RoleName);
            return new RolePrincipalIdentifier(roleName);
        }
    }
}