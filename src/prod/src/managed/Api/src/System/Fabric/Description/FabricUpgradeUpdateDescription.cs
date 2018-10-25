// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Used to modify the upgrade parameters describing the behavior cluster upgrades.
    /// See <see cref="System.Fabric.FabricClient.ClusterManagementClient.UpdateFabricUpgradeAsync(System.Fabric.Description.FabricUpgradeUpdateDescription)"/>.</para>
    /// </summary>
    public sealed class FabricUpgradeUpdateDescription : UpgradeUpdateDescriptionBase
    {
        /// <summary>
        /// <para>Creates an instance of the <see cref="System.Fabric.Description.FabricUpgradeUpdateDescription"/> class.</para>
        /// </summary>
        public FabricUpgradeUpdateDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Description.MonitoredRollingFabricUpgradePolicyDescription.HealthPolicy"/>.</para>
        /// </summary>
        /// <value>
        /// <para>The cluster health policy used to evaluate cluster health during upgrade.</para>
        /// </value>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.ClusterHealthPolicy)]
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
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public bool? EnableDeltaHealthEvaluation { get; set; }

        /// <summary>
        /// <para>Gets or sets the <see cref="System.Fabric.Description.MonitoredRollingFabricUpgradePolicyDescription.UpgradeHealthPolicy"/>.</para>
        /// </summary>
        /// <value>
        /// <para>The cluster upgrade health policy used to evaluate cluster health during upgrade.</para>
        /// </value>
        /// <remarks><para>
        /// The upgrade health policy is used when <see cref="System.Fabric.Description.FabricUpgradeUpdateDescription.EnableDeltaHealthEvaluation"/> is set to <languageKeyword>true</languageKeyword>. 
        /// The delta evaluation is disabled by default.
        /// </para></remarks>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.ClusterUpgradeHealthPolicy)]
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
        /// used to evaluate the application health of that application.
        /// </para>
        /// <para>
        /// If an application is not specified in the map, the ApplicationHealthPolicy found in the application manifest will be used for evaluation. </para>
        /// <para>
        /// The custom application health policies are also used to evaluate cluster health during upgrade, through
        /// <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(System.Fabric.Description.ClusterHealthQueryDescription, System.TimeSpan, System.Threading.CancellationToken)"/> or
        /// <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthChunkAsync(System.Fabric.Description.ClusterHealthChunkQueryDescription, System.TimeSpan, System.Threading.CancellationToken)"/>.
        /// </para>
        /// <para>
        /// The map is null by default, which means that the update doesn't apply to previously set application health policies.
        /// To update the application health policies, first create the map then add entries for desired applications.</para>
        /// </remarks>
        [JsonCustomization(IsDefaultValueIgnored = true)]
        public ApplicationHealthPolicyMap ApplicationHealthPolicyMap { get; set; }

        internal static void Validate(FabricUpgradeUpdateDescription description)
        {
            if (description.UpgradeMode.HasValue && description.UpgradeMode.Value == RollingUpgradeMode.Invalid)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ArgumentInvalid_Formatted,
                        "FabricUpgradeUpdateDescription.UpgradeMode",
                        "Validate"));
            }

            if (description.FailureAction.HasValue && description.FailureAction.Value == UpgradeFailureAction.Invalid)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ArgumentInvalid_Formatted,
                        "FabricUpgradeUpdateDescription.FailureAction",
                        "Validate"));
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            UInt32 flags = 0;

            var policyDescription = new MonitoredRollingFabricUpgradePolicyDescription();

            flags = policyDescription.FromUpdateDescription(this);

            if (this.HealthPolicy != null)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY;
                policyDescription.HealthPolicy = this.HealthPolicy;
            }

            if (this.EnableDeltaHealthEvaluation.HasValue)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_ENABLE_DELTAS;
                policyDescription.EnableDeltaHealthEvaluation = this.EnableDeltaHealthEvaluation.Value;
            }

            if (this.UpgradeHealthPolicy != null)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_HEALTH_POLICY;
                policyDescription.UpgradeHealthPolicy = this.UpgradeHealthPolicy;
            }

            if (this.ApplicationHealthPolicyMap != null)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_APPLICATION_HEALTH_POLICY_MAP;
                policyDescription.ApplicationHealthPolicyMap = this.ApplicationHealthPolicyMap;
            }

            var nativeDescription = new NativeTypes.FABRIC_UPGRADE_UPDATE_DESCRIPTION();

            nativeDescription.UpgradeKind = NativeTypes.FABRIC_UPGRADE_KIND.FABRIC_UPGRADE_KIND_ROLLING;
            nativeDescription.UpdateFlags = flags;
            nativeDescription.UpgradePolicyDescription = policyDescription.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}