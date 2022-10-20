// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client.Description
{
    using Microsoft.ServiceFabric.Preview.Client;
    using System;
    using System.Fabric.Description;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes a compose deployment to be created by using 
    /// <see cref="System.Fabric.FabricClient.ComposeDeploymentClient.UpgradeComposeDeploymentAsync(ComposeDeploymentUpgradeDescriptionWrapper)" />.</para>
    /// </summary>
    public sealed class ComposeDeploymentUpgradeDescription
    {
        /// <summary>
        /// Creates a new instance of <see cref="Microsoft.ServiceFabric.Preview.Client.Description.ComposeDeploymentUpgradeDescription"/>.
        /// </summary>
        public ComposeDeploymentUpgradeDescription(
            string deploymentName,
            string[] composeFilePaths)
        {
            this.DeploymentName = deploymentName;
            this.ComposeFilePaths = composeFilePaths;
            this.ContainerRegistryUserName = null;
            this.ContainerRegistryPassword = null;
            this.IsRegistryPasswordEncrypted = false;
            this.UpgradePolicyDescription = new MonitoredRollingApplicationUpgradePolicyDescription();
        }

        /// <summary>
        /// <para>Gets the name of the compose deployment which is to be upgraded.</para>
        /// </summary>
        /// <value>
        /// <para>The deployment name.</para>
        /// </value>
        public string DeploymentName { get; set; }

        /// <summary>
        /// <para>Gets the array of compose file paths, which describe the next version of this deployment.</para>
        /// </summary>
        /// <value>
        /// <para>Array of compose file paths.</para>
        /// </value>
        public string[] ComposeFilePaths { get; set; }

        /// <summary>
        /// <para>Gets the user name for the container registry where the images for the containers in this deployment are present.</para>
        /// </summary>
        /// <value>
        /// <para>The container registry user name.</para>
        /// </value>
        public string ContainerRegistryUserName { get; set; }

        /// <summary>
        /// <para>Gets the password associated with the username specified in the ContainerRegistryUserName parameter.</para>
        /// </summary>
        /// <value>
        /// <para>The container registry password.</para>
        /// </value>
        public string ContainerRegistryPassword { get; set; }

        /// <summary>
        /// <para>Gets if the password specified in the ContainerRegistryPassword parameter is encrypted.</para>
        /// </summary>
        /// <value>
        /// <para>True if the registry password is encrypted. False otherwise.</para>
        /// </value>
        public bool IsRegistryPasswordEncrypted { get; set; }

        /// <summary>
        /// <para>Gets or sets the description of the policy to be used for upgrading this compose deployment.</para>
        /// </summary>
        /// <value>
        /// <para>The description of the policy to be used for upgrading this compose deployment.</para>
        /// </value>
        public UpgradePolicyDescription UpgradePolicyDescription { get; set; }

        internal System.Fabric.Description.ComposeDeploymentUpgradeDescriptionWrapper ToWrapper()
        {
            return new System.Fabric.Description.ComposeDeploymentUpgradeDescriptionWrapper(
               this.DeploymentName,
               this.ComposeFilePaths,
               this.UpgradePolicyDescription,
               this.ContainerRegistryUserName,
               this.ContainerRegistryPassword,
               IsRegistryPasswordEncrypted);
        }
    }
}