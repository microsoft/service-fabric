// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the upgrade state when upgrading Service Fabric.</para>
    /// </summary>
    public enum FabricUpgradeState
    {
        /// <summary>
        /// <para>All Service Fabric enumerations have reserved Invalid flag.</para>
        /// </summary>
        Invalid                     = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_INVALID,
        /// <summary>
        /// <para>Specifies that the rolling back process is in progress.</para>
        /// </summary>
        RollingBackInProgress       = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLBACK_IN_PROGRESS,
        /// <summary>
        /// <para>Specifies that the rolling back process has completed.</para>
        /// </summary>
        RollingBackCompleted        = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLBACK_COMPLETED,
        /// <summary>
        /// <para>Specifies that the rolling forward process is pending client input.</para>
        /// </summary>
        RollingForwardPending       = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLFORWARD_PENDING,
        /// <summary>
        /// <para>Specifies that the rolling forward process is in progress.</para>
        /// </summary>
        RollingForwardInProgress    = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLFORWARD_IN_PROGRESS,
        /// <summary>
        /// <para>Specifies that the rolling forward process has completed.</para>
        /// </summary>
        RollingForwardCompleted     = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLFORWARD_COMPLETED,
        /// <summary>
        /// <para>Indicates that the upgrade has failed.</para>
        /// </summary>
        Failed                      = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_FAILED,
        /// <summary>
        /// <para>Specifies that the rolling back process has pending client input.</para>
        /// </summary>
        RollingBackPending        = NativeTypes.FABRIC_UPGRADE_STATE.FABRIC_UPGRADE_STATE_ROLLBACK_PENDING,
    }
}
