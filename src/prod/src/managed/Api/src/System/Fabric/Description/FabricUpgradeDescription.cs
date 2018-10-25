// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a class to encapsulate a Service Fabric upgrade description.</para>
    /// </summary>
    public sealed class FabricUpgradeDescription
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.FabricUpgradeDescription" /> class.</para>
        /// </summary>
        public FabricUpgradeDescription()
        {
        }

        /// <summary>
        /// <para>Gets or sets the target code version for the Service Fabric upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The target code version for the Service Fabric upgrade.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.CodeVersion)]
        public string TargetCodeVersion { get; set; }

        /// <summary>
        /// <para>Gets or sets the target configuration version for the Service Fabric upgrade.</para>
        /// </summary>
        /// <value>
        /// <para>The target configuration version for the Service Fabric upgrade.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ConfigVersion)]
        public string TargetConfigVersion { get; set; }

        /// <summary>
        /// <para>Gets or sets the upgrade policy description.</para>
        /// </summary>
        /// <value>
        /// <para>The upgrade policy description.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public UpgradePolicyDescription UpgradePolicyDescription { get; set; }

        internal static void Validate(FabricUpgradeDescription description)
        {
            Requires.Argument<UpgradePolicyDescription>(
                "UpgradePolicyDescription",
                description.UpgradePolicyDescription).NotNull();

            description.UpgradePolicyDescription.Validate();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_UPGRADE_DESCRIPTION();

            nativeDescription.CodeVersion = pinCollection.AddBlittable(this.TargetCodeVersion);
            nativeDescription.ConfigVersion = pinCollection.AddBlittable(this.TargetConfigVersion);

            var rollingUpgradePolicyDescription = this.UpgradePolicyDescription as RollingUpgradePolicyDescription;
            if (rollingUpgradePolicyDescription != null)
            {
                nativeDescription.UpgradeKind = NativeTypes.FABRIC_UPGRADE_KIND.FABRIC_UPGRADE_KIND_ROLLING;
                nativeDescription.UpgradePolicyDescription = rollingUpgradePolicyDescription.ToNative(pinCollection);
            }
            else
            {
                throw new ArgumentException("description.UpgradePolicyDescription");
            }

            return pinCollection.AddBlittable(nativeDescription);
        }

        internal static unsafe FabricUpgradeDescription FromNative(IntPtr descriptionPtr)
        {
            if (descriptionPtr == IntPtr.Zero) { return null; }

            var castedPtr = (NativeTypes.FABRIC_UPGRADE_DESCRIPTION*)descriptionPtr;

            var description = new FabricUpgradeDescription();

            description.TargetCodeVersion = NativeTypes.FromNativeString(castedPtr->CodeVersion);

            description.TargetConfigVersion = NativeTypes.FromNativeString(castedPtr->ConfigVersion);

            if (castedPtr->UpgradeKind == NativeTypes.FABRIC_UPGRADE_KIND.FABRIC_UPGRADE_KIND_ROLLING &&
                castedPtr->UpgradePolicyDescription != IntPtr.Zero)
            {
                RollingUpgradePolicyDescription policy = null;

                var castedPolicyPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)castedPtr->UpgradePolicyDescription;

                if (castedPolicyPtr->RollingUpgradeMode == NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED)
                {
                    policy = MonitoredRollingFabricUpgradePolicyDescription.FromNative(castedPtr->UpgradePolicyDescription);
                }
                else
                {
                    policy = RollingUpgradePolicyDescription.FromNative(castedPtr->UpgradePolicyDescription);
                }

                description.UpgradePolicyDescription = policy;
            }

            return description;
        }
    }
}