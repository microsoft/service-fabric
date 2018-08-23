// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the state of the compose deployment upgrade.</para>
    /// </summary>
    public enum ComposeDeploymentUpgradeState
    {
        /// <summary>
        /// <para>Indicates that the type is not valid. All Service Fabric enumerations have an invalid type.</para>
        /// </summary>
        Invalid                      = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_INVALID,

        /// <summary>
        /// <para>Indicates that the application type of the target compose deployment is being provisioned.</para>
        /// </summary>
        ProvisioningTarget           = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_PROVISIONING_TARGET,

        /// <summary>
        /// <para>Indicates that the upgrade to the target compose deployment is in progress.</para>
        /// </summary>
        RollingForwardInProgress     = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS,

        /// <summary>
        /// <para>Indicates that the upgrade to the target compose deployment is pending user input. The <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress)" /> is used to move the upgrade forward.</para>
        /// </summary>
        RollingForwardPending        = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_PENDING,

        /// <summary>
        /// <para>Indicates that the application type of the current compose deployment is being unprovisioned.</para>
        /// </summary>
        UnprovisioningCurrent = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_CURRENT,

        /// <summary>
        /// <para>Indicates that the upgrade to the target compose deployment is completed.</para>
        /// </summary>
        RollingForwardCompleted      = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED,

        /// <summary>
        /// <para>Indicates that the upgrade is in the process of rolling back. This indicates that upgrade to the target compose deployment has failed. Note that this state can also be observed temporarily if the upgrade is interrupted to start a new upgrade.</para>
        /// </summary>
        RollingBackInProgress        = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS,

        /// <summary>
        /// <para>Indicates that the application type of the target compose deployment is being unprovisioned. This state indicates the upgrade has failed and the upgrade is being rolled back.</para>
        /// </summary>
        UnprovisioningTarget         = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_TARGET,

        /// <summary>
        /// <para>Indicates that the rollback of the upgrade is completed. The completed rollback indicates that upgrade to the target version has failed.</para>
        /// </summary>
        RollingBackCompleted         = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_COMPLETED,

        /// <summary>
        /// <para>Indicates that the upgrade has failed.</para>
        /// </summary>
        Failed                       = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_FAILED,
            
        /// <summary>
        /// <para>Indicates that the rollback of the compose deployment is pending client input. <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress)" /> is used to continue the rollback.</para>
        /// </summary>
        RollingBackPending         = NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_PENDING,

    }
}
