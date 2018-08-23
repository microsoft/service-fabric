// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes a compose application to be created by using 
    /// <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.UpgradeComposeDeploymentAsync(ComposeDeploymentUpgradeDescriptionWrapper)" />.</para>
    /// </summary>
    internal sealed class ComposeDeploymentUpgradeDescriptionWrapper
    {
        public ComposeDeploymentUpgradeDescriptionWrapper(
            string deploymentName, 
            string[] composeFilePaths,
            UpgradePolicyDescription upgradePolicyDescription)
            : this(deploymentName, composeFilePaths, upgradePolicyDescription, null, null)
        {
        }

        public ComposeDeploymentUpgradeDescriptionWrapper(
                string deploymentName,
                string[] composeFilePaths,
                UpgradePolicyDescription upgradePolicyDescription,
                string containerRegistryUserName,
                string containerRegistryPassword,
                bool isRegistryPasswordEncrypted = false)
       {
            this.DeploymentName = deploymentName;
            this.ComposeFilePaths = composeFilePaths;
            this.ContainerRegistryUserName = containerRegistryUserName;
            this.ContainerRegistryPassword = containerRegistryPassword;
            this.IsRegistryPasswordEncrypted = isRegistryPasswordEncrypted;
            this.UpgradePolicyDescription = upgradePolicyDescription;
       }

       public string DeploymentName { get; private set; }

       public string[] ComposeFilePaths { get; private set; }

       public string ContainerRegistryUserName { get; private set; }

       public string ContainerRegistryPassword { get; private set; }

       public bool IsRegistryPasswordEncrypted { get; private set; }

       public UpgradePolicyDescription UpgradePolicyDescription { get; private set; }

       internal static void Validate(ComposeDeploymentUpgradeDescriptionWrapper description)
       {
           Requires.Argument<string>("DeploymentName", description.DeploymentName).NotNullOrWhiteSpace();
           Requires.Argument<UpgradePolicyDescription>("UpgradePolicyDescription", description.UpgradePolicyDescription).NotNull();

           description.UpgradePolicyDescription.Validate();
       }

       internal IntPtr ToNative(PinCollection pinCollection)
       {
            var nativeDescription = new NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION();

            nativeDescription.DeploymentName = pinCollection.AddObject(this.DeploymentName);
            nativeDescription.ComposeFilePaths.Count = (uint)this.ComposeFilePaths.Length;

            if (this.ComposeFilePaths.Length != 0)
            {
                var nativeArray = new IntPtr[this.ComposeFilePaths.Length];
                for (int i = 0; i < this.ComposeFilePaths.Length; ++i)
                {
                    nativeArray[i] = pinCollection.AddBlittable(this.ComposeFilePaths[i]);
                }

                nativeDescription.ComposeFilePaths.Items = pinCollection.AddBlittable(nativeArray);
            }
            else
            {
                nativeDescription.ComposeFilePaths.Items = IntPtr.Zero;
            }

            nativeDescription.ServiceFabricSettingsFilePaths.Count = 0;
            nativeDescription.ServiceFabricSettingsFilePaths.Items = IntPtr.Zero;

            var rollingUpgradePolicyDescription = this.UpgradePolicyDescription as RollingUpgradePolicyDescription;
            if (rollingUpgradePolicyDescription != null)
            {
                nativeDescription.UpgradeKind = NativeTypes.FABRIC_APPLICATION_UPGRADE_KIND.FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
                nativeDescription.UpgradePolicyDescription = rollingUpgradePolicyDescription.ToNative(pinCollection);
            }
            else
            {
                throw new ArgumentException("description.UpgradePolicyDescription");
            }

            if (this.ContainerRegistryUserName != null)
            {
                nativeDescription.ContainerRegistryUserName = pinCollection.AddObject(this.ContainerRegistryUserName);
            }
            else
            {
                nativeDescription.ContainerRegistryUserName = IntPtr.Zero;
            }

            if (this.ContainerRegistryPassword != null)
            {
                nativeDescription.ContainerRegistryPassword = pinCollection.AddObject(this.ContainerRegistryPassword);
            }
            else
            {
                nativeDescription.ContainerRegistryPassword = IntPtr.Zero;
            }

            nativeDescription.IsRegistryPasswordEncrypted = NativeTypes.ToBOOLEAN(this.IsRegistryPasswordEncrypted);

            return pinCollection.AddBlittable(nativeDescription);
       }

        internal static unsafe ComposeDeploymentUpgradeDescriptionWrapper FromNative(IntPtr descriptionPtr)
        {
            if (descriptionPtr == IntPtr.Zero) { return null; }

            var castedPtr = (NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION*)descriptionPtr;

            // Mandatory
            if (castedPtr->UpgradePolicyDescription == IntPtr.Zero) { return null; }

            RollingUpgradePolicyDescription policy = null;

            var castedPolicyPtr = (NativeTypes.FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*)castedPtr->UpgradePolicyDescription;

            if (castedPolicyPtr->RollingUpgradeMode == NativeTypes.FABRIC_ROLLING_UPGRADE_MODE.FABRIC_ROLLING_UPGRADE_MODE_MONITORED)
            {
                policy = MonitoredRollingApplicationUpgradePolicyDescription.FromNative(castedPtr->UpgradePolicyDescription);
            }
            else
            {
                policy = RollingUpgradePolicyDescription.FromNative(castedPtr->UpgradePolicyDescription);
            }

            var result = new ComposeDeploymentUpgradeDescriptionWrapper(
                NativeTypes.FromNativeString(castedPtr->DeploymentName),
                NativeTypes.FromNativeStringList(castedPtr->ComposeFilePaths).ToArray(),
                policy
                );

            result.ContainerRegistryUserName = NativeTypes.FromNativeString(castedPtr->ContainerRegistryUserName);
            result.ContainerRegistryPassword = NativeTypes.FromNativeString(castedPtr->ContainerRegistryPassword);
            result.IsRegistryPasswordEncrypted = NativeTypes.FromBOOLEAN(castedPtr->IsRegistryPasswordEncrypted);

            return result;
        }
    }
}