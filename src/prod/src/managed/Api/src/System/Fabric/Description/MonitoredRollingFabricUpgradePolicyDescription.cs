// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the behavior to use when performing a cluster upgrade.</para>
    /// </summary>
    public sealed class MonitoredRollingFabricUpgradePolicyDescription : MonitoredRollingUpgradePolicyDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.MonitoredRollingFabricUpgradePolicyDescription" /> class.</para>
        /// </summary>
        public MonitoredRollingFabricUpgradePolicyDescription()
            : base()
        {
            this.HealthPolicy = null;
            this.EnableDeltaHealthEvaluation = false;
            this.UpgradeHealthPolicy = null;

            // Create empty application health policies. 
            // Users can add items to it, but they can't replace it.
            this.ApplicationHealthPolicyMap = new ApplicationHealthPolicyMap();
        }

        /// <summary>
        /// <para>Gets or sets the health policy to use when performing health checks against an upgrading cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The health policy to use when performing health checks against an upgrading cluster.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ClusterHealthPolicy)]
        public ClusterHealthPolicy HealthPolicy { get; set; }

        /// <summary>
        /// <para>Gets or sets a flag indicating whether delta evaluation is enabled.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> when delta health evaluation is enabled; <languageKeyword>false</languageKeyword> otherwise.</para>
        /// </value>
        /// <remarks>
        /// <para>When delta evaluation is enabled, the cluster health evaluation ensures that the degradation of health respects tolerated limits,
        /// both globally, across all nodes, and per each upgrade domain that is evaluated. The tolerated thresholds are specified in 
        /// <see cref="System.Fabric.Health.ClusterUpgradeHealthPolicy"/>.</para>
        /// <para>Delta evaluation is disabled by default.</para></remarks>
        public bool EnableDeltaHealthEvaluation { get; set; }

        /// <summary>
        /// <para>Gets or sets the delta health policy to use when performing health checks against an upgrading cluster.</para>
        /// </summary>
        /// <value>
        /// <para>The delta health policy to use when performing health checks against an upgrading cluster.</para>
        /// </value>
        /// <remarks><para>
        /// The upgrade health policy is used when <see cref="System.Fabric.Description.FabricUpgradeUpdateDescription.EnableDeltaHealthEvaluation"/> is set to <languageKeyword>true</languageKeyword>. 
        /// Delta evaluation is disabled by default.
        /// </para></remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.ClusterUpgradeHealthPolicy)]
        public ClusterUpgradeHealthPolicy UpgradeHealthPolicy { get; set; }

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
        public ApplicationHealthPolicyMap ApplicationHealthPolicyMap { get; internal set; }

        new internal static unsafe MonitoredRollingFabricUpgradePolicyDescription FromNative(IntPtr policyPtr)
        {
            if (policyPtr == IntPtr.Zero) { return null; }

            var castedPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)policyPtr;

            var monitoringPolicy = new MonitoredRollingFabricUpgradePolicyDescription();

            if (castedPtr->Reserved != IntPtr.Zero)
            {
                var castedEx1Ptr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*)castedPtr->Reserved;
                if (castedEx1Ptr->MonitoringPolicy != IntPtr.Zero)
                {
                    monitoringPolicy.MonitoringPolicy = RollingUpgradeMonitoringPolicy.FromNative(castedEx1Ptr->MonitoringPolicy);
                }

                if (castedEx1Ptr->HealthPolicy != IntPtr.Zero)
                {
                    monitoringPolicy.HealthPolicy = ClusterHealthPolicy.FromNative(castedEx1Ptr->HealthPolicy);
                }

                if (castedEx1Ptr->Reserved != IntPtr.Zero)
                {
                    var castedEx2Ptr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2*)castedEx1Ptr->Reserved;
                    monitoringPolicy.EnableDeltaHealthEvaluation = NativeTypes.FromBOOLEAN(castedEx2Ptr->EnableDeltaHealthEvaluation);

                    if (castedEx2Ptr->UpgradeHealthPolicy != IntPtr.Zero)
                    {
                        monitoringPolicy.UpgradeHealthPolicy = ClusterUpgradeHealthPolicy.FromNative(castedEx2Ptr->UpgradeHealthPolicy);
                    }

                    if (castedEx2Ptr->Reserved != IntPtr.Zero)
                    {
                        var castedEx3Ptr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3*)castedEx2Ptr->Reserved;

                        if (castedEx3Ptr->ApplicationHealthPolicyMap != IntPtr.Zero)
                        {
                            monitoringPolicy.ApplicationHealthPolicyMap.FromNative(castedEx3Ptr->ApplicationHealthPolicyMap);
                        }
                    }
                }
            }

            monitoringPolicy.FromNativeHelper(policyPtr);

            return monitoringPolicy;
        }

        internal override IntPtr HealthPolicy_ToNative(PinCollection pinCollection)
        {
            if (this.HealthPolicy != null)
            {
                return this.HealthPolicy.ToNative(pinCollection);
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        internal override IntPtr ReservedEx_ToNative(PinCollection pinCollection)
        {
            var nativeEx2 = new NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2();

            nativeEx2.EnableDeltaHealthEvaluation = NativeTypes.ToBOOLEAN(this.EnableDeltaHealthEvaluation);

            if (this.UpgradeHealthPolicy != null)
            {
                nativeEx2.UpgradeHealthPolicy = this.UpgradeHealthPolicy.ToNative(pinCollection);
            }

            var nativeEx3 = new NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3();

            nativeEx3.ApplicationHealthPolicyMap = this.ApplicationHealthPolicyMap.ToNative(pinCollection);

            nativeEx2.Reserved = pinCollection.AddBlittable(nativeEx3);

            return pinCollection.AddBlittable(nativeEx2);
        }
    }
}