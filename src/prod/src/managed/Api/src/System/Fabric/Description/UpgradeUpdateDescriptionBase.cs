// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common.Serialization;

    /// <summary>
    /// <para>Represents the abstract base class for <see cref="System.Fabric.Description.ApplicationUpgradeUpdateDescription" /> and 
    /// <see cref="System.Fabric.Description.FabricUpgradeUpdateDescription" />.
    /// This class can be used to modify the upgrade parameters describing the behavior 
    /// of the application or cluster upgrades. Visit <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UpdateApplicationUpgradeAsync(System.Fabric.Description.ApplicationUpgradeUpdateDescription)" /> 
    /// and <see cref="System.Fabric.FabricClient.ClusterManagementClient.UpdateFabricUpgradeAsync(System.Fabric.Description.FabricUpgradeUpdateDescription)" /> to see the usage.</para>
    /// </summary>
    public abstract class UpgradeUpdateDescriptionBase
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.UpgradeUpdateDescriptionBase" /> class.</para>
        /// </summary>
        protected UpgradeUpdateDescriptionBase()
        {
        }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.RollingUpgradeMode" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.RollingUpgradeMode" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.RollingUpgradeMode, IsDefaultValueIgnored = true)]
        public RollingUpgradeMode? UpgradeMode { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradePolicyDescription.ForceRestart" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradePolicyDescription.ForceRestart" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public bool? ForceRestart { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradePolicyDescription.UpgradeReplicaSetCheckTimeout" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradePolicyDescription.UpgradeReplicaSetCheckTimeout" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ReplicaSetCheckTimeoutInMilliseconds, IsDefaultValueIgnored = true)]
        public TimeSpan? UpgradeReplicaSetCheckTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.UpgradeFailureAction" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.UpgradeFailureAction" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public UpgradeFailureAction? FailureAction { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckWaitDuration" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckWaitDuration" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.HealthCheckWaitDurationInMilliseconds)]
        public TimeSpan? HealthCheckWaitDuration { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckStableDuration" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckStableDuration" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.HealthCheckStableDurationInMilliseconds)]
        public TimeSpan? HealthCheckStableDuration { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckRetryTimeout" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckRetryTimeout" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.HealthCheckRetryTimeoutInMilliseconds)]
        public TimeSpan? HealthCheckRetryTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.UpgradeTimeout" />.</para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.UpgradeTimeout" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.UpgradeTimeoutInMilliseconds)]
        public TimeSpan? UpgradeTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.UpgradeDomainTimeout" />. </para>
        /// </summary>
        /// <value>
        /// <para>The new value of <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.UpgradeDomainTimeout" />.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.UpgradeDomainTimeoutInMilliseconds)]
        public TimeSpan? UpgradeDomainTimeout { get; set; }
    }
}