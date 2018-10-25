// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System.Fabric.Interop;

    internal enum PrincipalIdentifierKind
    {
        Invalid = NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_INVALID,
        X509 = NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_X509,
        Windows = NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_WINDOWS,
        Claim = NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_CLAIM,
        Role = NativeTypes.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND.FABRIC_SECURITY_PRINCIPAL_IDENTIFIER_KIND_ROLE
    }
}