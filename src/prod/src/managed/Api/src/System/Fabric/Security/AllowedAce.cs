// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    internal class AllowedAce : Ace
    {
        public AllowedAce(PrincipalIdentifier principal)
            : base(AceType.Allowed, principal)
        {
        }

        public AllowedAce(PrincipalIdentifier principal, AccessMask accessMask)
            : base(AceType.Allowed, principal, accessMask)
        {
        }
    }
}