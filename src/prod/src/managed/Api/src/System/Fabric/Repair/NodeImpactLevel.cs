// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the effective impact that a repair is expected to have on a particular node.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    public enum NodeImpactLevel
    {
        /// <summary>
        /// <para>Indicates that the node impact level is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LEVEL.FABRIC_REPAIR_NODE_IMPACT_LEVEL_INVALID,

        /// <summary>
        /// <para>Indicates that no impact is expected.</para>
        /// </summary>
        None = NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LEVEL.FABRIC_REPAIR_NODE_IMPACT_LEVEL_NONE,

        /// <summary>
        /// <para>Indicates that the node is expected to stop and eventually restart.</para>
        /// </summary>
        Restart = NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LEVEL.FABRIC_REPAIR_NODE_IMPACT_LEVEL_RESTART,

        /// <summary>
        /// <para>Indicates that the node is expected to stop, and may lose all of its persisted state prior to restarting.</para>
        /// </summary>
        RemoveData = NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LEVEL.FABRIC_REPAIR_NODE_IMPACT_LEVEL_REMOVE_DATA,

        /// <summary>
        /// <para>Indicates that the node is to be removed from the cluster.</para>
        /// </summary>
        RemoveNode = NativeTypes.FABRIC_REPAIR_NODE_IMPACT_LEVEL.FABRIC_REPAIR_NODE_IMPACT_LEVEL_REMOVE_NODE,
    }
}