// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// The safety check that is currently being performed for seed nodes.
    /// </para>
    /// </summary>
    public sealed class SeedNodeSafetyCheck : SafetyCheck
    {
        internal SeedNodeSafetyCheck(SafetyCheckKind kind)
            : base(kind)
        {
        }

        internal static unsafe SeedNodeSafetyCheck FromNative(
            SafetyCheckKind kind,
            NativeTypes.FABRIC_SEED_NODE_SAFETY_CHECK* nativePtr)
        {
            return new SeedNodeSafetyCheck(kind);
        }
    }
}