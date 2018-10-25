// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the type of rolling upgrade when upgrading an application instance or cluster.</para>
    /// </summary>
    /// <remarks>
    /// <para>For more information, see Upgrade and Scale a Service Fabric Cluster and Upgrade an Application.</para>
    /// </remarks>
    public enum RollingUpgradeMode
    {
        /// <summary>
        /// <para>All Service Fabric enumerations have reserved Invalid flag.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_INVALID,
        
        /// <summary>
        /// <para>Specifies that the upgrade should be unmonitored automatic.</para>
        /// </summary>
        /// <remarks>
        /// <para>Using this mode,  automatically proceeds to upgrading the next upgrade domain until all upgrade domains are upgraded.</para>
        /// </remarks>
        UnmonitoredAuto = NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO,
        
        /// <summary>
        /// <para>Specifies that the upgrade should be unmonitored manual.</para>
        /// </summary>
        /// <remarks>
        /// <para>Using this mode, the administrator needs to instruct  to move to the next upgrade domain (using the 
        /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress)" /> or 
        /// <see cref="System.Fabric.FabricClient.ClusterManagementClient.MoveNextFabricUpgradeDomainAsync(System.Fabric.FabricUpgradeProgress)" /> methods) and continue 
        /// the upgrade.</para>
        /// </remarks>
        UnmonitoredManual = NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL,
        
        /// <summary>
        /// <para>Specifies that the upgrade should be monitored automatic.</para>
        /// </summary>
        /// <remarks>
        /// <para>After an upgrade domain (UD) is upgraded, Service Fabric will proceed to upgrade the next UD if the health of the UD and the cluster meets the 
        /// defined health policies for that upgrade. Otherwise, the entire upgrade is marked as failed and the upgrade failure action is taken. The default failure 
        /// action is to roll-back the entire upgrade.</para>
        /// </remarks>
        Monitored = NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED
    }
}