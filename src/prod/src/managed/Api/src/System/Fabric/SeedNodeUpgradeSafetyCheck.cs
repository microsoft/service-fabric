// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>The safety check that is currently being performed for seed node.during upgrade </para>
    /// </summary>
    public sealed class SeedNodeUpgradeSafetyCheck : UpgradeSafetyCheck
    {
        internal SeedNodeUpgradeSafetyCheck(UpgradeSafetyCheckKind kind)
            : base(kind)
        {
        }

        internal static unsafe SeedNodeUpgradeSafetyCheck FromNative(
            UpgradeSafetyCheckKind kind,
            NativeTypes.FABRIC_UPGRADE_SEED_NODE_SAFETY_CHECK* nativePtr)
        {
            return new SeedNodeUpgradeSafetyCheck(kind);
        }
    }
}