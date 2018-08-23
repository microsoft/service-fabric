// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents a class to encapsulate parameters describing a Service Fabric cluster configuration upgrade.</para>
    /// </summary>
    public sealed class ConfigurationUpgradeDescription
    {
        private byte maxPercentUnhealthyApplications;
        private byte maxPercentUnhealthyNodes;
        private byte maxPercentDeltaUnhealthyNodes;
        private byte maxPercentUpgradeDomainDeltaUnhealthyNodes;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ConfigurationUpgradeDescription" /> class.</para>
        /// </summary>
        public ConfigurationUpgradeDescription()
        {
            // copy the default value from StartUpgradeDescription.cpp
            this.HealthCheckWaitDuration = TimeSpan.Zero;
            this.HealthCheckStableDuration = TimeSpan.Zero;
            this.HealthCheckRetryTimeout = TimeSpan.FromSeconds(600);
            this.UpgradeTimeout = TimeSpan.MaxValue;
            this.UpgradeDomainTimeout = TimeSpan.MaxValue;

            this.maxPercentUnhealthyApplications = 0;
            this.maxPercentUnhealthyNodes = 0;
            this.maxPercentDeltaUnhealthyNodes = 0;
            this.maxPercentUpgradeDomainDeltaUnhealthyNodes = 0;

            // Create empty application health policies. 
            // Users can add items to it, but they can't replace it.
            this.ApplicationHealthPolicies = new ApplicationHealthPolicyMap();
        }

        /// <summary>
        /// This is the cluster configuration that will be applied to cluster.
        /// </summary>
        public string ClusterConfiguration
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets or sets the length of time between attempts to perform a health check if the application or cluster is not healthy.</para>
        /// </summary>
        /// <value>
        /// <para>The length of time between attempts to perform a health checks if the application or cluster is not healthy.</para>
        /// </value>
        /// <remarks>
        /// <para>To prevent a retry of the health check, set the <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckRetryTimeout" /> 
        /// property value to <see cref="System.TimeSpan.Zero" />. The default is <see cref="System.TimeSpan.Zero" />.</para>
        /// </remarks>
        public TimeSpan HealthCheckRetryTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time to wait after completing an upgrade domain before starting the health check process.</para>
        /// </summary>
        /// <value>
        /// <para>The length of time to wait after completing an upgrade domain before starting the health checks process.</para>
        /// </value>
        /// <remarks>
        /// <para>To use an infinite wait for the health check, set the <see cref="System.Fabric.Description.RollingUpgradeMonitoringPolicy.HealthCheckWaitDuration" /> 
        /// property value to <see cref="System.TimeSpan.Zero" />. The default is <see cref="System.TimeSpan.Zero" />.</para>
        /// </remarks>
        public TimeSpan HealthCheckWaitDuration { get; set; }

        /// <summary>
        /// <para>Gets or sets the length of time that the application or cluster must remain healthy before the health check passes and the upgrade proceeds 
        /// to the next Upgrade Domain.</para>
        /// </summary>
        /// <value>
        /// <para>The length of time that the application or cluster must remain healthy.</para>
        /// </value>
        public TimeSpan HealthCheckStableDuration { get; set; }

        /// <summary>
        /// <para>Gets or sets the timeout for the upgrade domain.</para>
        /// </summary>
        /// <value>
        /// <para>The timeout for the upgrade domain.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is TimeSpan.FromSeconds(uint.MaxValue).</para>
        /// </remarks>
        public TimeSpan UpgradeDomainTimeout { get; set; }

        /// <summary>
        /// <para>Gets or sets the upgrade timeout.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade timeout.</para>
        /// </value>
        /// <remarks>
        /// <para>The default is TimeSpan.FromSeconds(uint.MaxValue).</para>
        /// </remarks>
        public TimeSpan UpgradeTimeout { get; set; }

        /// <summary>
        /// <para>Gets the maximum allowed percentage of unhealthy applications from the <see cref="System.Fabric.Health.ClusterHealthPolicy" />.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy applications.</para>
        /// </value>
        public byte MaxPercentUnhealthyApplications
        {
            get
            {
                return this.maxPercentUnhealthyApplications;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyApplications = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the maximum allowed percentage of unhealthy nodes.</para>
        /// </summary>
        /// <value>
        /// <para>The maximum allowed percentage of unhealthy nodes. Allowed values are integer values from zero to 100.</para>
        /// </value>
        /// <exception cref="System.ArgumentOutOfRangeException">
        /// <para>The specified value was outside the range of integer values from zero to 100.</para>
        /// </exception>
        /// <remarks>
        /// <para>
        /// The percentage represents the maximum tolerated percentage of nodes that can be unhealthy 
        /// before the cluster is considered in error. If the percentage is respected but there is at least one unhealthy node,
        /// the health is evaluated as Warning.
        /// This is calculated by dividing the number of unhealthy nodes
        /// over the total number of nodes in the cluster.
        /// The computation rounds up to tolerate one failure on small numbers of nodes. Default percentage: zero.
        /// </para>
        /// <para>In large clusters, some nodes will always be down or out for repairs, so this percentage should be configured to tolerate that.</para>
        /// </remarks>
        public byte MaxPercentUnhealthyNodes
        {
            get
            {
                return this.maxPercentUnhealthyNodes;
            }

            set
            {
                Requires.CheckPercentageArgument(value, "value");
                this.maxPercentUnhealthyNodes = value;
            }
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

        /// <summary>
        /// Gets or sets the application health policies used to evaluate the applications health as part of
        /// the cluster health evaluation. 
        /// </summary>
        /// <value>The application health policies used to evaluate the health of the specified applications.</value>
        /// <remarks>
        /// <para>
        /// During cluster upgrade, the health of the cluster is evaluated to determine whether the cluster is still healthy. 
        /// As part of the cluster health evaluation, all applications are evaluated and aggregated in the cluster health.
        /// The application health policy map is used to evaluate the applications as part of the cluster evaluation.
        /// </para>
        /// <para>
        /// Each entry specifies as key the application name and as value an <see cref="System.Fabric.Health.ApplicationHealthPolicy"/> 
        /// used to evaluate the application health of that application.</para>
        /// <para>
        /// If an application is not specified in the map, the ApplicationHealthPolicy found in the application manifest will be used for evaluation. </para>
        /// <para>
        /// The custom application health policies are also used to evaluate cluster health during upgrade, through
        /// <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(System.Fabric.Description.ClusterHealthQueryDescription, System.TimeSpan, System.Threading.CancellationToken)"/> or
        /// <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthChunkAsync(System.Fabric.Description.ClusterHealthChunkQueryDescription, System.TimeSpan, System.Threading.CancellationToken)"/>.</para>
        /// <para>
        /// The map is empty by default.
        /// </para></remarks>
        public ApplicationHealthPolicyMap ApplicationHealthPolicies { get; internal set; }

        /// <summary>
        /// Gets a string representation of the ConfigurationUpgradeDescription.
        /// </summary>
        /// <returns>A string representation of the ConfigurationUpgradeDescription.</returns>
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "ClusterConfiguration: {0}",
                this.ClusterConfiguration);
        }
        
        internal string ToStringDescription()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "HealthCheckWaitDuration: {0}, HealthCheckStableDuration: {1}, HealthCheckRetryTimeout: {2}, UpgradeTimeout: {3}, UpgradeDomainTimeout: {4}, MaxPercentUnhealthyApplications: {5}, MaxPercentUnhealthyNodes: {6}, MaxPercentDeltaUnhealthyNodes: {7}, MaxPercentUpgradeDomainDeltaUnhealthyNodes: {8}, ApplicationHealthPolicies: {9}",
                this.HealthCheckWaitDuration,
                this.HealthCheckStableDuration,
                this.HealthCheckRetryTimeout,
                this.UpgradeTimeout,
                this.UpgradeDomainTimeout,
                this.MaxPercentUnhealthyApplications,
                this.MaxPercentUnhealthyNodes,
                this.MaxPercentDeltaUnhealthyNodes,
                this.MaxPercentUpgradeDomainDeltaUnhealthyNodes,
                this.ApplicationHealthPolicies);
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var ex1 = new NativeTypes.FABRIC_START_UPGRADE_DESCRIPTION_EX1
            {
                ApplicationHealthPolicyMap = this.ApplicationHealthPolicies.ToNative(pinCollection),
            };

            var nativeStartUpgradeDescription = new NativeTypes.FABRIC_START_UPGRADE_DESCRIPTION();
            nativeStartUpgradeDescription.ClusterConfig = pinCollection.AddObject(this.ClusterConfiguration);
            nativeStartUpgradeDescription.HealthCheckWaitDurationInSeconds = (uint)this.HealthCheckWaitDuration.TotalSeconds;
            nativeStartUpgradeDescription.HealthCheckRetryTimeoutInSeconds = (uint)this.HealthCheckRetryTimeout.TotalSeconds;
            nativeStartUpgradeDescription.HealthCheckStableDurationInSeconds = (uint)this.HealthCheckStableDuration.TotalSeconds;
            nativeStartUpgradeDescription.UpgradeTimeoutInSeconds = (uint)this.UpgradeTimeout.TotalSeconds;
            nativeStartUpgradeDescription.UpgradeDomainTimeoutInSeconds = (uint)this.UpgradeDomainTimeout.TotalSeconds;
            nativeStartUpgradeDescription.MaxPercentUnhealthyApplications = this.MaxPercentUnhealthyApplications;
            nativeStartUpgradeDescription.MaxPercentUnhealthyNodes = this.MaxPercentUnhealthyNodes;
            nativeStartUpgradeDescription.MaxPercentDeltaUnhealthyNodes = this.MaxPercentDeltaUnhealthyNodes;
            nativeStartUpgradeDescription.MaxPercentUpgradeDomainDeltaUnhealthyNodes = this.MaxPercentUpgradeDomainDeltaUnhealthyNodes;
            nativeStartUpgradeDescription.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeStartUpgradeDescription);
        }

        internal static unsafe ConfigurationUpgradeDescription CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_START_UPGRADE_DESCRIPTION native = *(NativeTypes.FABRIC_START_UPGRADE_DESCRIPTION*)nativeRaw;

            string ClusterConfiguration = NativeTypes.FromNativeString(native.ClusterConfig);
            var healthCheckRetryTimeoutSec = FromNativeTimeInSeconds(native.HealthCheckRetryTimeoutInSeconds);
            var healthCheckWaitDurationSec = FromNativeTimeInSeconds(native.HealthCheckWaitDurationInSeconds);
            var healthCheckStableDurationSec = FromNativeTimeInSeconds(native.HealthCheckStableDurationInSeconds);
            var upgradeDomainTimeoutSec = FromNativeTimeInSeconds(native.UpgradeDomainTimeoutInSeconds);
            var upgradeTimeoutSec = FromNativeTimeInSeconds(native.UpgradeTimeoutInSeconds);

            byte maxPercentUnhealthyApplications = native.MaxPercentUnhealthyApplications;
            byte maxPercentUnhealthyNodes = native.MaxPercentUnhealthyNodes;
            byte maxPercentDeltaUnhealthyNodes = native.MaxPercentDeltaUnhealthyNodes;
            byte maxPercentUpgradeDomainDeltaUnhealthyNodes = native.MaxPercentUpgradeDomainDeltaUnhealthyNodes;

            var configurationUpgradeDescription = new ConfigurationUpgradeDescription
            {
                ClusterConfiguration = ClusterConfiguration,
                HealthCheckRetryTimeout = healthCheckRetryTimeoutSec,
                HealthCheckWaitDuration = healthCheckWaitDurationSec,
                HealthCheckStableDuration = healthCheckStableDurationSec,
                UpgradeDomainTimeout = upgradeDomainTimeoutSec,
                UpgradeTimeout = upgradeTimeoutSec,
                MaxPercentUnhealthyApplications = maxPercentUnhealthyApplications,
                MaxPercentUnhealthyNodes = maxPercentUnhealthyNodes,
                MaxPercentDeltaUnhealthyNodes = maxPercentDeltaUnhealthyNodes,
                MaxPercentUpgradeDomainDeltaUnhealthyNodes = maxPercentUpgradeDomainDeltaUnhealthyNodes
            };

            if (native.Reserved != IntPtr.Zero)
            {
                var ex1 = *((NativeTypes.FABRIC_START_UPGRADE_DESCRIPTION_EX1*)native.Reserved);

                if (ex1.ApplicationHealthPolicyMap != IntPtr.Zero)
                {
                    configurationUpgradeDescription.ApplicationHealthPolicies.FromNative(ex1.ApplicationHealthPolicyMap);
                }
            }

            return configurationUpgradeDescription;
        }

        private static TimeSpan FromNativeTimeInSeconds(uint time)
        {
            if (time == NativeTypes.FABRIC_INFINITE_DURATION)
            {
                return TimeSpan.MaxValue;
            }

            return TimeSpan.FromSeconds(time);
        }
    }
}