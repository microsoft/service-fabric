// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// <para>
    /// The safety check that is not known to the current version of Service Fabric.
    /// </para>
    /// </summary>
    public sealed class UnknownSafetyCheck : SafetyCheck
    {
        internal UnknownSafetyCheck(SafetyCheckKind kind)
            : base(kind)
        {
        }
    }
}