// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the state of the application upgrade.</para>
    /// </summary>
    public enum ApplicationUpgradeState
    {
        /// <summary>
        /// <para>Indicates that the type is not valid. All Service Fabric enumerations have an invalid type.</para>
        /// </summary>
        Invalid                     = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.FABRIC_APPLICATION_UPGRADE_STATE_INVALID,

        /// <summary>
        /// <para>Indicates that the upgrade is in the process of rolling back. This indicates that upgrade to the target version has failed. Note that this state can also be observed temporarily if the upgrade is interrupted to start a new upgrade.</para>
        /// </summary>
        RollingBackInProgress       = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLBACK_IN_PROGRESS,
        
        /// <summary>
        /// <para>Indicates that the rollback of the upgrade is completed. The completed rollback indicates that upgrade to the target version has failed.</para>
        /// </summary>
        RollingBackCompleted        = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLBACK_COMPLETED,
        
        /// <summary>
        /// <para>Indicates that the upgrade to the target application type version is pending client input. <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress)" /> is used to move the upgrade forward.</para>
        /// </summary>
        RollingForwardPending       = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLFORWARD_PENDING,
        
        /// <summary>
        /// <para>Indicates that the upgrade to the target application type version is in progress.</para>
        /// </summary>
        RollingForwardInProgress    = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLFORWARD_IN_PROGRESS,
        
        /// <summary>
        /// <para>Indicates that the upgrade to the target application type version is completed.</para>
        /// </summary>
        RollingForwardCompleted     = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLFORWARD_COMPLETED,
        
        /// <summary>
        /// <para>Indicates that the upgrade has failed.</para>
        /// </summary>
        Failed                      = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_FAILED,

        /// <summary>
        /// <para>Indicates that the rollback of the upgrade is pending client input. <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress)" /> is used to continue the rollback.</para>
        /// </summary>
        RollingBackPending        = NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE.APPLICATION_UPGRADE_STATE_ROLLBACK_PENDING,
        
    }
}
