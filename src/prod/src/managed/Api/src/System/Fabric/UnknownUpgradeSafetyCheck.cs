// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>The upgrade safety check kind is unknown. This is most likely because the client is running an older version.</para>
    /// </summary>
    public sealed class UnknownUpgradeSafetyCheck : UpgradeSafetyCheck
    {
        internal UnknownUpgradeSafetyCheck(UpgradeSafetyCheckKind kind)
            : base(kind)
        {
        }
    }
}