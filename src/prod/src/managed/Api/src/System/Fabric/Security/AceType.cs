// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System.Fabric.Interop;

    internal enum AceType
    {
        Invalid = NativeTypes.FABRIC_SECURITY_ACE_TYPE.FABRIC_SECURITY_ACE_TYPE_INVALID,
        Allowed = NativeTypes.FABRIC_SECURITY_ACE_TYPE.FABRIC_SECURITY_ACE_TYPE_ALLOWED,
    }
}