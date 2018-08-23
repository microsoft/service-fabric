// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the kind of the <see cref="System.Fabric.Repair.RepairTargetDescription" />.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    public enum RepairTargetKind
    {
        /// <summary>
        /// <para>The repair target is not valid or is of an unknown type.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_REPAIR_TARGET_KIND.FABRIC_REPAIR_TARGET_KIND_INVALID,
        /// <summary>
        /// <para>The repair target is a set of Service Fabric nodes.</para>
        /// </summary>
        Node = NativeTypes.FABRIC_REPAIR_TARGET_KIND.FABRIC_REPAIR_TARGET_KIND_NODE,
    }
}