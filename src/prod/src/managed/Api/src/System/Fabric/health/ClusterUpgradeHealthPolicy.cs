// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Defines a health policy used to evaluate the health of the cluster specific to cluster upgrade.</para>
    /// </summary>
    /// <remarks>
    /// <para>It’s used together 
    /// with <see cref="System.Fabric.Health.ClusterHealthPolicy" /> to evaluate cluster health and determine whether the monitored 
    /// cluster upgrade is successful or needs to be rolled back.</para>
    /// </remarks>
    public class ClusterUpgradeHealthPolicy
    {
        internal static readonly ClusterUpgradeHealthPolicy Default = new ClusterUpgradeHealthPolicy();

        private byte maxPercentDeltaUnhealthyNodes;
        private byte maxPercentUpgradeDomainDeltaUnhealthyNodes;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ClusterUpgradeHealthPolicy" /> class.</para>
        /// </summary>
        public ClusterUpgradeHealthPolicy()
        {
            // TODO: establish correct default values - should be taken from native/configuration?
            this.maxPercentDeltaUnhealthyNodes = 10;
            this.maxPercentUpgradeDomainDeltaUnhealthyNodes = 15;
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of nodes health degradation allowed during cluster upgrades.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of delta health degradation. Allowed values are integer values from zero to 100.</para>
        /// </value>
        /// <remarks>The delta is 
        /// measured between the state of the nodes at the beginning of upgrade and the state of the nodes at the time of the health evaluation. 
        /// The check is performed after every upgrade domain upgrade completion to make sure the global state of the cluster is within tolerated 
        /// limits. The default value is 10%.</remarks>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        public byte MaxPercentDeltaUnhealthyNodes
        {
            get
            {
                return this.maxPercentDeltaUnhealthyNodes;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentDeltaUnhealthyNodes = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of upgrade domain nodes health degradation 
        /// allowed during cluster upgrades.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of upgrade domain delta health degradation. Allowed values are integer values from zero to 100.</para>
        /// </value>
        /// <remarks>The delta 
        /// is measured between the state of the upgrade domain nodes at the beginning of upgrade and the state of the upgrade domain nodes at the 
        /// time of the health evaluation. The check is performed after every upgrade domain upgrade completion for all completed upgrade domains 
        /// to make sure the state of the upgrade domains is within tolerated limits. The default value is 15%.</remarks>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        public byte MaxPercentUpgradeDomainDeltaUnhealthyNodes
        {
            get
            {
                return this.maxPercentUpgradeDomainDeltaUnhealthyNodes;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUpgradeDomainDeltaUnhealthyNodes = value;
            }
        }

        internal static bool AreEqual(ClusterUpgradeHealthPolicy current, ClusterUpgradeHealthPolicy other)
        {
            if ((current != null) && (other != null))
            {
                return (current.MaxPercentDeltaUnhealthyNodes == other.MaxPercentDeltaUnhealthyNodes) &&
                    (current.MaxPercentUpgradeDomainDeltaUnhealthyNodes == other.MaxPercentUpgradeDomainDeltaUnhealthyNodes);
            }
            else
            {
                return (current == null) && (other == null);
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeClusterUpgradeHealthPolicy = new NativeTypes.FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY();

            nativeClusterUpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes = this.MaxPercentDeltaUnhealthyNodes;
            nativeClusterUpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes = this.MaxPercentUpgradeDomainDeltaUnhealthyNodes;

            return pinCollection.AddBlittable(nativeClusterUpgradeHealthPolicy);
        }

        internal static unsafe ClusterUpgradeHealthPolicy FromNative(IntPtr nativeClusterUpgradeHealthPolicyPtr)
        {
            var managedClusterUpgradeHealthPolicy = new ClusterUpgradeHealthPolicy();

            if (nativeClusterUpgradeHealthPolicyPtr != IntPtr.Zero)
            {
                var nativeClusterUpgradeHealthPolicy = *(NativeTypes.FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY*)nativeClusterUpgradeHealthPolicyPtr;
                managedClusterUpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes = nativeClusterUpgradeHealthPolicy.MaxPercentDeltaUnhealthyNodes;
                managedClusterUpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes = nativeClusterUpgradeHealthPolicy.MaxPercentUpgradeDomainDeltaUnhealthyNodes;
            }

            return managedClusterUpgradeHealthPolicy;
        }
    }
}
