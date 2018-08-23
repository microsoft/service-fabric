// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using Common.Serialization;
    using System.Fabric.Common;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Used to modify the upgrade parameters describing the behavior of application upgrades. See 
    /// <see cref="System.Fabric.FabricClient.ApplicationManagementClient.UpdateApplicationUpgradeAsync(System.Fabric.Description.ApplicationUpgradeUpdateDescription)"/>.</para>
    /// </summary>
    public sealed class ApplicationUpgradeUpdateDescription : UpgradeUpdateDescriptionBase
    {
        /// <summary>
        /// <para>Creates an instance of the <see cref="System.Fabric.Description.ApplicationUpgradeUpdateDescription"/> class.</para>
        /// </summary>
        public ApplicationUpgradeUpdateDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the name of the application with a current upgrade to modify.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application with a current upgrade to modify.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; set; }

        /// <summary>
        /// <para>Gets or sets the new value of <see cref="System.Fabric.Description.MonitoredRollingApplicationUpgradePolicyDescription.HealthPolicy"/>.</para>
        /// </summary>
        /// <value>
        /// <para>the new value of <see cref="System.Fabric.Description.MonitoredRollingApplicationUpgradePolicyDescription.HealthPolicy"/>.</para>
        /// </value>
        /// <remarks>The application health policy is used to evaluate the application health.</remarks>
        [JsonCustomization(IsDefaultValueIgnored = true, PropertyName = JsonPropertyNames.ApplicationHealthPolicy)]
        public ApplicationHealthPolicy HealthPolicy { get; set; }

        internal static void Validate(ApplicationUpgradeUpdateDescription description)
        {
            Requires.Argument<Uri>("ApplicationName", description.ApplicationName).NotNullOrWhiteSpace();

            if (description.UpgradeMode.HasValue && description.UpgradeMode.Value == RollingUpgradeMode.Invalid)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ArgumentInvalid_Formatted,
                        "ApplicationUpgradeUpdateDescription.UpgradeMode",
                        "Validate"));
            }

            if (description.FailureAction.HasValue && description.FailureAction.Value == UpgradeFailureAction.Invalid)
            {
                throw new ArgumentException(
                    string.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ArgumentInvalid_Formatted,
                        "ApplicationUpgradeUpdateDescription.FailureAction",
                        "Validate"));
            }
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            UInt32 flags = 0;

            var policyDescription = new MonitoredRollingApplicationUpgradePolicyDescription();

            flags = policyDescription.FromUpdateDescription(this);

            if (this.HealthPolicy != null)
            {
                flags |= (UInt32)NativeTypes.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS.FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY;
                policyDescription.HealthPolicy = this.HealthPolicy;
            }

            var nativeDescription = new NativeTypes.FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION();

            nativeDescription.ApplicationName = pinCollection.AddObject(this.ApplicationName);
            nativeDescription.UpgradeKind = NativeTypes.FABRIC_APPLICATION_UPGRADE_KIND.FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
            nativeDescription.UpdateFlags = flags;
            nativeDescription.UpgradePolicyDescription = policyDescription.ToNative(pinCollection);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}