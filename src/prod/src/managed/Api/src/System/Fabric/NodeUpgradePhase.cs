// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the upgrade phase of the corresponding node.</para>
    /// </summary>
    public enum NodeUpgradePhase
    {
        /// <summary>
        /// <para>Indicates that the node upgrade phase is invalid. This value is not used.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_NODE_UPGRADE_PHASE.FABRIC_NODE_UPGRADE_PHASE_INVALID,
        /// <summary>
        /// <para>Indicates that upgrade has not started on the node and Service Fabric is performing pre-upgrade safety checks.</para>
        /// </summary>
        PreUpgradeSafetyCheck = NativeTypes.FABRIC_NODE_UPGRADE_PHASE.FABRIC_NODE_UPGRADE_PHASE_PRE_UPGRADE_SAFETY_CHECK,
        /// <summary>
        /// <para>Indicates that the node is in the process of getting upgraded.</para>
        /// </summary>
        Upgrading = NativeTypes.FABRIC_NODE_UPGRADE_PHASE.FABRIC_NODE_UPGRADE_PHASE_UPGRADING,
        /// <summary>
        /// <para>Indicates that upgrade is complete on the node and Service Fabric is performing post-upgrade safety checks.</para>
        /// </summary>
        PostUpgradeSafetyCheck = NativeTypes.FABRIC_NODE_UPGRADE_PHASE.FABRIC_NODE_UPGRADE_PHASE_POST_UPGRADE_SAFETY_CHECK
    }
}